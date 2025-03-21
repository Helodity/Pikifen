/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Header for the struct that holds the game's configuration,
 * and related functions.
 */

#pragma once

#include <string>
#include <vector>

#include <allegro5/allegro.h>

#include "../content/mob_type/leader_type.h"
#include "../content/mob_type/pikmin_type.h"
#include "../content/other/spray_type.h"


using std::size_t;
using std::vector;


namespace GAME_CONFIG {
extern const bool DEF_CAN_THROW_LEADERS;
extern const ALLEGRO_COLOR DEF_CARRYING_COLOR_MOVE;
extern const ALLEGRO_COLOR DEF_CARRYING_COLOR_STOP;
extern const float DEF_CARRYING_SPEED_BASE_MULT;
extern const float DEF_CARRYING_SPEED_MAX_MULT;
extern const float DEF_CARRYING_SPEED_WEIGHT_MULT;
extern const float DEF_CURSOR_MAX_DIST;
extern const float DEF_CURSOR_SPIN_SPEED;
extern const float DEF_DAY_MINUTES_END;
extern const float DEF_DAY_MINUTES_START;
extern const float DEF_GROUP_MEMBER_GRAB_RANGE;
extern const float DEF_IDLE_BUMP_DELAY;
extern const float DEF_IDLE_BUMP_RANGE;
extern const float DEF_IDLE_TASK_RANGE;
extern const float DEF_MATURITY_POWER_MULT;
extern const float DEF_MATURITY_SPEED_MULT;
extern const size_t DEF_MAX_PIKMIN_IN_FIELD;
extern const float DEF_GAMEPLAY_MSG_CHAR_INTERVAL;
extern const float DEF_NEXT_PLUCK_RANGE;
extern const ALLEGRO_COLOR DEF_NO_PIKMIN_COLOR;
extern const float DEF_ONION_OPEN_RANGE;
extern const float DEF_PIKMIN_CHASE_RANGE;
extern const float DEF_PLUCK_RANGE;
extern const ALLEGRO_COLOR DEF_RADAR_BG_COLOR;
extern const ALLEGRO_COLOR DEF_RADAR_EDGE_COLOR;
extern const ALLEGRO_COLOR DEF_RADAR_HIGHEST_COLOR;
extern const ALLEGRO_COLOR DEF_RADAR_LOWEST_COLOR;
extern const float DEF_STANDARD_LEADER_HEIGHT;
extern const float DEF_STANDARD_LEADER_RADIUS;
extern const float DEF_STANDARD_PIKMIN_HEIGHT;
extern const float DEF_STANDARD_PIKMIN_RADIUS;
extern const float DEF_SWARM_TASK_RANGE;
extern const float DEF_THROW_MAX_DIST;
extern const float DEF_WHISTLE_GROWTH_SPEED;
extern const float DEF_WHISTLE_MAX_DIST;
extern const float DEF_ZOOM_MAX_LEVEL;
extern const float DEF_ZOOM_MIN_LEVEL;
}


/**
 * @brief The game's configuration. It controls some rules about the game.
 */
struct GameConfig {

    //--- Members ---
    
    //Can a leader throw other leaders?
    bool can_throw_leaders = GAME_CONFIG::DEF_CAN_THROW_LEADERS;
    
    //Color that represents a non-Onion carriable object when moving.
    ALLEGRO_COLOR carrying_color_move = GAME_CONFIG::DEF_CARRYING_COLOR_MOVE;
    
    //Color that represents a non-Onion carriable object when stopped.
    ALLEGRO_COLOR carrying_color_stop = GAME_CONFIG::DEF_CARRYING_COLOR_STOP;
    
    //Used for the slowest carrying speed an object can go.
    float carrying_speed_base_mult = GAME_CONFIG::DEF_CARRYING_SPEED_BASE_MULT;
    
    //Used for the fastest carrying speed an object can go.
    float carrying_speed_max_mult = GAME_CONFIG::DEF_CARRYING_SPEED_MAX_MULT;
    
    //Decreases carry speed by this much per unit of weight.
    float carrying_speed_weight_mult = GAME_CONFIG::DEF_CARRYING_SPEED_WEIGHT_MULT;
    
    //Maximum distance from the leader the cursor can go.
    float cursor_max_dist = GAME_CONFIG::DEF_CURSOR_MAX_DIST;
    
    //How much the cursor spins per second.
    float cursor_spin_speed = GAME_CONFIG::DEF_CURSOR_SPIN_SPEED;
    
    //The day ends when the in-game minutes reach this value.
    float day_minutes_end = GAME_CONFIG::DEF_DAY_MINUTES_END;
    
    //The in-game minutes start with this value every day.
    float day_minutes_start = GAME_CONFIG::DEF_DAY_MINUTES_START;
    
    //A leader can grab a group member only within this range.
    float group_member_grab_range = GAME_CONFIG::DEF_GROUP_MEMBER_GRAB_RANGE;
    
    //Idle Pikmin are only bumped if away from a leader for these many seconds.
    float idle_bump_delay = GAME_CONFIG::DEF_IDLE_BUMP_DELAY;
    
    //Idle Pikmin will be bumped by a leader within this distance.
    float idle_bump_range = GAME_CONFIG::DEF_IDLE_BUMP_RANGE;
    
    //Idle Pikmin will go for a task if they are within this distance of it.
    float idle_task_range = GAME_CONFIG::DEF_IDLE_TASK_RANGE;
    
    //List of leader types, ordered by the game configuration.
    vector<LeaderType*> leader_order;
    
    //Loaded strings representing the standard leader order. Used for later.
    vector<string> leader_order_strings;
    
    //Every level of maturity, multiply the attack power by 1 + this much.
    float maturity_power_mult = GAME_CONFIG::DEF_MATURITY_POWER_MULT;
    
    //Every level of maturity, multiply the speed by 1 + this much.
    float maturity_speed_mult = GAME_CONFIG::DEF_MATURITY_SPEED_MULT;
    
    //Maximum number of Pikmin that can be out in the field at once.
    size_t max_pikmin_in_field = GAME_CONFIG::DEF_MAX_PIKMIN_IN_FIELD;
    
    //These many seconds until a new character of the gameplay message is drawn.
    float gameplay_msg_char_interval =
        GAME_CONFIG::DEF_GAMEPLAY_MSG_CHAR_INTERVAL;
        
    //Name of the game.
    string name;
    
    //How far a leader can go to auto-pluck the next Pikmin.
    float next_pluck_range = GAME_CONFIG::DEF_NEXT_PLUCK_RANGE;
    
    //Color that represents the absence of Pikmin.
    ALLEGRO_COLOR no_pikmin_color = GAME_CONFIG::DEF_NO_PIKMIN_COLOR;
    
    //Onions can be opened if the leader is within this distance.
    float onion_open_range = GAME_CONFIG::DEF_ONION_OPEN_RANGE;
    
    //Pikmin will only chase enemies in this range.
    float pikmin_chase_range = GAME_CONFIG::DEF_PIKMIN_CHASE_RANGE;
    
    //List of Pikmin types, ordered by the game configuration.
    vector<PikminType*> pikmin_order;
    
    //Loaded strings representing the standard Pikmin order. Used for later.
    vector<string> pikmin_order_strings;
    
    //A leader can start the plucking mode if they're this close.
    float pluck_range = GAME_CONFIG::DEF_PLUCK_RANGE;
    
    //Color of the background in the radar.
    ALLEGRO_COLOR radar_background_color = GAME_CONFIG::DEF_RADAR_BG_COLOR;
    
    //Color of edges in the radar.
    ALLEGRO_COLOR radar_edge_color = GAME_CONFIG::DEF_RADAR_EDGE_COLOR;
    
    //Color for the highest sector in the radar.
    ALLEGRO_COLOR radar_highest_color = GAME_CONFIG::DEF_RADAR_HIGHEST_COLOR;
    
    //Color for the lowest sector in the radar.
    ALLEGRO_COLOR radar_lowest_color = GAME_CONFIG::DEF_RADAR_LOWEST_COLOR;
    
    //List of spray types, ordered by the game configuration.
    vector<SprayType*> spray_order;
    
    //Loaded strings representing the standard spray order. Used for later.
    vector<string> spray_order_strings;
    
    //A standard leader is this tall.
    float standard_leader_height = GAME_CONFIG::DEF_STANDARD_LEADER_HEIGHT;
    
    //A standard leader has this radius.
    float standard_leader_radius = GAME_CONFIG::DEF_STANDARD_LEADER_RADIUS;
    
    //A standard Pikmin is this tall.
    float standard_pikmin_height = GAME_CONFIG::DEF_STANDARD_PIKMIN_HEIGHT;
    
    //A standard Pikmin has this radius.
    float standard_pikmin_radius = GAME_CONFIG::DEF_STANDARD_PIKMIN_RADIUS;
    
    //Pikmin that are swarming can go for tasks within this range.
    float swarm_task_range = GAME_CONFIG::DEF_SWARM_TASK_RANGE;
    
    //Maximum distance from the leader that a throw can be aimed to.
    float throw_max_dist = GAME_CONFIG::DEF_THROW_MAX_DIST;
    
    //Version of the game.
    string version;
    
    //Speed at which the whistle grows.
    float whistle_growth_speed = GAME_CONFIG::DEF_WHISTLE_GROWTH_SPEED;
    
    //Maximum distance from the leader that the whistle can start from.
    float whistle_max_dist = GAME_CONFIG::DEF_WHISTLE_MAX_DIST;
    
    //The closest zoom level the player can get.
    float zoom_max_level = GAME_CONFIG::DEF_ZOOM_MAX_LEVEL;
    
    //The farthest zoom level the player can get.
    float zoom_min_level = GAME_CONFIG::DEF_ZOOM_MIN_LEVEL;
    
    
    //--- Function declarations ---
    
    void load(DataNode* file);
    
};
