/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Script classes and related functions.
 */

#include <algorithm>

#include "../mob/mob.hpp"
#include "../../core/game.hpp"
#include "script.hpp"


#pragma region Script definition


/**
 * @brief Constructs a new script definition object.
 */
ScriptDef::ScriptDef() :
    fsm(this) {
    
}


/**
 * @brief Returns whether the current script context is valid for the given
 * context flags.
 *
 * @param flags The flags.
 * @return Whether it is valid.
 */
bool ScriptDef::checkContextFlags(Bitmask8 flags) const {
    int flagBitmask = getIdxBitmask(getContext());
    return hasFlag(flags, flagBitmask);
}


/**
 * @brief Returns what the current script context is.
 *
 * @return The context.
 */
SCRIPT_CONTEXT ScriptDef::getContext() const {
    return mobType ? SCRIPT_CONTEXT_MOB : SCRIPT_CONTEXT_AREA;
}


/**
 * @brief Returns the name of the current script context, in lowercase.
 *
 * @return The name.
 */
string ScriptDef::getContextName() const {
    switch(getContext()) {
    case SCRIPT_CONTEXT_MOB: {
        return "mob";
        break;
    } case SCRIPT_CONTEXT_AREA: {
        return "area";
        break;
    }
    }
    return "";
}


/**
 * @brief Loads a script definition from a data node.
 *
 * @param node Node to load from.
 * @return Whether it succeeded.
 */
bool ScriptDef::loadFromDataNode(DataNode* node) {
    bool success = true;
    
    //Init actions.
    success &=
        initActions.loadFromDataNode(
            node->getChildByName("init"),
            this
        );
        
    //Ready actions.
    success &=
        readyActions.loadFromDataNode(
            node->getChildByName("ready"),
            this
        );
        
    //The FSM.
    success &=
        fsm.loadFromDataNode(
            node->getChildByName("script"), node->getChildByName("global"),
            node->getChildByName("first_state"), node
        );
        
    return success;
}


/**
 * @brief Unloads the script definition and its contents from memory.
 */
void ScriptDef::unload() {
    readyActions.unload();
    initActions.unload();
    fsm.unload();
}


#pragma endregion
#pragma region Script VM


/**
 * @brief Constructs a new script VM object.
 */
ScriptVM::ScriptVM() {
    fsm.script = this;
}


/**
 * @brief Clears the script VM's data.
 */
void ScriptVM::clear() {
    scriptDef = nullptr;
    mob = nullptr;
    timer.stop();
    vars.clear();
}


/**
 * @brief Initializes the script VM, setting up its data, running its
 * init actions, etc.
 *
 * @param scriptDef Script definition it should follow.
 * @param mobPtr Whether there is an associated mob.
 */
void ScriptVM::init(ScriptDef* scriptDef, Mob* mobPtr) {
    this->scriptDef = scriptDef;
    mob = mobPtr;
    
    game.nConsecutiveScriptActions = 0;
    scriptDef->initActions.run(this);
    fsm.init();
}


/**
 * @brief Makes the script focus on the given mob.
 *
 * @param m2 The mob to focus on.
 */
void ScriptVM::focusOnMob(Mob* mob) {
    unfocusFromMob();
    focusedMob = mob;
}


/**
 * @brief Returns a string representing the values of all script vars,
 * formatted in a way that's friendly for the info maker tool.
 *
 * @return The string.
 */
string ScriptVM::getMakerToolVarsStr() const {
    string result = "Vars: ";
    
    if(vars.empty()) {
        result += "(None)";
        return result;
    }
    
    const map<string, string>& varMap = vars.toMap();
    for(const auto& v : varMap) {
        result += v.first + "=" + v.second + "; ";
    }
    result.erase(result.size() - 2, 2);
    result = wordWrap(result, 98, 2);
    
    return result;
}


/**
 * @brief Returns the mob under which the action should be run.
 *
 * @return The mob.
 */
Mob* ScriptVM::getRunnerMob() const {
    if(nextActionSurrogateMob) return nextActionSurrogateMob;
    return mob;
}


/**
 * @brief Returns the script VM under which the action should be run.
 *
 * @return The mob.
 */
ScriptVM* ScriptVM::getRunnerScriptVM() {
    if(nextActionSurrogateMob) return &nextActionSurrogateMob->scriptVM;
    return this;
}


/**
 * @brief Runs the block of actions that are meant to run when the
 * mob or area are fully loaded and ready.
 */
void ScriptVM::runReadyActions() {
    if(!scriptDef) return;
    scriptDef->readyActions.run(this);
}


/**
 * @brief Changes the timer's time and interval.
 *
 * @param time New time.
 */
void ScriptVM::setTimer(float time) {
    timer.duration = time;
    timer.start();
}


/**
 * @brief Ticks time by one frame of logic. This processes the timer and
 * runs some convenient events.
 *
 * @param deltaT How long the frame's tick is, in seconds.
 */
void ScriptVM::tick(float deltaT) {
    //Check the focused mob.
    if(focusedMob) {
    
        if(focusedMob->health <= 0) {
            fsm.runEvent(FSM_EV_FOCUS_DIED);
            fsm.runEvent(FSM_EV_FOCUS_OFF_REACH);
        }
        
    }
    
    //Timer event.
    FsmEventDef* timerEv = fsm.getEvent(FSM_EV_TIMER);
    if(timer.duration > 0) {
        if(timer.timeLeft > 0) {
            timer.tick(deltaT);
            if(timer.timeLeft == 0.0f && timerEv) {
                timerEv->run(this);
            }
        }
    }
    
    //Tick event.
    fsm.runEvent(FSM_EV_ON_TICK);
}


/**
 * @brief Makes the script lose focus on its currently focused mob.
 */
void ScriptVM::unfocusFromMob() {
    focusedMob = nullptr;
}


#pragma endregion
