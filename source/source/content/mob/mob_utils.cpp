/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Mob utility classes and functions.
 */

#include <algorithm>
#include <unordered_set>

#include "mob_utils.h"

#include "../../core/misc_functions.h"
#include "../../core/game.h"
#include "../../util/general_utils.h"
#include "../../util/string_utils.h"
#include "../other/mob_script_action.h"
#include "mob.h"


using std::unordered_set;
using std::size_t;


/**
 * @brief Constructs a new carrier spot struct object.
 *
 * @param pos The spot's relative coordinates.
 */
CarrierSpot::CarrierSpot(const Point &pos) :
    pos(pos) {
    
}


/**
 * @brief Constructs a new carry info struct object.
 *
 * @param m The mob this info belongs to.
 * @param destination Where to deliver the mob.
 */
CarryInfo::CarryInfo(
    Mob* m, const CARRY_DESTINATION destination
) :
    m(m),
    destination(destination) {
    
    for(size_t c = 0; c < m->type->maxCarriers; c++) {
        Point p;
        if(m->type->customCarrySpots.empty()) {
            float angle = TAU / m->type->maxCarriers * c;
            p =
                Point(
                    cos(angle) *
                    (m->radius + game.config.pikmin.standardRadius),
                    sin(angle) *
                    (m->radius + game.config.pikmin.standardRadius)
                );
        } else {
            p = m->type->customCarrySpots[c];
        }
        spotInfo.push_back(CarrierSpot(p));
    }
}


/**
 * @brief Returns true if the carriers can all fly, and thus, the object can
 * be carried through the air.
 *
 * @return Whether it can fly.
 */
bool CarryInfo::canFly() const {
    for(size_t c = 0; c < spotInfo.size(); c++) {
        Mob* carrier_ptr = spotInfo[c].pikPtr;
        if(!carrier_ptr) continue;
        if(!hasFlag(spotInfo[c].pikPtr->flags, MOB_FLAG_CAN_MOVE_MIDAIR)) {
            return false;
        }
    }
    return true;
}


/**
 * @brief Returns a list of hazards to which all carrier Pikmin
 * are invulnerable.
 *
 * @return The invulnerabilities.
 */
vector<Hazard*> CarryInfo::getCarrierInvulnerabilities() const {
    //Get all types to save on the amount of hazard checks.
    unordered_set<MobType*> carrier_types;
    for(size_t c = 0; c < spotInfo.size(); c++) {
        Mob* carrier_ptr = spotInfo[c].pikPtr;
        if(!carrier_ptr) continue;
        carrier_types.insert(carrier_ptr->type);
    }
    
    return getMobTypeListInvulnerabilities(carrier_types);
}


/**
 * @brief Returns the speed at which the object should move,
 * given the carrier Pikmin.
 *
 * @return The speed.
 */
float CarryInfo::getSpeed() const {
    if(curNCarriers == 0) {
        return 0;
    }
    
    float max_speed = 0;
    
    //Begin by obtaining the average walking speed of the carriers.
    for(size_t s = 0; s < spotInfo.size(); s++) {
        const CarrierSpot* s_ptr = &spotInfo[s];
        
        if(s_ptr->state != CARRY_SPOT_STATE_USED) continue;
        
        Pikmin* p_ptr = (Pikmin*) s_ptr->pikPtr;
        max_speed += p_ptr->getBaseSpeed() * p_ptr->getSpeedMultiplier();
    }
    max_speed /= curNCarriers;
    
    //If the object has all carriers, the Pikmin move as fast
    //as possible, which looks bad, since they're not jogging,
    //they're carrying. Let's add a penalty for the weight...
    max_speed *= (1 - game.config.carrying.speedWeightMult * m->type->weight);
    //...and a global carrying speed penalty.
    max_speed *= game.config.carrying.speedMaxMult;
    
    //The closer the mob is to having full carriers,
    //the closer to the max speed we get.
    //The speed goes from carrying_speed_base_mult (0 carriers)
    //to max_speed (all carriers).
    return
        max_speed * (
            game.config.carrying.speedBaseMult +
            (curNCarriers / (float) spotInfo.size()) *
            (1 - game.config.carrying.speedBaseMult)
        );
}


/**
 * @brief Returns true if no spot is reserved or used. False otherwise.
 *
 * @return Whether it is empty.
 */
bool CarryInfo::isEmpty() const {
    for(size_t s = 0; s < spotInfo.size(); s++) {
        if(spotInfo[s].state != CARRY_SPOT_STATE_FREE) return false;
    }
    return true;
}


/**
 * @brief Returns true if all spots are reserved. False otherwise.
 *
 * @return Whether it is full.
 */
bool CarryInfo::isFull() const {
    for(size_t s = 0; s < spotInfo.size(); s++) {
        if(spotInfo[s].state == CARRY_SPOT_STATE_FREE) return false;
    }
    return true;
}


/**
 * @brief Rotates all points in the struct, making it so spot 0 faces
 * the specified angle away from the mob.
 * This is useful when the first Pikmin is coming, to make the first carry
 * spot be closer to that Pikmin.
 *
 * @param angle Angle to rotate to.
 */
void CarryInfo::rotatePoints(float angle) {
    for(size_t s = 0; s < spotInfo.size(); s++) {
        float s_angle = angle + (TAU / m->type->maxCarriers * s);
        Point p(
            cos(s_angle) *
            (m->radius + game.config.pikmin.standardRadius),
            sin(s_angle) *
            (m->radius + game.config.pikmin.standardRadius)
        );
        spotInfo[s].pos = p;
    }
}


/**
 * @brief Constructs a new circling info struct object.
 *
 * @param m Mob this circling info struct belongs to.
 */
CirclingInfo::CirclingInfo(Mob* m) :
    m(m) {
    
}


/**
 * @brief Constructs a new delivery info struct object.
 */
DeliveryInfo::DeliveryInfo() :
    color(game.config.aestheticGen.carryingColorMove) {
}


/**
 * @brief Constructs a new group info struct object.
 *
 * @param leader_ptr Mob this group info struct belongs to.
 */
Group::Group(Mob* leader_ptr) :
    anchor(leader_ptr->pos),
    transform(game.identityTransform) {
}


/**
 * @brief Sets the standby group member type to the next available one,
 * or nullptr if none.
 *
 * @param move_backwards If true, go through the list backwards.
 * @return Whether it succeeded.
 */
bool Group::changeStandbyType(bool move_backwards) {
    return getNextStandbyType(move_backwards, &curStandbyType);
}


/**
 * @brief Changes to a different standby subgroup type in case there are no more
 * Pikmin of the current one. Or to no type.
 */
void Group::changeStandbyTypeIfNeeded() {
    for(size_t m = 0; m < members.size(); m++) {
        if(members[m]->subgroupTypePtr == curStandbyType) {
            //Never mind, there is a member of this subgroup type.
            return;
        }
    }
    //No members of the current type? Switch to the next.
    changeStandbyType(false);
}


/**
 * @brief Returns how many members of the given type exist in the group.
 *
 * @param type Type to check.
 * @return The amount.
 */
size_t Group::getAmountByType(const MobType* type) const {
    size_t amount = 0;
    for(size_t m = 0; m < members.size(); m++) {
        if(members[m]->type == type) {
            amount++;
        }
    }
    return amount;
}


/**
 * @brief Returns the average position of the members.
 *
 * @return The average position.
 */
Point Group::getAverageMemberPos() const {
    Point avg;
    for(size_t m = 0; m < members.size(); m++) {
        avg += members[m]->pos;
    }
    return avg / members.size();
}


/**
 * @brief Returns a list of hazards to which all of a leader's group mobs
 * are invulnerable.
 *
 * @param include_leader If not nullptr, include the group leader mob.
 * @return The list of invulnerabilities.
 */
vector<Hazard*> Group::getGroupInvulnerabilities(
    Mob* include_leader
) const {
    //Get all types to save on the amount of hazard checks.
    unordered_set<MobType*> member_types;
    for(size_t m = 0; m < members.size(); m++) {
        Mob* member_ptr = members[m];
        if(!member_ptr) continue;
        member_types.insert(member_ptr->type);
    }
    
    if(include_leader) member_types.insert(include_leader->type);
    
    return getMobTypeListInvulnerabilities(member_types);
}


/**
 * @brief Returns the next available standby group member type, or nullptr if none.
 *
 * @param move_backwards If true, go through the list backwards.
 * @param new_type The new type is returned here.
 * @return Whether it succeeded.
 */
bool Group::getNextStandbyType(
    bool move_backwards, SubgroupType** new_type
) {

    if(members.empty()) {
        *new_type = nullptr;
        return true;
    }
    
    bool success = false;
    SubgroupType* starting_type = curStandbyType;
    SubgroupType* final_type = curStandbyType;
    if(!starting_type) {
        starting_type =
            game.states.gameplay->subgroupTypes.getFirstType();
    }
    SubgroupType* scanning_type = starting_type;
    SubgroupType* leader_subgroup_type =
        game.states.gameplay->subgroupTypes.getType(
            SUBGROUP_TYPE_CATEGORY_LEADER
        );
        
    if(move_backwards) {
        scanning_type =
            game.states.gameplay->subgroupTypes.getPrevType(
                scanning_type
            );
    } else {
        scanning_type =
            game.states.gameplay->subgroupTypes.getNextType(
                scanning_type
            );
    }
    while(scanning_type != starting_type && !success) {
        //For each type, let's check if there's any group member that matches.
        if(
            scanning_type == leader_subgroup_type &&
            !game.config.rules.canThrowLeaders
        ) {
            //If this is a leader, and leaders cannot be thrown, skip.
        } else {
            for(size_t m = 0; m < members.size(); m++) {
                if(members[m]->subgroupTypePtr == scanning_type) {
                    final_type = scanning_type;
                    success = true;
                    break;
                }
            }
        }
        
        if(move_backwards) {
            scanning_type =
                game.states.gameplay->subgroupTypes.getPrevType(
                    scanning_type
                );
        } else {
            scanning_type =
                game.states.gameplay->subgroupTypes.getNextType(
                    scanning_type
                );
        }
    }
    
    *new_type = final_type;
    return success;
}


/**
 * @brief Returns a point's offset from the anchor,
 * given the current group transformation.
 *
 * @param spot_idx Index of the spot to check.
 * @return The offset.
 */
Point Group::getSpotOffset(size_t spot_idx) const {
    Point res = spots[spot_idx].pos;
    al_transform_coordinates(&transform, &res.x, &res.y);
    return res;
}


/**
 * @brief (Re-)Initializes the group spots. This resizes it to the current
 * number of group members. Any old group members are moved to the appropriate
 * new spot.
 *
 * @param affected_mob_ptr If this initialization is because a new mob entered
 * or left the group, this should point to said mob.
 */
void Group::initSpots(Mob* affected_mob_ptr) {
    if(members.empty()) {
        spots.clear();
        radius = 0;
        return;
    }
    
    //First, backup the old mob indexes.
    vector<Mob*> old_mobs;
    old_mobs.resize(spots.size());
    for(size_t m = 0; m < spots.size(); m++) {
        old_mobs[m] = spots[m].mobPtr;
    }
    
    //Now, rebuild the spots. Let's draw wheels from the center, for now.
    
    /**
     * @brief Initial spot.
     */
    struct AlphaSpot {
    
        //--- Members ---
        
        //Position of the spot.
        Point pos;
        
        //How far away it is from the rightmost spot.
        Distance distance_to_rightmost;
        
        
        //--- Function definitions ---
        
        /**
         * @brief Constructs a new alpha spot object.
         *
         * @param p The position.
         */
        explicit AlphaSpot(const Point &p) :
            pos(p) { }
            
    };
    
    vector<AlphaSpot> alpha_spots;
    size_t current_wheel = 1;
    radius = game.config.pikmin.standardRadius;
    
    //Center spot first.
    alpha_spots.push_back(AlphaSpot(Point()));
    
    while(alpha_spots.size() < members.size()) {
    
        //First, calculate how far the center
        //of these spots are from the central spot.
        float dist_from_center =
            game.config.pikmin.standardRadius * current_wheel + //Spots.
            MOB::GROUP_SPOT_INTERVAL * current_wheel; //Interval between spots.
            
        /* Now we need to figure out what's the angular distance
         * between each spot. For that, we need the actual diameter
         * (distance from one point to the other),
         * and the central distance, which is distance between the center
         * and the middle of two spots.
         *
         * We can get the middle distance because we know the actual diameter,
         * which should be the size of a Pikmin and one interval unit,
         * and we know the distance from one spot to the center.
         */
        float actual_diameter =
            game.config.pikmin.standardRadius * 2.0 + MOB::GROUP_SPOT_INTERVAL;
            
        //Just calculate the remaining side of the triangle, now that we know
        //the hypotenuse and the actual diameter (one side of the triangle).
        float middle_distance =
            sqrt(
                (dist_from_center * dist_from_center) -
                (actual_diameter * 0.5 * actual_diameter * 0.5)
            );
            
        //Now, get the angular distance.
        float angular_dist =
            atan2(actual_diameter, middle_distance * 2.0f) * 2.0;
            
        //Finally, we can calculate where the other spots are.
        size_t n_spots_on_wheel = floor(TAU / angular_dist);
        //Get a better angle. One that can evenly distribute the spots.
        float angle = TAU / n_spots_on_wheel;
        
        for(unsigned s = 0; s < n_spots_on_wheel; s++) {
            alpha_spots.push_back(
                AlphaSpot(
                    Point(
                        dist_from_center * cos(angle * s) +
                        game.rng.f(
                            -MOB::GROUP_SPOT_MAX_DEVIATION,
                            MOB::GROUP_SPOT_MAX_DEVIATION
                        ),
                        dist_from_center * sin(angle * s) +
                        game.rng.f(
                            -MOB::GROUP_SPOT_MAX_DEVIATION,
                            MOB::GROUP_SPOT_MAX_DEVIATION
                        )
                    )
                )
            );
        }
        
        current_wheel++;
        radius = dist_from_center;
    }
    
    //Now, given all of these points, create our final spot vector,
    //with the rightmost points coming first.
    
    //Start by sorting the points.
    for(size_t a = 0; a < alpha_spots.size(); a++) {
        alpha_spots[a].distance_to_rightmost =
            Distance(
                alpha_spots[a].pos,
                Point(radius, 0)
            );
    }
    
    std::sort(
        alpha_spots.begin(), alpha_spots.end(),
    [] (const AlphaSpot & a1, const AlphaSpot & a2) -> bool {
        return a1.distance_to_rightmost < a2.distance_to_rightmost;
    }
    );
    
    //Finally, create the group spots.
    spots.clear();
    spots.resize(members.size(), GroupSpot());
    for(size_t s = 0; s < members.size(); s++) {
        spots[s] =
            GroupSpot(
                Point(
                    alpha_spots[s].pos.x - radius,
                    alpha_spots[s].pos.y
                ),
                nullptr
            );
    }
    
    //Pass the old mobs over.
    if(old_mobs.size() < spots.size()) {
        for(size_t m = 0; m < old_mobs.size(); m++) {
            spots[m].mobPtr = old_mobs[m];
            spots[m].mobPtr->groupSpotIdx = m;
        }
        spots[old_mobs.size()].mobPtr = affected_mob_ptr;
        affected_mob_ptr->groupSpotIdx = old_mobs.size();
        
    } else if(old_mobs.size() > spots.size()) {
        for(size_t m = 0, s = 0; m < old_mobs.size(); m++) {
            if(old_mobs[m] == affected_mob_ptr) {
                old_mobs[m]->groupSpotIdx = INVALID;
                continue;
            }
            spots[s].mobPtr = old_mobs[m];
            spots[s].mobPtr->groupSpotIdx = s;
            s++;
        }
        
    } else {
        for(size_t m = 0; m < old_mobs.size(); m++) {
            spots[m].mobPtr = old_mobs[m];
            spots[m].mobPtr->groupSpotIdx = m;
        }
    }
}


/**
 * @brief Assigns each mob a new spot, given how close each one of them is to
 * each spot.
 */
void Group::reassignSpots() {
    for(size_t m = 0; m < members.size(); m++) {
        members[m]->groupSpotIdx = INVALID;
    }
    
    for(size_t s = 0; s < spots.size(); s++) {
        Point spot_pos = anchor + getSpotOffset(s);
        Mob* closest_mob = nullptr;
        Distance closest_dist;
        
        for(size_t m = 0; m < members.size(); m++) {
            Mob* m_ptr = members[m];
            if(m_ptr->groupSpotIdx != INVALID) continue;
            
            Distance d(m_ptr->pos, spot_pos);
            
            if(!closest_mob || d < closest_dist) {
                closest_mob = m_ptr;
                closest_dist = d;
            }
        }
        
        if(closest_mob) closest_mob->groupSpotIdx = s;
    }
}


/**
 * @brief Sorts the group with the specified type at the front, and the
 * other types (in order) behind.
 *
 * @param leading_type The subgroup type that will be at the front of
 * the group.
 */
void Group::sort(SubgroupType* leading_type) {

    for(size_t m = 0; m < members.size(); m++) {
        members[m]->groupSpotIdx = INVALID;
    }
    
    SubgroupType* cur_type = leading_type;
    size_t cur_spot = 0;
    
    while(cur_spot != spots.size()) {
        Point spot_pos = anchor + getSpotOffset(cur_spot);
        
        //Find the member closest to this spot.
        Mob* closest_member = nullptr;
        Distance closest_dist;
        for(size_t m = 0; m < members.size(); m++) {
            Mob* m_ptr = members[m];
            if(m_ptr->subgroupTypePtr != cur_type) continue;
            if(m_ptr->groupSpotIdx != INVALID) continue;
            
            Distance d(m_ptr->pos, spot_pos);
            
            if(!closest_member || d < closest_dist) {
                closest_member = m_ptr;
                closest_dist = d;
            }
            
        }
        
        if(!closest_member) {
            //There are no more members of the current type left!
            //Next type.
            cur_type =
                game.states.gameplay->subgroupTypes.getNextType(cur_type);
        } else {
            spots[cur_spot].mobPtr = closest_member;
            closest_member->groupSpotIdx = cur_spot;
            cur_spot++;
        }
        
    }
    
}


/**
 * @brief Clears the information.
 */
void HoldInfo::clear() {
    m = nullptr;
    hitboxIdx = INVALID;
    offsetDist = 0;
    offsetAngle = 0;
    verticalDist = 0;
}


/**
 * @brief Returns the final coordinates this mob should be at.
 *
 * @param out_z The Z coordinate is returned here.
 * @return The (X and Y) coordinates.
 */
Point HoldInfo::getFinalPos(float* out_z) const {
    if(!m) return Point();
    
    Hitbox* h_ptr = nullptr;
    if(hitboxIdx != INVALID) {
        h_ptr = m->getHitbox(hitboxIdx);
    }
    
    Point final_pos;
    
    if(h_ptr) {
        //Hitbox.
        final_pos = rotatePoint(h_ptr->pos, m->angle);
        final_pos += m->pos;
        
        final_pos +=
            angleToCoordinates(
                offsetAngle + m->angle,
                offsetDist * h_ptr->radius
            );
        *out_z = m->z + h_ptr->z + (h_ptr->height * verticalDist);
    } else {
        //Body center.
        final_pos = m->pos;
        
        final_pos +=
            angleToCoordinates(
                offsetAngle + m->angle,
                offsetDist * m->radius
            );
        *out_z = m->z + (m->height * verticalDist);
    }
    
    return final_pos;
}


/**
 * @brief Constructs a new parent info struct object.
 *
 * @param m The parent mob.
 */
Parent::Parent(Mob* m) :
    m(m) {
    
}


/**
 * @brief Constructs a new path info struct object.
 *
 * @param m Mob this path info struct belongs to.
 * @param settings Settings about how the path should be followed.
 */
Path::Path(
    Mob* m,
    const PathFollowSettings &settings
) :
    m(m),
    settings(settings) {
    
    result =
        getPath(
            m->pos, settings.targetPoint, settings,
            path, nullptr, nullptr, nullptr
        );
}


/**
 * @brief Calculates whether or not the way forward is currently blocked.
 *
 * @param out_reason If not nullptr, the reason is returned here.
 * @return Whether there is a blockage.
 */
bool Path::checkBlockage(PATH_BLOCK_REASON* out_reason) {
    if(
        path.size() >= 2 &&
        cur_path_stop_idx > 0 &&
        cur_path_stop_idx < path.size()
    ) {
        PathStop* cur_stop = path[cur_path_stop_idx - 1];
        PathStop* next_stop = path[cur_path_stop_idx];
        
        return
            !canTraversePathLink(
                cur_stop->get_link(next_stop),
                settings,
                out_reason
            );
    }
    
    if(out_reason) *out_reason = PATH_BLOCK_REASON_NONE;
    return false;
}


/**
 * @brief Constructs a new Pikmin nest struct object.
 *
 * @param m_ptr Nest mob responsible.
 * @param type Type of nest.
 */
PikminNest::PikminNest(
    Mob* m_ptr, PikminNestType* type
) :
    m_ptr(m_ptr),
    nest_type(type) {
    
    for(size_t t = 0; t < nest_type->pik_types.size(); t++) {
        pikmin_inside.push_back(vector<size_t>(N_MATURITIES, 0));
        call_queue.push_back(0);
    }
}


/**
 * @brief Calls out a Pikmin from inside the nest, if possible.
 * Gives priority to the higher maturities.
 *
 * @param m_ptr Pointer to the nest mob.
 * @param type_idx Index of the Pikmin type, from the types this nest manages.
 * @return Whether a Pikmin spawned.
 */
bool PikminNest::callPikmin(Mob* m_ptr, size_t type_idx) {
    if(
        game.states.gameplay->mobs.pikmin.size() >=
        game.config.rules.maxPikminInField
    ) {
        return false;
    }
    
    for(size_t m = 0; m < N_MATURITIES; m++) {
        //Let's check the maturities in reverse order.
        size_t cur_m = N_MATURITIES - m - 1;
        
        if(pikmin_inside[type_idx][cur_m] == 0) continue;
        
        //Spawn the Pikmin!
        //Update the Pikmin count.
        pikmin_inside[type_idx][cur_m]--;
        
        //Decide a leg to come out of.
        size_t leg_idx =
            game.rng.i(0, (int) (nest_type->leg_body_parts.size() / 2) - 1);
        size_t leg_hole_bp_idx =
            m_ptr->anim.animDb->findBodyPart(
                nest_type->leg_body_parts[leg_idx * 2]
            );
        size_t leg_foot_bp_idx =
            m_ptr->anim.animDb->findBodyPart(
                nest_type->leg_body_parts[leg_idx * 2 + 1]
            );
        Point spawn_coords =
            m_ptr->getHitbox(leg_hole_bp_idx)->getCurPos(
                m_ptr->pos, m_ptr->angle
            );
        float spawn_angle =
            getAngle(m_ptr->pos, spawn_coords);
            
        //Create the Pikmin.
        Pikmin* new_pikmin =
            (Pikmin*)
            createMob(
                game.mobCategories.get(MOB_CATEGORY_PIKMIN),
                spawn_coords, nest_type->pik_types[type_idx], spawn_angle,
                "maturity=" + i2s(cur_m)
            );
            
        //Set its data to start sliding.
        new_pikmin->fsm.setState(PIKMIN_STATE_LEAVING_ONION, (void*) this);
        vector<size_t> checkpoints;
        checkpoints.push_back(leg_hole_bp_idx);
        checkpoints.push_back(leg_foot_bp_idx);
        new_pikmin->trackInfo =
            new TrackRideInfo(
            m_ptr, checkpoints, nest_type->pikmin_exit_speed
        );
        new_pikmin->leaderToReturnTo = calling_leader;
        
        return true;
    }
    
    return false;
}


/**
 * @brief Returns how many Pikmin of the given type exist inside.
 *
 * @param type Type to check.
 * @return The amount.
 */
size_t PikminNest::getAmountByType(const PikminType* type) {
    size_t amount = 0;
    for(size_t t = 0; t < nest_type->pik_types.size(); t++) {
        if(nest_type->pik_types[t] == type) {
            for(size_t m = 0; m < N_MATURITIES; m++) {
                amount += pikmin_inside[t][m];
            }
            break;
        }
    }
    return amount;
}


/**
 * @brief Reads the provided script variables, if any, and does stuff with
 * any that are related to nests.
 *
 * @param svr Script var reader to use.
 */
void PikminNest::readScriptVars(const ScriptVarReader &svr) {
    string pikmin_inside_var;
    
    if(svr.get("pikmin_inside", pikmin_inside_var)) {
        vector<string> pikmin_inside_vars = split(pikmin_inside_var);
        size_t word = 0;
        
        for(size_t t = 0; t < nest_type->pik_types.size(); t++) {
            for(size_t m = 0; m < N_MATURITIES; m++) {
                if(word < pikmin_inside_vars.size()) {
                    pikmin_inside[t][m] = s2i(pikmin_inside_vars[word]);
                    word++;
                }
            }
        }
    }
}


/**
 * @brief Requests that Pikmin of the given type get called out.
 *
 * @param type_idx Index of the type of Pikmin to call out, from the
 * nest's types.
 * @param amount How many to call out.
 * @param l_ptr Leader responsible.
 */
void PikminNest::requestPikmin(
    size_t type_idx, size_t amount, Leader* l_ptr
) {
    call_queue[type_idx] += amount;
    next_call_time = MOB::PIKMIN_NEST_CALL_INTERVAL;
    calling_leader = l_ptr;
}


/**
 * @brief Stores the given Pikmin inside the nest. This basically deletes the
 * Pikmin and updates the amount inside the nest.
 *
 * @param p_ptr Pikmin to store.
 */
void PikminNest::storePikmin(Pikmin* p_ptr) {
    for(size_t t = 0; t < nest_type->pik_types.size(); t++) {
        if(p_ptr->type == nest_type->pik_types[t]) {
            pikmin_inside[t][p_ptr->maturity]++;
            break;
        }
    }
    
    p_ptr->toDelete = true;
}


/**
 * @brief Ticks time by one frame of logic.
 *
 * @param delta_t How long the frame's tick is, in seconds.
 */
void PikminNest::tick(float delta_t) {
    if(calling_leader && calling_leader->toDelete) {
        calling_leader = nullptr;
    }
    
    //Call out Pikmin, if the timer agrees.
    if(next_call_time > 0.0f) {
        next_call_time -= delta_t;
    }
    
    while(next_call_time < 0.0f) {
        size_t best_type = INVALID;
        size_t best_type_amount = 0;
        
        for(size_t t = 0; t < nest_type->pik_types.size(); t++) {
            if(call_queue[t] == 0) continue;
            if(call_queue[t] > best_type_amount) {
                best_type = t;
                best_type_amount = call_queue[t];
            }
        }
        
        if(best_type != INVALID) {
            //Try to call a Pikmin.
            if(callPikmin(m_ptr, best_type)) {
                //Call successful! Update the queue.
                call_queue[best_type]--;
            } else {
                //Call failed. Forget the player's request.
                call_queue[best_type] = 0;
            }
        }
        
        next_call_time += MOB::PIKMIN_NEST_CALL_INTERVAL;
    }
}


/**
 * @brief Loads nest-related properties from a data file.
 *
 * @param file File to read from.
 */
void PikminNestType::loadProperties(
    DataNode* file
) {
    ReaderSetter rs(file);
    
    string pik_types_str;
    string legs_str;
    DataNode* pik_types_node = nullptr;
    DataNode* legs_node = nullptr;
    
    rs.set("leg_body_parts", legs_str, &legs_node);
    rs.set("pikmin_types", pik_types_str, &pik_types_node);
    rs.set("pikmin_enter_speed", pikmin_enter_speed);
    rs.set("pikmin_exit_speed", pikmin_exit_speed);
    
    leg_body_parts = semicolonListToVector(legs_str);
    if(pik_types_node && leg_body_parts.empty()) {
        game.errors.report(
            "A nest-like object type needs a list of leg body parts!",
            file
        );
    } else if(legs_node && leg_body_parts.size() % 2 == 1) {
        game.errors.report(
            "A nest-like object type needs an even number of leg body parts!",
            legs_node
        );
    }
    
    vector<string> pik_types_strs = semicolonListToVector(pik_types_str);
    for(size_t t = 0; t < pik_types_strs.size(); t++) {
        string &str = pik_types_strs[t];
        if(
            game.content.mobTypes.list.pikmin.find(str) ==
            game.content.mobTypes.list.pikmin.end()
        ) {
            game.errors.report(
                "Unknown Pikmin type \"" + str + "\"!",
                pik_types_node
            );
        } else {
            pik_types.push_back(game.content.mobTypes.list.pikmin[str]);
        }
    }
}


/**
 * @brief Constructs a new track ride info struct object.
 *
 * @param m Mob this track info struct belongs to.
 * @param checkpoints List of checkpoints (body part indexes) to cross.
 * @param ride_speed Speed to ride at, in ratio per second.
 */
TrackRideInfo::TrackRideInfo(
    Mob* m, const vector<size_t> &checkpoints, float ride_speed
) :
    m(m),
    checkpoints(checkpoints),
    ride_speed(ride_speed) {
    
}


/**
 * @brief Calculates the maximum physical span that a mob can ever reach
 * from its center.
 *
 * @param radius The mob's radius.
 * @param anim_hitbox_span Maximum span of its hitboxes data.
 * @param rectangular_dim Rectangular dimensions of the mob, if any.
 * @return The span.
 */
float calculateMobPhysicalSpan(
    float radius, float anim_hitbox_span,
    const Point &rectangular_dim
) {
    float final_span = std::max(radius, anim_hitbox_span);
    
    if(rectangular_dim.x != 0) {
        final_span =
            std::max(
                final_span, Distance(Point(0.0f), rectangular_dim / 2.0).toFloat()
            );
    }
    
    return final_span;
}


/**
 * @brief Creates a mob, adding it to the corresponding vectors.
 *
 * @param category The category the new mob belongs to.
 * @param pos Initial position.
 * @param type Type of the new mob.
 * @param angle Initial facing angle.
 * @param vars Script variables.
 * @param code_after_creation Code to run right after the mob is created,
 * if any. This is run before any scripting takes place.
 * @param first_state_override If this is INVALID, use the first state
 * index defined in the mob's FSM struct, or the standard first state index.
 * Otherwise, use this.
 * @return The new mob.
 */
Mob* createMob(
    MobCategory* category, const Point &pos, MobType* type,
    float angle, const string &vars,
    std::function<void(Mob*)> code_after_creation,
    size_t first_state_override
) {
    Mob* m_ptr = category->createMob(pos, type, angle);
    
    if(m_ptr->type->walkable) {
        game.states.gameplay->mobs.walkables.push_back(m_ptr);
    }
    
    if(code_after_creation) {
        code_after_creation(m_ptr);
    }
    
    for(size_t a = 0; a < type->initActions.size(); a++) {
        type->initActions[a]->run(m_ptr, nullptr, nullptr);
    }
    
    if(!vars.empty()) {
        map<string, string> vars_map = getVarMap(vars);
        ScriptVarReader svr(vars_map);
        
        m_ptr->readScriptVars(svr);
        
        for(auto &v : vars_map) {
            m_ptr->vars[v.first] = v.second;
        }
    }
    
    if(
        !m_ptr->fsm.setState(
            first_state_override != INVALID ?
            first_state_override :
            m_ptr->fsm.firstStateOverride != INVALID ?
            m_ptr->fsm.firstStateOverride :
            type->firstStateIdx
        )
    ) {
        //If something went wrong, give it some dummy state.
        m_ptr->fsm.curState = game.dummyMobState;
    };
    
    for(size_t c = 0; c < type->children.size(); c++) {
        MobType::Child* child_info =
            &type->children[c];
        MobType::SpawnInfo* spawn_info =
            getSpawnInfoFromChildInfo(m_ptr->type, &type->children[c]);
            
        if(!spawn_info) {
            game.errors.report(
                "Object \"" + type->name + "\" tried to spawn a child with the "
                "spawn name \"" + child_info->spawnName + "\", but that name "
                "does not exist in the list of spawn data!"
            );
            continue;
        }
        
        Mob* new_mob = m_ptr->spawn(spawn_info);
        
        if(!new_mob) continue;
        
        Parent* p_info = new Parent(m_ptr);
        new_mob->parent = p_info;
        p_info->handle_damage = child_info->handleDamage;
        p_info->relay_damage = child_info->relayDamage;
        p_info->handle_events = child_info->handleEvents;
        p_info->relay_events = child_info->relayEvents;
        p_info->handle_statuses = child_info->handleStatuses;
        p_info->relay_statuses = child_info->relayStatuses;
        if(!child_info->limbAnimName.empty()) {
            p_info->limb_anim.animDb = m_ptr->anim.animDb;
            Animation* anim_to_use = nullptr;
            for(size_t a = 0; a < m_ptr->anim.animDb->animations.size(); a++) {
                if(
                    m_ptr->anim.animDb->animations[a]->name ==
                    child_info->limbAnimName
                ) {
                    anim_to_use = m_ptr->anim.animDb->animations[a];
                }
            }
            
            if(anim_to_use) {
                p_info->limb_anim.curAnim = anim_to_use;
                p_info->limb_anim.toStart();
            } else {
                game.errors.report(
                    "Object \"" + new_mob->type->name + "\", child object of "
                    "object \"" + type->name + "\", tried to use animation \"" +
                    child_info->limbAnimName + "\" for a limb, but that "
                    "animation doesn't exist in the parent object's animations!"
                );
            }
        }
        p_info->limb_thickness = child_info->limbThickness;
        p_info->limb_parent_body_part =
            type->animDb->findBodyPart(child_info->limbParentBodyPart);
        p_info->limb_parent_offset = child_info->limbParentOffset;
        p_info->limb_child_body_part =
            new_mob->type->animDb->findBodyPart(
                child_info->limbChildBodyPart
            );
        p_info->limb_child_offset = child_info->limbChildOffset;
        p_info->limb_draw_method = child_info->limbDrawMethod;
        
        if(child_info->parentHolds) {
            m_ptr->hold(
                new_mob,
                type->animDb->findBodyPart(child_info->holdBodyPart),
                child_info->holdOffsetDist,
                child_info->holdOffsetAngle,
                child_info->holdOffsetVertDist,
                false,
                child_info->holdRotationMethod
            );
        }
    }
    
    game.states.gameplay->mobs.all.push_back(m_ptr);
    return m_ptr;
}


/**
 * @brief Deletes a mob from the relevant vectors.
 *
 * It's always removed from the vector of mobs, but it's
 * also removed from the vector of Pikmin if it's a Pikmin,
 * leaders if it's a leader, etc.
 *
 * @param m_ptr The mob to delete.
 * @param complete_destruction If true, don't bother removing it from groups
 * and such, since everything is going to be destroyed.
 */
void deleteMob(Mob* m_ptr, bool complete_destruction) {
    if(game.makerTools.infoLock == m_ptr) game.makerTools.infoLock = nullptr;
    
    if(!complete_destruction) {
        m_ptr->leaveGroup();
        
        for(size_t m = 0; m < game.states.gameplay->mobs.all.size(); m++) {
            Mob* m2_ptr = game.states.gameplay->mobs.all[m];
            if(m2_ptr->focusedMob == m_ptr) {
                m2_ptr->fsm.runEvent(MOB_EV_FOCUSED_MOB_UNAVAILABLE);
                m2_ptr->fsm.runEvent(MOB_EV_FOCUS_OFF_REACH);
                m2_ptr->fsm.runEvent(MOB_EV_FOCUS_DIED);
                m2_ptr->focusedMob = nullptr;
            }
            if(m2_ptr->parent && m2_ptr->parent->m == m_ptr) {
                delete m2_ptr->parent;
                m2_ptr->parent = nullptr;
                m2_ptr->toDelete = true;
            }
            for(size_t f = 0; f < m2_ptr->focusedMobMemory.size(); f++) {
                if(m2_ptr->focusedMobMemory[f] == m_ptr) {
                    m2_ptr->focusedMobMemory[f] = nullptr;
                }
            }
            for(size_t c = 0; c < m2_ptr->chompingMobs.size(); c++) {
                if(m2_ptr->chompingMobs[c] == m_ptr) {
                    m2_ptr->chompingMobs[c] = nullptr;
                }
            }
            for(size_t l = 0; l < m2_ptr->links.size(); l++) {
                if(m2_ptr->links[l] == m_ptr) {
                    m2_ptr->links[l] = nullptr;
                }
            }
            if(m2_ptr->storedInsideAnother == m_ptr) {
                m_ptr->release(m2_ptr);
                m2_ptr->storedInsideAnother = nullptr;
            }
            if(m2_ptr->carryInfo) {
                for(
                    size_t c = 0; c < m2_ptr->carryInfo->spotInfo.size(); c++
                ) {
                    if(m2_ptr->carryInfo->spotInfo[c].pikPtr == m_ptr) {
                        m2_ptr->carryInfo->spotInfo[c].pikPtr =
                            nullptr;
                        m2_ptr->carryInfo->spotInfo[c].state =
                            CARRY_SPOT_STATE_FREE;
                    }
                }
            }
        }
        
        if(m_ptr->holder.m) {
            m_ptr->holder.m->release(m_ptr);
        }
        
        while(!m_ptr->holding.empty()) {
            m_ptr->release(m_ptr->holding[0]);
        }
        
        m_ptr->setCanBlockPaths(false);
        
        m_ptr->fsm.setState(INVALID);
    }
    
    game.audio.handleMobDeletion(m_ptr);
    
    m_ptr->type->category->eraseMob(m_ptr);
    game.states.gameplay->mobs.all.erase(
        find(
            game.states.gameplay->mobs.all.begin(),
            game.states.gameplay->mobs.all.end(),
            m_ptr
        )
    );
    if(m_ptr->type->walkable) {
        game.states.gameplay->mobs.walkables.erase(
            find(
                game.states.gameplay->mobs.walkables.begin(),
                game.states.gameplay->mobs.walkables.end(),
                m_ptr
            )
        );
    }
    
    delete m_ptr;
}


/**
 * @brief Returns a string that describes the given mob. Used in error messages
 * where you have to indicate a specific mob in the area.
 *
 * @param m The mob.
 * @return The string.
 */
string getErrorMessageMobInfo(Mob* m) {
    return
        "type \"" + m->type->name + "\", coordinates " +
        p2s(m->pos) + ", area \"" + game.curAreaData->name + "\"";
}


/**
 * @brief Returns a list of hazards to which all mob types given
 * are invulnerable.
 *
 * @param types Mob types to check.
 * @return The invulnerabilities.
 */
vector<Hazard*> getMobTypeListInvulnerabilities(
    const unordered_set<MobType*> &types
) {
    //Count how many types are invulnerable to each detected hazard.
    map<Hazard*, size_t> inv_instances;
    for(auto &t : types) {
        for(auto &h : t->hazardVulnerabilities) {
            if(h.second.effectMult == 0.0f) {
                inv_instances[h.first]++;
            }
        }
    }
    
    //Only accept those that ALL types are invulnerable to.
    vector<Hazard*> invulnerabilities;
    for(auto &i : inv_instances) {
        if(i.second == types.size()) {
            invulnerabilities.push_back(i.first);
        }
    }
    
    return invulnerabilities;
}


/**
 * @brief Given a child info block, returns the spawn info block that matches.
 *
 * @param type Mob type that owns the children and spawn blocks.
 * @param child_info Child info to check.
 * @return The spawn info, or nullptr if not found.
 */
MobType::SpawnInfo* getSpawnInfoFromChildInfo(
    MobType* type, const MobType::Child* child_info
) {
    for(size_t s = 0; s < type->spawns.size(); s++) {
        if(type->spawns[s].name == child_info->spawnName) {
            return &type->spawns[s];
        }
    }
    return nullptr;
}


/**
 * @brief Returns whether a given mob is in reach or out of reach of another,
 * given the positional and reach data.
 *
 * @param reach_t_ptr Pointer to the reach information.
 * @param dist_between Distance between the two mobs.
 * @param angle_diff Angle difference between the two mobs.
 * @return Whether it's in reach.
 */
bool isMobInReach(
    MobType::Reach* reach_t_ptr, const Distance &dist_between, float angle_diff
) {
    bool in_reach =
        (
            dist_between <= reach_t_ptr->radius1 &&
            angle_diff <= reach_t_ptr->angle1 / 2.0
        );
    if(in_reach) return true;
    in_reach =
        (
            dist_between <= reach_t_ptr->radius2 &&
            angle_diff <= reach_t_ptr->angle2 / 2.0
        );
    return in_reach;
}


/**
 * @brief Converts a string to the numeric representation of a mob target type.
 *
 * @param type_str Text representation of the target type.
 * @return The type, or INVALID if invalid.
 */
MOB_TARGET_FLAG stringToMobTargetType(const string &type_str) {
    if(type_str == "none") {
        return MOB_TARGET_FLAG_NONE;
    } else if(type_str == "player") {
        return MOB_TARGET_FLAG_PLAYER;
    } else if(type_str == "enemy") {
        return MOB_TARGET_FLAG_ENEMY;
    } else if(type_str == "weak_plain_obstacle") {
        return MOB_TARGET_FLAG_WEAK_PLAIN_OBSTACLE;
    } else if(type_str == "strong_plain_obstacle") {
        return MOB_TARGET_FLAG_STRONG_PLAIN_OBSTACLE;
    } else if(type_str == "pikmin_obstacle") {
        return MOB_TARGET_FLAG_PIKMIN_OBSTACLE;
    } else if(type_str == "explodable") {
        return MOB_TARGET_FLAG_EXPLODABLE;
    } else if(type_str == "explodable_pikmin_obstacle") {
        return MOB_TARGET_FLAG_EXPLODABLE_PIKMIN_OBSTACLE;
    } else if(type_str == "fragile") {
        return MOB_TARGET_FLAG_FRAGILE;
    }
    return (MOB_TARGET_FLAG) INVALID;
}


/**
 * @brief Converts a string to the numeric representation of a team.
 *
 * @param team_str Text representation of the team.
 * @return The team, or INVALID if invalid.
 */
MOB_TEAM stringToTeamNr(const string &team_str) {
    for(size_t t = 0; t < N_MOB_TEAMS; t++) {
        if(team_str == game.teamInternalNames[t]) {
            return (MOB_TEAM) t;
        }
    }
    return (MOB_TEAM) INVALID;
}
