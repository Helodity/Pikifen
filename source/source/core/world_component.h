/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Header for the world component class.
 */

#pragma once

#include <cstdio>
#include <functional>

struct Sector;
class Mob;
struct Particle;


/**
 * @brief Something that makes up the interactable game world and can be drawn.
 * This contains information about how it should be drawn.
 */
class WorldComponent {

public:

    //--- Members ---
    //Its Z coordinate.
    float z;
    
    //Index in the list of world components. Used for sorting.
    size_t idx;
    
    //The function that should be called to draw the component.
    std::function<void()> drawCallback;

    //--- Function declarations ---
    WorldComponent(float z, std::function<void()> drawCallback) :
    z(z),
    idx(0),
    drawCallback(drawCallback) 
    {}
};
