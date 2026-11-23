/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 *
 * === FILE DESCRIPTION ===
 * Source code for the Inpution middleware.
 * Please read the included readme file.
 */

#include <algorithm>
#include <cmath>

#include "inpution.hpp"


namespace Inpution {


/**
 * @brief Given an input value, converts it to an analog or boolean value,
 * according to the action type.
 *
 * @param actionTypeId ID of the action type.
 * @param value Value to convert.
 * @return The converted value.
 */
float Manager::convertActionValue(
    ActionTypeId actionTypeId, float value
) const {
    auto it = actionTypes.find(actionTypeId);
    if(it != actionTypes.end()) {
        switch(it->second.valueType) {
        case ACTION_VALUE_TYPE_ANALOG: {
            return value;
            break;
        } case ACTION_VALUE_TYPE_DIGITAL: {
            return value >= options.digitalThreshold ? 1.0f : 0.0f;
            break;
        } case ACTION_VALUE_TYPE_1_ONLY: {
            return 1.0f;
            break;
        }
        }
    }
    
    return value;
}


/**
 * @brief Returns the current value of an input source.
 * Ignored input sources return 0.
 *
 * @param source The source.
 * @return The value, or 0 if not found.
 */
float Manager::getInputSourceValue(
    const Inpution::InputSource& source
) const {
    for(size_t i = 0; i < ignoredInputSources.size(); i++) {
        if(ignoredInputSources[i].source == source) {
            //It's currently ignored.
            return 0.0f;
        }
    }
    return hardware.getInputSourceValue(source);
}


/**
 * @brief Returns which modifier action types are currently pressed down.
 *
 * @return The modifiers.
 */
vector<ActionTypeId> Manager::getModifiersPressed() const {
    vector<ActionTypeId> result;
    
    for(const auto& m : translator.modifiers) {
        if(getInputSourceValue(m.second) >= options.digitalThreshold) {
            result.push_back(m.first);
        }
    }
    
    return result;
}


/**
 * @brief Returns the current value of a given action type.
 *
 * @param actionTypeId ID of the action type.
 * @return The value, or 0 on failure.
 */
float Manager::getValue(ActionTypeId actionTypeId) const {
    float highestValue = 0.0f;
    for(const auto& bind : translator.binds) {
        if(bind.actionTypeId != actionTypeId) continue;
        if(!translator.areBindRequirementsMet(bind, getModifiersPressed())) {
            continue;
        }
        float value = getInputSourceValue(bind.inputSource);
        highestValue = std::max(highestValue, value);
    }
    return convertActionValue(actionTypeId, highestValue);
}


/**
 * @brief Handles a hardware input from the player.
 *
 * @param input The input.
 * @return Whether it succeeded.
 */
bool Manager::handleInput(const Input& input) {
    vector<Input> simpleInputs = hardware.handleInputAndSimplify(input);
    
    for(size_t i = 0; i < simpleInputs.size(); i++) {
        Input& simpleInput = simpleInputs[i];
        
        bool forceDirectEvent =
            HardwareMediator::isInputSourceTypeStateless(
                simpleInput.source.type
            );
            
        if(!forceDirectEvent) {
            if(processInputIgnoring(simpleInput)) {
                //We have to ignore this one.
                return true;
            }
        }
        
        //Find what game actions result from this input.
        vector<Action> actions =
            translator.getActionsFromInput(simpleInput, getModifiersPressed());
            
        for(size_t a = 0; a < actions.size(); a++) {
            const ActionType& actionType = actionTypes[actions[a].actionTypeId];
            bool mustAddDirectly =
                forceDirectEvent ||
                actionType.directEvents ||
                actionType.valueType == ACTION_VALUE_TYPE_1_ONLY;
                
            if(mustAddDirectly) {
                //Add it to the action queue directly.
                Action newAction;
                newAction.actionTypeId = actions[a].actionTypeId;
                newAction.value =
                    convertActionValue(
                        actions[a].actionTypeId, simpleInput.value
                    );
                newAction.flags |= ACTION_FLAG_DIRECT;
                newAction.reinsertionLifetime =
                    actionTypes[actions[a].actionTypeId].reinsertionTTL;
                actionQueue.push_back(newAction);
            }
        }
    }
    
    return true;
}


/**
 * @brief Returns the actions that occurred during the last frame of
 * gameplay, and begins a new frame.
 *
 * @param deltaT How much time has passed since the last frame.
 * @return The actions.
 */
vector<Action> Manager::newFrame(float deltaT) {
    lastDeltaT = deltaT;
    auto& curGameState = gameStates[curGameStateName];
    
    //Get the current value of each action type.
    for(auto& a : actionTypes) {
        GameState& state = a.second.freezable ? curGameState : globalState;
        state.actionTypeStatuses[a.first].value = getValue(a.first);
    }
    
    //Add actions to the queue as needed.
    for(auto& a : actionTypes) {
        if(actionTypes[a.first].directEvents) {
            //Already added to the queue in handleCleanInput().
            continue;
        }
        GameState& state = a.second.freezable ? curGameState : globalState;
        if(
            state.actionTypeStatuses[a.first].oldValue !=
            state.actionTypeStatuses[a.first].value
        ) {
            Action newAction;
            newAction.actionTypeId = a.first;
            newAction.value = state.actionTypeStatuses[a.first].value;
            newAction.reinsertionLifetime = actionTypes[a.first].reinsertionTTL;
            actionQueue.push_back(newAction);
        }
    }
    
    //Process timers and auto-repeats.
    for(auto& a : actionTypes) {
        GameState& state = a.second.freezable ? curGameState : globalState;
        processTimers(
            state.actionTypeStatuses[a.first], a.second, deltaT
        );
        processAutoRepeats(
            state.actionTypeStatuses[a.first], a.first, a.second, deltaT
        );
    }
    
    vector<Action> result;
    if(!ignoringActions) {
        result = actionQueue;
    }
    
    //Clear any ignore rules that were meant to apply now only, but their
    //input isn't > 0, so they are no longer valid.
    for(size_t i = 0; i < ignoredInputSources.size();) {
        if(
            ignoredInputSources[i].nowOnly &&
            hardware.getInputSourceValue(ignoredInputSources[i].source) == 0.0f
        ) {
            ignoredInputSources.erase(ignoredInputSources.begin() + i);
        } else {
            i++;
        }
    }
    
    //Prepare things for the next frame.
    for(auto& a : actionTypes) {
        GameState& state = a.second.freezable ? curGameState : globalState;
        state.actionTypeStatuses[a.first].oldValue =
            state.actionTypeStatuses[a.first].value;
    }
    actionQueue.clear();
    
    //Return the final list of actions.
    return result;
}


/**
 * @brief Processes logic for auto-repeating actions.
 *
 * @param status Status object of the action type.
 * @param actionTypeId ID of the action type.
 * @param actionType Data about the action type.
 * @param deltaT How much time has passed since the last frame.
 */
void Manager::processAutoRepeats(
    ActionTypeStatus& status, ActionTypeId actionTypeId,
    const ActionType& actionType, float deltaT
) {
    float actionTypeAutoRepeat = actionType.autoRepeat;
    if(actionTypeAutoRepeat == 0.0f) return;
    
    float autoRepeatFactor =
        (status.value - actionTypeAutoRepeat) /
        (1.0f - actionTypeAutoRepeat);
    if(autoRepeatFactor <= 0.0f) return;
    if(status.value == 0.0f) return;
    if(status.activationTimer == 0.0f) return;
    float oldDuration = status.activationTimer - deltaT;
    if(oldDuration >= status.nextAutoRepeatTimer) return;
    
    while(
        status.activationTimer >= status.nextAutoRepeatTimer
    ) {
        //Auto-repeat!
        Action newAction;
        newAction.actionTypeId = actionTypeId;
        newAction.value = status.value;
        newAction.flags |= ACTION_FLAG_REPEAT;
        newAction.reinsertionLifetime = actionType.reinsertionTTL;
        actionQueue.push_back(newAction);
        
        //Set the next activation.
        float currentFrequency =
            options.autoRepeatMaxInterval +
            (status.activationTimer / options.autoRepeatRampTime) *
            (options.autoRepeatMinInterval - options.autoRepeatMaxInterval);
        currentFrequency =
            std::max(options.autoRepeatMinInterval, currentFrequency);
        currentFrequency =
            std::min(currentFrequency, options.autoRepeatMaxInterval);
        status.nextAutoRepeatTimer += currentFrequency;
    }
}


/**
 * @brief Processes a received input, updates the list of ignored inputs if
 * necessary, and returns whether or not this one should be ignored.
 *
 * @param input Input to check.
 * @return Whether it should be ignored.
 */
bool Manager::processInputIgnoring(const Input& input) {
    for(size_t i = 0; i < ignoredInputSources.size(); i++) {
        if(ignoredInputSources[i].source == input.source) {
            if(input.value != 0.0f) {
                //We just ignore it and keep it on the list.
                return true;
            } else {
                //Remove it from the list since it's finally at 0,
                //and let the 0 go through.
                ignoredInputSources.erase(ignoredInputSources.begin() + i);
                return false;
            }
        }
    }
    
    return false;
}


/**
 * @brief Processes the timers for action types in a frame.
 *
 * @param status Status object of the action type.
 * @param actionType Data about the action type.
 * @param deltaT How much time has passed since the last frame.
 */
void Manager::processTimers(
    ActionTypeStatus& status, const ActionType& actionType, float deltaT
) {
    bool isActive = status.value != 0.0f;
    bool wasActive = status.oldValue != 0.0f;
    if(isActive != wasActive) {
        //Activation changed. Reset the timer.
        status.activationTimer = 0.0f;
        status.nextAutoRepeatTimer = options.autoRepeatMaxInterval;
    } else {
        //Same activation, increase the timer.
        status.activationTimer += deltaT;
    }
}


/**
 * @brief Reinserts an action event into the queue, so it can have a chance
 * at being processed again at a later frame. See ActionType::reinsertionTTL
 * for more information.
 *
 * @param action Action to reinsert.
 * @return Whether it succeeded.
 */
bool Manager::reinsertAction(const Action& action) {
    if(actionTypes[action.actionTypeId].reinsertionTTL <= 0.0f) return false;
    if(action.reinsertionLifetime <= 0.0f) return false;
    if(lastDeltaT == 0.0f) return false;
    
    Action newAction = action;
    newAction.reinsertionLifetime -= lastDeltaT;
    newAction.flags |= ACTION_FLAG_REINSERTED;
    actionQueue.push_back(newAction);
    return true;
}


/**
 * @brief Acts as if all buttons, keys, analog sticks, etc. have been released.
 *
 * @return Whether it succeeded.
 */
bool Manager::releaseEverything() {
    hardware.releaseEverything();
    return true;
}


/**
 * @brief Sets which game state to use from here on out, given its name. An
 * empty string is the default game state name when no game state is specified.
 * Changing to a different game state is useful when you want the previous
 * game state to not be aware of any action changes that are happening. A good
 * example is when you open the pause menu mid-gameplay. If the player was
 * holding B to charge the character's special move, paused, let go off B for
 * a second, held B again, and unpaused, tou probably don't want the regular
 * gameplay state to be aware of the special move 0 and special move 1
 * actions.
 * For menus, ideally you'd have some interval after the menu is closed in which
 * no actions are processed, so that the "menu close 1" input doesn't get
 * immediately consumed by normal gameplay.
 * Only action types that have the freezable property set to true will
 * be affected.
 *
 * @param name Name of the game state.
 * @return Whether it succeeded.
 */
bool Manager::setGameState(const string& name) {
    curGameStateName = name;
    return true;
}


/**
 * @brief Same as Manager::startIgnoringInputSource(), but applies to
 * all input sources of a given action.
 *
 * @param actionTypeId Action type whose input sources to ignore.
 * @param nowOnly If true, only apply to inputs that are currently already
 * held down (> 0) this frame.
 * If false, keep the ignore rule up to the next time it's pressed down.
 * @return Whether it succeeded.
 */
bool Manager::startIgnoringActionInputSources(
    ActionTypeId actionTypeId, bool nowOnly
) {
    bool result = false;
    for(size_t b = 0; b < translator.binds.size(); b++) {
        Bind* bPtr = &translator.binds[b];
        if(bPtr->actionTypeId != actionTypeId) continue;
        result |= startIgnoringInputSource(bPtr->inputSource, nowOnly);
    }
    return result;
}


/**
 * @brief Ignores an input source from now on until the player performs the
 * input with value 0, at which point it becomes unignored.
 *
 * This is useful, for instance, when you want to detect the player's input
 * in the options menu's control binds screen, so you can assign the bind
 * but not have Inpution immediately right afterwards perform the action.
 * Also useful if you, for instance, have "unpause" and "attack" bound to the
 * same input, the player unpauses and immediately attacks.
 *
 * See also Manager::startIgnoringActionInputSources().
 *
 * @param inputSource Input source to ignore.
 * @param nowOnly If true, only apply to inputs that are currently already
 * held down (> 0) this frame.
 * If false, keep the ignore rule up to the next time it's pressed down.
 * @return Whether it succeeded.
 */
bool Manager::startIgnoringInputSource(
    const InputSource& inputSource, bool nowOnly
) {
    for(size_t i = 0; i < ignoredInputSources.size(); i++) {
        if(ignoredInputSources[i].source == inputSource) {
            //Already ignored.
            return false;
        }
    }
    ignoredInputSources.push_back({ .source = inputSource, .nowOnly = nowOnly});
    return true;
}


};
