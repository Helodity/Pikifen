/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Bouncer mob category class.
 */

#include <algorithm>

#include "bouncer_category.h"

#include "../../core/game.h"
#include "../mob/bouncer.h"


/**
 * @brief Constructs a new bouncer category object.
 */
BouncerCategory::BouncerCategory() :
    MobCategory(
        MOB_CATEGORY_BOUNCERS, "bouncer",
        "Bouncer", "Bouncers",
        "bouncers", al_map_rgb(192, 139, 204)
    ) {
    
}


/**
 * @brief Clears the list of registered types of bouncers.
 */
void BouncerCategory::clear_types() {
    for(auto &t : game.content.mob_types.list.bouncer) {
        delete t.second;
    }
    game.content.mob_types.list.bouncer.clear();
}


/**
 * @brief Creates a bouncer and adds it to the list of bouncers.
 *
 * @param pos Starting coordinates.
 * @param type Mob type.
 * @param angle Starting angle.
 * @return The created mob.
 */
Mob* BouncerCategory::create_mob(
    const Point &pos, MobType* type, float angle
) {
    Bouncer* m = new Bouncer(pos, (BouncerType*) type, angle);
    game.states.gameplay->mobs.bouncers.push_back(m);
    return m;
}


/**
 * @brief Creates a new, empty type of bouncer.
 *
 * @return The type.
 */
MobType* BouncerCategory::create_type() {
    return new BouncerType();
}


/**
 * @brief Clears a bouncer from the list of bouncers.
 *
 * @param m The mob to erase.
 */
void BouncerCategory::erase_mob(Mob* m) {
    game.states.gameplay->mobs.bouncers.erase(
        find(
            game.states.gameplay->mobs.bouncers.begin(),
            game.states.gameplay->mobs.bouncers.end(),
            (Bouncer*) m
        )
    );
}


/**
 * @brief Returns a type of bouncer given its internal name,
 * or nullptr on error.
 *
 * @param internal_name Internal name of the mob type to get.
 * @return The type, or nullptr on error.
 */
MobType* BouncerCategory::get_type(const string &internal_name) const {
    auto it = game.content.mob_types.list.bouncer.find(internal_name);
    if(it == game.content.mob_types.list.bouncer.end()) return nullptr;
    return it->second;
}


/**
 * @brief Returns all types of bouncer by internal name.
 *
 * @param list This list gets filled with the mob type internal names.
 */
void BouncerCategory::get_type_names(vector<string> &list) const {
    for(auto &t : game.content.mob_types.list.bouncer) {
        list.push_back(t.first);
    }
}


/**
 * @brief Registers a created type of bouncer.
 *
 * @param internal_name Internal name of the mob type.
 * @param type Mob type to register.
 */
void BouncerCategory::register_type(const string &internal_name, MobType* type) {
    game.content.mob_types.list.bouncer[internal_name] = (BouncerType*) type;
}
