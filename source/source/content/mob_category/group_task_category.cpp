/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Group task mob category class.
 */

#include <algorithm>

#include "group_task_category.h"

#include "../../core/game.h"
#include "../mob/group_task.h"


/**
 * @brief Constructs a new group task category object.
 */
GroupTaskCategory::GroupTaskCategory() :
    MobCategory(
        MOB_CATEGORY_GROUP_TASKS, "group_task",
        "Group task", "Group tasks",
        "group_tasks", al_map_rgb(152, 204, 139)
    ) {
    
}


/**
 * @brief Clears the list of registered types of group tasks.
 */
void GroupTaskCategory::clearTypes() {
    for(auto& t : game.content.mobTypes.list.groupTask) {
        delete t.second;
    }
    game.content.mobTypes.list.groupTask.clear();
}


/**
 * @brief Creates a group task and adds it to the list of group tasks.
 *
 * @param pos Starting coordinates.
 * @param type Mob type.
 * @param angle Starting angle.
 * @return The mob.
 */
Mob* GroupTaskCategory::createMob(
    const Point& pos, MobType* type, float angle
) {
    GroupTask* m = new GroupTask(pos, (GroupTaskType*) type, angle);
    game.states.gameplay->mobs.groupTasks.push_back(m);
    return m;
}


/**
 * @brief Creates a new, empty type of group task.
 *
 * @return The type.
 */
MobType* GroupTaskCategory::createType() {
    return new GroupTaskType();
}


/**
 * @brief Clears a group task from the list of group task.
 *
 * @param m The mob to erase.
 */
void GroupTaskCategory::eraseMob(Mob* m) {
    game.states.gameplay->mobs.groupTasks.erase(
        find(
            game.states.gameplay->mobs.groupTasks.begin(),
            game.states.gameplay->mobs.groupTasks.end(),
            (GroupTask*) m
        )
    );
}


/**
 * @brief Returns a type of group task given its internal name,
 * or nullptr on error.
 *
 * @param internalName Internal name of the mob type to get.
 * @return The type.
 */
MobType* GroupTaskCategory::getType(const string& internalName) const {
    auto it = game.content.mobTypes.list.groupTask.find(internalName);
    if(it == game.content.mobTypes.list.groupTask.end()) return nullptr;
    return it->second;
}


/**
 * @brief Returns all types of group tasks by internal name.
 *
 * @param list This list gets filled with the mob type internal names.
 */
void GroupTaskCategory::getTypeNames(vector<string>& list) const {
    for(auto& t : game.content.mobTypes.list.groupTask) {
        list.push_back(t.first);
    }
}


/**
 * @brief Registers a created type of group task.
 *
 * @param internalName Internal name of the mob type.
 * @param type The mob type to register.
 */
void GroupTaskCategory::registerType(
    const string& internalName, MobType* type
) {
    game.content.mobTypes.list.groupTask[internalName] = (GroupTaskType*) type;
}
