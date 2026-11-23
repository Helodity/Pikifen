/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 *
 * === FILE DESCRIPTION ===
 * Header for the Inpution middleware's hardware management logic.
 * Please read the included readme file.
 */

#pragma once

#include <map>
#include <vector>

using std::map;
using std::vector;


namespace Inpution {


#pragma region Constants


//Possible types of hardware sources for inputs.
enum INPUT_SOURCE_TYPE {

    //None.
    INPUT_SOURCE_TYPE_NONE,
    
    //Keyboard key.
    INPUT_SOURCE_TYPE_KEYBOARD_KEY,
    
    //Written character via keyboard.
    INPUT_SOURCE_TYPE_KEYBOARD_CHAR,
    
    //Mouse button.
    INPUT_SOURCE_TYPE_MOUSE_BUTTON,
    
    //Mouse wheel scrolled up.
    INPUT_SOURCE_TYPE_MOUSE_WHEEL_UP,
    
    //Mouse wheel scrolled down.
    INPUT_SOURCE_TYPE_MOUSE_WHEEL_DOWN,
    
    //Mouse wheel scrolled left.
    INPUT_SOURCE_TYPE_MOUSE_WHEEL_LEFT,
    
    //Mouse wheel scrolled right.
    INPUT_SOURCE_TYPE_MOUSE_WHEEL_RIGHT,
    
    //Game controller button.
    INPUT_SOURCE_TYPE_CONTROLLER_BUTTON,
    
    //Game controller stick/D-pad axis tilted in a positive position.
    INPUT_SOURCE_TYPE_CONTROLLER_AXIS_POS,
    
    //Game controller stick/D-pad axis tilted in a negative position.
    INPUT_SOURCE_TYPE_CONTROLLER_AXIS_NEG,
    
    //Game controller analog button.
    //Input value ranges [-1 - 1] before Inpution normalizes it.
    INPUT_SOURCE_TYPE_CONTROLLER_ANALOG_BUTTON,
    
    //Some unknown type.
    INPUT_SOURCE_TYPE_UNKNOWN,
    
};


#pragma endregion
#pragma region Input source


/**
 * @brief Defines a specific interactable thing in the player's hardware,
 * like a specific button on a specific game controller, a specific key on
 * the keyboard, etc.
 */
class InputSource {

public:

    //--- Public members ---
    
    //Type of input source.
    INPUT_SOURCE_TYPE type = INPUT_SOURCE_TYPE_NONE;
    
    //Device number. i.e. the game controller number. 0 if N/A.
    int deviceNr = 0;
    
    //Game controller button, keyboard key, mouse button, etc. 0 if N/A.
    int buttonNr = 0;
    
    //Game controller stick. Also used for analog buttons. 0 if N/A.
    int stickNr = 0;
    
    //Game controller stick axis. Also used for analog buttons. 0 if N/A.
    int axisNr = 0;
    
    
    //--- Public function declarations ---
    
    bool operator==(const InputSource& s2) const;
    bool operator<(const InputSource& s2) const;
    
};


#pragma endregion
#pragma region Input


/**
 * @brief Defines an instance of a specific input: a specific gesture made
 * by a human on a specific source of a specific piece of hardware.
 */
class Input {

public:

    //--- Public members ---
    
    //Hardware source.
    InputSource source;
    
    //Value associated, if applicable, [0 - 1].
    float value = 0.0f;
    
};


#pragma endregion
#pragma region Hardware mediator


/**
 * @brief Manages the state of the player's hardware.
 */
class HardwareMediator {

public:

    //--- Public members ---
    
    //Values of each input source.
    map<InputSource, float> inputSourceValues;
    
    
    //--- Public function declarations ---
    
    vector<Input> handleInputAndSimplify(const Input& input);
    float getInputSourceValue(const InputSource& source) const;
    bool releaseEverything();
    
    static bool isInputSourceTypeStateless(INPUT_SOURCE_TYPE type);
    
};


#pragma endregion


}
