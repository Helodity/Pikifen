/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 *
 * === FILE DESCRIPTION ===
 * Header for the Inpution middleware's translator.
 * Please read the included readme file.
 */

#pragma once

#include <cstdint>
#include <map>
#include <vector>

#include "hardware.hpp"

using std::map;
using std::vector;


namespace Inpution {


//Identifier for an action type.
typedef int ActionTypeId;


#pragma region Misc. classes


/**
 * @brief Defines a bind between a specific input source and
 * an action type. This is what's typically set in the game's options.
 */
class Bind {

public:

    //--- Public members ---
    
    //Action type ID.
    ActionTypeId actionTypeId = 0;
    
    //Player number, starting at 1. 0 if N/A.
    int playerNr = 0;
    
    //Main input source bound.
    InputSource inputSource;
    
    //If true, the modifiers have to be respected.
    //See Manager::modifiers.
    bool requireModifiers = false;
    
    //List of modifiers that need to be on for the bind to work.
    //See Manager::modifiers.
    vector<ActionTypeId> modifiers;
    
};


/**
 * @brief Defines an instance of a specific action. This is an abstract
 * gameplay activity, without any notion of hardware input. It's a pure
 * representation of what the player wants to do regardless of how they did it.
 */
class Action {

public:

    //--- Public members ---
    
    //Action type ID.
    ActionTypeId actionTypeId = 0;
    
    //Player number, starting at 1. 0 if N/A.
    int playerNr = 0;
    
    //Value associated, if applicable, [0 - 1].
    float value = 0.0f;
    
    //TODO move to other part of Inpution?
    //Flags. Use ACTION_FLAG.
    uint8_t flags = 0;
    
    //TODO move to other part of Inpution?
    //Queue reinsertion lifetime. See ActionType::insertionTTL.
    float reinsertionLifetime = 0.0f;
    
};


#pragma endregion
#pragma region Translator


/**
 * @brief Responsible for converting raw user inputs into an Action.
 * In order to do this, it needs a registered list of control binds.
 */
class Translator {

public:

    //--- Public members ---
    
    //All registered control binds.
    vector<Bind> binds;
    
    //Map of all modifiers, using their IDs as the key.
    //Modifiers are like Shift, Ctrl, and Alt, though they can be anything.
    //Binds that have modifier requirements will only work if those exact
    //modifiers are pressed down, and others aren't.
    //For example, a bind for a "save" action bound to Ctrl + S will only
    //trigger if Ctrl is held before S, and won't work for Ctrl + Shift + S
    //in case Shift is also registered as a modifier.
    map<ActionTypeId, InputSource> modifiers;
    
    
    //--- Public function declarations ---
    
    vector<Action> getActionsFromInput(
        const Input& input, const vector<ActionTypeId>& modifiersPressed
    ) const;
    
    bool areBindRequirementsMet(
        const Bind& bind, const vector<ActionTypeId>& actionTypesPressed
    ) const;
    
};


#pragma endregion


}
