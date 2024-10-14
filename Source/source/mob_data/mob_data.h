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

#include <map>


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

    //- Vulnerabilities -

    //For every hazard, multiply damage taken by this much.
    map<hazard*, vulnerability_t> hazard_vulnerabilities;

    //What sort of spike damage it causes, if any.
    spike_damage_type* spike_damage = nullptr;

    //For every type of spike damage, multiply damage taken by this much.
    map<spike_damage_type*, vulnerability_t> spike_damage_vulnerabilities;

    //For every type of status, multiply damage taken by this much.
    map<status_type*, vulnerability_t> status_vulnerabilities;

};
