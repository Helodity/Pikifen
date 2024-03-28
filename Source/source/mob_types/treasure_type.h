/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Header for the treasure type class and treasure type-related functions.
 */

#ifndef TREASURE_TYPE_INCLUDED
#define TREASURE_TYPE_INCLUDED

#include "../libs/data_file.h"
#include "mob_type.h"


//Treasure object states.
enum TREASURE_STATE {
    
    //Waiting.
    TREASURE_STATE_IDLE_WAITING,
    
    //Moving.
    TREASURE_STATE_IDLE_MOVING,
    
    //Stuck.
    TREASURE_STATE_IDLE_STUCK,
    
    //Thrown.
    TREASURE_STATE_IDLE_THROWN,
    
    //Being delivered.
    TREASURE_STATE_BEING_DELIVERED,
    
    //Total amount of treasure object states.
    N_TREASURE_STATES,
    
};


/**
 * @brief A type of treasure.
 * Although uncommon, there can be several treasures of the same type at once.
 * Like the "small red marble" treasure type in Pikmin 2; you can see multiple
 * treasures of that type in some Challenge Mode levels.
 */
class treasure_type : public mob_type {

public:
    
    //--- Members ---

    //How many points it is worth.
    size_t points = 10;
    

    //--- Function declarations ---
    
    treasure_type();
    void load_properties(data_node* file) override;
    anim_conversion_vector get_anim_conversions() const override;
    
};


#endif //ifndef TREASURE_TYPE_INCLUDED
