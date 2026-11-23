/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 *
 * === FILE DESCRIPTION ===
 * Source code for the Inpution middleware's hardware management logic.
 * Please read the included readme file.
 */

#include <algorithm>
#include <cmath>

#include "hardware.hpp"


namespace Inpution {


/**
 * @brief Returns the current value of an input source.
 *
 * @param source The source.
 * @return The value, or 0 if not found.
 */
float HardwareMediator::getInputSourceValue(const InputSource& source) const {
    const auto it = inputSourceValues.find(source);
    if(it == inputSourceValues.end()) return 0.0f;
    return it->second;
}


/**
 * @brief Handles a raw hardware input from the player.
 * Returns a list of simplified inputs. This is because some raw hardware
 * inputs can be a bit complex by their nature.
 *
 * @param input The input.
 * @return The simplified inputs.
 */
vector<Input> HardwareMediator::handleInputAndSimplify(const Input& input) {
    vector<Input> simpleInputs;
    
    //Simplify the input, if needed.
    if(
        input.source.type == INPUT_SOURCE_TYPE_CONTROLLER_AXIS_POS ||
        input.source.type == INPUT_SOURCE_TYPE_CONTROLLER_AXIS_NEG
    ) {
        //We need to process imaginary tilts in the opposite direction.
        //If a player goes from tilting left to tilting right very quickly
        //in one frame, the "walking right" action will work fine but the
        //"walking left" action may never receive a zero value.
        //So we should inject the zero manually with an extra input.
        simpleInputs.push_back(input);
        Input zeroInput = input;
        zeroInput.source.type =
            input.source.type ==
            INPUT_SOURCE_TYPE_CONTROLLER_AXIS_POS ?
            INPUT_SOURCE_TYPE_CONTROLLER_AXIS_NEG :
            INPUT_SOURCE_TYPE_CONTROLLER_AXIS_POS;
        zeroInput.value = 0.0f;
        simpleInputs.push_back(zeroInput);
        
    } else if(
        input.source.type == INPUT_SOURCE_TYPE_CONTROLLER_ANALOG_BUTTON
    ) {
        //Game controller analog buttons have a value ranging from -1 to 1.
        //Let's normalize it.
        Input normalizedInput = input;
        normalizedInput.value = (input.value + 1.0f) / 2.0f;
        simpleInputs.push_back(normalizedInput);
        
    } else if(
        input.source.type == INPUT_SOURCE_TYPE_MOUSE_WHEEL_UP ||
        input.source.type == INPUT_SOURCE_TYPE_MOUSE_WHEEL_DOWN
    ) {
        //Mouse wheel inputs can have values over 1 to indicate the wheel
        //spun a lot. We should process each one as an individual input.
        //Plus, because mouse wheels have no physical state, the player
        //has no way of changing the value of a player action back to 0
        //using the mouse wheel. So whatever player actions we decide here
        //have to be added to this frame's action queue directly.
        for(unsigned int i = 0; i < input.value; i++) {
            Input singleInput = input;
            singleInput.value = 1.0f;
            simpleInputs.push_back(singleInput);
        }
        
    } else {
        //Regular input.
        simpleInputs.push_back(input);
        
    }
    
    //Update the hardware state.
    for(size_t i = 0; i < simpleInputs.size(); i++) {
        Input& simpleInput = simpleInputs[i];
        if(!isInputSourceTypeStateless(simpleInput.source.type)) {
            inputSourceValues[simpleInput.source] = simpleInput.value;
        }
    }
    
    //Return the list.
    return simpleInputs;
}


/**
 * @brief Returns whether a given input source type is,
 * by its nature, stateless.
 *
 * @return Whether it is stateless.
 */
bool HardwareMediator::isInputSourceTypeStateless(
    INPUT_SOURCE_TYPE type
) {
    switch(type) {
    case INPUT_SOURCE_TYPE_MOUSE_WHEEL_UP:
    case INPUT_SOURCE_TYPE_MOUSE_WHEEL_DOWN:
    case INPUT_SOURCE_TYPE_KEYBOARD_CHAR: {
        return true;
    } default:
        return false;
    }
}


/**
 * @brief Acts as if all buttons, keys, analog sticks, etc. have been released.
 *
 * @return Whether it succeeded.
 */
bool HardwareMediator::releaseEverything() {
    inputSourceValues.clear();
    return true;
}


/**
 * @brief Returns whether two input sources are the same.
 *
 * @param s2 The other input source.
 * @return Whether they are the same.
 */
bool InputSource::operator==(const InputSource& s2) const {
    return
        type == s2.type &&
        deviceNr == s2.deviceNr &&
        buttonNr == s2.buttonNr &&
        stickNr == s2.stickNr &&
        axisNr == s2.axisNr;
}


/**
 * @brief Returns which input source should come first when sorting.
 *
 * @param s2 The other input source.
 * @return Whether the current should come before s2.
 */
bool InputSource::operator<(const InputSource& s2) const {
    if(type != s2.type) {
        return type < s2.type;
    }
    
    if(deviceNr != s2.deviceNr) {
        return deviceNr < s2.deviceNr;
    }
    
    if(buttonNr != s2.buttonNr) {
        return buttonNr < s2.buttonNr;
    }
    
    if(stickNr != s2.stickNr) {
        return stickNr < s2.stickNr;
    }
    
    return axisNr < s2.axisNr;
    
}


}
