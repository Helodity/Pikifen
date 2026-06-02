/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Header for the pellet type class and pellet type-related functions.
 */

#pragma once

#include "../../lib/data_file/data_file.hpp"
#include "mob_type.hpp"
#include "pikmin_type.hpp"


//Pellet object states.
enum PELLET_STATE {

    //Idle, waiting to move.
    PELLET_STATE_IDLE_WAITING,
    
    //Idle, moving.
    PELLET_STATE_IDLE_MOVING,
    
    //Idle, stuck.
    PELLET_STATE_IDLE_STUCK,
    
    //Idle, being thrown.
    PELLET_STATE_IDLE_THROWN,
    
    //Being delivered.
    PELLET_STATE_BEING_DELIVERED,
    
    //Total amount of pellet object states.
    N_PELLET_STATES,
    
};


/**
 * @brief A pellet type.
 *
 * Contains info on how many nutrients the Onion should receive,
 * depending on whether it matches the Pikmin type or not.
 */
class PelletType : public MobType {

public:

    //--- Public members ---
    
    //Type of Pikmin this pellet relates to.
    PikminType* pikType = nullptr;
    
    //Number on the pellet, and hence, its weight.
    size_t number = 0;
    
    //Family of nutrients it can receive. Onions that do not support this family
    //cannot accept this pellet.
    string nutrientFamily = "pellet";
    
    //Number of nutrients given out if the pellet's taken to a matching Onion.
    size_t matchNutrients = 0;
    
    //Number of nutrients given out if the pellet's taken to a non-matching Onion.
    size_t nonMatchNutrients = 0;
    
    //Bitmap to use to represent the number on the pellet.
    ALLEGRO_BITMAP* bmpNumber = nullptr;
    
    //Whether to draw the number on it.
    bool drawNumber = true;
    
    
    //--- Public function declarations ---
    
    PelletType();
    void loadCatProperties(DataNode* file) override;
    void loadCatResources(DataNode* file) override;
    AnimConversionVector getAnimConversions() const override;
    void unloadResources() override;
    
};
