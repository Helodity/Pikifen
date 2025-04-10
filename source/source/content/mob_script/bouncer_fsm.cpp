/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Bouncer finite state machine logic.
 */

#include <algorithm>

#include "bouncer_fsm.h"

#include "../../core/misc_functions.h"
#include "../../core/game.h"
#include "../../util/general_utils.h"
#include "../../util/string_utils.h"
#include "../mob/bouncer.h"


/**
 * @brief Creates the finite state machine for the bouncer's logic.
 *
 * @param typ Mob type to create the finite state machine for.
 */
void bouncer_fsm::create_fsm(MobType* typ) {
    EasyFsmCreator efc;
    efc.new_state("idling", BOUNCER_STATE_IDLING); {
        efc.new_event(MOB_EV_ON_ENTER); {
            efc.run(bouncer_fsm::set_idling_animation);
        }
        efc.new_event(MOB_EV_RIDER_ADDED); {
            efc.run(bouncer_fsm::handle_mob);
        }
    }
    efc.new_state("bouncing", BOUNCER_STATE_BOUNCING); {
        efc.new_event(MOB_EV_ON_ENTER); {
            efc.run(bouncer_fsm::set_bouncing_animation);
        }
        efc.new_event(MOB_EV_RIDER_ADDED); {
            efc.run(bouncer_fsm::handle_mob);
        }
        efc.new_event(MOB_EV_ANIMATION_END); {
            efc.change_state("idling");
        }
    }
    
    
    typ->states = efc.finish();
    typ->first_state_idx = fix_states(typ->states, "idling", typ);
    
    //Check if the number in the enum and the total match up.
    engine_assert(
        typ->states.size() == N_BOUNCER_STATES,
        i2s(typ->states.size()) + " registered, " +
        i2s(N_BOUNCER_STATES) + " in enum."
    );
}


/**
 * @brief When something is on top of the bouncer.
 *
 * @param m The mob.
 * @param info1 Points to the mob that is on top of it.
 * @param info2 Unused.
 */
void bouncer_fsm::handle_mob(Mob* m, void* info1, void* info2) {
    Bouncer* bou_ptr = (Bouncer*) m;
    Mob* toucher = (Mob*) info1;
    Mob* target_mob = nullptr;
    
    if(!bou_ptr->links.empty()) {
        target_mob = bou_ptr->links[0];
    }
    
    if(!target_mob) {
        game.errors.report(
            "The bouncer (" + get_error_message_mob_info(m) +
            ") has no linked mob to serve as a target!"
        );
        return;
    }
    
    MobEvent* ev = nullptr;
    
    //Check if a compatible mob touched it.
    if(
        has_flag(bou_ptr->bou_type->riders, BOUNCER_RIDER_FLAG_PIKMIN) &&
        toucher->type->category->id == MOB_CATEGORY_PIKMIN
    ) {
    
        //Pikmin is about to be bounced.
        ev = toucher->fsm.get_event(MOB_EV_TOUCHED_BOUNCER);
        
    } else if(
        has_flag(bou_ptr->bou_type->riders, BOUNCER_RIDER_FLAG_LEADERS) &&
        toucher->type->category->id == MOB_CATEGORY_LEADERS
    ) {
    
        //Leader is about to be bounced.
        ev = toucher->fsm.get_event(MOB_EV_TOUCHED_BOUNCER);
        
    } else if(
        has_flag(bou_ptr->bou_type->riders, BOUNCER_RIDER_FLAG_PIKMIN) &&
        toucher->path_info &&
        has_flag(
            toucher->path_info->settings.flags,
            PATH_FOLLOW_FLAG_LIGHT_LOAD
        )
    ) {
    
        //Pikmin carrying light load is about to be bounced.
        ev = toucher->fsm.get_event(MOB_EV_TOUCHED_BOUNCER);
    }
    
    if(!ev) return;
    
    toucher->stop_chasing();
    toucher->leave_group();
    enable_flag(toucher->flags, MOB_FLAG_WAS_THROWN);
    toucher->start_height_effect();
    
    float angle;
    //The maximum height has a guaranteed minimum (useful if the destination
    //is below the bouncer), and scales up with how much higher the thrown
    //mob needs to go, to make for a nice smooth arc.
    float max_h = std::max(128.0f, (target_mob->z - toucher->z) * 1.5f);
    calculate_throw(
        toucher->pos,
        toucher->z,
        target_mob->pos,
        target_mob->z + target_mob->height,
        max_h, MOB::GRAVITY_ADDER,
        &toucher->speed,
        &toucher->speed_z,
        &angle
    );
    
    toucher->face(angle, nullptr, true);
    
    ev->run(toucher, (void*) m);
    
    m->fsm.set_state(BOUNCER_STATE_BOUNCING, info1, info2);
}


/**
 * @brief When it must change to the bouncing animation.
 *
 * @param m The mob.
 * @param info1 Unused.
 * @param info2 Unused.
 */
void bouncer_fsm::set_bouncing_animation(Mob* m, void* info1, void* info2) {
    m->set_animation(BOUNCER_ANIM_BOUNCING);
}


/**
 * @brief When it must change to the idling animation.
 *
 * @param m The mob.
 * @param info1 Unused.
 * @param info2 Unused.
 */
void bouncer_fsm::set_idling_animation(Mob* m, void* info1, void* info2) {
    m->set_animation(
        BOUNCER_ANIM_IDLING, START_ANIM_OPTION_RANDOM_TIME_ON_SPAWN, true
    );
}
