/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Onion finite state machine logic.
 */

#include "onion_fsm.h"

#include "../functions.h"
#include "../game.h"
#include "../mobs/onion.h"
#include "../mobs/pellet.h"
#include "../particle.h"
#include "../utils/string_utils.h"


/**
 * @brief Creates the finite state machine for the Onion's logic.
 *
 * @param typ Mob type to create the finite state machine for.
 */
void onion_fsm::create_fsm(mob_type* typ) {
    easy_fsm_creator efc;
    
    efc.new_state("idling", ONION_STATE_IDLING); {
        efc.new_event(MOB_EV_FINISHED_RECEIVING_DELIVERY); {
            efc.run(onion_fsm::receive_mob);
        }
    }
    
    typ->states = efc.finish();
    typ->first_state_idx = fix_states(typ->states, "idling", typ);
    
    //Check if the number in the enum and the total match up.
    engine_assert(
        typ->states.size() == N_ONION_STATES,
        i2s(typ->states.size()) + " registered, " +
        i2s(N_ONION_STATES) + " in enum."
    );
}


/**
 * @brief When an Onion finishes receiving a mob carried by Pikmin.
 *
 * @param m The mob.
 * @param info1 Pointer to the mob being received.
 * @param info2 Unused.
 */
void onion_fsm::receive_mob(mob* m, void* info1, void* info2) {
    engine_assert(info1 != nullptr, m->print_state_history());
    
    mob* delivery = (mob*) info1;
    onion* oni_ptr = (onion*) m;
    size_t seeds = 0;
    
    switch(delivery->type->category->id) {
    case MOB_CATEGORY_ENEMIES: {
        seeds = ((enemy*) delivery)->ene_type->pikmin_seeds;
        break;
    } case MOB_CATEGORY_PELLETS: {
        pellet* pel_ptr = (pellet*) delivery;
        if(
            pel_ptr->pel_type->pik_type ==
            delivery->delivery_info->intended_pik_type
        ) {
            seeds = pel_ptr->pel_type->match_seeds;
        } else {
            seeds = pel_ptr->pel_type->non_match_seeds;
        }
        break;
    } default: {
        break;
    }
    }
    
    size_t type_idx = 0;
    for(; type_idx < oni_ptr->oni_type->nest->pik_types.size(); ++type_idx) {
        if(
            oni_ptr->oni_type->nest->pik_types[type_idx] ==
            delivery->delivery_info->intended_pik_type
        ) {
            break;
        }
    }
    
    oni_ptr->full_spew_timer.start();
    oni_ptr->next_spew_timer.stop();
    oni_ptr->spew_queue[type_idx] += seeds;
    
    particle p(
        m->pos, m->z + m->height - 0.01,
        24, 1.5, PARTICLE_PRIORITY_MEDIUM
    );
    p.bitmap = game.sys_assets.bmp_smoke;
    p.outwards_speed = keyframe_interpolator<float>(70);
    particle_generator pg(0, p, 15);
    pg.emission.number_deviation = 5;
    pg.linear_speed_deviation.x = 10;
    pg.duration_deviation = 0.5;
    pg.emit(game.states.gameplay->particles);
    
}
