/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 *
 * === FILE DESCRIPTION ===
 * Header for the Inpution middleware.
 * Please read the included readme file.
 */

#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <tuple>
#include <vector>

#include "translator.hpp"


using std::map;
using std::string;
using std::tuple;
using std::vector;


namespace Inpution {

//Identifier for an action type.
typedef int ActionTypeId;


#pragma region Constants


//Possible flags for emitted actions.
enum ACTION_FLAG {

    //This action was inserted into the queue directly, when the hardware event
    //was received, as opposed to the hardware being updated internally and
    //then the action being inserted in Manager::newFrame as needed.
    ACTION_FLAG_DIRECT = 1 << 0,
    
    //This action was issued as an auto-repeat.
    ACTION_FLAG_REPEAT = 1 << 1,
    
    //This action was manually reinserted into the queue.
    ACTION_FLAG_REINSERTED = 1 << 2,
    
};


//Possible types of value an action can have.
enum ACTION_VALUE_TYPE {

    //A float in the range [0 - 1].
    //e.g. Forward movement amount.
    ACTION_VALUE_TYPE_ANALOG,
    
    //Either 0 or 1 (basically up or down).
    //e.g. jump button pressed or released.
    ACTION_VALUE_TYPE_DIGITAL,
    
    //Just 1.
    //e.g. Save command in the editor.
    ACTION_VALUE_TYPE_1_ONLY,
    
};


#pragma endregion
#pragma region Misc. classes


/**
 * @brief Represents one of the game's possible actions.
 */
class ActionType {

public:

    //--- Public members ---
    
    //Type of value it can take. If digital but the input source is analog,
    //the value is rounded to 0 or 1 (i.e. depends on whether it's more than
    //or less than pressed halfway in).
    ACTION_VALUE_TYPE valueType = ACTION_VALUE_TYPE_ANALOG;
    
    //Auto-repeat. 0 if disabled, otherwise this indicates the threshold [0 - 1]
    //after which the input will start auto-repeating. The manager's
    //auto-repeating settings have to be configured for this to work.
    float autoRepeat = 0.0f;
    
    //If true, any input event that Inpution receives that's bound to this
    //action type will immediately add an event to the list that gets returned
    //by newFrame(). This can be useful, for instance, if you have two buttons
    //bound to the same action, and want either button's down press event
    //to trigger an action, like in a fast-paced rhythm game.
    //If false, input events feed an internal hardware state inside Inpution,
    //which is processed every frame to deliver the list in newFrame(). This
    //is useful for most cases, since if you have two inputs for the same
    //action, only the highest value of the two at any moment will be returned.
    //For example, if you have an analog stick and a D-pad bound to menu
    //focus movement.
    bool directEvents = false;
    
    //Actions can be told to return to the queue given by newFrame(). This is
    //useful as a buffer, where if the game can't handle that action, it can
    //throw it back into the queue to try to handle it in one of the next few
    //frames. For instance, the player pressed the kick button while mid-air,
    //close to the ground. The game can't perform a kick now, so it sends the
    //action back to the queue. In a few frames, the character lands, and the
    //action event can now be processed, even if it happened some frames after
    //the player really pressed the button.
    //This number controls the maximum time to live for a re-inserted action.
    //0 means it cannot be reinserted.
    float reinsertionTTL = 0.0f;
    
    //If true, this action's value in a given game state can remain "frozen".
    //If false, the action will always update regardless of game state.
    //See Manager::setGameState.
    bool freezable = false;
    
};


/**
 * @brief Info about a control manager's options.
 */
class ManagerOptions {

public:

    //--- Public members ---
    
    //Threshold for converting analog values to digital.
    //If the value is equal to or higher than this, it will convert to 1.
    //Otherwise to 0.
    float digitalThreshold = 0.5f;
    
    //Interval between auto-repeat activations, at the slowest speed.
    float autoRepeatMaxInterval = 0.3f;
    
    //Interval between auto-repeat activations, at the fastest speed.
    float autoRepeatMinInterval = 0.05f;
    
    //How long it takes for the auto-repeat activations to reach max speed.
    float autoRepeatRampTime = 0.9f;
    
};


#pragma endregion
#pragma region Manager


/**
 * @brief Manages the connections between inputs and actions.
 *
 * The idea of this class is to be game-agnostic.
 * An input is data about some hardware signal. For instance, the fact
 * that a key was pressed along with its key code, or the fact that a game
 * controller's button was released, along with the button code and game
 * controller number.
 * The manager holds a list of control binds, and when an input is received,
 * it scans through all binds to figure out what actions should be
 * triggered.
 * It also has logic to do some cleanup like normalizing a game controller's
 * analog stick positions.
 */
class Manager {

public:

    //--- Public members ---
    
    //Map of all registered action types, using their IDs as the key.
    map<ActionTypeId, ActionType> actionTypes;
    
    //Hardware state mediator.
    HardwareMediator hardware;
    
    //Input to action translator.
    Translator translator;
    
    //Are we ignoring actions right now?
    bool ignoringActions = false;
    
    //Options of the manager itself.
    ManagerOptions options;
    
    
    //--- Public function declarations ---
    
    float getInputSourceValue(const Inpution::InputSource& source) const;
    float getValue(ActionTypeId actionTypeId) const;
    bool handleInput(const Input& input);
    vector<Action> newFrame(float deltaT);
    bool reinsertAction(const Action& action);
    bool releaseEverything();
    bool setGameState(const string& name = "");
    bool startIgnoringActionInputSources(
        ActionTypeId actionTypeId, bool nowOnly
    );
    bool startIgnoringInputSource(const InputSource& inputSource, bool nowOnly);
    
    
protected:

    //--- Protected structs ---
    
    /**
     * @brief Information about an action type's current status.
     */
    struct ActionTypeStatus {
    
        public:
        
        //--- Public members ---
        
        //Current value [0 - 1].
        float value = 0.0f;
        
        //Old value [0 - 1].
        float oldValue = 0.0f;
        
        //How long it's been been active (!= 0) or inactive (== 0) for.
        float activationTimer = 0.0f;
        
        //How long until the next auto-repeat activation.
        float nextAutoRepeatTimer = 0.0f;
        
    };
    
    
    /**
     * @brief Represents one of the game's macro states.
     */
    struct GameState {
    
        public:
        
        //--- Public members ---
        
        //Status of each action type in this game state.
        map<ActionTypeId, ActionTypeStatus> actionTypeStatuses;
        
    };
    
    
    /**
     * @brief Rule for ignoring a given input source.
     */
    struct IgnoreRule {
    
        public:
        
        //--- Public members ---
        
        //The input source to ignore.
        InputSource source;
        
        //Whether it should only be ignored if it's active now, or if it can
        //wait until the next time it's active.
        bool nowOnly = true;
        
    };
    
    
    //--- Protected members ---
    
    //Queue of actions the game needs to handle this frame.
    vector<Action> actionQueue;
    
    //Input sources currently being ignored.
    vector<IgnoreRule> ignoredInputSources;
    
    //Name of the current game state, or empty if none specified.
    string curGameStateName;
    
    //Game states. Used for freezable action types.
    map<string, GameState> gameStates;
    
    //Global "game state". Used for non-freezable action types.
    GameState globalState;
    
    //Last known time delta.
    float lastDeltaT = 0.0f;
    
    
    //--- Protected function declarations ---
    
    float convertActionValue(ActionTypeId actionTypeId, float value) const;
    vector<ActionTypeId> getModifiersPressed() const;
    void processAutoRepeats(
        ActionTypeStatus& status, ActionTypeId actionTypeId,
        const ActionType& actionType, float deltaT
    );
    bool processInputIgnoring(const Input& input);
    void processTimers(
        ActionTypeStatus& status, const ActionType& actionType, float deltaT
    );
};


#pragma endregion


};
