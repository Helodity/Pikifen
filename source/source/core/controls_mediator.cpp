/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Control-related functions.
 */

#include <algorithm>
#include <typeinfo>

#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>

#include "controls_mediator.h"

#include "../game_state/gameplay/gameplay.h"
#include "../util/general_utils.h"
#include "../util/string_utils.h"
#include "const.h"
#include "drawing.h"
#include "game.h"
#include "misc_functions.h"


/**
 * @brief Adds a new player action to the list.
 *
 * @param id Its ID.
 * @param category Its category.
 * @param name Its name.
 * @param description Its descripton.
 * @param internal_name The name of its property in the options file.
 * @param default_bind_str String representing of this action's default
 * control bind.
 * @param auto_repeat Auto-repeat threshold.
 */
void ControlsMediator::addPlayerActionType(
    PLAYER_ACTION_TYPE id, PLAYER_ACTION_CAT category,
    const string &name, const string &description, const string &internal_name,
    const string &default_bind_str, float auto_repeat
) {
    PfePlayerActionType a;
    a.id = id;
    a.category = category;
    a.name = name;
    a.description = description;
    a.internalName = internal_name;
    a.defaultBindStr = default_bind_str;
    a.autoRepeat = auto_repeat;
    
    playerActionTypes.push_back(a);
    mgr.actionTypes[id] = a;
}


/**
 * @brief Returns the parsed input from an Allegro event.
 *
 * @param ev The Allegro event.
 * @return The input.
 * If this event does not pertain to any valid input, an input of type
 * INPUT_SOURCE_TYPE_NONE is returned.
 */
PlayerInput ControlsMediator::allegroEventToInput(
    const ALLEGRO_EVENT &ev
) const {
    PlayerInput input;
    
    switch(ev.type) {
    case ALLEGRO_EVENT_KEY_DOWN:
    case ALLEGRO_EVENT_KEY_UP: {
        input.source.type = INPUT_SOURCE_TYPE_KEYBOARD_KEY;
        input.source.buttonNr = ev.keyboard.keycode;
        input.value = (ev.type == ALLEGRO_EVENT_KEY_DOWN) ? 1 : 0;
        break;
        
    } case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
    case ALLEGRO_EVENT_MOUSE_BUTTON_UP: {
        input.source.type = INPUT_SOURCE_TYPE_MOUSE_BUTTON;
        input.source.buttonNr = ev.mouse.button;
        input.value = (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) ? 1 : 0;
        break;
        
    } case ALLEGRO_EVENT_MOUSE_AXES: {
        if(ev.mouse.dz > 0) {
            input.source.type = INPUT_SOURCE_TYPE_MOUSE_WHEEL_UP;
            input.value = ev.mouse.dz;
        } else if(ev.mouse.dz < 0) {
            input.source.type = INPUT_SOURCE_TYPE_MOUSE_WHEEL_DOWN;
            input.value = -ev.mouse.dz;
        } else if(ev.mouse.dw > 0) {
            input.source.type = INPUT_SOURCE_TYPE_MOUSE_WHEEL_RIGHT;
            input.value = ev.mouse.dw;
        } else if(ev.mouse.dw < 0) {
            input.source.type = INPUT_SOURCE_TYPE_MOUSE_WHEEL_LEFT;
            input.value = -ev.mouse.dw;
        }
        break;
        
    } case ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN:
    case ALLEGRO_EVENT_JOYSTICK_BUTTON_UP: {
        input.source.type = INPUT_SOURCE_TYPE_CONTROLLER_BUTTON;
        input.source.deviceNr = game.controllerNumbers[ev.joystick.id];
        input.source.buttonNr = ev.joystick.button;
        input.value = (ev.type == ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN) ? 1 : 0;
        break;
        
    } case ALLEGRO_EVENT_JOYSTICK_AXIS: {
        if(ev.joystick.pos >= 0.0f) {
            input.source.type = INPUT_SOURCE_TYPE_CONTROLLER_AXIS_POS;
            input.value = ev.joystick.pos;
        } else {
            input.source.type = INPUT_SOURCE_TYPE_CONTROLLER_AXIS_NEG;
            input.value = -ev.joystick.pos;
        }
        input.source.deviceNr = game.controllerNumbers[ev.joystick.id];
        input.source.stickNr = ev.joystick.stick;
        input.source.axisNr = ev.joystick.axis;
        break;
    }
    }
    
    return input;
}


/**
 * @brief Returns the array of registered bind.
 *
 * @return The binds.
 */
vector<ControlBind> &ControlsMediator::binds() {
    return mgr.binds;
}


/**
 * @brief Finds a registered control bind for player 1 that matches
 * the requested action. Returns an empty bind if none is found.
 *
 * @param action_type_id ID of the action type.
 * @return The bind.
 */
ControlBind ControlsMediator::findBind(
    const PLAYER_ACTION_TYPE action_type_id
) const {
    for(size_t b = 0; b < mgr.binds.size(); b++) {
        if(mgr.binds[b].actionTypeId == action_type_id) {
            return mgr.binds[b];
        }
    }
    return ControlBind();
}


/**
 * @brief Finds a registered control bind for player 1 that matches
 * the requested action. Returns an empty bind if none is found.
 *
 * @param action_name Name of the action.
 * @return The bind.
 */
ControlBind ControlsMediator::findBind(
    const string &action_name
) const {
    for(size_t b = 0; b < playerActionTypes.size(); b++) {
        if(playerActionTypes[b].internalName == action_name) {
            return findBind(playerActionTypes[b].id);
        }
    }
    return ControlBind();
}


/**
 * @brief Returns the current list of registered player action types.
 *
 * @return The types.
 */
const vector<PfePlayerActionType>
&ControlsMediator::getAllPlayerActionTypes() const {
    return playerActionTypes;
}


/**
 * @brief Returns a registered type, given its ID.
 *
 * @param action_id ID of the player action.
 * @return The type, or an empty type on failure.
 */
PfePlayerActionType ControlsMediator::getPlayerActionType(
    int action_id
) const {
    for(size_t b = 0; b < playerActionTypes.size(); b++) {
        if(playerActionTypes[b].id == action_id) {
            return playerActionTypes[b];
        }
    }
    return PfePlayerActionType();
}


/**
 * @brief Returns the internal name from an input ID,
 * used in the on_input_recieved event.
 *
 * @param action_id ID of the player action.
 * @return The name, or an empty string on failure.
 */
string ControlsMediator::getPlayerActionTypeInternalName(
    int action_id
) {
    for(size_t b = 0; b < playerActionTypes.size(); b++) {
        if(playerActionTypes[b].id == action_id) {
            return playerActionTypes[b].internalName;
        }
    }
    return "";
}


/**
 * @brief Returns the current input value of a given action type.
 *
 * @param player_action_type_id Action type to use.
 * @return The value.
 */
float ControlsMediator::getPlayerActionTypeValue(
    PLAYER_ACTION_TYPE player_action_type_id
) {
    return mgr.getValue((int) player_action_type_id);
}


/**
 * @brief Handles an Allegro event.
 *
 * @param ev The Allegro event.
 * @return Whether the event was handled.
 */
bool ControlsMediator::handleAllegroEvent(const ALLEGRO_EVENT &ev) {
    PlayerInput input = allegroEventToInput(ev);
    
    if(input.source.type != INPUT_SOURCE_TYPE_NONE) {
        mgr.handleInput(input);
        return true;
    } else {
        return false;
    }
}


/**
 * @brief Creates a string that represents an input.
 * Ignores the player number.
 *
 * @param s Input source to read from.
 * @return The string, or an empty string on error.
 */
string ControlsMediator::inputSourceToStr(
    const PlayerInputSource &s
) const {
    switch(s.type) {
    case INPUT_SOURCE_TYPE_KEYBOARD_KEY: {
        return "k_" + i2s(s.buttonNr);
    } case INPUT_SOURCE_TYPE_MOUSE_BUTTON: {
        return "mb_" + i2s(s.buttonNr);
    } case INPUT_SOURCE_TYPE_MOUSE_WHEEL_UP: {
        return "mwu";
    } case INPUT_SOURCE_TYPE_MOUSE_WHEEL_DOWN: {
        return "mwd";
    } case INPUT_SOURCE_TYPE_MOUSE_WHEEL_LEFT: {
        return "mwl";
    } case INPUT_SOURCE_TYPE_MOUSE_WHEEL_RIGHT: {
        return "mwr";
    } case INPUT_SOURCE_TYPE_CONTROLLER_BUTTON: {
        return "jb_" + i2s(s.deviceNr) + "_" + i2s(s.buttonNr);
    } case INPUT_SOURCE_TYPE_CONTROLLER_AXIS_POS: {
        return
            "jap_" + i2s(s.deviceNr) +
            "_" + i2s(s.stickNr) + "_" + i2s(s.axisNr);
    } case INPUT_SOURCE_TYPE_CONTROLLER_AXIS_NEG: {
        return
            "jan_" + i2s(s.deviceNr) +
            "_" + i2s(s.stickNr) + "_" + i2s(s.axisNr);
    } default: {
        return "";
    }
    }
}


/**
 * @brief Loads a list of binds from a data node. Binds are formatted like so:
 * "<action type>=<input 1>;<input 2>;<...>"
 *
 * @param node The node.
 * @param player_nr Player number.
 */
void ControlsMediator::loadBindsFromDataNode(
    DataNode* node, unsigned char player_nr
) {
    const vector<PfePlayerActionType> &player_action_types =
        getAllPlayerActionTypes();
        
    for(size_t a = 0; a < player_action_types.size(); a++) {
        string action_type_name = player_action_types[a].internalName;
        if(action_type_name.empty()) continue;
        
        DataNode* bind_node = node->getChildByName(action_type_name);
        vector<string> inputs = semicolonListToVector(bind_node->value);
        
        for(size_t c = 0; c < inputs.size(); c++) {
            PlayerInputSource input_source = strToInputSource(inputs[c]);
            if(input_source.type == INPUT_SOURCE_TYPE_NONE) continue;
            
            ControlBind new_bind;
            new_bind.actionTypeId = player_action_types[a].id;
            new_bind.playerNr = player_nr;
            new_bind.inputSource = input_source;
            binds().push_back(new_bind);
        }
    }
}


/**
 * @brief Ignore player actions from here on.
 */
void ControlsMediator::startIgnoringActions() {
    mgr.ignoringActions = true;
}


/**
 * @brief Ignores an input source from now on until the player performs the
 * input with value 0, at which point it becomes unignored.
 *
 * @param input_source Input source to ignore.
 */
void ControlsMediator::startIgnoringInputSource(
    const PlayerInputSource &input_source
) {
    mgr.startIgnoringInputSource(input_source);
}


/**
 * @brief No longer ignore player actions from here on.
 */
void ControlsMediator::stopIgnoringActions() {
    mgr.ignoringActions = false;
}


/**
 * @brief Returns the player actions that occurred during the last frame
 * of gameplay, and begins a new frame.
 *
 * @param delta_t How much time has passed since the last frame.
 * @return The player actions.
 */
vector<PlayerAction> ControlsMediator::newFrame(float delta_t) {
    return mgr.newFrame(delta_t);
}


/**
 * @brief Releases all player inputs. Basically, set all of their values to 0.
 * Useful for when the game state is changed, or the window is out of focus.
 */
void ControlsMediator::releaseAll() {
    for(auto &a : mgr.actionTypes) {
        mgr.setValue(a.first, 0.0f);
    }
}


/**
 * @brief Loads the list of binds to a data node.
 *
 * @param node The node.
 * @param player_nr Player number.
 */
void ControlsMediator::saveBindsToDataNode(
    DataNode* node, unsigned char player_nr
) {
    map<string, string> bind_strs;
    const vector<PfePlayerActionType> &player_action_types =
        getAllPlayerActionTypes();
    const vector<ControlBind> &all_binds = binds();
    
    //Fill the defaults, which are all empty strings.
    for(size_t b = 0; b < player_action_types.size(); b++) {
        string action_type_name = player_action_types[b].internalName;
        if(action_type_name.empty()) continue;
        bind_strs[action_type_name].clear();
    }
    
    //Fill their input strings.
    for(size_t b = 0; b < all_binds.size(); b++) {
        if(all_binds[b].playerNr != player_nr) continue;
        PfePlayerActionType action_type =
            getPlayerActionType(all_binds[b].actionTypeId);
        bind_strs[action_type.internalName] +=
            inputSourceToStr(all_binds[b].inputSource) + ";";
    }
    
    //Save them all.
    for(auto &c : bind_strs) {
        //Remove the final character, which is always an extra semicolon.
        if(c.second.size()) c.second.erase(c.second.size() - 1);
        
        node->addNew(c.first, c.second);
    }
}


/**
 * @brief Sets the options for the controls manager.
 *
 * @param options Options.
 */
void ControlsMediator::setOptions(const ControlsManagerOptions &options) {
    mgr.options = options;
}


/**
 * @brief Creates an input from a string representation.
 * Ignores the player number. Input strings are formatted like so:
 * "<input type>_<parameters, underscore separated>"
 * Input types are:
 * "k" (keyboard key), "mb" (mouse button),
 * "mwu" (mouse wheel up), "mwd" (down),
 * "mwl" (left), "mwr" (right), "jb" (joystick button),
 * "jap" (joystick axis, positive), "jan" (joystick axis, negative).
 * The parameters are the key/button number, controller number,
 * controller stick and axis, etc.
 * @param s String to read from.
 * @return The input, or a default input instance on error.
 */
PlayerInputSource ControlsMediator::strToInputSource(
    const string &s
) const {
    PlayerInputSource input_source;
    
    vector<string> parts = split(s, "_");
    size_t n_parts = parts.size();
    
    if(n_parts == 0) return input_source;
    
    if(parts[0] == "k" && n_parts >= 2) {
        //Keyboard.
        input_source.type = INPUT_SOURCE_TYPE_KEYBOARD_KEY;
        input_source.buttonNr = s2i(parts[1]);
        
    } else if(parts[0] == "mb" && n_parts >= 2) {
        //Mouse button.
        input_source.type = INPUT_SOURCE_TYPE_MOUSE_BUTTON;
        input_source.buttonNr = s2i(parts[1]);
        
    } else if(parts[0] == "mwu") {
        //Mouse wheel up.
        input_source.type = INPUT_SOURCE_TYPE_MOUSE_WHEEL_UP;
        
    } else if(parts[0] == "mwd") {
        //Mouse wheel down.
        input_source.type = INPUT_SOURCE_TYPE_MOUSE_WHEEL_DOWN;
        
    } else if(parts[0] == "mwl") {
        //Mouse wheel left.
        input_source.type = INPUT_SOURCE_TYPE_MOUSE_WHEEL_LEFT;
        
    } else if(parts[0] == "mwr") {
        //Mouse wheel right.
        input_source.type = INPUT_SOURCE_TYPE_MOUSE_WHEEL_RIGHT;
        
    } else if(parts[0] == "jb" && n_parts >= 3) {
        //Controller button.
        input_source.type = INPUT_SOURCE_TYPE_CONTROLLER_BUTTON;
        input_source.deviceNr = s2i(parts[1]);
        input_source.buttonNr = s2i(parts[2]);
        
    } else if(parts[0] == "jap" && n_parts >= 4) {
        //Controller stick axis, positive.
        input_source.type = INPUT_SOURCE_TYPE_CONTROLLER_AXIS_POS;
        input_source.deviceNr = s2i(parts[1]);
        input_source.stickNr = s2i(parts[2]);
        input_source.axisNr = s2i(parts[3]);
        
    } else if(parts[0] == "jan" && n_parts >= 4) {
        //Controller stick axis, negative.
        input_source.type = INPUT_SOURCE_TYPE_CONTROLLER_AXIS_NEG;
        input_source.deviceNr = s2i(parts[1]);
        input_source.stickNr = s2i(parts[2]);
        input_source.axisNr = s2i(parts[3]);
        
    } else {
        game.errors.report(
            "Unrecognized input \"" + s + "\"!"
        );
    }
    
    return input_source;
}
