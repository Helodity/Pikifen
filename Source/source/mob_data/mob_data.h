/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Header for the mob data class and related functions
 */

#pragma once

#include <functional>
#include <map>

#include <allegro5/allegro.h>

#include "../animation.h"
#include "../audio.h"
#include "../const.h"
#include "../content.h"
#include "../misc_structs.h"
#include "../mob_categories/mob_category.h"
#include "../mob_script.h"
#include "../status.h"
#include "../libs/data_file.h"
#include "../mobs/mob_enums.h"
#include "../spike_damage.h"
#include "../utils/general_utils.h"

/**
 * @brief Contents of a mob type that can be altered on a per-mob basis
 */
struct mob_data {

public:
    //--- Members ---

    //-Physical Space-

    //Current radius.
    float radius = 0.0f;

    //Current height.
    float height = 0.0f;

    //Current rectangular dimensions.
    point rectangular_dim = point();

    //-Brain and behavior-

    //Maximum health.
    float max_health = 100.0f;
};
