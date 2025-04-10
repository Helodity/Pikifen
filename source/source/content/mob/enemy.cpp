/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Enemy class and enemy-related functions.
 */

#include <algorithm>
#include <unordered_set>

#include "enemy.h"

#include "../../core/drawing.h"
#include "../../core/misc_functions.h"
#include "../../core/game.h"
#include "../../util/general_utils.h"
#include "../../util/math_utils.h"
#include "../../util/string_utils.h"
#include "../mob_type/mob_type.h"


namespace ENEMY {

//Maximum diameter an enemy's spirit can be.
const float SPIRIT_MAX_SIZE = 128;

//Minimum diameter an enemy's spirit can be.
const float SPIRIT_MIN_SIZE = 16;

//Normally, the spirit's diameter is the enemy's. Multiply the spirit by this.
const float SPIRIT_SIZE_MULT = 0.7;

}


/**
 * @brief Constructs a new enemy object.
 *
 * @param pos Starting coordinates.
 * @param type Enemy type this mob belongs to.
 * @param angle Starting angle.
 */
Enemy::Enemy(const Point &pos, EnemyType* type, float angle) :
    Mob(pos, type, angle),
    ene_type(type) {
    
}


/**
 * @brief Returns whether or not an enemy can receive a given status effect.
 *
 * @param s Status type to check.
 * @return Whether it can receive the status.
 */
bool Enemy::can_receive_status(StatusType* s) const {
    return has_flag(s->affects, STATUS_AFFECTS_FLAG_ENEMIES);
}


/**
 * @brief Draws an enemy.
 */
void Enemy::draw_mob() {
    Sprite* cur_s_ptr;
    Sprite* next_s_ptr;
    float interpolation_factor;
    get_sprite_data(&cur_s_ptr, &next_s_ptr, &interpolation_factor);
    if(!cur_s_ptr) return;
    
    BitmapEffect eff;
    get_sprite_bitmap_effects(
        cur_s_ptr, next_s_ptr, interpolation_factor,
        &eff,
        SPRITE_BMP_EFFECT_FLAG_STANDARD |
        SPRITE_BMP_EFFECT_FLAG_STATUS |
        SPRITE_BMP_EFFECT_FLAG_SECTOR_BRIGHTNESS |
        SPRITE_BMP_EFFECT_FLAG_HEIGHT |
        SPRITE_BMP_EFFECT_DELIVERY |
        SPRITE_BMP_EFFECT_DAMAGE |
        SPRITE_BMP_EFFECT_CARRY
    );
    draw_bitmap_with_effects(cur_s_ptr->bitmap, eff);
    draw_status_effect_bmp(this, eff);
}


/**
 * @brief Logic specific to enemies for when they finish dying.
 */
void Enemy::finish_dying_class_specifics() {
    //Corpse.
    if(ene_type->drops_corpse) {
        become_carriable(CARRY_DESTINATION_SHIP_NO_ONION);
        fsm.set_state(ENEMY_EXTRA_STATE_CARRIABLE_WAITING);
    }
    
    //Soul.
    Particle par(
        pos, LARGE_FLOAT,
        std::clamp(
            radius * 2 * ENEMY::SPIRIT_SIZE_MULT,
            ENEMY::SPIRIT_MIN_SIZE, ENEMY::SPIRIT_MAX_SIZE
        ),
        2, PARTICLE_PRIORITY_MEDIUM
    );
    par.bitmap = game.sys_content.bmp_enemy_spirit;
    par.friction = 0.5f;
    par.linear_speed = KeyframeInterpolator<Point>(Point(-50, -50));
    par.linear_speed.add(0.5f, Point(50, -50));
    par.linear_speed.add(1, Point(-50, -50));
    
    par.color = KeyframeInterpolator<ALLEGRO_COLOR>(al_map_rgba(255, 192, 255, 0));
    par.color.add(0.1f, al_map_rgb(255, 192, 255));
    par.color.add(0.6f, al_map_rgb(255, 192, 255));
    par.color.add(1, al_map_rgba(255, 192, 255, 0));
    game.states.gameplay->particles.add(par);
}


/**
 * @brief Sets up stuff for the beginning of the enemy's death process.
 */
void Enemy::start_dying_class_specifics() {
    //Numbers.
    game.states.gameplay->enemy_deaths++;
    if(!game.cur_area_data->mission.enemy_points_on_collection) {
        game.states.gameplay->enemy_points_collected += ene_type->points;
    }
    game.states.gameplay->last_enemy_killed_pos = pos;
    game.statistics.enemy_deaths++;
    
    if(game.cur_area_data->mission.goal == MISSION_GOAL_BATTLE_ENEMIES) {
        game.states.gameplay->mission_remaining_mob_ids.erase(id);
    }
    
    //Music.
    if(ene_type->is_boss) {
        switch(game.states.gameplay->boss_music_state) {
        case BOSS_MUSIC_STATE_PLAYING: {
            bool near_boss;
            game.states.gameplay->is_near_enemy_and_boss(nullptr, &near_boss);
            if(!near_boss) {
                //Only play the victory fanfare if they're not near another one.
                game.audio.set_current_song(
                    game.sys_content_names.sng_boss_victory, true, false, false
                );
                game.states.gameplay->boss_music_state =
                    BOSS_MUSIC_STATE_VICTORY;
            }
        } default: {
            break;
        }
        }
    }
    
    //Particles.
    ParticleGenerator pg =
        standard_particle_gen_setup(
            game.sys_content_names.part_enemy_death, this
        );
    particle_generators.push_back(pg);
}
