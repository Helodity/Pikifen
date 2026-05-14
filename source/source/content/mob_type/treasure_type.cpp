/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Treasure type class and treasure type-related functions.
 */

#include "treasure_type.hpp"

#include "../../core/misc_functions.hpp"
#include "../../util/string_utils.hpp"
#include "../mob/mob.hpp"
#include "../mob/treasure.hpp"
#include "../mob_script/gen_mob_fsm.hpp"
#include "../mob_script/treasure_fsm.hpp"


/**
 * @brief Constructs a new treasure type object.
 */
TreasureType::TreasureType() :
    MobType(MOB_CATEGORY_TREASURES) {
    
    targetType = MOB_TARGET_FLAG_NONE;
    
    TreasureFsm::createFsm(this);
}


/**
 * @brief Returns the vector of animation conversions.
 *
 * @return The vector.
 */
AnimConversionVector TreasureType::getAnimConversions() const {
    AnimConversionVector v;
    v.push_back(std::make_pair(MOB_TYPE::ANIM_IDLING, "idling"));
    return v;
}


/**
 * @brief Loads properties from a data file.
 *
 * @param file File to read from.
 */
void TreasureType::loadCatProperties(DataNode* file) {
    ReaderSetter tRS(file);
    
    tRS.set("points", points);
}
