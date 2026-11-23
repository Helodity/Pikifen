/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 *
 * === FILE DESCRIPTION ===
 * Source code for the Inpution middleware's translator.
 * Please read the included readme file.
 */

#include <algorithm>

#include "translator.hpp"


namespace Inpution {


/**
 * @brief Returns whether a bind's requirement modifiers are
 * currently being met.
 *
 * @param bind Bind to check.
 * @param modifiersPressed List of what action types are currently
 * pressed down. Since the list only needs to know about action
 * types registered as modifiers, you only need those in the list if
 * you want.
 * @return Whether they are met.
 */
bool Translator::areBindRequirementsMet(
    const Bind& bind, const vector<ActionTypeId>& modifiersPressed
) const {
    if(!bind.requireModifiers) return true;
    
    for(const auto& mod : modifiers) {
        bool modIsDown =
            std::find(
                modifiersPressed.begin(),
                modifiersPressed.end(),
                mod.first
            ) != modifiersPressed.end();
        bool needsDown =
            std::find(
                bind.modifiers.begin(),
                bind.modifiers.end(),
                mod.first
            ) != bind.modifiers.end();
            
        if(needsDown != modIsDown) return false;
    }
    
    return true;
}


/**
 * @brief Returns a list of action that happened as a result of
 * the given input.
 *
 * @param input The input.
 * @param modifiersPressed List of what action types are currently
 * pressed down. Since the list only needs to know about action
 * types registered as modifiers, you only need those in the list if
 * you want.
 * @return The actions.
 */
vector<Action> Translator::getActionsFromInput(
    const Input& input, const vector<ActionTypeId>& modifiersPressed
) const {
    vector<Action> actions;
    
    for(size_t b = 0; b < binds.size(); b++) {
        const Bind& bind = binds[b];
        if(!(bind.inputSource == input.source)) continue;
        if(!areBindRequirementsMet(bind, modifiersPressed)) continue;
        
        Action newAction;
        newAction.actionTypeId = bind.actionTypeId;
        newAction.playerNr = bind.playerNr;
        newAction.value = input.value;
        actions.push_back(newAction);
    }
    
    return actions;
}


}
