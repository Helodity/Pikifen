/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Header for the script-related utilities and enums.
 */


#pragma once


//Contexts in which a script can run.
enum SCRIPT_CONTEXT {

    //Mob script.
    SCRIPT_CONTEXT_MOB,
    
    //Area script.
    SCRIPT_CONTEXT_AREA,
    
};

//Types of script construct.
enum SCRIPT_CONSTRUCT_TYPE {

    //None, or not applicable.
    SCRIPT_CONSTRUCT_TYPE_NONE,

    //Condition (if, else, else_if, end_if).
    SCRIPT_CONSTRUCT_TYPE_IF,

    //Do-while loop.
    SCRIPT_CONSTRUCT_TYPE_DO_WHILE,

    //While-do loop.
    SCRIPT_CONSTRUCT_TYPE_WHILE_DO,

    //For loop.
    SCRIPT_CONSTRUCT_TYPE_FOR,

    //For-each loop.
    SCRIPT_CONSTRUCT_TYPE_FOR_EACH,

};

buildEnumNames(constructTypeActionNames, SCRIPT_CONSTRUCT_TYPE)({
    { SCRIPT_CONSTRUCT_TYPE_NONE, "none" },
    { SCRIPT_CONSTRUCT_TYPE_IF, "if" },
    { SCRIPT_CONSTRUCT_TYPE_DO_WHILE, "do_while" },
    { SCRIPT_CONSTRUCT_TYPE_WHILE_DO, "while_do" },
    { SCRIPT_CONSTRUCT_TYPE_FOR, "for" },
    { SCRIPT_CONSTRUCT_TYPE_FOR_EACH, "for_each" },
});
