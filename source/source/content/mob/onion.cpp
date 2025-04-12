/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Onion class and Onion-related functions.
 */

#include <algorithm>

#include "onion.h"

#include "../../core/drawing.h"
#include "../../core/misc_functions.h"
#include "../../core/game.h"
#include "../../util/geometry_utils.h"
#include "../../util/string_utils.h"


using std::size_t;
using std::string;


namespace ONION {

//How quickly an Onion fades to and from see-through, in values per second.
const float FADE_SPEED = 255.0f;

//Delay before the Onion starts the generation process.
const float GENERATION_DELAY = 2.0f;

//An Onion-spat seed starts with this Z offset from the Onion.
const float NEW_SEED_Z_OFFSET = 320.0f;

//Interval between each individual Pikmin generation.
const float NEXT_GENERATION_INTERVAL = 0.10f;

//Onion opacity when it goes see-through.
const unsigned char SEETHROUGH_ALPHA = 128;

//After spitting a seed, the next seed's angle shifts by this much.
const float SPEW_ANGLE_SHIFT = TAU * 0.12345;

//An Onion-spat seed is this quick, horizontally.
const float SPEW_H_SPEED = 80.0f;

//Deviate the seed's horizontal speed by this much, more or less.
const float SPEW_H_SPEED_DEVIATION = 10.0f;

//An Onion-spat seed is this quick, vertically.
const float SPEW_V_SPEED = 600.0f;
}




/**
 * @brief Constructs a new Onion object.
 *
 * @param pos Starting coordinates.
 * @param type Onion type this mob belongs to.
 * @param angle Starting angle.
 */
Onion::Onion(const Point &pos, OnionType* type, float angle) :
    Mob(pos, type, angle),
    oniType(type) {
    
    nest = new PikminNest(this, oniType->nest);
    
    //Increase its Z by one so that mobs that walk at
    //ground level next to it will appear under it.
    gravityMult = 0.0f;
    z++;
    
    generationDelayTimer.onEnd =
    [this] () { startGenerating(); };
    nextGenerationTimer.onEnd =
    [this] () {
        for(size_t t = 0; t < oniType->nest->pik_types.size(); t++) {
            if(generationQueue[t] > 0) {
                nextGenerationTimer.start();
                generate();
                return;
            }
        }
        stopGenerating();
    };
    
    for(size_t t = 0; t < oniType->nest->pik_types.size(); t++) {
        generationQueue.push_back(0);
    }
}


/**
 * @brief Destroys the Onion object.
 */
Onion::~Onion() {
    delete nest;
}


/**
 * @brief Draws an Onion.
 */
void Onion::drawMob() {
    Sprite* cur_s_ptr;
    Sprite* next_s_ptr;
    float interpolation_factor;
    getSpriteData(&cur_s_ptr, &next_s_ptr, &interpolation_factor);
    if(!cur_s_ptr) return;
    
    BitmapEffect eff;
    getSpriteBitmapEffects(
        cur_s_ptr, next_s_ptr, interpolation_factor,
        &eff,
        SPRITE_BMP_EFFECT_FLAG_STANDARD |
        SPRITE_BMP_EFFECT_FLAG_STATUS |
        SPRITE_BMP_EFFECT_FLAG_SECTOR_BRIGHTNESS |
        SPRITE_BMP_EFFECT_FLAG_HEIGHT |
        SPRITE_BMP_EFFECT_DELIVERY
    );
    
    eff.tintColor.a *= (seethrough / 255.0f);
    
    drawBitmapWithEffects(cur_s_ptr->bitmap, eff);
}


/**
 * @brief Reads the provided script variables, if any, and does stuff with them.
 *
 * @param svr Script var reader to use.
 */
void Onion::readScriptVars(const ScriptVarReader &svr) {
    Mob::readScriptVars(svr);
    
    nest->readScriptVars(svr);
}


/**
 * @brief Spew a Pikmin seed in the queue or add it to the Onion's storage.
 */
void Onion::generate() {
    for(size_t t = 0; t < generationQueue.size(); t++) {
        if(generationQueue[t] == 0) continue;
        
        generationQueue[t]--;
        
        game.statistics.pikminBirths++;
        game.states.gameplay->pikminBorn++;
        game.states.gameplay->pikminBornPerType[
            oniType->nest->pik_types[t]
        ]++;
        game.states.gameplay->lastPikminBornPos = pos;
        
        size_t total_after =
            game.states.gameplay->mobs.pikmin.size() + 1;
            
        if(total_after > game.config.rules.maxPikminInField) {
            nest->pikmin_inside[t][0]++;
            
            ParticleGenerator pg =
                standardParticleGenSetup(
                    game.sysContentNames.parOnionGenInside, this
                );
            pg.baseParticle.priority = PARTICLE_PRIORITY_LOW;
            particleGenerators.push_back(pg);
            
            return;
        }
        
        float horizontal_strength =
            ONION::SPEW_H_SPEED +
            game.rng.f(
                -ONION::SPEW_H_SPEED_DEVIATION,
                ONION::SPEW_H_SPEED_DEVIATION
            );
        spewPikminSeed(
            pos, z + ONION::NEW_SEED_Z_OFFSET, oniType->nest->pik_types[t],
            nextSpewAngle, horizontal_strength, ONION::SPEW_V_SPEED
        );
        
        nextSpewAngle += ONION::SPEW_ANGLE_SHIFT;
        nextSpewAngle = normalizeAngle(nextSpewAngle);
        
        playSound(oniType->soundPopIdx);
        
        return;
    }
}


/**
 * @brief Starts generating Pikmin.
 */
void Onion::startGenerating() {
    generationDelayTimer.stop();
    nextGenerationTimer.start();
    string msg = "started_generation";
    sendScriptMessage(this, msg);
}


/**
 * @brief Stops generating Pikmin.
 */
void Onion::stopGenerating() {
    generationDelayTimer.stop();
    nextGenerationTimer.stop();
    string msg = "stopped_generation";
    sendScriptMessage(this, msg);
}


/**
 * @brief Ticks time by one frame of logic.
 *
 * @param delta_t How long the frame's tick is, in seconds.
 */
void Onion::tickClassSpecifics(float delta_t) {
    generationDelayTimer.tick(delta_t);
    nextGenerationTimer.tick(delta_t);
    
    unsigned char final_alpha = 255;
    
    if(
        game.states.gameplay->curLeaderPtr &&
        BBoxCheck(
            game.states.gameplay->curLeaderPtr->pos, pos,
            game.states.gameplay->curLeaderPtr->radius +
            radius * 3
        )
    ) {
        final_alpha = ONION::SEETHROUGH_ALPHA;
    }
    
    if(
        game.states.gameplay->curLeaderPtr &&
        BBoxCheck(
            game.states.gameplay->leaderCursorW, pos,
            game.states.gameplay->curLeaderPtr->radius +
            radius * 3
        )
    ) {
        final_alpha = ONION::SEETHROUGH_ALPHA;
    }
    
    if(seethrough != final_alpha) {
        if(final_alpha < seethrough) {
            seethrough =
                std::max(
                    (double) final_alpha,
                    (double) seethrough - ONION::FADE_SPEED * delta_t
                );
        } else {
            seethrough =
                std::min(
                    (double) final_alpha,
                    (double) seethrough + ONION::FADE_SPEED * delta_t
                );
        }
    }
    
    nest->tick(delta_t);
}
