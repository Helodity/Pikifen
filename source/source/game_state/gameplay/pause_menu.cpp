/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Pause menu classes and functions.
 */

#include <algorithm>

#include "gameplay.h"

#include "../../content/mob/resource.h"
#include "../../core/drawing.h"
#include "../../core/misc_functions.h"
#include "../../core/game.h"
#include "../../util/allegro_utils.h"
#include "../../util/general_utils.h"
#include "../../util/string_utils.h"


using DrawInfo = GuiItem::DrawInfo;


namespace PAUSE_MENU {

//Name of the leaving confirmation page GUI information file.
const string CONFIRMATION_GUI_FILE_NAME = "pause_confirmation";

//Control lockout time after entering the menu.
const float ENTRY_LOCKOUT_TIME = 0.15f;

//Interval between calculations of the Go Here path.
const float GO_HERE_CALC_INTERVAL = 0.15f;

//Name of the GUI information file.
const string GUI_FILE_NAME = "pause_menu";

//Name of the mission page GUI information file.
const string MISSION_GUI_FILE_NAME = "pause_mission";

//Default radar zoom level.
const float RADAR_DEF_ZOOM = 0.4f;

//Name of the radar page GUI information file.
const string RADAR_GUI_FILE_NAME = "pause_radar";

//Maximum radar zoom level.
const float RADAR_MAX_ZOOM = 4.0f;

//Minimum radar zoom level.
const float RADAR_MIN_ZOOM = 0.03f;

//How long an Onion waits before fading to the next color.
const float RADAR_ONION_COLOR_FADE_CYCLE_DUR = 1.0f;

//How long an Onion fades between two colors.
const float RADAR_ONION_COLOR_FADE_DUR = 0.2f;

//Max radar pan speed when not using mouse, in pixels per second.
const float RADAR_PAN_SPEED = 600.0f;

//Max radar zoom speed when not using mouse, in amount per second.
const float RADAR_ZOOM_SPEED = 2.5f;

//Name of the status page GUI information file.
const string STATUS_GUI_FILE_NAME = "pause_status";

}


/**
 * @brief Constructs a new pause menu struct object.
 *
 * @param start_on_radar True if the page to start on should be the radar,
 * false if it should be the system page.
 */
PauseMenu::PauseMenu(bool start_on_radar) {

    pages.push_back(PAUSE_MENU_PAGE_SYSTEM);
    pages.push_back(PAUSE_MENU_PAGE_RADAR);
    pages.push_back(PAUSE_MENU_PAGE_STATUS);
    if(game.curAreaData->type == AREA_TYPE_MISSION) {
        pages.push_back(PAUSE_MENU_PAGE_MISSION);
    }
    
    initMainPauseMenu();
    initRadarPage();
    initStatusPage();
    initMissionPage();
    initConfirmationPage();
    
    //Initialize some radar things.
    bool found_valid_sector = false;
    lowestSectorZ = FLT_MAX;
    highestSectorZ = -FLT_MAX;
    
    for(size_t s = 0; s < game.curAreaData->sectors.size(); s++) {
        Sector* s_ptr = game.curAreaData->sectors[s];
        if(s_ptr->type == SECTOR_TYPE_BLOCKING) continue;
        lowestSectorZ = std::min(lowestSectorZ, s_ptr->z);
        highestSectorZ = std::max(highestSectorZ, s_ptr->z);
        found_valid_sector = true;
    }
    
    if(!found_valid_sector || lowestSectorZ == highestSectorZ) {
        lowestSectorZ = -32.0f;
        highestSectorZ = 32.0f;
    }
    
    bool found_valid_edge = false;
    radarMinCoords = Point(FLT_MAX);
    radarMaxCoords = Point(-FLT_MAX);
    
    for(size_t e = 0; e < game.curAreaData->edges.size(); e++) {
        Edge* e_ptr = game.curAreaData->edges[e];
        if(!e_ptr->sectors[0] || !e_ptr->sectors[1]) continue;
        if(
            e_ptr->sectors[0]->type == SECTOR_TYPE_BLOCKING &&
            e_ptr->sectors[1]->type == SECTOR_TYPE_BLOCKING
        ) {
            continue;
        }
        found_valid_edge = true;
        updateMinMaxCoords(
            radarMinCoords, radarMaxCoords,
            v2p(e_ptr->vertexes[0])
        );
        updateMinMaxCoords(
            radarMinCoords, radarMaxCoords,
            v2p(e_ptr->vertexes[1])
        );
    }
    
    if(!found_valid_edge) {
        radarMinCoords = Point();
        radarMaxCoords = Point();
    }
    radarMinCoords = radarMinCoords - 16.0f;
    radarMaxCoords = radarMaxCoords + 16.0f;
    
    radarSelectedLeader = game.states.gameplay->curLeaderPtr;
    
    if(radarSelectedLeader) {
        radarCam.setPos(radarSelectedLeader->pos);
    }
    radarCam.set_zoom(game.states.gameplay->radarZoom);
    
    //Start the process.
    openingLockoutTimer = PAUSE_MENU::ENTRY_LOCKOUT_TIME;
    GuiManager* first_gui = start_on_radar ? &radarGui : &gui;
    first_gui->responsive = true;
    first_gui->startAnimation(
        GUI_MANAGER_ANIM_UP_TO_CENTER, GAMEPLAY::MENU_ENTRY_HUD_MOVE_TIME
    );
}


/**
 * @brief Destroys the pause menu struct object.
 */
PauseMenu::~PauseMenu() {
    gui.destroy();
    radarGui.destroy();
    statusGui.destroy();
    missionGui.destroy();
    confirmationGui.destroy();
    
    if(secondaryMenu) {
        secondaryMenu->unload();
        delete secondaryMenu;
        secondaryMenu = nullptr;
    }
    
    game.content.bitmaps.list.free(bmpRadarCursor);
    game.content.bitmaps.list.free(bmpRadarPikmin);
    game.content.bitmaps.list.free(bmpRadarTreasure);
    game.content.bitmaps.list.free(bmpRadarEnemyAlive);
    game.content.bitmaps.list.free(bmpRadarEnemyDead);
    game.content.bitmaps.list.free(bmpRadarLeaderBubble);
    game.content.bitmaps.list.free(bmpRadarLeaderX);
    game.content.bitmaps.list.free(bmpRadarObstacle);
    game.content.bitmaps.list.free(bmpRadarOnionSkeleton);
    game.content.bitmaps.list.free(bmpRadarOnionBulb);
    game.content.bitmaps.list.free(bmpRadarShip);
    game.content.bitmaps.list.free(bmpRadarPath);
    bmpRadarCursor = nullptr;
    bmpRadarPikmin = nullptr;
    bmpRadarTreasure = nullptr;
    bmpRadarEnemyAlive = nullptr;
    bmpRadarEnemyDead = nullptr;
    bmpRadarLeaderBubble = nullptr;
    bmpRadarLeaderX = nullptr;
    bmpRadarObstacle = nullptr;
    bmpRadarOnionSkeleton = nullptr;
    bmpRadarOnionBulb = nullptr;
    bmpRadarShip = nullptr;
    bmpRadarPath = nullptr;
}


/**
 * @brief Adds a new bullet point to either the fail condition list, or the
 * grading explanation list.
 *
 * @param list List to add to.
 * @param text Text.
 * @param color Text color.
 */
void PauseMenu::addBullet(
    ListGuiItem* list, const string &text,
    const ALLEGRO_COLOR &color
) {
    size_t bullet_idx = list->children.size();
    const float BULLET_HEIGHT = 0.18f;
    const float BULLET_PADDING = 0.01f;
    const float BULLETS_OFFSET = 0.01f;
    const float bullet_center_y =
        (BULLETS_OFFSET + BULLET_HEIGHT / 2.0f) +
        ((BULLET_HEIGHT + BULLET_PADDING) * bullet_idx);
        
    BulletGuiItem* bullet =
        new BulletGuiItem(
        text, game.sysContent.fntStandard, color
    );
    bullet->ratioCenter = Point(0.50f, bullet_center_y);
    bullet->ratioSize = Point(0.96f, BULLET_HEIGHT);
    list->addChild(bullet);
    missionGui.addItem(bullet);
}


/**
 * @brief Adds a new line to one of the Pikmin status boxes.
 *
 * @param list List to add to.
 * @param pik_type Relevant Pikmin type, if applicable.
 * @param group_text Text to display on the "group" cell.
 * @param idle_text Text to display on the "idle" cell.
 * @param field_text Text to display on the "field" cell.
 * @param onion_text Text to display on the "Onion" cell.
 * @param total_text Text to display on the "total" cell.
 * @param new_text Text to display on the "new" cell.
 * @param lost_text Text to display on the "lost" cell.
 * @param is_single True if this is a box with a single row.
 * @param is_totals True if this is the totals box.
 */
void PauseMenu::addPikminStatusLine(
    ListGuiItem* list,
    PikminType* pik_type,
    const string &group_text,
    const string &idle_text,
    const string &field_text,
    const string &onion_text,
    const string &total_text,
    const string &new_text,
    const string &lost_text,
    bool is_single, bool is_totals
) {

    const float x1 = 0.00f;
    const float x2 = 1.00f;
    const float working_width = x2 - x1;
    const float item_x_interval = working_width / 8.0f;
    const float first_x = x1 + item_x_interval / 2.0f;
    const float item_width = item_x_interval - 0.02f;
    
    const float y1 = is_single ? 0.0f : list->getChildBottom();
    const float item_height = is_single ? 1.0f : 0.17f;
    const float number_item_height = is_single ? item_height : item_height * 0.60f;
    const float item_y_spacing = is_single ? 0.0f : 0.03f;
    const float item_y = y1 + item_height / 2.0f + item_y_spacing;
    
    ALLEGRO_FONT* font =
        (is_single && !is_totals) ? game.sysContent.fntStandard : game.sysContent.fntCounter;
    string tooltip_start =
        pik_type ?
        "Number of " + pik_type->name + " " :
        "Total number of Pikmin ";
    bool can_select = pik_type || is_totals;
    
    if(pik_type) {
    
        //Pikmin type.
        GuiItem* type_item = new GuiItem();
        type_item->onDraw =
        [pik_type] (const DrawInfo & draw) {
            drawBitmapInBox(
                pik_type->bmpIcon, draw.center, draw.size, true
            );
        };
        type_item->ratioCenter =
            Point(
                first_x + item_x_interval * 0,
                item_y
            );
        type_item->ratioSize = Point(item_width, item_height);
        list->addChild(type_item);
        statusGui.addItem(type_item);
        
    } else if(is_totals) {
    
        //Totals header.
        TextGuiItem* totals_header_item =
            new TextGuiItem("Total", game.sysContent.fntAreaName);
        totals_header_item->ratioCenter =
            Point(
                first_x + item_x_interval * 0,
                item_y
            );
        totals_header_item->ratioSize = Point(item_width, item_height);
        list->addChild(totals_header_item);
        statusGui.addItem(totals_header_item);
        
    }
    
    //Group Pikmin.
    TextGuiItem* group_text_item =
        new TextGuiItem(group_text, font);
    group_text_item->selectable = can_select;
    group_text_item->showSelectionBox = can_select;
    group_text_item->ratioCenter =
        Point(
            first_x + item_x_interval * 1,
            item_y
        );
    group_text_item->ratioSize = Point(item_width, number_item_height);
    if(can_select) {
        group_text_item->onGetTooltip =
        [tooltip_start] () {
            return tooltip_start + "in your active leader's group.";
        };
    }
    if(group_text == "0") {
        group_text_item->color = changeAlpha(group_text_item->color, 64);
    }
    list->addChild(group_text_item);
    statusGui.addItem(group_text_item);
    
    //Idle Pikmin.
    TextGuiItem* idle_text_item =
        new TextGuiItem(idle_text, font);
    idle_text_item->selectable = can_select;
    idle_text_item->showSelectionBox = can_select;
    idle_text_item->ratioCenter =
        Point(
            first_x + item_x_interval * 2,
            item_y
        );
    idle_text_item->ratioSize = Point(item_width, number_item_height);
    if(can_select) {
        idle_text_item->onGetTooltip =
        [tooltip_start] () {
            return tooltip_start + "idling in the field.";
        };
    }
    if(idle_text == "0") {
        idle_text_item->color = changeAlpha(idle_text_item->color, 64);
    }
    list->addChild(idle_text_item);
    statusGui.addItem(idle_text_item);
    
    //Field Pikmin.
    TextGuiItem* field_text_item =
        new TextGuiItem(field_text, font);
    field_text_item->selectable = can_select;
    field_text_item->showSelectionBox = can_select;
    field_text_item->ratioCenter =
        Point(
            first_x + item_x_interval * 3,
            item_y
        );
    field_text_item->ratioSize = Point(item_width, number_item_height);
    if(can_select) {
        field_text_item->onGetTooltip =
        [tooltip_start] () {
            return tooltip_start + "out in the field.";
        };
    }
    if(field_text == "0") {
        field_text_item->color = changeAlpha(field_text_item->color, 64);
    }
    list->addChild(field_text_item);
    statusGui.addItem(field_text_item);
    
    //Onion Pikmin.
    TextGuiItem* onion_text_item =
        new TextGuiItem(onion_text, font);
    onion_text_item->selectable = can_select;
    onion_text_item->showSelectionBox = can_select;
    onion_text_item->ratioCenter =
        Point(
            first_x + item_x_interval * 4,
            item_y
        );
    onion_text_item->ratioSize = Point(item_width, number_item_height);
    if(can_select) {
        onion_text_item->onGetTooltip =
        [tooltip_start] () {
            return tooltip_start + "inside Onions.";
        };
    }
    if(onion_text == "0") {
        onion_text_item->color = changeAlpha(onion_text_item->color, 64);
    }
    list->addChild(onion_text_item);
    statusGui.addItem(onion_text_item);
    
    //Total Pikmin.
    TextGuiItem* total_text_item =
        new TextGuiItem(total_text, font, COLOR_GOLD);
    total_text_item->selectable = can_select;
    total_text_item->showSelectionBox = can_select;
    total_text_item->ratioCenter =
        Point(
            first_x + item_x_interval * 5,
            item_y
        );
    total_text_item->ratioSize = Point(item_width, number_item_height);
    if(can_select) {
        total_text_item->onGetTooltip =
        [tooltip_start] () {
            return tooltip_start + "you have.";
        };
    }
    if(total_text == "0") {
        total_text_item->color = changeAlpha(total_text_item->color, 64);
    }
    list->addChild(total_text_item);
    statusGui.addItem(total_text_item);
    
    //Separator.
    GuiItem* separator_item = new GuiItem();
    separator_item->ratioCenter =
        Point(first_x + item_x_interval * 5.5f, item_y);
    separator_item->ratioSize = Point(1.0f, item_height);
    separator_item->onDraw =
    [] (const DrawInfo & draw) {
        al_draw_line(
            draw.center.x, draw.center.y - draw.size.y / 2.0f,
            draw.center.x, draw.center.y + draw.size.y / 2.0f,
            COLOR_TRANSPARENT_WHITE, 5.0f
        );
    };
    list->addChild(separator_item);
    statusGui.addItem(separator_item);
    
    //New Pikmin.
    TextGuiItem* new_text_item =
        new TextGuiItem(new_text, font, al_map_rgb(210, 255, 210));
    new_text_item->selectable = can_select;
    new_text_item->showSelectionBox = can_select;
    new_text_item->ratioCenter =
        Point(
            first_x + item_x_interval * 6,
            item_y
        );
    new_text_item->ratioSize = Point(item_width, number_item_height);
    if(can_select) {
        new_text_item->onGetTooltip =
        [tooltip_start] () {
            return tooltip_start + "born today.";
        };
    }
    if(new_text == "0") {
        new_text_item->color = changeAlpha(new_text_item->color, 64);
    }
    list->addChild(new_text_item);
    statusGui.addItem(new_text_item);
    
    //Lost Pikmin.
    TextGuiItem* lost_text_item =
        new TextGuiItem(lost_text, font, al_map_rgb(255, 210, 210));
    lost_text_item->selectable = can_select;
    lost_text_item->showSelectionBox = can_select;
    lost_text_item->ratioCenter =
        Point(
            first_x + item_x_interval * 7,
            item_y
        );
    lost_text_item->ratioSize = Point(item_width, number_item_height);
    if(can_select) {
        lost_text_item->onGetTooltip =
        [tooltip_start] () {
            return tooltip_start + "lost today.";
        };
    }
    if(lost_text == "0") {
        lost_text_item->color = changeAlpha(lost_text_item->color, 64);
    }
    list->addChild(lost_text_item);
    statusGui.addItem(lost_text_item);
}


/**
 * @brief Calculates the Go Here path from the selected leader to the radar
 * cursor, if applicable, and stores the results in goHerePath and
 * goHerePathResult.
 */
void PauseMenu::calculateGoHerePath() {
    radarCursorLeader = nullptr;
    for(size_t l = 0; l < game.states.gameplay->mobs.leaders.size(); l++) {
        Leader* l_ptr = game.states.gameplay->mobs.leaders[l];
        if(
            l_ptr->health > 0 &&
            Distance(l_ptr->pos, radarCursor) <= 24.0f / radarCam.zoom
        ) {
            radarCursorLeader = l_ptr;
            break;
        }
    }
    
    if(
        !radarSelectedLeader ||
        radarCursorLeader ||
        Distance(radarSelectedLeader->pos, radarCursor) < 128.0f
    ) {
        goHerePath.clear();
        goHerePathResult = PATH_RESULT_ERROR;
        return;
    }
    
    if(!radarSelectedLeader->fsm.getEvent(LEADER_EV_GO_HERE)) {
        goHerePath.clear();
        goHerePathResult = PATH_RESULT_ERROR;
        return;
    }
    
    Sector* cursor_sector = getSector(radarCursor, nullptr, true);
    
    if(!cursor_sector || cursor_sector->type == SECTOR_TYPE_BLOCKING) {
        goHerePath.clear();
        goHerePathResult = PATH_RESULT_ERROR;
        return;
    }
    
    PathFollowSettings settings;
    settings.flags =
        PATH_FOLLOW_FLAG_CAN_CONTINUE | PATH_FOLLOW_FLAG_LIGHT_LOAD;
    settings.invulnerabilities =
        radarSelectedLeader->group->getGroupInvulnerabilities(
            radarSelectedLeader
        );
        
    goHerePathResult =
        getPath(
            radarSelectedLeader->pos,
            radarCursor,
            settings,
            goHerePath, nullptr, nullptr, nullptr
        );
}


/**
 * @brief Either asks the player to confirm if they wish to leave, or leaves
 * outright, based on the player's confirmation question preferences.
 *
 */
void PauseMenu::confirmOrLeave() {
    bool do_confirmation = false;
    switch(game.options.misc.leavingConfMode) {
    case LEAVING_CONF_MODE_NEVER: {
        do_confirmation = false;
        break;
    } case LEAVING_CONF_MODE_1_MIN: {
        do_confirmation =
            game.states.gameplay->gameplayTimePassed >= 60.0f;
        break;
    } case LEAVING_CONF_MODE_ALWAYS: {
        do_confirmation = true;
        break;
    } case N_LEAVING_CONF_MODES: {
        break;
    }
    }
    
    if(do_confirmation) {
        switch(leaveTarget) {
        case GAMEPLAY_LEAVE_TARGET_RETRY: {
            confirmationExplanationText->text =
                "If you retry, you will LOSE all of your progress "
                "and start over. Are you sure you want to retry?";
            break;
        } case GAMEPLAY_LEAVE_TARGET_END: {
            confirmationExplanationText->text =
                "If you end now, you will stop playing and will go to the "
                "results menu.";
            if(game.curAreaData->type == AREA_TYPE_MISSION) {
                if(
                    game.curAreaData->mission.goal ==
                    MISSION_GOAL_END_MANUALLY
                ) {
                    confirmationExplanationText->text +=
                        " The goal of this mission is to end through here, so "
                        "make sure you've done everything you need first.";
                } else {
                    confirmationExplanationText->text +=
                        " This will end the mission as a fail, "
                        "even though you may still get a medal from it.";
                    if(
                        game.curAreaData->mission.gradingMode ==
                        MISSION_GRADING_MODE_POINTS
                    ) {
                        confirmationExplanationText->text +=
                            " Note that since you fail the mission, you may "
                            "lose out on some points. You should check the "
                            "pause menu's mission page for more information.";
                    }
                    
                }
            }
            confirmationExplanationText->text +=
                " Are you sure you want to end?";
            break;
        } case GAMEPLAY_LEAVE_TARGET_AREA_SELECT: {
            confirmationExplanationText->text =
                "If you quit, you will LOSE all of your progress and instantly "
                "stop playing. Are you sure you want to quit?";
            break;
        }
        }
        
        gui.responsive = false;
        gui.startAnimation(
            GUI_MANAGER_ANIM_CENTER_TO_UP,
            GAMEPLAY::MENU_EXIT_HUD_MOVE_TIME
        );
        confirmationGui.responsive = true;
        confirmationGui.startAnimation(
            GUI_MANAGER_ANIM_UP_TO_CENTER,
            GAMEPLAY::MENU_EXIT_HUD_MOVE_TIME
        );
        
    } else {
        startLeavingGameplay();
    }
}


/**
 * @brief Creates a button meant for changing to a page either to the left or
 * to the right of the current one.
 *
 * @param target_page Which page this button leads to.
 * @param left True if this page is to the left of the current,
 * false if to the right.
 * @param cur_gui Pointer to the current page's GUI manager.
 * @return The button.
 */
ButtonGuiItem* PauseMenu::createPageButton(
    PAUSE_MENU_PAGE target_page, bool left,
    GuiManager* cur_gui
) {
    string page_name;
    string tooltip_name;
    switch(target_page) {
    case PAUSE_MENU_PAGE_SYSTEM: {
        page_name = "System";
        tooltip_name = "system";
        break;
    } case PAUSE_MENU_PAGE_RADAR: {
        page_name = "Radar";
        tooltip_name = "radar";
        break;
    } case PAUSE_MENU_PAGE_STATUS: {
        page_name = "Status";
        tooltip_name = "status";
        break;
    } case PAUSE_MENU_PAGE_MISSION: {
        page_name = "Mission";
        tooltip_name = "mission";
        break;
    }
    }
    
    ButtonGuiItem* new_button =
        new ButtonGuiItem(
        left ?
        "< " + page_name :
        page_name + " >",
        game.sysContent.fntStandard
    );
    new_button->onActivate =
    [this, cur_gui, target_page, left] (const Point &) {
        switchPage(cur_gui, target_page, left);
    };
    new_button->onGetTooltip =
    [tooltip_name] () {
        return "Go to the pause menu's " + tooltip_name + " page.";
    };
    
    return new_button;
}


/**
 * @brief Creates the buttons and input GUI items that allow switching pages.
 *
 * @param cur_page Page that these creations belong to.
 * @param cur_gui Pointer to the current page's GUI manager.
 */
void PauseMenu::createPageButtons(
    PAUSE_MENU_PAGE cur_page, GuiManager* cur_gui
) {
    size_t cur_page_idx =
        std::distance(
            pages.begin(),
            std::find(pages.begin(), pages.end(), cur_page)
        );
    size_t left_page_idx = sumAndWrap((int) cur_page_idx, -1, (int) pages.size());
    size_t right_page_idx = sumAndWrap((int) cur_page_idx, 1, (int) pages.size());
    
    //Left page button.
    ButtonGuiItem* left_page_button =
        createPageButton(pages[left_page_idx], true, cur_gui);
    cur_gui->addItem(left_page_button, "left_page");
    
    //Left page input icon.
    GuiItem* left_page_input = new GuiItem();
    left_page_input->onDraw =
    [this] (const DrawInfo & draw) {
        if(!game.options.misc.showHudInputIcons) return;
        const PlayerInputSource &s =
            game.controls.findBind(PLAYER_ACTION_TYPE_MENU_PAGE_LEFT).
            inputSource;
        if(s.type == INPUT_SOURCE_TYPE_NONE) return;
        drawPlayerInputSourceIcon(
            game.sysContent.fntSlim, s, true, draw.center, draw.size
        );
    };
    cur_gui->addItem(left_page_input, "left_page_input");
    
    //Right page button.
    ButtonGuiItem* right_page_button =
        createPageButton(pages[right_page_idx], false, cur_gui);
    cur_gui->addItem(right_page_button, "right_page");
    
    //Right page input icon.
    GuiItem* right_page_input = new GuiItem();
    right_page_input->onDraw =
    [this] (const DrawInfo & draw) {
        if(!game.options.misc.showHudInputIcons) return;
        const PlayerInputSource &s =
            game.controls.findBind(PLAYER_ACTION_TYPE_MENU_PAGE_RIGHT).
            inputSource;
        if(s.type == INPUT_SOURCE_TYPE_NONE) return;
        drawPlayerInputSourceIcon(
            game.sysContent.fntSlim, s, true, draw.center, draw.size
        );
    };
    cur_gui->addItem(right_page_input, "right_page_input");
}


/**
 * @brief Draws the pause menu.
 */
void PauseMenu::draw() {
    gui.draw();
    radarGui.draw();
    statusGui.draw();
    missionGui.draw();
    confirmationGui.draw();
    if(secondaryMenu) secondaryMenu->draw();
}


/**
 * @brief Draws a segment of the Go Here path.
 *
 * @param start Starting point.
 * @param end Ending point.
 * @param color Color of the segment.
 * @param texture_point Pointer to a variable keeping track of what point of
 * the texture we've drawn so far for this path, so that the effect is seamless
 * between segments.
 */
void PauseMenu::drawGoHereSegment(
    const Point &start, const Point &end,
    const ALLEGRO_COLOR &color, float* texture_point
) {
    const float PATH_SEGMENT_THICKNESS = 12.0f / radarCam.zoom;
    const float PATH_SEGMENT_TIME_MULT = 10.0f;
    
    ALLEGRO_VERTEX av[4];
    for(unsigned char a = 0; a < 4; a++) {
        av[a].color = color;
        av[a].z = 0.0f;
    }
    int bmp_h = al_get_bitmap_height(bmpRadarPath);
    float texture_scale = bmp_h / PATH_SEGMENT_THICKNESS / radarCam.zoom;
    float angle = getAngle(start, end);
    float distance = Distance(start, end).toFloat() * radarCam.zoom;
    float texture_offset = game.timePassed * PATH_SEGMENT_TIME_MULT;
    float texture_start = *texture_point;
    float texture_end = texture_start + distance;
    Point rot_offset = rotatePoint(Point(0, PATH_SEGMENT_THICKNESS), angle);
    
    av[0].x = start.x - rot_offset.x;
    av[0].y = start.y - rot_offset.y;
    av[1].x = start.x + rot_offset.x;
    av[1].y = start.y + rot_offset.y;
    av[2].x = end.x - rot_offset.x;
    av[2].y = end.y - rot_offset.y;
    av[3].x = end.x + rot_offset.x;
    av[3].y = end.y + rot_offset.y;
    
    av[0].u = (texture_start - texture_offset) * texture_scale;
    av[0].v = 0.0f;
    av[1].u = (texture_start - texture_offset) * texture_scale;
    av[1].v = bmp_h;
    av[2].u = (texture_end - texture_offset) * texture_scale;
    av[2].v = 0.0f;
    av[3].u = (texture_end - texture_offset) * texture_scale;
    av[3].v = bmp_h;
    
    al_draw_prim(
        av, nullptr, bmpRadarPath, 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP
    );
    
    *texture_point = texture_end;
}


/**
 * @brief Draws the radar itself.
 *
 * @param center Center coordinates of the radar on-screen.
 * @param size Width and height of the radar on-screen.
 */
void PauseMenu::drawRadar(
    const Point &center, const Point &size
) {
    //Setup.
    ALLEGRO_TRANSFORM old_transform;
    int old_cr_x = 0;
    int old_cr_y = 0;
    int old_cr_w = 0;
    int old_cr_h = 0;
    al_copy_transform(&old_transform, al_get_current_transform());
    al_get_clipping_rectangle(&old_cr_x, &old_cr_y, &old_cr_w, &old_cr_h);
    
    al_use_transform(&worldToRadarScreenTransform);
    al_set_clipping_rectangle(
        center.x - size.x / 2.0f,
        center.y - size.y / 2.0f,
        size.x,
        size.y
    );
    
    //Background fill.
    al_clear_to_color(game.config.aestheticRadar.backgroundColor);
    
    //Draw each sector.
    for(size_t s = 0; s < game.curAreaData->sectors.size(); s++) {
        Sector* s_ptr = game.curAreaData->sectors[s];
        
        if(s_ptr->type == SECTOR_TYPE_BLOCKING) continue;
        ALLEGRO_COLOR color =
            interpolateColor(
                s_ptr->z, lowestSectorZ, highestSectorZ,
                game.config.aestheticRadar.lowestColor,
                game.config.aestheticRadar.highestColor
            );
            
        for(size_t h = 0; h < s_ptr->hazards.size(); h++) {
            if(!s_ptr->hazards[h]->associatedLiquid) continue;
            color =
                interpolateColor(
                    0.80f, 0.0f, 1.0f,
                    color, s_ptr->hazards[h]->associatedLiquid->radarColor
                );
        }
        
        for(size_t t = 0; t < s_ptr->triangles.size(); t++) {
            ALLEGRO_VERTEX av[3];
            for(size_t v = 0; v < 3; v++) {
                av[v].u = 0;
                av[v].v = 0;
                av[v].x = s_ptr->triangles[t].points[v]->x;
                av[v].y = s_ptr->triangles[t].points[v]->y;
                av[v].z = 0;
                av[v].color = color;
            }
            
            al_draw_prim(
                av, nullptr, nullptr,
                0, 3, ALLEGRO_PRIM_TRIANGLE_LIST
            );
        }
    }
    
    //Draw each edge.
    for(size_t e = 0; e < game.curAreaData->edges.size(); e++) {
        Edge* e_ptr = game.curAreaData->edges[e];
        
        if(!e_ptr->sectors[0] || !e_ptr->sectors[1]) {
            //The other side is already the void, so no need for an edge.
            continue;
        }
        
        if(
            fabs(e_ptr->sectors[0]->z - e_ptr->sectors[1]->z) <=
            GEOMETRY::STEP_HEIGHT
        ) {
            //Step.
            continue;
        }
        
        al_draw_line(
            e_ptr->vertexes[0]->x,
            e_ptr->vertexes[0]->y,
            e_ptr->vertexes[1]->x,
            e_ptr->vertexes[1]->y,
            game.config.aestheticRadar.edgeColor,
            1.5f / radarCam.zoom
        );
    }
    
    //Onion icons.
    for(size_t o = 0; o < game.states.gameplay->mobs.onions.size(); o++) {
        Onion* o_ptr =
            game.states.gameplay->mobs.onions[o];
        vector<PikminType*>* pik_types_ptr =
            &o_ptr->nest->nest_type->pik_types;
            
        size_t nr_pik_types = pik_types_ptr->size();
        if(nr_pik_types > 0) {
            float fade_cycle_pos =
                std::min(
                    (float) fmod(
                        game.timePassed,
                        PAUSE_MENU::RADAR_ONION_COLOR_FADE_CYCLE_DUR
                    ),
                    PAUSE_MENU::RADAR_ONION_COLOR_FADE_DUR
                );
                
            size_t pik_type_idx_target =
                (int) (
                    game.timePassed /
                    PAUSE_MENU::RADAR_ONION_COLOR_FADE_CYCLE_DUR
                ) % nr_pik_types;
            size_t pik_type_idx_prev =
                (pik_type_idx_target + nr_pik_types - 1) % nr_pik_types;
                
            ALLEGRO_COLOR target_color =
                interpolateColor(
                    fade_cycle_pos, 0.0f,
                    PAUSE_MENU::RADAR_ONION_COLOR_FADE_DUR,
                    pik_types_ptr->at(pik_type_idx_prev)->mainColor,
                    pik_types_ptr->at(pik_type_idx_target)->mainColor
                );
                
            drawBitmap(
                bmpRadarOnionBulb, o_ptr->pos,
                Point(24.0f / radarCam.zoom),
                0.0f,
                target_color
            );
        }
        drawBitmap(
            bmpRadarOnionSkeleton, o_ptr->pos,
            Point(24.0f / radarCam.zoom)
        );
    }
    
    //Ship icons.
    for(size_t s = 0; s < game.states.gameplay->mobs.ships.size(); s++) {
        Ship* s_ptr = game.states.gameplay->mobs.ships[s];
        
        drawBitmap(
            bmpRadarShip, s_ptr->pos,
            Point(24.0f / radarCam.zoom)
        );
    }
    
    //Enemy icons.
    for(size_t e = 0; e < game.states.gameplay->mobs.enemies.size(); e++) {
        Enemy* e_ptr = game.states.gameplay->mobs.enemies[e];
        if(e_ptr->parent) continue;
        
        drawBitmap(
            e_ptr->health > 0 ? bmpRadarEnemyAlive : bmpRadarEnemyDead,
            e_ptr->pos,
            Point(24.0f / radarCam.zoom),
            e_ptr->health > 0 ? game.timePassed : 0.0f
        );
    }
    
    //Leader icons.
    for(size_t l = 0; l < game.states.gameplay->mobs.leaders.size(); l++) {
        Leader* l_ptr = game.states.gameplay->mobs.leaders[l];
        
        drawBitmap(
            l_ptr->leaType->bmpIcon, l_ptr->pos,
            Point(40.0f / radarCam.zoom)
        );
        drawBitmap(
            bmpRadarLeaderBubble, l_ptr->pos,
            Point(48.0f / radarCam.zoom),
            0.0f,
            radarSelectedLeader == l_ptr ?
            al_map_rgb(0, 255, 255) :
            COLOR_WHITE
        );
        drawFilledEquilateralTriangle(
            l_ptr->pos +
            rotatePoint(Point(24.5f / radarCam.zoom, 0.0f), l_ptr->angle),
            6.0f / radarCam.zoom,
            l_ptr->angle,
            radarSelectedLeader == l_ptr ?
            al_map_rgb(0, 255, 255) :
            l_ptr->health > 0 ?
            COLOR_WHITE :
            al_map_rgb(128, 128, 128)
        );
        if(l_ptr->health <= 0) {
            drawBitmap(
                bmpRadarLeaderX, l_ptr->pos,
                Point(36.0f / radarCam.zoom)
            );
        }
    }
    
    //Treasure icons.
    for(size_t t = 0; t < game.states.gameplay->mobs.treasures.size(); t++) {
        Treasure* t_ptr = game.states.gameplay->mobs.treasures[t];
        
        drawBitmap(
            bmpRadarTreasure, t_ptr->pos,
            Point(32.0f / radarCam.zoom),
            sin(game.timePassed * 2.0f) * (TAU * 0.05f)
        );
    }
    for(size_t r = 0; r < game.states.gameplay->mobs.resources.size(); r++) {
        Resource* r_ptr = game.states.gameplay->mobs.resources[r];
        if(
            r_ptr->resType->deliveryResult !=
            RESOURCE_DELIVERY_RESULT_ADD_TREASURE_POINTS
        ) {
            continue;
        }
        
        drawBitmap(
            bmpRadarTreasure, r_ptr->pos,
            Point(32.0f / radarCam.zoom),
            sin(game.timePassed * 2.0f) * (TAU * 0.05f)
        );
    }
    for(size_t p = 0; p < game.states.gameplay->mobs.piles.size(); p++) {
        Pile* p_ptr = game.states.gameplay->mobs.piles[p];
        if(
            !p_ptr->pilType->contents ||
            p_ptr->amount == 0 ||
            p_ptr->pilType->contents->deliveryResult !=
            RESOURCE_DELIVERY_RESULT_ADD_TREASURE_POINTS
        ) {
            continue;
        }
        
        drawBitmap(
            bmpRadarTreasure, p_ptr->pos,
            Point(32.0f / radarCam.zoom),
            sin(game.timePassed * 2.0f) * (TAU * 0.05f)
        );
    }
    
    //Pikmin icons.
    for(size_t p = 0; p < game.states.gameplay->mobs.pikmin.size(); p++) {
        Pikmin* p_ptr = game.states.gameplay->mobs.pikmin[p];
        
        drawBitmap(
            bmpRadarPikmin, p_ptr->pos,
            Point(16.0f / radarCam.zoom),
            0.0f,
            p_ptr->pikType->mainColor
        );
    }
    
    //Obstacle icons.
    unordered_set<Mob*> obstacles;
    for(const auto &o : game.states.gameplay->pathMgr.obstructions) {
        obstacles.insert(o.second.begin(), o.second.end());
    }
    for(const auto &o : obstacles) {
        drawBitmap(
            bmpRadarObstacle, o->pos,
            Point(40.0f / radarCam.zoom),
            o->angle
        );
    }
    
    //Currently-active Go Here paths.
    for(size_t l = 0; l < game.states.gameplay->mobs.leaders.size(); l++) {
        Leader* l_ptr = game.states.gameplay->mobs.leaders[l];
        if(!l_ptr->midGoHere) continue;
        
        float path_texture_point = 0.0f;
        ALLEGRO_COLOR color = al_map_rgba(120, 140, 170, 192);
        
        switch(l_ptr->pathInfo->result) {
        case PATH_RESULT_DIRECT:
        case PATH_RESULT_DIRECT_NO_STOPS: {
            //Go directly from A to B.
            
            drawGoHereSegment(
                l_ptr->pos,
                l_ptr->pathInfo->settings.targetPoint,
                color, &path_texture_point
            );
            
            break;
            
        } case PATH_RESULT_NORMAL_PATH:
        case PATH_RESULT_PATH_WITH_SINGLE_STOP:
        case PATH_RESULT_PATH_WITH_OBSTACLES: {
    
            size_t first_stop = l_ptr->pathInfo->cur_path_stop_idx;
            if(first_stop >= l_ptr->pathInfo->path.size()) continue;
            
            drawGoHereSegment(
                l_ptr->pos,
                l_ptr->pathInfo->path[first_stop]->pos,
                color, &path_texture_point
            );
            for(
                size_t s = first_stop + 1;
                s < l_ptr->pathInfo->path.size();
                s++
            ) {
                drawGoHereSegment(
                    l_ptr->pathInfo->path[s - 1]->pos,
                    l_ptr->pathInfo->path[s]->pos,
                    color, &path_texture_point
                );
            }
            drawGoHereSegment(
                l_ptr->pathInfo->path.back()->pos,
                l_ptr->pathInfo->settings.targetPoint,
                color, &path_texture_point
            );
            
            break;
            
        } default: {
    
            break;
        }
        }
    }
    
    //Go Here choice path.
    float path_texture_point = 0.0f;
    switch(goHerePathResult) {
    case PATH_RESULT_DIRECT:
    case PATH_RESULT_DIRECT_NO_STOPS: {
        //Go directly from A to B.
        
        drawGoHereSegment(
            radarSelectedLeader->pos,
            radarCursor,
            al_map_rgb(64, 200, 240), &path_texture_point
        );
        
        break;
        
    } case PATH_RESULT_NORMAL_PATH:
    case PATH_RESULT_PATH_WITH_SINGLE_STOP:
    case PATH_RESULT_PATH_WITH_OBSTACLES: {
        //Regular path.
        ALLEGRO_COLOR color;
        if(goHerePathResult == PATH_RESULT_PATH_WITH_OBSTACLES) {
            color = al_map_rgb(200, 64, 64);
        } else {
            color = al_map_rgb(64, 200, 240);
        }
        
        if(!goHerePath.empty()) {
            drawGoHereSegment(
                radarSelectedLeader->pos,
                goHerePath[0]->pos,
                color, &path_texture_point
            );
            for(size_t s = 1; s < goHerePath.size(); s++) {
                drawGoHereSegment(
                    goHerePath[s - 1]->pos,
                    goHerePath[s]->pos,
                    color, &path_texture_point
                );
            }
            drawGoHereSegment(
                goHerePath.back()->pos,
                radarCursor,
                color, &path_texture_point
            );
        }
        
        break;
        
    } default: {

        break;
    }
    }
    
    //Radar cursor.
    drawBitmap(
        bmpRadarCursor, radarCursor,
        Point(48.0f / radarCam.zoom),
        game.timePassed * TAU * 0.3f
    );
    
    //Debugging feature -- show area active cells.
    /*
    for(
        size_t cell_x = 0;
        cell_x < game.states.gameplay->area_active_cells.size();
        cell_x++
    ) {
        for(
            size_t cell_y = 0;
            cell_y < game.states.gameplay->area_active_cells[cell_x].size();
            cell_y++
        ) {
            float start_x =
                game.curAreaData->bmap.top_left_corner.x +
                cell_x * GEOMETRY::AREA_CELL_SIZE;
            float start_y =
                game.curAreaData->bmap.top_left_corner.y +
                cell_y * GEOMETRY::AREA_CELL_SIZE;
            al_draw_rectangle(
                start_x + (1.0f / radar_cam.zoom),
                start_y + (1.0f / radar_cam.zoom),
                start_x + GEOMETRY::AREA_CELL_SIZE - (1.0f / radar_cam.zoom),
                start_y + GEOMETRY::AREA_CELL_SIZE - (1.0f / radar_cam.zoom),
                game.states.gameplay->area_active_cells[cell_x][cell_y] ?
                al_map_rgb(32, 192, 32) :
                al_map_rgb(192, 32, 32),
                1.0f / radar_cam.zoom
            );
        }
    }
    */
    
    //Return to normal drawing.
    al_use_transform(&old_transform);
    al_set_clipping_rectangle(old_cr_x, old_cr_y, old_cr_w, old_cr_h);
    
    //North indicator.
    Point north_ind_center(
        center.x - size.x / 2.0f + 20.0f,
        center.y - size.y / 2.0f + 20.0f
    );
    al_draw_filled_circle(
        north_ind_center.x, north_ind_center.y,
        12.0f, game.config.aestheticRadar.backgroundColor
    );
    drawText(
        "N", game.sysContent.fntSlim,
        Point(
            north_ind_center.x,
            north_ind_center.y + 1.0f
        ),
        Point(12.0f),
        game.config.aestheticRadar.highestColor
    );
    al_draw_filled_triangle(
        north_ind_center.x,
        north_ind_center.y - 12.0f,
        north_ind_center.x - 6.0f,
        north_ind_center.y - 6.0f,
        north_ind_center.x + 6.0f,
        north_ind_center.y - 6.0f,
        game.config.aestheticRadar.highestColor
    );
    
    //Area name.
    Point area_name_size(
        size.x * 0.40f,
        20.0f
    );
    Point area_name_center(
        center.x + size.x / 2.0f - area_name_size.x / 2.0f - 8.0f,
        center.y - size.y / 2.0f + area_name_size.y / 2.0f + 8.0f
    );
    drawFilledRoundedRectangle(
        area_name_center, area_name_size,
        12.0f, game.config.aestheticRadar.backgroundColor
    );
    drawText(
        game.curAreaData->name, game.sysContent.fntStandard,
        area_name_center, area_name_size, game.config.aestheticRadar.highestColor
    );
    
    //Draw some scan lines.
    float scan_line_y = center.y - size.y / 2.0f;
    while(scan_line_y < center.y + size.y / 2.0f) {
        al_draw_line(
            center.x - size.x / 2.0f,
            scan_line_y,
            center.x + size.x / 2.0f,
            scan_line_y,
            al_map_rgba(255, 255, 255, 8),
            2.0f
        );
        scan_line_y += 16.0f;
    }
    float scan_line_x = center.x - size.x / 2.0f;
    while(scan_line_x < center.x + size.x / 2.0f) {
        al_draw_line(
            scan_line_x,
            center.y - size.y / 2.0f,
            scan_line_x,
            center.y + size.y / 2.0f,
            al_map_rgba(255, 255, 255, 8),
            2.0f
        );
        scan_line_x += 16.0f;
    }
    
    //Draw a rectangle all around.
    drawTexturedBox(
        center, size, game.sysContent.bmpFrameBox,
        COLOR_TRANSPARENT_WHITE
    );
}


/**
 * @brief Fills the list of mission fail conditions.
 *
 * @param list List item to fill.
 */
void PauseMenu::fillMissionFailList(ListGuiItem* list) {
    for(size_t f = 0; f < game.missionFailConds.size(); f++) {
        if(
            hasFlag(
                game.curAreaData->mission.failConditions,
                getIdxBitmask(f)
            )
        ) {
            MissionFail* cond = game.missionFailConds[f];
            
            string description =
                cond->getPlayerDescription(&game.curAreaData->mission);
            addBullet(list, description, al_map_rgb(255, 200, 200));
            
            float percentage = 0.0f;
            int cur =
                cond->getCurAmount(game.states.gameplay);
            int req =
                cond->getReqAmount(game.states.gameplay);
            if(req != 0.0f) {
                percentage = cur / (float) req;
            }
            percentage *= 100;
            string status = cond->getStatus(cur, req, percentage);
            
            if(status.empty()) continue;
            addBullet(list, "    " + status);
        }
    }
    
    if(game.curAreaData->mission.failConditions == 0) {
        addBullet(list, "(None)");
    }
}


/**
 * @brief Fills the list of mission grading information.
 *
 * @param list List item to fill.
 */
void PauseMenu::fillMissionGradingList(ListGuiItem* list) {
    switch(game.curAreaData->mission.gradingMode) {
    case MISSION_GRADING_MODE_POINTS: {
        addBullet(
            list,
            "Your medal depends on your score:"
        );
        addBullet(
            list,
            "    Platinum: " +
            i2s(game.curAreaData->mission.platinumReq) + "+ points.",
            al_map_rgb(255, 255, 200)
        );
        addBullet(
            list,
            "    Gold: " +
            i2s(game.curAreaData->mission.goldReq) + "+ points.",
            al_map_rgb(255, 255, 200)
        );
        addBullet(
            list,
            "    Silver: " +
            i2s(game.curAreaData->mission.silverReq) + "+ points.",
            al_map_rgb(255, 255, 200)
        );
        addBullet(
            list,
            "    Bronze: " +
            i2s(game.curAreaData->mission.bronzeReq) + "+ points.",
            al_map_rgb(255, 255, 200)
        );
        
        vector<string> score_notes;
        for(size_t c = 0; c < game.missionScoreCriteria.size(); c++) {
            MissionScoreCriterion* c_ptr =
                game.missionScoreCriteria[c];
            int mult = c_ptr->getMultiplier(&game.curAreaData->mission);
            if(mult != 0) {
                score_notes.push_back(
                    "    " + c_ptr->getName() + " x " + i2s(mult) + "."
                );
            }
        }
        if(!score_notes.empty()) {
            addBullet(
                list,
                "Your score is calculated like so:"
            );
            for(size_t s = 0; s < score_notes.size(); s++) {
                addBullet(list, score_notes[s]);
            }
        } else {
            addBullet(
                list,
                "In this mission, your score will always be 0."
            );
        }
        
        vector<string> loss_notes;
        for(size_t c = 0; c < game.missionScoreCriteria.size(); c++) {
            MissionScoreCriterion* c_ptr =
                game.missionScoreCriteria[c];
            if(
                hasFlag(
                    game.curAreaData->mission.pointLossData,
                    getIdxBitmask(c)
                )
            ) {
                loss_notes.push_back("    " + c_ptr->getName());
            }
        }
        if(!loss_notes.empty()) {
            addBullet(
                list,
                "If you fail, you'll lose your score for:"
            );
            for(size_t l = 0; l < loss_notes.size(); l++) {
                addBullet(list, loss_notes[l]);
            }
        }
        break;
    } case MISSION_GRADING_MODE_GOAL: {
        addBullet(
            list,
            "You get a platinum medal if you clear the goal."
        );
        addBullet(
            list,
            "You get no medal if you fail."
        );
        break;
    } case MISSION_GRADING_MODE_PARTICIPATION: {
        addBullet(
            list,
            "You get a platinum medal just by playing the mission."
        );
        break;
    }
    }
}


/**
 * @brief Returns a string representing the player's status towards the
 * mission goal.
 *
 * @return The status.
 */
string PauseMenu::getMissionGoalStatus() {
    float percentage = 0.0f;
    int cur =
        game.missionGoals[game.curAreaData->mission.goal]->
        getCurAmount(game.states.gameplay);
    int req =
        game.missionGoals[game.curAreaData->mission.goal]->
        getReqAmount(game.states.gameplay);
    if(req != 0.0f) {
        percentage = cur / (float) req;
    }
    percentage *= 100;
    return
        game.missionGoals[game.curAreaData->mission.goal]->
        getStatus(cur, req, percentage);
}


/**
 * @brief Handles an Allegro event.
 *
 * @param ev Event to handle.
 */
void PauseMenu::handleAllegroEvent(const ALLEGRO_EVENT &ev) {
    gui.handleAllegroEvent(ev);
    radarGui.handleAllegroEvent(ev);
    statusGui.handleAllegroEvent(ev);
    missionGui.handleAllegroEvent(ev);
    confirmationGui.handleAllegroEvent(ev);
    if(secondaryMenu) secondaryMenu->handleAllegroEvent(ev);
    
    //Handle some radar logic.
    DrawInfo radar_draw;
    radarGui.getItemDrawInfo(radarItem, &radar_draw);
    bool mouse_in_radar =
        radarGui.responsive &&
        isPointInRectangle(
            game.mouseCursor.sPos,
            radar_draw.center, radar_draw.size
        );
        
    if(ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
        if(mouse_in_radar) {
            radarMouseDown = true;
            radarMouseDownPoint = game.mouseCursor.sPos;
        }
        
    } else if(ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) {
        if(mouse_in_radar && !radarMouseDragging) {
            //Clicked somewhere.
            radarConfirm();
        }
        
        radarMouseDown = false;
        radarMouseDragging = false;
        
    } else if(ev.type == ALLEGRO_EVENT_MOUSE_AXES) {
        if(
            radarMouseDown &&
            (
                fabs(game.mouseCursor.sPos.x - radarMouseDownPoint.x) >
                4.0f ||
                fabs(game.mouseCursor.sPos.y - radarMouseDownPoint.y) >
                4.0f
            )
        ) {
            //Consider the mouse down as part of a mouse drag, not a click.
            radarMouseDragging = true;
        }
        
        if(
            radarMouseDragging &&
            (ev.mouse.dx != 0.0f || ev.mouse.dy != 0.0f)
        ) {
            //Pan the radar around.
            panRadar(Point(-ev.mouse.dx, -ev.mouse.dy));
            
        } else if(
            mouse_in_radar && ev.mouse.dz != 0.0f
        ) {
            //Zoom in or out, using the radar/mouse cursor as the anchor.
            zoomRadarWithMouse(ev.mouse.dz * 0.1f, radar_draw.center, radar_draw.size);
            
        }
    }
}


/**
 * @brief Handles a player action.
 *
 * @param action Data about the player action.
 */
void PauseMenu::handlePlayerAction(const PlayerAction &action) {
    if(openingLockoutTimer > 0.0f) {
        //Don't accept inputs shortly after the menu opens.
        //This helps errant inputs from before the menu bleeding into the menu
        //immediately after it opens, like the "radar toggle" action.
        return;
    }
    if(closing) return;
    
    bool handled_by_radar = false;
    
    if(radarGui.responsive) {
        switch(action.actionTypeId) {
        case PLAYER_ACTION_TYPE_RADAR: {
            if(action.value >= 0.5f) {
                startClosing(&radarGui);
                handled_by_radar = true;
            }
            break;
        } case PLAYER_ACTION_TYPE_RADAR_RIGHT: {
            radarPan.right = action.value;
            handled_by_radar = true;
            break;
        } case PLAYER_ACTION_TYPE_RADAR_UP: {
            radarPan.up = action.value;
            handled_by_radar = true;
            break;
        } case PLAYER_ACTION_TYPE_RADAR_LEFT: {
            radarPan.left = action.value;
            handled_by_radar = true;
            break;
        } case PLAYER_ACTION_TYPE_RADAR_DOWN: {
            radarPan.down = action.value;
            handled_by_radar = true;
            break;
        } case PLAYER_ACTION_TYPE_RADAR_ZOOM_IN: {
            radarZoomIn = action.value;
            handled_by_radar = true;
            break;
        } case PLAYER_ACTION_TYPE_RADAR_ZOOM_OUT: {
            radarZoomOut = action.value;
            handled_by_radar = true;
            break;
        } case PLAYER_ACTION_TYPE_MENU_OK: {
            radarConfirm();
            handled_by_radar = true;
            break;
        }
        }
    }
    
    if(!handled_by_radar) {
        //Only let the GUIs handle it if the radar didn't need it, otherwise
        //we could see GUI item selections move around or such because
        //radar and menus actions share binds.
        gui.handlePlayerAction(action);
        radarGui.handlePlayerAction(action);
        statusGui.handlePlayerAction(action);
        missionGui.handlePlayerAction(action);
        confirmationGui.handlePlayerAction(action);
        if(secondaryMenu) secondaryMenu->handlePlayerAction(action);
        
        switch(action.actionTypeId) {
        case PLAYER_ACTION_TYPE_MENU_PAGE_LEFT:
        case PLAYER_ACTION_TYPE_MENU_PAGE_RIGHT: {
            if(action.value >= 0.5f) {
                GuiManager* cur_gui = &gui;
                PAUSE_MENU_PAGE cur_page = PAUSE_MENU_PAGE_SYSTEM;
                if(radarGui.responsive) {
                    cur_gui = &radarGui;
                    cur_page = PAUSE_MENU_PAGE_RADAR;
                } else if(statusGui.responsive) {
                    cur_gui = &statusGui;
                    cur_page = PAUSE_MENU_PAGE_STATUS;
                } else if(missionGui.responsive) {
                    cur_gui = &missionGui;
                    cur_page = PAUSE_MENU_PAGE_MISSION;
                }
                size_t cur_page_idx =
                    std::distance(
                        pages.begin(),
                        std::find(pages.begin(), pages.end(), cur_page)
                    );
                size_t new_page_idx =
                    sumAndWrap(
                        (int) cur_page_idx,
                        action.actionTypeId == PLAYER_ACTION_TYPE_MENU_PAGE_LEFT ?
                        -1 :
                        1,
                        (int) pages.size()
                    );
                switchPage(
                    cur_gui,
                    pages[new_page_idx],
                    action.actionTypeId == PLAYER_ACTION_TYPE_MENU_PAGE_LEFT
                );
            }
            
            break;
        }
        }
    }
}


/**
 * @brief Initializes the leaving confirmation page.
 */
void PauseMenu::initConfirmationPage() {
    DataNode* gui_file = &game.content.guiDefs.list[PAUSE_MENU::CONFIRMATION_GUI_FILE_NAME];
    
    //Menu items.
    confirmationGui.registerCoords("cancel",           19, 83, 30, 10);
    confirmationGui.registerCoords("cancel_input",      5, 87,  4,  4);
    confirmationGui.registerCoords("confirm",          81, 83, 30, 10);
    confirmationGui.registerCoords("header",           50,  8, 92,  8);
    confirmationGui.registerCoords("explanation",      50, 40, 84, 20);
    confirmationGui.registerCoords("options_reminder", 50, 69, 92, 10);
    confirmationGui.registerCoords("tooltip",          50, 96, 96,  4);
    confirmationGui.readCoords(gui_file->getChildByName("positions"));
    
    //Cancel button.
    confirmationGui.backItem =
        new ButtonGuiItem(
        "Cancel", game.sysContent.fntStandard
    );
    confirmationGui.backItem->onActivate =
    [this] (const Point &) {
        confirmationGui.responsive = false;
        confirmationGui.startAnimation(
            GUI_MANAGER_ANIM_CENTER_TO_UP,
            GAMEPLAY::MENU_EXIT_HUD_MOVE_TIME
        );
        gui.responsive = true;
        gui.startAnimation(
            GUI_MANAGER_ANIM_UP_TO_CENTER,
            GAMEPLAY::MENU_EXIT_HUD_MOVE_TIME
        );
    };
    confirmationGui.backItem->onGetTooltip =
    [] () { return "Return to the pause menu."; };
    confirmationGui.addItem(confirmationGui.backItem, "cancel");
    
    //Cancel input icon.
    guiAddBackInputIcon(&confirmationGui, "cancel_input");
    
    //Confirm button.
    ButtonGuiItem* confirm_button =
        new ButtonGuiItem("Confirm", game.sysContent.fntStandard);
    confirm_button->onActivate =
    [this] (const Point &) {
        startLeavingGameplay();
    };
    confirm_button->onGetTooltip =
    [] () {
        return "Yes, I'm sure.";
    };
    confirmationGui.addItem(confirm_button, "confirm");
    
    //Header text.
    TextGuiItem* header_text =
        new TextGuiItem("Are you sure?", game.sysContent.fntAreaName);
    confirmationGui.addItem(header_text, "header");
    
    //Explanation text.
    confirmationExplanationText =
        new TextGuiItem("", game.sysContent.fntStandard);
    confirmationExplanationText->lineWrap = true;
    confirmationGui.addItem(confirmationExplanationText, "explanation");
    
    //Options reminder text.
    TextGuiItem* options_reminder_text =
        new TextGuiItem(
        "You can disable this confirmation question in the options menu.",
        game.sysContent.fntStandard
    );
    confirmationGui.addItem(options_reminder_text, "options_reminder");
    
    //Tooltip text.
    TooltipGuiItem* tooltip_text =
        new TooltipGuiItem(&gui);
    confirmationGui.addItem(tooltip_text, "tooltip");
    
    //Finishing touches.
    confirmationGui.setSelectedItem(confirmationGui.backItem, true);
    confirmationGui.responsive = false;
    confirmationGui.hideItems();
}


/**
 * @brief Initializes the pause menu's main menu.
 */
void PauseMenu::initMainPauseMenu() {
    //Menu items.
    gui.registerCoords("header",           50,    5, 52,  6);
    gui.registerCoords("left_page",        12,    5, 20,  6);
    gui.registerCoords("left_page_input",   3,    7,  4,  4);
    gui.registerCoords("right_page",       88,    5, 20,  6);
    gui.registerCoords("right_page_input", 97,    7,  4,  4);
    gui.registerCoords("line",             50,   11, 96,  2);
    gui.registerCoords("area_name",        50,   20, 96,  8);
    gui.registerCoords("area_subtitle",    50, 28.5, 88,  9);
    gui.registerCoords("continue",         13,   88, 22,  8);
    gui.registerCoords("continue_input",    3,   91,  4,  4);
    gui.registerCoords("retry",            28,   44, 38, 12);
    gui.registerCoords("end",              72,   44, 38, 12);
    gui.registerCoords("help",             19,   65, 26, 10);
    gui.registerCoords("options",          50,   65, 26, 10);
    gui.registerCoords("stats",            81,   65, 26, 10);
    gui.registerCoords("quit",             87,   88, 22,  8);
    gui.registerCoords("tooltip",          50,   96, 96,  4);
    gui.readCoords(
        game.content.guiDefs.list[PAUSE_MENU::GUI_FILE_NAME].getChildByName("positions")
    );
    
    //Header.
    TextGuiItem* header_text =
        new TextGuiItem(
        "PAUSED", game.sysContent.fntAreaName,
        COLOR_TRANSPARENT_WHITE
    );
    gui.addItem(header_text, "header");
    
    //Page buttons and inputs.
    createPageButtons(PAUSE_MENU_PAGE_SYSTEM, &gui);
    
    //Line.
    GuiItem* line = new GuiItem();
    line->onDraw =
    [] (const DrawInfo & draw) {
        drawFilledRoundedRectangle(
            draw.center,
            Point(draw.size.x, 3.0f),
            2.0f,
            COLOR_TRANSPARENT_WHITE
        );
    };
    gui.addItem(line, "line");
    
    //Area name.
    TextGuiItem* area_name_text =
        new TextGuiItem(
        game.curAreaData->name, game.sysContent.fntAreaName,
        changeAlpha(COLOR_GOLD, 192)
    );
    gui.addItem(area_name_text, "area_name");
    
    //Area subtitle.
    TextGuiItem* area_subtitle_text =
        new TextGuiItem(
        getSubtitleOrMissionGoal(
            game.curAreaData->subtitle, game.curAreaData->type,
            game.curAreaData->mission.goal
        ),
        game.sysContent.fntAreaName,
        changeAlpha(COLOR_WHITE, 192)
    );
    gui.addItem(area_subtitle_text, "area_subtitle");
    
    //Continue button.
    gui.backItem =
        new ButtonGuiItem("Continue", game.sysContent.fntStandard);
    gui.backItem->onActivate =
    [this] (const Point &) {
        startClosing(&gui);
    };
    gui.backItem->onGetTooltip =
    [] () { return "Unpause and continue playing."; };
    gui.addItem(gui.backItem, "continue");
    
    //Continue input icon.
    guiAddBackInputIcon(&gui, "continue_input");
    
    //Retry button.
    ButtonGuiItem* retry_button =
        new ButtonGuiItem(
        game.curAreaData->type == AREA_TYPE_SIMPLE ?
        "Restart exploration" :
        "Retry mission",
        game.sysContent.fntStandard
    );
    retry_button->onActivate =
    [this] (const Point &) {
        leaveTarget = GAMEPLAY_LEAVE_TARGET_RETRY;
        confirmOrLeave();
    };
    retry_button->onGetTooltip =
    [] () {
        return
            game.curAreaData->type == AREA_TYPE_SIMPLE ?
            "Restart this area's exploration." :
            "Retry the mission from the start.";
    };
    gui.addItem(retry_button, "retry");
    
    //End button.
    ButtonGuiItem* end_button =
        new ButtonGuiItem(
        game.curAreaData->type == AREA_TYPE_SIMPLE ?
        "End exploration" :
        "End mission",
        game.sysContent.fntStandard
    );
    end_button->onActivate =
    [this] (const Point &) {
        leaveTarget = GAMEPLAY_LEAVE_TARGET_END;
        confirmOrLeave();
    };
    end_button->onGetTooltip =
    [] () {
        bool as_fail =
            hasFlag(
                game.curAreaData->mission.failConditions,
                getIdxBitmask(MISSION_FAIL_COND_PAUSE_MENU)
            );
        return
            game.curAreaData->type == AREA_TYPE_SIMPLE ?
            "End this area's exploration." :
            as_fail ?
            "End this mission as a fail." :
            "End this mission successfully.";
    };
    gui.addItem(end_button, "end");
    
    //Help button.
    ButtonGuiItem* help_button =
        new ButtonGuiItem("Help", game.sysContent.fntStandard);
    help_button->onActivate =
    [this] (const Point &) {
        gui.responsive = false;
        gui.startAnimation(
            GUI_MANAGER_ANIM_CENTER_TO_UP,
            GAMEPLAY::MENU_EXIT_HUD_MOVE_TIME
        );
        HelpMenu* help_menu = new HelpMenu();
        help_menu->gui.responsive = true;
        help_menu->gui.startAnimation(
            GUI_MANAGER_ANIM_DOWN_TO_CENTER,
            GAMEPLAY::MENU_EXIT_HUD_MOVE_TIME
        );
        help_menu->leaveCallback = [this, help_menu] () {
            help_menu->unloadTimer = GAMEPLAY::MENU_EXIT_HUD_MOVE_TIME;
            help_menu->gui.responsive = false;
            help_menu->gui.startAnimation(
                GUI_MANAGER_ANIM_CENTER_TO_DOWN,
                GAMEPLAY::MENU_EXIT_HUD_MOVE_TIME
            );
            gui.responsive = true;
            gui.startAnimation(
                GUI_MANAGER_ANIM_UP_TO_CENTER,
                GAMEPLAY::MENU_EXIT_HUD_MOVE_TIME
            );
        };
        help_menu->load();
        help_menu->enter();
        secondaryMenu = help_menu;
    };
    help_button->onGetTooltip =
    [] () {
        return
            "Quick help and tips about how to play. "
            "You can also find this in the title screen.";
    };
    gui.addItem(help_button, "help");
    
    //Options button.
    ButtonGuiItem* options_button =
        new ButtonGuiItem("Options", game.sysContent.fntStandard);
    options_button->onActivate =
    [this] (const Point &) {
        gui.responsive = false;
        gui.startAnimation(
            GUI_MANAGER_ANIM_CENTER_TO_UP,
            GAMEPLAY::MENU_EXIT_HUD_MOVE_TIME
        );
        OptionsMenu* options_menu = new OptionsMenu();
        options_menu->topGui.responsive = true;
        options_menu->topGui.startAnimation(
            GUI_MANAGER_ANIM_DOWN_TO_CENTER,
            GAMEPLAY::MENU_EXIT_HUD_MOVE_TIME
        );
        options_menu->leaveCallback = [this, options_menu] () {
            options_menu->unloadTimer = GAMEPLAY::MENU_EXIT_HUD_MOVE_TIME;
            options_menu->topGui.responsive = false;
            options_menu->topGui.startAnimation(
                GUI_MANAGER_ANIM_CENTER_TO_DOWN,
                GAMEPLAY::MENU_EXIT_HUD_MOVE_TIME
            );
            gui.responsive = true;
            gui.startAnimation(
                GUI_MANAGER_ANIM_UP_TO_CENTER,
                GAMEPLAY::MENU_EXIT_HUD_MOVE_TIME
            );
        };
        options_menu->load();
        options_menu->enter();
        secondaryMenu = options_menu;
    };
    options_button->onGetTooltip =
    [] () {
        return
            "Customize your playing experience. "
            "You can also find this in the title screen.";
    };
    gui.addItem(options_button, "options");
    
    //Statistics button.
    ButtonGuiItem* stats_button =
        new ButtonGuiItem("Statistics", game.sysContent.fntStandard);
    stats_button->onActivate =
    [this] (const Point &) {
        gui.responsive = false;
        gui.startAnimation(
            GUI_MANAGER_ANIM_CENTER_TO_UP,
            GAMEPLAY::MENU_EXIT_HUD_MOVE_TIME
        );
        StatsMenu* stats_menu = new StatsMenu();
        stats_menu->gui.responsive = true;
        stats_menu->gui.startAnimation(
            GUI_MANAGER_ANIM_DOWN_TO_CENTER,
            GAMEPLAY::MENU_EXIT_HUD_MOVE_TIME
        );
        stats_menu->leaveCallback = [this, stats_menu] () {
            stats_menu->unloadTimer = GAMEPLAY::MENU_EXIT_HUD_MOVE_TIME;
            stats_menu->gui.responsive = false;
            stats_menu->gui.startAnimation(
                GUI_MANAGER_ANIM_CENTER_TO_DOWN,
                GAMEPLAY::MENU_EXIT_HUD_MOVE_TIME
            );
            gui.responsive = true;
            gui.startAnimation(
                GUI_MANAGER_ANIM_UP_TO_CENTER,
                GAMEPLAY::MENU_EXIT_HUD_MOVE_TIME
            );
        };
        stats_menu->load();
        stats_menu->enter();
        secondaryMenu = stats_menu;
    };
    stats_button->onGetTooltip =
    [] () {
        return
            "Check out some fun lifetime statistics. "
            "You can also find this in the title screen.";
    };
    gui.addItem(stats_button, "stats");
    
    //Quit button.
    ButtonGuiItem* quit_button =
        new ButtonGuiItem(
        game.states.areaEd->quickPlayAreaPath.empty() ?
        "Quit" :
        "Back to editor",
        game.sysContent.fntStandard
    );
    quit_button->onActivate =
    [this] (const Point &) {
        leaveTarget = GAMEPLAY_LEAVE_TARGET_AREA_SELECT;
        confirmOrLeave();
    };
    quit_button->onGetTooltip =
    [] () {
        return
            "Lose your progress and return to the " +
            string(
                game.states.areaEd->quickPlayAreaPath.empty() ?
                "area selection menu" :
                "area editor"
            ) + ".";
    };
    gui.addItem(quit_button, "quit");
    
    //Tooltip text.
    TooltipGuiItem* tooltip_text =
        new TooltipGuiItem(&gui);
    gui.addItem(tooltip_text, "tooltip");
    
    //Finishing touches.
    gui.setSelectedItem(gui.backItem, true);
    gui.responsive = false;
    gui.hideItems();
}


/**
 * @brief Initializes the mission page.
 */
void PauseMenu::initMissionPage() {
    DataNode* gui_file = &game.content.guiDefs.list[PAUSE_MENU::MISSION_GUI_FILE_NAME];
    
    //Menu items.
    missionGui.registerCoords("header",           50,  5, 52,  6);
    missionGui.registerCoords("left_page",        12,  5, 20,  6);
    missionGui.registerCoords("left_page_input",   3,  7,  4,  4);
    missionGui.registerCoords("right_page",       88,  5, 20,  6);
    missionGui.registerCoords("right_page_input", 97,  7,  4,  4);
    missionGui.registerCoords("line",             50, 11, 96,  2);
    missionGui.registerCoords("continue",         10, 16, 16,  4);
    missionGui.registerCoords("continue_input",    3, 17,  4,  4);
    missionGui.registerCoords("goal_header",      50, 16, 60,  4);
    missionGui.registerCoords("goal",             50, 22, 96,  4);
    missionGui.registerCoords("goal_status",      50, 26, 96,  4);
    missionGui.registerCoords("fail_header",      50, 32, 96,  4);
    missionGui.registerCoords("fail_list",        48, 48, 92, 24);
    missionGui.registerCoords("fail_scroll",      97, 48,  2, 24);
    missionGui.registerCoords("grading_header",   50, 64, 96,  4);
    missionGui.registerCoords("grading_list",     48, 80, 92, 24);
    missionGui.registerCoords("grading_scroll",   97, 80,  2, 24);
    missionGui.registerCoords("tooltip",          50, 96, 96,  4);
    missionGui.readCoords(gui_file->getChildByName("positions"));
    
    //Header.
    TextGuiItem* header_text =
        new TextGuiItem(
        "MISSION", game.sysContent.fntAreaName,
        COLOR_TRANSPARENT_WHITE
    );
    missionGui.addItem(header_text, "header");
    
    //Page buttons and inputs.
    createPageButtons(PAUSE_MENU_PAGE_MISSION, &missionGui);
    
    //Line.
    GuiItem* line = new GuiItem();
    line->onDraw =
    [] (const DrawInfo & draw) {
        drawFilledRoundedRectangle(
            draw.center,
            Point(draw.size.x, 3.0f),
            2.0f,
            COLOR_TRANSPARENT_WHITE
        );
    };
    missionGui.addItem(line, "line");
    
    //Continue button.
    missionGui.backItem =
        new ButtonGuiItem("Continue", game.sysContent.fntStandard);
    missionGui.backItem->onActivate =
    [this] (const Point &) {
        startClosing(&missionGui);
    };
    missionGui.backItem->onGetTooltip =
    [] () { return "Unpause and continue playing."; };
    missionGui.addItem(missionGui.backItem, "continue");
    
    //Continue input icon.
    guiAddBackInputIcon(&missionGui, "continue_input");
    
    //Goal header text.
    TextGuiItem* goal_header_text =
        new TextGuiItem("Goal", game.sysContent.fntAreaName);
    missionGui.addItem(goal_header_text, "goal_header");
    
    //Goal explanation text.
    TextGuiItem* goal_text =
        new TextGuiItem(
        game.missionGoals[game.curAreaData->mission.goal]->
        getPlayerDescription(&game.curAreaData->mission),
        game.sysContent.fntStandard,
        al_map_rgb(255, 255, 200)
    );
    missionGui.addItem(goal_text, "goal");
    
    //Goal status text.
    TextGuiItem* goal_status_text =
        new TextGuiItem(
        getMissionGoalStatus(),
        game.sysContent.fntStandard
    );
    missionGui.addItem(goal_status_text, "goal_status");
    
    //Fail conditions header text.
    TextGuiItem* fail_header_text =
        new TextGuiItem("Fail conditions", game.sysContent.fntAreaName);
    missionGui.addItem(fail_header_text, "fail_header");
    
    //Fail condition explanation list.
    ListGuiItem* mission_fail_list = new ListGuiItem();
    missionGui.addItem(mission_fail_list, "fail_list");
    fillMissionFailList(mission_fail_list);
    
    //Fail condition explanation scrollbar.
    ScrollGuiItem* fail_scroll = new ScrollGuiItem();
    fail_scroll->listItem = mission_fail_list;
    missionGui.addItem(fail_scroll, "fail_scroll");
    
    //Grading header text.
    TextGuiItem* grading_header_text =
        new TextGuiItem("Grading", game.sysContent.fntAreaName);
    missionGui.addItem(grading_header_text, "grading_header");
    
    //Grading explanation list.
    ListGuiItem* mission_grading_list = new ListGuiItem();
    missionGui.addItem(mission_grading_list, "grading_list");
    fillMissionGradingList(mission_grading_list);
    
    //Grading explanation scrollbar.
    ScrollGuiItem* grading_scroll = new ScrollGuiItem();
    grading_scroll->listItem = mission_grading_list;
    missionGui.addItem(grading_scroll, "grading_scroll");
    
    //Tooltip text.
    TooltipGuiItem* tooltip_text =
        new TooltipGuiItem(&missionGui);
    missionGui.addItem(tooltip_text, "tooltip");
    
    //Finishing touches.
    missionGui.setSelectedItem(missionGui.backItem, true);
    missionGui.responsive = false;
    missionGui.hideItems();
}


/**
 * @brief Initializes the radar page.
 */
void PauseMenu::initRadarPage() {
    DataNode* gui_file = &game.content.guiDefs.list[PAUSE_MENU::RADAR_GUI_FILE_NAME];
    
    //Assets.
    DataNode* bitmaps_node = gui_file->getChildByName("files");
    
#define loader(var, name) \
    var = \
          game.content.bitmaps.list.get( \
                                         bitmaps_node->getChildByName(name)->value, \
                                         bitmaps_node->getChildByName(name) \
                                       );
    
    loader(bmpRadarCursor,         "cursor");
    loader(bmpRadarPikmin,         "pikmin");
    loader(bmpRadarTreasure,       "treasure");
    loader(bmpRadarEnemyAlive,    "enemy_alive");
    loader(bmpRadarEnemyDead,     "enemy_dead");
    loader(bmpRadarLeaderBubble,  "leader_bubble");
    loader(bmpRadarLeaderX,       "leader_x");
    loader(bmpRadarObstacle,       "obstacle");
    loader(bmpRadarOnionSkeleton, "onion_skeleton");
    loader(bmpRadarOnionBulb,     "onion_bulb");
    loader(bmpRadarShip,           "ship");
    loader(bmpRadarPath,           "path");
    
#undef loader
    
    //Menu items.
    radarGui.registerCoords("header",              50,     5,    52,    6);
    radarGui.registerCoords("left_page",           12,     5,    20,    6);
    radarGui.registerCoords("left_page_input",      3,     7,     4,    4);
    radarGui.registerCoords("right_page",          88,     5,    20,    6);
    radarGui.registerCoords("right_page_input",    97,     7,     4,    4);
    radarGui.registerCoords("line",                50,    11,    96,    2);
    radarGui.registerCoords("continue",            10,    16,    16,    4);
    radarGui.registerCoords("continue_input",       3,    17,     4,    4);
    radarGui.registerCoords("radar",               37.5,  56.25, 70,   72.5);
    radarGui.registerCoords("group_pikmin_label",  86.25, 77.5,  22.5,  5);
    radarGui.registerCoords("group_pikmin_number", 86.25, 85,    22.5,  5);
    radarGui.registerCoords("idle_pikmin_label",   86.25, 62.5,  22.5,  5);
    radarGui.registerCoords("idle_pikmin_number",  86.25, 70,    22.5,  5);
    radarGui.registerCoords("field_pikmin_label",  86.25, 47.5,  22.5,  5);
    radarGui.registerCoords("field_pikmin_number", 86.25, 55,    22.5,  5);
    radarGui.registerCoords("cursor_info",         86.25, 33.75, 22.5, 17.5);
    radarGui.registerCoords("instructions",        58.75, 16,    77.5,  4);
    radarGui.registerCoords("tooltip",             50,    96,    96,    4);
    radarGui.readCoords(gui_file->getChildByName("positions"));
    
    //Header.
    TextGuiItem* header_text =
        new TextGuiItem(
        "RADAR", game.sysContent.fntAreaName,
        COLOR_TRANSPARENT_WHITE
    );
    radarGui.addItem(header_text, "header");
    
    //Page buttons and inputs.
    createPageButtons(PAUSE_MENU_PAGE_RADAR, &radarGui);
    
    //Line.
    GuiItem* line = new GuiItem();
    line->onDraw =
    [] (const DrawInfo & draw) {
        drawFilledRoundedRectangle(
            draw.center,
            Point(draw.size.x, 3.0f),
            2.0f,
            COLOR_TRANSPARENT_WHITE
        );
    };
    radarGui.addItem(line, "line");
    
    //Continue button.
    radarGui.backItem =
        new ButtonGuiItem("Continue", game.sysContent.fntStandard);
    radarGui.backItem->onActivate =
    [this] (const Point &) {
        startClosing(&radarGui);
    };
    radarGui.backItem->onGetTooltip =
    [] () { return "Unpause and continue playing."; };
    radarGui.addItem(radarGui.backItem, "continue");
    
    //Continue input icon.
    guiAddBackInputIcon(&radarGui, "continue_input");
    
    //Radar item.
    radarItem = new GuiItem();
    radarItem->onDraw =
    [this] (const DrawInfo & draw) {
        drawRadar(draw.center, draw.size);
    };
    radarGui.addItem(radarItem, "radar");
    
    //Group Pikmin label text.
    TextGuiItem* group_pik_label_text =
        new TextGuiItem(
        "Group Pikmin:", game.sysContent.fntStandard,
        COLOR_WHITE, ALLEGRO_ALIGN_LEFT
    );
    radarGui.addItem(group_pik_label_text, "group_pikmin_label");
    
    //Group Pikmin number text.
    TextGuiItem* group_pik_nr_text =
        new TextGuiItem(
        i2s(game.states.gameplay->getAmountOfGroupPikmin()),
        game.sysContent.fntCounter,
        COLOR_WHITE, ALLEGRO_ALIGN_RIGHT
    );
    radarGui.addItem(group_pik_nr_text, "group_pikmin_number");
    
    //Idle Pikmin label text.
    TextGuiItem* idle_pik_label_text =
        new TextGuiItem(
        "Idle Pikmin:", game.sysContent.fntStandard,
        COLOR_WHITE, ALLEGRO_ALIGN_LEFT
    );
    radarGui.addItem(idle_pik_label_text, "idle_pikmin_label");
    
    //Idle Pikmin number text.
    TextGuiItem* idle_pik_nr_text =
        new TextGuiItem(
        i2s(game.states.gameplay->getAmountOfIdlePikmin()),
        game.sysContent.fntCounter,
        COLOR_WHITE, ALLEGRO_ALIGN_RIGHT
    );
    radarGui.addItem(idle_pik_nr_text, "idle_pikmin_number");
    
    //Field Pikmin label text.
    TextGuiItem* field_pik_label_text =
        new TextGuiItem(
        "Field Pikmin:", game.sysContent.fntStandard,
        COLOR_WHITE, ALLEGRO_ALIGN_LEFT
    );
    radarGui.addItem(field_pik_label_text, "field_pikmin_label");
    
    //Field Pikmin number text.
    TextGuiItem* field_pik_nr_text =
        new TextGuiItem(
        i2s(game.states.gameplay->getAmountOfFieldPikmin()),
        game.sysContent.fntCounter, COLOR_WHITE, ALLEGRO_ALIGN_RIGHT
    );
    radarGui.addItem(field_pik_nr_text, "field_pikmin_number");
    
    //Cursor info text.
    TextGuiItem* cursor_info_text =
        new TextGuiItem("", game.sysContent.fntStandard);
    cursor_info_text->lineWrap = true;
    cursor_info_text->onDraw =
    [this, cursor_info_text] (const DrawInfo & draw) {
        if(cursor_info_text->text.empty()) return;
        
        //Draw the text.
        int line_height = al_get_font_line_height(cursor_info_text->font);
        vector<StringToken> tokens = tokenizeString(cursor_info_text->text);
        setStringTokenWidths(
            tokens, game.sysContent.fntStandard, game.sysContent.fntSlim, line_height, false
        );
        vector<vector<StringToken> > tokens_per_line =
            splitLongStringWithTokens(tokens, draw.size.x);
        float text_h = tokens_per_line.size() * line_height;
        
        for(size_t l = 0; l < tokens_per_line.size(); l++) {
            drawStringTokens(
                tokens_per_line[l], game.sysContent.fntStandard, game.sysContent.fntSlim,
                false,
                Point(
                    draw.center.x,
                    draw.center.y - text_h / 2.0f + l * line_height
                ),
                cursor_info_text->flags,
                Point(draw.size.x, line_height)
            );
        }
        
        //Draw a box around it.
        drawTexturedBox(
            draw.center, draw.size, game.sysContent.bmpFrameBox,
            COLOR_TRANSPARENT_WHITE
        );
        
        //Draw a connection from here to the radar cursor.
        Point line_anchor(draw.center.x - draw.size.x / 2.0f - 16.0f, draw.center.y);
        Point cursor_screen_pos = radarCursor;
        al_transform_coordinates(
            &worldToRadarScreenTransform,
            &cursor_screen_pos.x, &cursor_screen_pos.y
        );
        
        al_draw_line(
            draw.center.x - draw.size.x / 2.0f, draw.center.y,
            line_anchor.x, line_anchor.y,
            COLOR_TRANSPARENT_WHITE, 2.0f
        );
        
        cursor_screen_pos =
            cursor_screen_pos +
            rotatePoint(
                Point(24.0f, 0.0f),
                getAngle(cursor_screen_pos, line_anchor)
            );
        al_draw_line(
            line_anchor.x, line_anchor.y,
            cursor_screen_pos.x, cursor_screen_pos.y,
            COLOR_TRANSPARENT_WHITE, 2.0f
        );
    };
    cursor_info_text->onTick =
    [this, cursor_info_text] (float delta_t) {
        if(radarCursorLeader) {
            cursor_info_text->text =
                (
                    radarCursorLeader == radarSelectedLeader ?
                    "" :
                    "\\k menu_ok \\k "
                ) + radarCursorLeader->type->name;
        } else if(
            radarSelectedLeader &&
            !radarSelectedLeader->fsm.getEvent(LEADER_EV_GO_HERE)
        ) {
            cursor_info_text->text =
                "Can't go here... Leader is busy!";
            cursor_info_text->color = COLOR_WHITE;
        } else {
            switch(goHerePathResult) {
            case PATH_RESULT_DIRECT:
            case PATH_RESULT_DIRECT_NO_STOPS:
            case PATH_RESULT_NORMAL_PATH:
            case PATH_RESULT_PATH_WITH_SINGLE_STOP: {
                cursor_info_text->text = "\\k menu_ok \\k Go here!";
                cursor_info_text->color = COLOR_GOLD;
                break;
            } case PATH_RESULT_PATH_WITH_OBSTACLES: {
                cursor_info_text->text = "Can't go here... Path blocked!";
                cursor_info_text->color = COLOR_WHITE;
                break;
            } case PATH_RESULT_END_STOP_UNREACHABLE: {
                cursor_info_text->text = "Can't go here...";
                cursor_info_text->color = COLOR_WHITE;
                break;
            } default: {
                cursor_info_text->text.clear();
                cursor_info_text->color = COLOR_WHITE;
                break;
            }
            }
        }
    };
    radarGui.addItem(cursor_info_text, "cursor_info");
    
    //Instructions text.
    TextGuiItem* instructions_text =
        new TextGuiItem(
        "\\k menu_radar_up \\k"
        "\\k menu_radar_left \\k"
        "\\k menu_radar_down \\k"
        "\\k menu_radar_right \\k Pan   "
        "\\k menu_radar_zoom_in \\k"
        "\\k menu_radar_zoom_out \\k Zoom",
        game.sysContent.fntSlim,
        COLOR_TRANSPARENT_WHITE, ALLEGRO_ALIGN_RIGHT
    );
    instructions_text->lineWrap = true;
    radarGui.addItem(instructions_text, "instructions");
    
    //Tooltip text.
    TooltipGuiItem* tooltip_text =
        new TooltipGuiItem(&radarGui);
    radarGui.addItem(tooltip_text, "tooltip");
    
    //Finishing touches.
    radarGui.setSelectedItem(nullptr);
    radarGui.responsive = false;
    radarGui.hideItems();
}


/**
 * @brief Initializes the status page.
 */
void PauseMenu::initStatusPage() {
    DataNode* gui_file = &game.content.guiDefs.list[PAUSE_MENU::STATUS_GUI_FILE_NAME];
    
    //Menu items.
    statusGui.registerCoords("header",           50,     5,   52,    6);
    statusGui.registerCoords("left_page",        12,     5,   20,    6);
    statusGui.registerCoords("left_page_input",   3,     7,    4,    4);
    statusGui.registerCoords("right_page",       88,     5,   20,    6);
    statusGui.registerCoords("right_page_input", 97,     7,    4,    4);
    statusGui.registerCoords("line",             50,    11,   96,    2);
    statusGui.registerCoords("continue",         10,    16,   16,    4);
    statusGui.registerCoords("continue_input",    3,    17,    4,    4);
    statusGui.registerCoords("list_header",      50,    23.5, 88,    7);
    statusGui.registerCoords("list",             50,    56,   88,   56);
    statusGui.registerCoords("list_scroll",      97,    56,    2,   56);
    statusGui.registerCoords("totals",           50,    89,   88,    8);
    statusGui.registerCoords("instructions",     58.75, 16,   77.5,  4);
    statusGui.registerCoords("tooltip",          50,    96,   96,    4);
    statusGui.readCoords(gui_file->getChildByName("positions"));
    
    //Header.
    TextGuiItem* header_text =
        new TextGuiItem(
        "STATUS", game.sysContent.fntAreaName,
        COLOR_TRANSPARENT_WHITE
    );
    statusGui.addItem(header_text, "header");
    
    //Page buttons and inputs.
    createPageButtons(PAUSE_MENU_PAGE_STATUS, &statusGui);
    
    //Line.
    GuiItem* line = new GuiItem();
    line->onDraw =
    [] (const DrawInfo & draw) {
        drawFilledRoundedRectangle(
            draw.center,
            Point(draw.size.x, 3.0f),
            2.0f,
            COLOR_TRANSPARENT_WHITE
        );
    };
    statusGui.addItem(line, "line");
    
    //Continue button.
    statusGui.backItem =
        new ButtonGuiItem("Continue", game.sysContent.fntStandard);
    statusGui.backItem->onActivate =
    [this] (const Point &) {
        startClosing(&statusGui);
    };
    statusGui.backItem->onGetTooltip =
    [] () { return "Unpause and continue playing."; };
    statusGui.addItem(statusGui.backItem, "continue");
    
    //Continue input icon.
    guiAddBackInputIcon(&statusGui, "continue_input");
    
    //Pikmin list header box.
    ListGuiItem* list_header = new ListGuiItem();
    list_header->onDraw =
    [] (const DrawInfo &) {};
    statusGui.addItem(list_header, "list_header");
    
    //Pikmin list box.
    pikminList = new ListGuiItem();
    statusGui.addItem(pikminList, "list");
    
    //Pikmin list scrollbar.
    ScrollGuiItem* list_scroll = new ScrollGuiItem();
    list_scroll->listItem = pikminList;
    statusGui.addItem(list_scroll, "list_scroll");
    
    //Pikmin totals box.
    ListGuiItem* totals = new ListGuiItem();
    totals->onDraw =
    [] (const DrawInfo &) {};
    statusGui.addItem(totals, "totals");
    
    //Tooltip text.
    TooltipGuiItem* tooltip_text =
        new TooltipGuiItem(&statusGui);
    statusGui.addItem(tooltip_text, "tooltip");
    
    //Setup the list header.
    addPikminStatusLine(
        list_header,
        nullptr,
        "Group",
        "Idle",
        "Field",
        "Onion",
        "Total",
        "New",
        "Lost",
        true, false
    );
    
    size_t total_in_group = 0;
    size_t total_idling = 0;
    size_t total_on_field = 0;
    long total_in_onion = 0;
    long grand_total = 0;
    long total_new = 0;
    long total_lost = 0;
    
    //Setup the list rows.
    for(size_t p = 0; p < game.config.pikmin.order.size(); p++) {
        PikminType* pt_ptr = game.config.pikmin.order[p];
        
        size_t in_group =
            game.states.gameplay->getAmountOfGroupPikmin(pt_ptr);
        size_t idling =
            game.states.gameplay->getAmountOfIdlePikmin(pt_ptr);
        size_t on_field =
            game.states.gameplay->getAmountOfFieldPikmin(pt_ptr);
        long in_onion =
            game.states.gameplay->getAmountOfOnionPikmin(pt_ptr);
        long total = (long) on_field + in_onion;
        
        long new_piks = 0;
        auto new_it =
            game.states.gameplay->pikminBornPerType.find(pt_ptr);
        if(new_it != game.states.gameplay->pikminBornPerType.end()) {
            new_piks = new_it->second;
        }
        long lost = 0;
        auto lost_it =
            game.states.gameplay->pikminDeathsPerType.find(pt_ptr);
        if(lost_it != game.states.gameplay->pikminDeathsPerType.end()) {
            lost = lost_it->second;
        }
        
        if(total + new_piks + lost > 0) {
            addPikminStatusLine(
                pikminList,
                pt_ptr,
                i2s(in_group),
                i2s(idling),
                i2s(on_field),
                i2s(in_onion),
                i2s(total),
                i2s(new_piks),
                i2s(lost),
                false, false
            );
        }
        
        total_in_group += in_group;
        total_idling += idling;
        total_on_field += on_field;
        total_in_onion += in_onion;
        grand_total += total;
        total_new += new_piks;
        total_lost += lost;
    }
    
    //Setup the list totals.
    addPikminStatusLine(
        totals,
        nullptr,
        i2s(total_in_group),
        i2s(total_idling),
        i2s(total_on_field),
        i2s(total_in_onion),
        i2s(grand_total),
        i2s(total_new),
        i2s(total_lost),
        true, true
    );
    
    //Finishing touches.
    statusGui.setSelectedItem(statusGui.backItem, true);
    statusGui.responsive = false;
    statusGui.hideItems();
}


/**
 * @brief Pans the radar by an amount.
 *
 * @param amount How much to pan by.
 */
void PauseMenu::panRadar(Point amount) {
    Point delta = amount / radarCam.zoom;
    radarCam.pos += delta;
    radarCam.pos.x =
        std::clamp(radarCam.pos.x, radarMinCoords.x, radarMaxCoords.x);
    radarCam.pos.y =
        std::clamp(radarCam.pos.y, radarMinCoords.y, radarMaxCoords.y);
}


/**
 * @brief When the player confirms their action in the radar.
 */
void PauseMenu::radarConfirm() {
    calculateGoHerePath();
    
    if(radarCursorLeader) {
        //Select a leader.
        radarSelectedLeader = radarCursorLeader;
        
    } else if(
        goHerePathResult == PATH_RESULT_DIRECT ||
        goHerePathResult == PATH_RESULT_DIRECT_NO_STOPS ||
        goHerePathResult == PATH_RESULT_NORMAL_PATH ||
        goHerePathResult == PATH_RESULT_PATH_WITH_SINGLE_STOP
    ) {
        //Start Go Here.
        radarSelectedLeader->fsm.runEvent(
            LEADER_EV_GO_HERE, (void*) &radarCursor
        );
        startClosing(&radarGui);
        
    }
}


/**
 * @brief Starts the closing process.
 *
 * @param cur_gui The currently active GUI manager.
 */
void PauseMenu::startClosing(GuiManager* cur_gui) {
    cur_gui->responsive = false;
    cur_gui->startAnimation(
        GUI_MANAGER_ANIM_CENTER_TO_UP,
        GAMEPLAY::MENU_EXIT_HUD_MOVE_TIME
    );
    game.states.gameplay->hud->gui.startAnimation(
        GUI_MANAGER_ANIM_OUT_TO_IN,
        GAMEPLAY::MENU_EXIT_HUD_MOVE_TIME
    );
    closing = true;
    closingTimer = GAMEPLAY::MENU_EXIT_HUD_MOVE_TIME;
    
    game.states.gameplay->radarZoom = radarCam.zoom;
}


/**
 * @brief Starts the process of leaving the gameplay state.
 */
void PauseMenu::startLeavingGameplay() {
    if(
        leaveTarget == GAMEPLAY_LEAVE_TARGET_END &&
        hasFlag(
            game.curAreaData->mission.failConditions,
            getIdxBitmask(MISSION_FAIL_COND_PAUSE_MENU)
        )
    ) {
        game.states.gameplay->missionFailReason =
            MISSION_FAIL_COND_PAUSE_MENU;
    }
    game.states.gameplay->startLeaving(leaveTarget);
}


/**
 * @brief Switches pages in the pause menu.
 *
 * @param cur_gui Pointer to the current page's GUI manager.
 * @param new_page The new page to switch to.
 * @param left Is the new page to the left of the current one, or the right?
 */
void PauseMenu::switchPage(
    GuiManager* cur_gui, PAUSE_MENU_PAGE new_page, bool left
) {
    GuiManager* new_gui = nullptr;
    switch(new_page) {
    case PAUSE_MENU_PAGE_SYSTEM: {
        new_gui = &gui;
        break;
    } case PAUSE_MENU_PAGE_RADAR: {
        new_gui = &radarGui;
        break;
    } case PAUSE_MENU_PAGE_STATUS: {
        new_gui = &statusGui;
        break;
    } case PAUSE_MENU_PAGE_MISSION: {
        new_gui = &missionGui;
        break;
    }
    }
    
    cur_gui->responsive = false;
    cur_gui->startAnimation(
        left ?
        GUI_MANAGER_ANIM_CENTER_TO_RIGHT :
        GUI_MANAGER_ANIM_CENTER_TO_LEFT,
        GAMEPLAY::MENU_EXIT_HUD_MOVE_TIME
    );
    new_gui->responsive = true;
    new_gui->startAnimation(
        left ?
        GUI_MANAGER_ANIM_LEFT_TO_CENTER :
        GUI_MANAGER_ANIM_RIGHT_TO_CENTER,
        GAMEPLAY::MENU_EXIT_HUD_MOVE_TIME
    );
}


/**
 * @brief Ticks time by one frame of logic.
 *
 * @param delta_t How long the frame's tick is, in seconds.
 */
void PauseMenu::tick(float delta_t) {
    //Tick the GUI.
    gui.tick(delta_t);
    radarGui.tick(delta_t);
    statusGui.tick(delta_t);
    missionGui.tick(delta_t);
    confirmationGui.tick(delta_t);
    
    if(secondaryMenu) {
        if(secondaryMenu->loaded) {
            secondaryMenu->tick(game.deltaT);
        }
        if(!secondaryMenu->loaded) {
            delete secondaryMenu;
            secondaryMenu = nullptr;
        }
    }
    
    //Tick the background.
    const float bg_alpha_mult_speed =
        1.0f / GAMEPLAY::MENU_ENTRY_HUD_MOVE_TIME;
    const float diff =
        closing ? -bg_alpha_mult_speed : bg_alpha_mult_speed;
    bgAlphaMult = std::clamp(bgAlphaMult + diff * delta_t, 0.0f, 1.0f);
    
    //Tick the menu opening and closing.
    if(openingLockoutTimer > 0.0f) {
        openingLockoutTimer -= delta_t;
    }
    if(closing) {
        closingTimer -= delta_t;
        if(closingTimer <= 0.0f) {
            toDelete = true;
        }
    }
    
    //Tick radar things.
    DrawInfo radar_draw;
    radarGui.getItemDrawInfo(radarItem, &radar_draw);
    
    updateRadarTransformations(radar_draw.center, radar_draw.size);
    
    if(radarGui.responsive) {
    
        Point radar_pan_coords;
        float dummy_angle;
        float dummy_magnitude;
        radarPan.getInfo(&radar_pan_coords, &dummy_angle, &dummy_magnitude);
        if(radar_pan_coords.x != 0.0f || radar_pan_coords.y != 0.0f) {
            panRadar(radar_pan_coords * PAUSE_MENU::RADAR_PAN_SPEED * delta_t);
        }
        
        if(radarZoomIn && !radarZoomOut) {
            zoomRadar(PAUSE_MENU::RADAR_ZOOM_SPEED * delta_t);
        } else if(radarZoomOut && !radarZoomIn) {
            zoomRadar(-PAUSE_MENU::RADAR_ZOOM_SPEED * delta_t);
        }
        
        bool mouse_in_radar =
            isPointInRectangle(
                game.mouseCursor.sPos,
                radar_draw.center, radar_draw.size
            );
            
        if(mouse_in_radar) {
            radarCursor = game.mouseCursor.sPos;
            al_transform_coordinates(
                &radarScreenToWorldTransform,
                &radarCursor.x, &radarCursor.y
            );
        } else {
            radarCursor = radarCam.pos;
        }
        
        goHereCalcTime -= delta_t;
        if(goHereCalcTime <= 0.0f) {
            goHereCalcTime = PAUSE_MENU::GO_HERE_CALC_INTERVAL;
            
            calculateGoHerePath();
        }
        
    }
    
}


/**
 * @brief Updates the radar transformations.
 *
 * @param radar_center Coordinates of the radar's center.
 * @param radar_size Dimensions of the radar.
 */
void PauseMenu::updateRadarTransformations(
    const Point &radar_center, const Point &radar_size
) {
    worldToRadarScreenTransform = game.identityTransform;
    al_translate_transform(
        &worldToRadarScreenTransform,
        -radarCam.pos.x + radar_center.x / radarCam.zoom,
        -radarCam.pos.y + radar_center.y / radarCam.zoom
    );
    al_scale_transform(
        &worldToRadarScreenTransform, radarCam.zoom, radarCam.zoom
    );
    
    radarScreenToWorldTransform = worldToRadarScreenTransform;
    al_invert_transform(&radarScreenToWorldTransform);
}


/**
 * @brief Zooms the radar by an amount.
 *
 * @param amount How much to zoom by.
 */
void PauseMenu::zoomRadar(float amount) {
    float delta = amount * radarCam.zoom;
    radarCam.zoom += delta;
    radarCam.zoom =
        std::clamp(
            radarCam.zoom,
            PAUSE_MENU::RADAR_MIN_ZOOM, PAUSE_MENU::RADAR_MAX_ZOOM
        );
}


/**
 * @brief Zooms the radar by an amount, anchored on the radar cursor.
 *
 * @param amount How much to zoom by.
 * @param radar_center Coordinates of the radar's center.
 * @param radar_size Dimensions of the radar.
 */
void PauseMenu::zoomRadarWithMouse(
    float amount, const Point &radar_center, const Point &radar_size
) {
    //Keep a backup of the old cursor coordinates.
    Point old_cursor_pos = radarCursor;
    
    //Do the zoom.
    zoomRadar(amount);
    updateRadarTransformations(radar_center, radar_size);
    
    //Figure out where the cursor will be after the zoom.
    radarCursor = game.mouseCursor.sPos;
    al_transform_coordinates(
        &radarScreenToWorldTransform,
        &radarCursor.x, &radarCursor.y
    );
    
    //Readjust the transformation by shifting the camera
    //so that the cursor ends up where it was before.
    panRadar(
        Point(
            (old_cursor_pos.x - radarCursor.x) * radarCam.zoom,
            (old_cursor_pos.y - radarCursor.y) * radarCam.zoom
        )
    );
    
    //Update the cursor coordinates again.
    updateRadarTransformations(radar_center, radar_size);
    radarCursor = game.mouseCursor.sPos;
    al_transform_coordinates(
        &radarScreenToWorldTransform,
        &radarCursor.x, &radarCursor.y
    );
}
