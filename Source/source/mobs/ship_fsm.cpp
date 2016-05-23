/*
 * Copyright (c) Andre 'Espyo' Silva 2013-2016.
 * The following source file belongs to the open-source project
 * Pikmin fangame engine. Please read the included
 * README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Ship finite state machine logic.
 */

#include "../functions.h"
#include "../particle.h"
#include "ship.h"
#include "ship_fsm.h"
#include "../vars.h"

/* ----------------------------------------------------------------------------
 * Creates the finite state machine for the ship's logic.
 */
void ship_fsm::create_fsm(mob_type* typ) {
    easy_fsm_creator efc;
    
    efc.new_state("idle", SHIP_STATE_IDLE); {
        efc.new_event(MOB_EVENT_ON_ENTER); {
            efc.run_function(ship_fsm::set_anim);
        }
        efc.new_event(MOB_EVENT_RECEIVE_DELIVERY); {
            efc.run_function(ship_fsm::receive_mob);
        }
    }
    
    typ->states = efc.finish();
    typ->first_state_nr = fix_states(typ->states, "idle");
    
    if(typ->states.size() != N_SHIP_STATES) {
        log_error(
            "ENGINE WARNING: Number of ship states on the FSM (" +
            i2s(typ->states.size()) +
            ") and the enum (" + i2s(N_SHIP_STATES) + ") do not match."
        );
    }
}


/* ----------------------------------------------------------------------------
 * When a ship receives a mob carried by Pikmin.
 */
void ship_fsm::receive_mob(mob* m, void* info1, void* info2) {
    float pokos = *((float*) info1);
    ship* s_ptr = (ship*) m;
    
    random_particle_explosion(
        PARTICLE_TYPE_BITMAP, bmp_smoke,
        s_ptr->beam_final_x,
        s_ptr->beam_final_y,
        60, 80, 10, 20,
        1, 2, 24, 24, al_map_rgb(255, 255, 255)
    );
    
}


/* ----------------------------------------------------------------------------
 * When a ship needs to enter its default "idle" animation.
 */
void ship_fsm::set_anim(mob* m, void* info1, void* info2) {
    m->set_animation(SHIP_ANIM_IDLE);
}
