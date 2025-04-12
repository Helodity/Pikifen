/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Pikmin class and Pikmin-related functions.
 */

#include <algorithm>

#include "pikmin.h"

#include "../../core/drawing.h"
#include "../../core/misc_functions.h"
#include "../../core/game.h"
#include "../../util/allegro_utils.h"
#include "../../util/general_utils.h"
#include "../../util/geometry_utils.h"
#include "../../util/string_utils.h"
#include "../mob_script/pikmin_fsm.h"
#include "mob_enums.h"
#include "mob.h"


namespace PIKMIN {

//Maximum amount of time for the random boredom animation delay.
const float BORED_ANIM_MAX_DELAY = 5.0f;

//Minimum amount of time for hte random boredom animation delay.
const float BORED_ANIM_MIN_DELAY = 1.0f;

//Chance of circling the opponent instead of striking, when grounded.
const float CIRCLE_OPPONENT_CHANCE_GROUNDED = 0.2f;

//Chance of circling the opponent instead of latching, if it can latch.
const float CIRCLE_OPPONENT_CHANCE_PRE_LATCH = 0.5f;

//Time until moving Pikmin timeout and stay in place, after being dismissed.
const float DISMISS_TIMEOUT = 4.0f;

//Height above the floor that a flying Pikmin prefers to stay at.
const float FLIER_ABOVE_FLOOR_HEIGHT = 55.0f;

//Timeout before a Pikmin gives up, when ordered to go to something.
const float GOTO_TIMEOUT = 5.0f;

//If the Pikmin is within this distance of the mob, it can ground attack.
const float GROUNDED_ATTACK_DIST = 5.0f;

//The idle glow spins these many radians per second.
const float IDLE_GLOW_SPIN_SPEED = TAU / 4;

//Invulnerability period after getting hit.
const float INVULN_PERIOD = 0.7f;

//How long to remember a missed incoming attack for.
const float MISSED_ATTACK_DURATION = 1.5f;

//Interval for when a Pikmin decides a new chase spot, when panicking.
const float PANIC_CHASE_INTERVAL = 0.2f;

//A plucked Pikmin is thrown behind the leader at this speed, horizontally.
const float THROW_HOR_SPEED = 80.0f;

//A plucked Pikmin is thrown behind the leader at this speed, vertically.
const float THROW_VER_SPEED = 900.0f;

}




/**
 * @brief Constructs a new Pikmin object.
 *
 * @param pos Starting coordinates.
 * @param type Pikmin type this mob belongs to.
 * @param angle Starting angle.
 */
Pikmin::Pikmin(const Point &pos, PikminType* type, float angle) :
    Mob(pos, type, angle),
    pikType(type) {
    
    invulnPeriod = Timer(PIKMIN::INVULN_PERIOD);
    team = MOB_TEAM_PLAYER_1;
    subgroupTypePtr =
        game.states.gameplay->subgroupTypes.getType(
            SUBGROUP_TYPE_CATEGORY_PIKMIN, pikType
        );
    nearReach = 0;
    farReach = 2;
    updateInteractionSpan();
    missedAttackTimer =
        Timer(
            PIKMIN::MISSED_ATTACK_DURATION,
    [this] () { this->missedAttackPtr = nullptr; }
        );
        
    if(pikType->canFly) {
        enableFlag(flags, MOB_FLAG_CAN_MOVE_MIDAIR);
    }
}


/**
 * @brief Returns whether or not a Pikmin can receive a given status effect.
 *
 * @param s Status type to check.
 * @return Whether it can receive it.
 */
bool Pikmin::canReceiveStatus(StatusType* s) const {
    return hasFlag(s->affects, STATUS_AFFECTS_FLAG_PIKMIN);
}


/**
 * @brief Draws a Pikmin, including its leaf/bud/flower, idle glow, etc.
 */
void Pikmin::drawMob() {
    Sprite* cur_s_ptr;
    Sprite* next_s_ptr;
    float interpolation_factor;
    getSpriteData(&cur_s_ptr, &next_s_ptr, &interpolation_factor);
    if(!cur_s_ptr) return;
    
    //The Pikmin itself.
    BitmapEffect mob_eff;
    getSpriteBitmapEffects(
        cur_s_ptr, next_s_ptr, interpolation_factor,
        &mob_eff,
        SPRITE_BMP_EFFECT_FLAG_STATUS |
        SPRITE_BMP_EFFECT_FLAG_SECTOR_BRIGHTNESS |
        SPRITE_BMP_EFFECT_FLAG_HEIGHT |
        SPRITE_BMP_EFFECT_DELIVERY
    );
    BitmapEffect pik_sprite_eff = mob_eff;
    getSpriteBitmapEffects(
        cur_s_ptr, next_s_ptr, interpolation_factor,
        &pik_sprite_eff,
        SPRITE_BMP_EFFECT_FLAG_STANDARD
    );
    
    bool is_idle =
        fsm.curState->id == PIKMIN_STATE_IDLING ||
        fsm.curState->id == PIKMIN_STATE_IDLING_H ||
        fsm.curState->id == PIKMIN_STATE_SPROUT;
        
    if(is_idle) {
        mob_eff.glowColor = COLOR_WHITE;
    }
    
    drawBitmapWithEffects(cur_s_ptr->bitmap, pik_sprite_eff);
    
    //Top.
    if(cur_s_ptr->topVisible) {
        Point top_coords;
        float top_angle;
        Point top_size;
        BitmapEffect top_eff = mob_eff;
        ALLEGRO_BITMAP* top_bmp = pikType->bmpTop[maturity];
        getSpriteBasicTopEffects(
            cur_s_ptr, next_s_ptr, interpolation_factor,
            &top_coords, &top_angle, &top_size
        );
        //To get the height effect to work, we'll need to scale the translation
        //too, otherwise the top will detach from the Pikmin visually as
        //the Pikmin falls into a pit. The "right" scale is a bit of a guess
        //at this point in the code, but honestly, either X scale or Y scale
        //will work. In the off-chance they are different, using an average
        //will be more than enough.
        float avg_scale = (top_eff.scale.x + top_eff.scale.y) / 2.0f;
        Point top_bmp_size = getBitmapDimensions(top_bmp);
        top_eff.translation +=
            pos + rotatePoint(top_coords, angle) * avg_scale;
        top_eff.scale *= top_size / top_bmp_size;
        top_eff.rotation +=
            angle + top_angle;
        top_eff.glowColor =
            mapGray(0);
            
        drawBitmapWithEffects(top_bmp, top_eff);
    }
    
    //Idle glow.
    if(is_idle) {
        BitmapEffect idle_eff = pik_sprite_eff;
        Point glow_bmp_size =
            getBitmapDimensions(game.sysContent.bmpIdleGlow);
        idle_eff.translation = pos;
        idle_eff.scale =
            (game.config.pikmin.standardRadius * 8) / glow_bmp_size;
        idle_eff.rotation =
            game.states.gameplay->areaTimePassed *
            PIKMIN::IDLE_GLOW_SPIN_SPEED;
        idle_eff.tintColor = type->mainColor;
        idle_eff.glowColor = mapGray(0);
        
        drawBitmapWithEffects(game.sysContent.bmpIdleGlow, idle_eff);
    }
    
    drawStatusEffectBmp(this, pik_sprite_eff);
}


/**
 * @brief Logic specific to Pikmin for when they finish dying.
 */
void Pikmin::finishDyingClassSpecifics() {
    //Essentials.
    toDelete = true;
    
    //Soul.
    Particle par(
        pos, LARGE_FLOAT,
        radius * 2, 2.0f
    );
    par.bitmap = game.sysContent.bmpPikminSpirit;
    par.friction = 0.8;
    Point base_speed = Point(game.rng.f(-20, 20), game.rng.f(-70, -30));
    par.linearSpeed = KeyframeInterpolator<Point>(base_speed);
    par.linearSpeed.add(1, Point(Point(base_speed.x, base_speed.y - 20)));
    par.color.setKeyframeValue(0, changeAlpha(pikType->mainColor, 0));
    par.color.add(0.1f, pikType->mainColor);
    par.color.add(1, changeAlpha(pikType->mainColor, 0));
    game.states.gameplay->particles.add(par);
    
    //Sound. Create a positional sound source instead of a mob sound source,
    //since the Pikmin object is now practically deleted.
    size_t dying_sound_idx =
        pikType->soundDataIdxs[PIKMIN_SOUND_DYING];
    if(dying_sound_idx != INVALID) {
        MobType::Sound* dying_sound =
            &type->sounds[dying_sound_idx];
        game.audio.createPosSoundSource(
            dying_sound->sample,
            pos, false, dying_sound->config
        );
    }
}


/**
 * @brief Forces the Pikmin to start carrying the given mob.
 * This quickly runs over several steps in the usual FSM logic, just to
 * instantly get to the end result.
 * As such, be careful when using it.
 *
 * @param m The mob to carry.
 */
void Pikmin::forceCarry(Mob* m) {
    fsm.setState(PIKMIN_STATE_GOING_TO_CARRIABLE_OBJECT, (void*) m, nullptr);
    fsm.runEvent(MOB_EV_REACHED_DESTINATION);
}


/**
 * @brief Returns a Pikmin's base speed, without status effects and the like.
 * This depends on the maturity.
 *
 * @return The base speed.
 */
float Pikmin::getBaseSpeed() const {
    float base = Mob::getBaseSpeed();
    return base + (base * this->maturity * game.config.pikmin.maturitySpeedMult);
}


/**
 * @brief Returns its group spot information.
 * Basically, when it's in a leader's group, what point it should be following,
 * and within what distance.
 *
 * @param out_spot The final coordinates are returned here.
 * @param out_dist The final distance to those coordinates is returned here.
 */
void Pikmin::getGroupSpotInfo(
    Point* out_spot, float* out_dist
) const {
    out_spot->x = 0.0f;
    out_spot->y = 0.0f;
    *out_dist = 0.0f;
    
    if(!followingGroup || !followingGroup->group) {
        return;
    }
    
    *out_spot =
        followingGroup->group->anchor +
        followingGroup->group->getSpotOffset(groupSpotIdx);
    *out_dist = 5.0f;
}


/**
 * @brief Handles a status effect being applied.
 *
 * @param sta_type Status effect to handle.
 */
void Pikmin::handleStatusEffectGain(StatusType* sta_type) {
    Mob::handleStatusEffectGain(sta_type);
    
    switch(sta_type->stateChangeType) {
    case STATUS_STATE_CHANGE_FLAILING: {
        fsm.setState(PIKMIN_STATE_FLAILING);
        break;
    }
    case STATUS_STATE_CHANGE_HELPLESS: {
        fsm.setState(PIKMIN_STATE_HELPLESS);
        break;
    }
    case STATUS_STATE_CHANGE_PANIC: {
        fsm.setState(PIKMIN_STATE_PANICKING);
        break;
    }
    default: {
        break;
    }
    }
    
    increaseMaturity(sta_type->maturityChangeAmount);
    
    if(carryingMob) {
        carryingMob->chaseInfo.maxSpeed =
            carryingMob->carryInfo->getSpeed();
    }
}


/**
 * @brief Handles a status effect being removed.
 *
 * @param sta_type Status effect to handle.
 */
void Pikmin::handleStatusEffectLoss(StatusType* sta_type) {
    bool still_has_flailing = false;
    bool still_has_helplessness = false;
    bool still_has_panic = false;
    for(size_t s = 0; s < statuses.size(); s++) {
        if(statuses[s].type == sta_type) continue;
        
        switch(statuses[s].type->stateChangeType) {
        case STATUS_STATE_CHANGE_FLAILING: {
            still_has_flailing = true;
            break;
        }
        case STATUS_STATE_CHANGE_HELPLESS: {
            still_has_helplessness = true;
            break;
        }
        case STATUS_STATE_CHANGE_PANIC: {
            still_has_panic = true;
            break;
        }
        default: {
            break;
        }
        }
    }
    
    if(
        sta_type->stateChangeType == STATUS_STATE_CHANGE_FLAILING &&
        !still_has_flailing &&
        fsm.curState->id == PIKMIN_STATE_FLAILING
    ) {
        fsm.setState(PIKMIN_STATE_IDLING);
        setAnimation(PIKMIN_ANIM_SHAKING);
        inShakingAnimation = true;
        setTimer(0); //The boredom animation timeout.
        pikmin_fsm::standStill(this, nullptr, nullptr);
        invulnPeriod.start();
    }
    
    if(
        sta_type->stateChangeType == STATUS_STATE_CHANGE_HELPLESS &&
        !still_has_helplessness &&
        fsm.curState->id == PIKMIN_STATE_HELPLESS
    ) {
        fsm.setState(PIKMIN_STATE_IDLING);
        pikmin_fsm::standStill(this, nullptr, nullptr);
        invulnPeriod.start();
        
    } else if(
        sta_type->stateChangeType == STATUS_STATE_CHANGE_PANIC &&
        !still_has_panic &&
        fsm.curState->id == PIKMIN_STATE_PANICKING
    ) {
        fsm.setState(PIKMIN_STATE_IDLING);
        pikmin_fsm::standStill(this, nullptr, nullptr);
        invulnPeriod.start();
        
    }
    
    if(carryingMob) {
        carryingMob->chaseInfo.maxSpeed =
            carryingMob->carryInfo->getSpeed();
    }
}


/**
 * @brief Increases (or decreases) the Pikmin's maturity by the given amount.
 * This makes sure that the maturity doesn't overflow.
 *
 * @param amount Amount to increase by.
 * @return Whether it changed the maturity.
 */
bool Pikmin::increaseMaturity(int amount) {
    int old_maturity = maturity;
    int new_maturity = maturity + amount;
    maturity = std::clamp(new_maturity, 0, (int) (N_MATURITIES - 1));
    
    if(maturity > old_maturity) {
        game.statistics.pikminBlooms++;
    }
    return maturity != old_maturity;
}


/**
 * @brief Latches on to the specified mob.
 *
 * @param m Mob to latch on to.
 * @param h Hitbox to latch on to.
 */
void Pikmin::latch(Mob* m, const Hitbox* h) {
    speed.x = speed.y = speedZ = 0;
    
    //Shuffle it slightly, randomly, so that multiple Pikmin thrown
    //at the exact same spot aren't perfectly overlapping each other.
    pos.x += game.rng.f(-2.0f, 2.0f);
    pos.y += game.rng.f(-2.0f, 2.0f);
    
    float h_offset_dist;
    float h_offset_angle;
    float v_offset_dist;
    m->getHitboxHoldPoint(
        this, h, &h_offset_dist, &h_offset_angle, &v_offset_dist
    );
    m->hold(
        this, h->bodyPartIdx, h_offset_dist, h_offset_angle, v_offset_dist,
        true,
        HOLD_ROTATION_METHOD_NEVER //pikmin_fsm::prepareToAttack handles it.
    );
    
    latched = true;
}


/**
 * @brief Checks if an incoming attack should miss, and returns the result.
 *
 * If it was already decided that it missed in a previous frame, that's a
 * straight no. If not, it will roll with the hit rate to check.
 * If the attack is a miss, it also registers the miss, so that we can keep
 * memory of it for the next frames.
 *
 * @param info Info about the hitboxes involved.
 * @return Whether it hit.
 */
bool Pikmin::processAttackMiss(HitboxInteraction* info) {
    if(info->mob2->anim.curAnim == missedAttackPtr) {
        //In a previous frame, we had already considered this animation a miss.
        return false;
    }
    
    unsigned char hit_rate = info->mob2->anim.curAnim->hitRate;
    if(hit_rate == 0) return false;
    
    unsigned char hit_roll = game.rng.i(0, 100);
    if(hit_roll > hit_rate) {
        //This attack was randomly decided to be a miss.
        //Record this animation so it won't be considered a hit next frame.
        missedAttackPtr = info->mob2->anim.curAnim;
        missedAttackTimer.start();
        return false;
    }
    
    return true;
}


/**
 * @brief Reads the provided script variables, if any, and does stuff with them.
 *
 * @param svr Script var reader to use.
 */
void Pikmin::readScriptVars(const ScriptVarReader &svr) {
    Mob::readScriptVars(svr);
    
    int maturity_var;
    bool sprout_var;
    bool follow_link_var;
    
    if(svr.get("maturity", maturity_var)) {
        maturity = std::clamp(maturity_var, 0, (int) (N_MATURITIES - 1));
    }
    if(svr.get("sprout", sprout_var)) {
        if(sprout_var) {
            fsm.firstStateOverride = PIKMIN_STATE_SPROUT;
        }
    }
    if(svr.get("follow_link_as_leader", follow_link_var)) {
        if(follow_link_var) {
            mustFollowLinkAsLeader = true;
        }
    }
}


/**
 * @brief Sets up stuff for the beginning of the Pikmin's death process.
 */
void Pikmin::startDyingClassSpecifics() {
    game.states.gameplay->pikminDeaths++;
    game.states.gameplay->pikminDeathsPerType[pikType]++;
    game.states.gameplay->lastPikminDeathPos = pos;
    game.statistics.pikminDeaths++;
    
    enableFlag(flags, MOB_FLAG_INTANGIBLE);
}


/**
 * @brief Starts the particle generator that leaves a trail behind
 * a thrown Pikmin.
 */
void Pikmin::startThrowTrail() {
    ParticleGenerator pg =
        standardParticleGenSetup(
            game.sysContentNames.parThrowTrail, this
        );
    pg.followZOffset = 0.0f;
    adjustKeyframeInterpolatorValues<float>(
        pg.baseParticle.size,
    [ = ] (const float & f) { return f * radius; }
    );
    adjustKeyframeInterpolatorValues<ALLEGRO_COLOR>(
        pg.baseParticle.color,
    [ = ] (const ALLEGRO_COLOR & c) {
        ALLEGRO_COLOR new_c = c;
        new_c.r *= type->mainColor.r;
        new_c.g *= type->mainColor.g;
        new_c.b *= type->mainColor.b;
        new_c.a *= type->mainColor.a;
        return new_c;
    }
    );
    pg.id = MOB_PARTICLE_GENERATOR_ID_THROW;
    particleGenerators.push_back(pg);
}


/**
 * @brief Ticks time by one frame of logic.
 *
 * @param delta_t How long the frame's tick is, in seconds.
 */
void Pikmin::tickClassSpecifics(float delta_t) {
    //Carrying object.
    if(carryingMob) {
        if(!carryingMob->carryInfo) {
            fsm.runEvent(MOB_EV_FOCUSED_MOB_UNAVAILABLE);
        }
    }
    
    //Tick some timers.
    missedAttackTimer.tick(delta_t);
    bumpLock = std::max(bumpLock - delta_t, 0.0f);
    
    //Forcefully follow another mob as a leader.
    if(mustFollowLinkAsLeader) {
        if(!links.empty() && links[0]) {
            fsm.runEvent(
                MOB_EV_TOUCHED_ACTIVE_LEADER,
                (void*) (links[0]),
                (void*) 1 //Be silent.
            );
        }
        //Since this leader is likely an enemy, let's keep these Pikmin safe.
        enableFlag(flags, MOB_FLAG_NON_HUNTABLE);
        enableFlag(flags, MOB_FLAG_NON_HURTABLE);
        mustFollowLinkAsLeader = false;
    }
    
}


/**
 * @brief Returns the sprout closest to a leader. Used when auto-plucking.
 *
 * @param pos Coordinates of the leader.
 * @param d Variable to return the distance to. nullptr for none.
 * @param ignore_reserved If true, ignore any sprouts that are "reserved"
 * (i.e. already chosen to be plucked by another leader).
 * @return The sprout.
 */
Pikmin* getClosestSprout(
    const Point &pos, Distance* d, bool ignore_reserved
) {
    Distance closest_distance;
    Pikmin* closest_pikmin = nullptr;
    
    size_t n_pikmin = game.states.gameplay->mobs.pikmin.size();
    for(size_t p = 0; p < n_pikmin; p++) {
        if(
            game.states.gameplay->mobs.pikmin[p]->fsm.curState->id !=
            PIKMIN_STATE_SPROUT
        ) {
            continue;
        }
        
        Distance dis(pos, game.states.gameplay->mobs.pikmin[p]->pos);
        if(closest_pikmin == nullptr || dis < closest_distance) {
        
            if(
                !(
                    ignore_reserved ||
                    game.states.gameplay->mobs.pikmin[p]->pluckReserved
                )
            ) {
                closest_distance = dis;
                closest_pikmin = game.states.gameplay->mobs.pikmin[p];
            }
        }
    }
    
    if(d) *d = closest_distance;
    return closest_pikmin;
}
