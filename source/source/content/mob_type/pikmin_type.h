/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Header for the Pikmin type class and Pikmin type-related functions.
 */

#pragma once

#include <string>
#include <vector>

#include <allegro5/allegro.h>

#include "../../lib/data_file/data_file.h"
#include "../other/hazard.h"
#include "mob_type.h"


namespace PIKMIN_TYPE {
extern const float DEF_KNOCKED_DOWN_DURATION;
extern const float DEF_KNOCKED_DOWN_WHISTLE_BONUS;
}


//Pikmin object states.
enum PIKMIN_STATE {

    //In group, chasing.
    PIKMIN_STATE_IN_GROUP_CHASING,
    
    //In group, stopped.
    PIKMIN_STATE_IN_GROUP_STOPPED,
    
    //Swarming, chasing.
    PIKMIN_STATE_SWARM_CHASING,
    
    //Swarming, stopped.
    PIKMIN_STATE_SWARM_STOPPED,
    
    //Idling.
    PIKMIN_STATE_IDLING,
    
    //Called.
    PIKMIN_STATE_CALLED,
    
    //As a seed.
    PIKMIN_STATE_SEED,
    
    //As a sprout.
    PIKMIN_STATE_SPROUT,
    
    //Being plucked.
    PIKMIN_STATE_PLUCKING,
    
    //Thrown backwards after being plucked.
    PIKMIN_STATE_PLUCKING_THROWN,
    
    //Leaving Onion.
    PIKMIN_STATE_LEAVING_ONION,
    
    //Entering Onion.
    PIKMIN_STATE_ENTERING_ONION,
    
    //Grabbed by leader.
    PIKMIN_STATE_GRABBED_BY_LEADER,
    
    //Grabbed by enemy.
    PIKMIN_STATE_GRABBED_BY_ENEMY,
    
    //Getting knocked back.
    PIKMIN_STATE_KNOCKED_BACK,
    
    //Knocked down on the floor.
    PIKMIN_STATE_KNOCKED_DOWN,
    
    //Getting up.
    PIKMIN_STATE_GETTING_UP,
    
    //Bouncing after performing an impact attack.
    PIKMIN_STATE_IMPACT_BOUNCE,
    
    //Lunging to perform an impact attack.
    PIKMIN_STATE_IMPACT_LUNGE,
    
    //Thrown.
    PIKMIN_STATE_THROWN,
    
    //Landing on mob.
    PIKMIN_STATE_MOB_LANDING,
    
    //Going to dismiss spot.
    PIKMIN_STATE_GOING_TO_DISMISS_SPOT,
    
    //Picking something up.
    PIKMIN_STATE_PICKING_UP,
    
    //Working on grop task.
    PIKMIN_STATE_ON_GROUP_TASK,
    
    //Sighing.
    PIKMIN_STATE_SIGHING,
    
    //Carrying.
    PIKMIN_STATE_CARRYING,
    
    //Returning to a pile.
    PIKMIN_STATE_RETURNING,
    
    //Attacking, grounded.
    PIKMIN_STATE_ATTACKING_GROUNDED,
    
    //Attacking, latched.
    PIKMIN_STATE_ATTACKING_LATCHED,
    
    //Goign to a carriable object.
    PIKMIN_STATE_GOING_TO_CARRIABLE_OBJECT,
    
    //Going to a tool.
    PIKMIN_STATE_GOING_TO_TOOL,
    
    //Going to an opponent.
    PIKMIN_STATE_GOING_TO_OPPONENT,
    
    //Circling around opponent.
    PIKMIN_STATE_CIRCLING_OPPONENT,
    
    //Going to a group task.
    PIKMIN_STATE_GOING_TO_GROUP_TASK,
    
    //Going to an Onion.
    PIKMIN_STATE_GOING_TO_ONION,
    
    //Riding a track.
    PIKMIN_STATE_RIDING_TRACK,
    
    //Helpless.
    PIKMIN_STATE_HELPLESS,
    
    //Flailing.
    PIKMIN_STATE_FLAILING,
    
    //Panicking.
    PIKMIN_STATE_PANICKING,
    
    //Drinking a drop.
    PIKMIN_STATE_DRINKING,
    
    //Celebrating.
    PIKMIN_STATE_CELEBRATING,
    
    //In group, chasing, whilst holding a tool.
    PIKMIN_STATE_IN_GROUP_CHASING_H,
    
    //In group, stopped, whilst holding a tool.
    PIKMIN_STATE_IN_GROUP_STOPPED_H,
    
    //Swarming, chasing, whilst holding a tool.
    PIKMIN_STATE_SWARM_CHASING_H,
    
    //Swarming, stopped, whilst holding a tool.
    PIKMIN_STATE_SWARM_STOPPED_H,
    
    //Idling, whilst holding a tool.
    PIKMIN_STATE_IDLING_H,
    
    //Called, whilst holding a tool.
    PIKMIN_STATE_CALLED_H,
    
    //Grabbed by a leader, whilst holding a tool.
    PIKMIN_STATE_GRABBED_BY_LEADER_H,
    
    //Thrown, whilst holding a tool.
    PIKMIN_STATE_THROWN_H,
    
    //Going to dismiss spot, whilst holding a tool.
    PIKMIN_STATE_GOING_TO_DISMISS_SPOT_H,
    
    //Crushed, shrinking away, and dying.
    PIKMIN_STATE_CRUSHED,
    
    //Knocked down, shrinking away, and dying.
    PIKMIN_STATE_KNOCKED_DOWN_DYING,
    
    //Collapsing, shrinking away, and dying.
    PIKMIN_STATE_DYING,
    
    //Total amount of Pikmin object states.
    N_PIKMIN_STATES
    
};


//Pikmin object animations.
enum PIKMIN_ANIM {

    //Idling.
    PIKMIN_ANIM_IDLING,
    
    //Called.
    PIKMIN_ANIM_CALLED,
    
    //Walking.
    PIKMIN_ANIM_WALKING,
    
    //Thrown.
    PIKMIN_ANIM_THROWN,
    
    //Landing on mob.
    PIKMIN_ANIM_MOB_LANDING,
    
    //Attacking.
    PIKMIN_ANIM_ATTACKING,
    
    //Carrying something that isn't light.
    PIKMIN_ANIM_CARRYING,
    
    //Carrying something light.
    PIKMIN_ANIM_CARRYING_LIGHT,
    
    //Carrying, but struggling to lift.
    PIKMIN_ANIM_CARRYING_STRUGGLE,
    
    //Backflip celebration.
    PIKMIN_ANIM_BACKFLIP,
    
    //Twirling celebration.
    PIKMIN_ANIM_TWIRLING,
    
    //Sighing.
    PIKMIN_ANIM_SIGHING,
    
    //Shaking, like after leaving water.
    PIKMIN_ANIM_SHAKING,
    
    //As a sprout.
    PIKMIN_ANIM_SPROUT,
    
    //Being plucked.
    PIKMIN_ANIM_PLUCKING,
    
    //Backflipping after being thrown.
    PIKMIN_ANIM_PLUCKING_THROWN,
    
    //Getting knocked back.
    PIKMIN_ANIM_KNOCKED_BACK,
    
    //Bounced back from an impact attack.
    PIKMIN_ANIM_BOUNCED_BACK,
    
    //Lying on the ground.
    PIKMIN_ANIM_LYING,
    
    //Getting up.
    PIKMIN_ANIM_GETTING_UP,
    
    //Flailing.
    PIKMIN_ANIM_FLAILING,
    
    //Drinking a drop.
    PIKMIN_ANIM_DRINKING,
    
    //Picking something up.
    PIKMIN_ANIM_PICKING_UP,
    
    //Arms stretched out sideways.
    PIKMIN_ANIM_ARMS_OUT,
    
    //Pushing forward.
    PIKMIN_ANIM_PUSHING,
    
    //Climbing.
    PIKMIN_ANIM_CLIMBING,
    
    //Sliding.
    PIKMIN_ANIM_SLIDING,
    
    //Crushed, shrinking away, and dying.
    PIKMIN_ANIM_CRUSHED,
    
    //Knocked down, shrinking away, and dying.
    PIKMIN_ANIM_KNOCKED_DOWN_DYING,
    
    //Collapsing, shrinking away, and dying.
    PIKMIN_ANIM_DYING,
    
};


//Pikmin object sounds.
enum PIKMIN_SOUND {

    //Called by a leader.
    PIKMIN_SOUND_CALLED,
    
    //Carrying something.
    PIKMIN_SOUND_CARRYING,
    
    //Grabbing on to start carrying something.
    PIKMIN_SOUND_CARRYING_GRAB,
    
    //Caught by an enemy.
    PIKMIN_SOUND_CAUGHT,
    
    //Dying.
    PIKMIN_SOUND_DYING,
    
    //Held by a leader.
    PIKMIN_SOUND_HELD,
    
    //Becoming idle.
    PIKMIN_SOUND_IDLE,
    
    //Latching on.
    PIKMIN_SOUND_LATCH,
    
    //Maturing from one maturity to the next.
    PIKMIN_SOUND_MATURING,
    
    //Seed landing.
    PIKMIN_SOUND_SEED_LANDING,
    
    //Suffering from a hazard.
    PIKMIN_SOUND_SUFFERING,
    
    //Thrown by a leader.
    PIKMIN_SOUND_THROWN,
    
    //Total amount.
    N_PIKMIN_SOUNDS,
    
};


//Pikmin type attack methods.
enum PIKMIN_ATTACK {
    //Latches on and attacks.
    PIKMIN_ATTACK_LATCH,
    
    //Lunges forward for an impact.
    PIKMIN_ATTACK_IMPACT,
    
};


/**
 * @brief Pikmin types, almost the basic meat of the fan games.
 * The canon ones (at the time of writing this) are
 * Red, Yellow, Blue, White, Purple, Bulbmin, Winged, and Rock,
 * but with the engine, loads of fan-made ones can be made.
 */
class PikminType : public MobType {

public:

    //--- Members ---
    
    //How many Pikmin they are worth when carrying.
    float carryStrength = 1.0f;
    
    //How many Pikmin they are worth when pushing.
    float pushStrength = 1.0f;
    
    //Maximum height that the peak of their throw arc can reach.
    float maxThrowHeight = 260.0f;
    
    //What the main method of attack is.
    PIKMIN_ATTACK attackMethod = PIKMIN_ATTACK_LATCH;
    
    //How long it stays on the floor for after knocked down, if left alone.
    float knockedDownDuration = PIKMIN_TYPE::DEF_KNOCKED_DOWN_DURATION;
    
    //A whistled Pikmin that got knocked down loses this much in lie-down time.
    float knockedDownWhistleBonus = PIKMIN_TYPE::DEF_KNOCKED_DOWN_WHISTLE_BONUS;
    
    //Whether it can fly or not.
    bool canFly = false;
    
    //Whether it can carry tool-type objects or not.
    bool canCarryTools = true;
    
    //Modifier for enemy hit rates when attacking this Pikmin, latched.
    //0 = normal. -1 = always misses. 1 = always hits.
    //-0.5 = misses twice as often. etc.
    float enemyHitRateModifierLatched = 0.0f;
    
    //Modifier for enemy hit rates when attacking this Pikmin, not latched.
    //0 = normal. -1 = always misses. 1 = always hits.
    //-0.5 = misses twice as often. etc.
    float enemyHitRateModifierStanding = 0.0f;
    
    //How long it takes to evolve in maturity, as a sprout.
    float sproutEvolutionTime[N_MATURITIES] = { 0.0f, 0.0f, 0.0f };
    
    //Top (leaf/bud/flower) bitmap for each maturity.
    ALLEGRO_BITMAP* bmpTop[N_MATURITIES] = { nullptr, nullptr, nullptr };
    
    //Standby icon.
    ALLEGRO_BITMAP* bmpIcon = nullptr;
    
    //Standby maturity icons.
    ALLEGRO_BITMAP* bmpMaturityIcon[N_MATURITIES] =
    { nullptr, nullptr, nullptr };
    
    //Icon for its Onion.
    ALLEGRO_BITMAP* bmpOnionIcon = nullptr;
    
    //Sound data index for each sound. Cache for performance.
    size_t soundDataIdxs[N_PIKMIN_SOUNDS];
    
    
    //--- Function declarations ---
    
    PikminType();
    void loadCatProperties(DataNode* file) override;
    void loadCatResources(DataNode* file) override;
    AnimConversionVector getAnimConversions() const override;
    void unloadResources() override;
    
};
