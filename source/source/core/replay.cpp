/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Particle class and particle-related functions.
 */

#include <algorithm>

#include "replay.h"


using std::size_t;
using std::string;
using std::vector;


/**
 * @brief Construct a new replay object.
 */
Replay::Replay() {

    clear();
}


/**
 * @brief Adds a new state to the replay, filling it with data from the supplied
 * mob vectors.
 *
 * @param leader_list List of leaders.
 * @param pikmin_list List of Pikmin.
 * @param enemy_list List of enemies.
 * @param treasure_list List of treasures.
 * @param onion_list List of Onions.
 * @param obstacle_list List of mobs that represent obstacles.
 * @param cur_leader_idx Index number of the current leader.
 */
void Replay::addState(
    const vector<Leader*> &leader_list,
    const vector<Pikmin*> &pikmin_list,
    const vector<Enemy*> &enemy_list,
    const vector<Treasure*> &treasure_list,
    const vector<Onion*> &onion_list,
    const vector<Mob*> &obstacle_list,
    size_t cur_leader_idx
) {
    states.push_back(ReplayState());
    ReplayState* new_state_ptr = &(states[states.size() - 1]);
    
    vector<Mob*> new_state_mobs;
    new_state_mobs.insert(
        new_state_mobs.end(), leader_list.begin(), leader_list.end()
    );
    new_state_mobs.insert(
        new_state_mobs.end(), pikmin_list.begin(), pikmin_list.end()
    );
    new_state_mobs.insert(
        new_state_mobs.end(), enemy_list.begin(), enemy_list.end()
    );
    new_state_mobs.insert(
        new_state_mobs.end(), treasure_list.begin(), treasure_list.end()
    );
    new_state_mobs.insert(
        new_state_mobs.end(), onion_list.begin(), onion_list.end()
    );
    new_state_mobs.insert(
        new_state_mobs.end(), obstacle_list.begin(), obstacle_list.end()
    );
    
    if(!prevStateMobs.empty()) {
        for(size_t pm = 0; pm < prevStateMobs.size(); pm++) {
            //Is this mob in the list of new mobs?
            auto m =
                find(
                    new_state_mobs.begin(), new_state_mobs.end(),
                    prevStateMobs[pm]
                );
            if(m == new_state_mobs.end()) {
                //It isn't. That means it was removed.
                ReplayEvent ev(REPLAY_EVENT_REMOVED, pm);
                new_state_ptr->events.push_back(ev);
            }
        }
        
        for(size_t m = 0; m < new_state_mobs.size(); m++) {
            //Is this mob in the list of previous mobs?
            auto pm =
                find(
                    prevStateMobs.begin(), prevStateMobs.end(),
                    new_state_mobs[m]
                );
            if(pm == prevStateMobs.end()) {
                //It isn't. That means it's new.
                ReplayEvent ev(REPLAY_EVENT_ADDED, m);
                new_state_ptr->events.push_back(ev);
            }
        }
    }
    
    if(cur_leader_idx != prevLeaderIdx) {
        ReplayEvent ev(REPLAY_EVENT_LEADER_SWITCHED, cur_leader_idx);
        new_state_ptr->events.push_back(ev);
        prevLeaderIdx = cur_leader_idx;
    }
    
    new_state_ptr->elements.reserve(
        leader_list.size() +
        pikmin_list.size() +
        enemy_list.size() +
        treasure_list.size() +
        onion_list.size() +
        obstacle_list.size()
    );
    for(size_t l = 0; l < leader_list.size(); l++) {
        new_state_ptr->elements.push_back(
            ReplayElement(REPLAY_ELEMENT_LEADER, leader_list[l]->pos)
        );
    }
    for(size_t p = 0; p < pikmin_list.size(); p++) {
        new_state_ptr->elements.push_back(
            ReplayElement(REPLAY_ELEMENT_PIKMIN, pikmin_list[p]->pos)
        );
    }
    for(size_t e = 0; e < enemy_list.size(); e++) {
        new_state_ptr->elements.push_back(
            ReplayElement(REPLAY_ELEMENT_ENEMY, enemy_list[e]->pos)
        );
    }
    for(size_t t = 0; t < treasure_list.size(); t++) {
        new_state_ptr->elements.push_back(
            ReplayElement(REPLAY_ELEMENT_TREASURE, treasure_list[t]->pos)
        );
    }
    for(size_t o = 0; o < onion_list.size(); o++) {
        new_state_ptr->elements.push_back(
            ReplayElement(REPLAY_ELEMENT_ONION, onion_list[o]->pos)
        );
    }
    for(size_t o = 0; o < obstacle_list.size(); o++) {
        new_state_ptr->elements.push_back(
            ReplayElement(REPLAY_ELEMENT_OBSTACLE, obstacle_list[o]->pos)
        );
    }
    
    prevStateMobs = new_state_mobs;
}


/**
 * @brief Clears all data about this replay.
 */
void Replay::clear() {
    states.clear();
    prevLeaderIdx = INVALID;
    prevStateMobs.clear();
}


/**
 * @brief Finishes the recording of a new replay.
 */
void Replay::finishRecording() {
    clear();
}


/**
 * @brief Loads replay data from a file in the disk.
 *
 * @param file_path Path to the file to load from.
 */
void Replay::loadFromFile(const string &file_path) {
    clear();
    ALLEGRO_FILE* file = al_fopen(file_path.c_str(), "rb");
    
    size_t n_states = al_fread32be(file);
    states.reserve(n_states);
    
    for(size_t s = 0; s < n_states; s++) {
        states.push_back(ReplayState());
        ReplayState* s_ptr = &states[states.size() - 1];
        
        size_t n_elements = al_fread32be(file);
        s_ptr->elements.reserve(n_elements);
        
        for(size_t e = 0; e < n_elements; e++) {
            s_ptr->elements.push_back(
                ReplayElement(
                    (REPLAY_ELEMENT) al_fgetc(file),
                    Point(al_fread32be(file), al_fread32be(file))
                )
            );
        }
        
        size_t n_events = al_fread32be(file);
        if(n_events > 0) {
            s_ptr->events.reserve(n_events);
            
            for(size_t e = 0; e < n_events; e++) {
                s_ptr->events.push_back(
                    ReplayEvent(
                        (REPLAY_EVENT) al_fgetc(file),
                        al_fread32be(file)
                    )
                );
            }
        }
    }
}


/**
 * @brief Saves replay data to a file in the disk.
 *
 * @param file_path Path to the file to save to.
 */
void Replay::saveToFile(const string &file_path) const {
    ALLEGRO_FILE* file = al_fopen(file_path.c_str(), "wb");
    
    al_fwrite32be(file, (int32_t) states.size());
    for(size_t s = 0; s < states.size(); s++) {
        const ReplayState* s_ptr = &states[s];
        
        al_fwrite32be(file, (int32_t) s_ptr->elements.size());
        for(size_t e = 0; e < s_ptr->elements.size(); e++) {
            al_fputc(file, s_ptr->elements[e].type);
            al_fwrite32be(file, floor(s_ptr->elements[e].pos.x));
            al_fwrite32be(file, floor(s_ptr->elements[e].pos.y));
        }
        
        al_fwrite32be(file, (int32_t) s_ptr->events.size());
        for(size_t e = 0; e < s_ptr->events.size(); e++) {
            al_fputc(file, s_ptr->events[e].type);
            al_fwrite32be(file, (int32_t) s_ptr->events[e].data);
        }
    }
    
    al_fclose(file);
}


/**
 * @brief Constructs a new replay element object.
 *
 * @param type Type of element.
 * @param pos Its coordinates.
 */
ReplayElement::ReplayElement(
    const REPLAY_ELEMENT type, const Point &pos
) :
    type(type),
    pos(pos) {
    
}


/**
 * @brief Constructs a new replay event object.
 *
 * @param type Type of event.
 * @param data Any numerical data this event needs.
 */
ReplayEvent::ReplayEvent(
    const REPLAY_EVENT type, size_t data
) :
    type(type),
    data(data) {
    
}
