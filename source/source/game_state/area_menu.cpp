/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Area menu state class and area menu state-related functions.
 */

#include <algorithm>

#include "menus.h"

#include "../core/drawing.h"
#include "../core/misc_functions.h"
#include "../core/game.h"
#include "../core/load.h"
#include "../util/allegro_utils.h"
#include "../util/general_utils.h"
#include "../util/string_utils.h"


namespace AREA_MENU {

//Name of the main GUI information file.
const string GUI_FILE_NAME = "area_menu";

//Path to the area info GUI information file.
const string INFO_GUI_FILE_NAME = "area_menu_info";

//How long to animate the page swapping for.
const float PAGE_SWAP_DURATION = 0.5f;

//Path to the mission specs GUI information file.
const string SPECS_GUI_FILE_NAME = "area_menu_specs";

}


/**
 * @brief Adds a new bullet point to either the fail condition list, or the
 * grading explanation list.
 *
 * @param list List to add to.
 * @param text Text.
 */
void area_menu_state::add_bullet(list_gui_item* list, const string &text) {
    size_t bullet_idx = list->children.size();
    const float BULLET_HEIGHT = 0.18f;
    const float BULLET_PADDING = 0.01f;
    const float BULLETS_OFFSET = 0.01f;
    const float bullet_center_y =
        (BULLETS_OFFSET + BULLET_HEIGHT / 2.0f) +
        ((BULLET_HEIGHT + BULLET_PADDING) * bullet_idx);
        
    bullet_gui_item* bullet =
        new bullet_gui_item(
        text, game.sys_content.fnt_standard, COLOR_WHITE
    );
    bullet->center = point(0.50f, bullet_center_y);
    bullet->size = point(0.96f, BULLET_HEIGHT);
    list->add_child(bullet);
    gui.add_item(bullet);
}


/**
 * @brief Animates the GUI items inside of the info and specs pages.
 */
void area_menu_state::animate_info_and_specs() {
    info_name_text->start_juice_animation(
        gui_item::JUICE_TYPE_GROW_TEXT_ELASTIC_LOW
    );
    subtitle_text->start_juice_animation(
        gui_item::JUICE_TYPE_GROW_TEXT_ELASTIC_LOW
    );
    description_text->start_juice_animation(
        gui_item::JUICE_TYPE_GROW_TEXT_ELASTIC_MEDIUM
    );
    difficulty_text->start_juice_animation(
        gui_item::JUICE_TYPE_GROW_TEXT_ELASTIC_LOW
    );
    tags_text->start_juice_animation(
        gui_item::JUICE_TYPE_GROW_TEXT_ELASTIC_LOW
    );
    maker_text->start_juice_animation(
        gui_item::JUICE_TYPE_GROW_TEXT_ELASTIC_LOW
    );
    version_text->start_juice_animation(
        gui_item::JUICE_TYPE_GROW_TEXT_ELASTIC_LOW
    );
    if(area_type == AREA_TYPE_MISSION) {
        record_info_text->start_juice_animation(
            gui_item::JUICE_TYPE_GROW_TEXT_ELASTIC_LOW
        );
        record_date_text->start_juice_animation(
            gui_item::JUICE_TYPE_GROW_TEXT_ELASTIC_LOW
        );
        specs_name_text->start_juice_animation(
            gui_item::JUICE_TYPE_GROW_TEXT_ELASTIC_LOW
        );
        goal_text->start_juice_animation(
            gui_item::JUICE_TYPE_GROW_TEXT_ELASTIC_LOW
        );
        for(size_t c = 0; c < fail_list->children.size(); c++) {
            fail_list->children[c]->start_juice_animation(
                gui_item::JUICE_TYPE_GROW_TEXT_ELASTIC_LOW
            );
        }
        for(size_t c = 0; c < grading_list->children.size(); c++) {
            grading_list->children[c]->start_juice_animation(
                gui_item::JUICE_TYPE_GROW_TEXT_ELASTIC_LOW
            );
        }
    }
}


/**
 * @brief Changes the area information to a new area's information.
 *
 * @param area_idx Index of the newly-selected area.
 */
void area_menu_state::change_info(size_t area_idx) {
    if(area_idx == cur_area_idx) return;
    cur_area_idx = area_idx;
    
    //Start by clearing them all, for sanitization's sake.
    info_name_text->text.clear();
    subtitle_text->text.clear();
    description_text->text.clear();
    difficulty_text->text.clear();
    cur_thumb = nullptr;
    tags_text->text.clear();
    maker_text->text.clear();
    version_text->text.clear();
    cur_stamp = nullptr;
    cur_medal = nullptr;
    if(area_type == AREA_TYPE_MISSION) {
        record_info_text->text.clear();
        record_date_text->text.clear();
        goal_text->text.clear();
        specs_name_text->text.clear();
        fail_list->delete_all_children();
        grading_list->delete_all_children();
    }
    
    //Fill in the area's info.
    area_data* area_ptr = game.content.areas.list[area_type][area_idx];
    info_name_text->text = area_ptr->name;
    subtitle_text->text =
        get_subtitle_or_mission_goal(
            area_ptr->subtitle,
            area_type,
            area_ptr->mission.goal
        );
    description_text->text = area_ptr->description;
    if(area_ptr->difficulty == 0) {
        difficulty_text->text.clear();
    } else {
        difficulty_text->text =
            "Difficulty: " +
            i2s(area_ptr->difficulty) + "/5 - ";
        switch(area_ptr->difficulty) {
        case 1: {
            difficulty_text->text += "Very easy";
            break;
        } case 2: {
            difficulty_text->text += "Easy";
            break;
        } case 3: {
            difficulty_text->text += "Medium";
            break;
        } case 4: {
            difficulty_text->text += "Hard";
            break;
        } case 5: {
            difficulty_text->text += "Very hard";
            break;
        }
        }
    }
    tags_text->text =
        (area_ptr->tags.empty() ? "" : "Tags: " + area_ptr->tags);
    maker_text->text =
        (area_ptr->maker.empty() ? "" : "Maker: " + area_ptr->maker);
    version_text->text =
        (area_ptr->version.empty() ? "" : "Version: " + area_ptr->version);
    cur_thumb = area_ptr->thumbnail.get();
    if(area_type == AREA_TYPE_MISSION) {
        int score = area_records[area_idx].score;
        bool record_exists = !area_records[area_idx].date.empty();
        record_info_text->text =
            !record_exists ?
            "(None)" :
            area_ptr->mission.grading_mode ==
            MISSION_GRADING_MODE_POINTS ?
            amount_str(score, "point") :
            "";
        cur_stamp =
            !record_exists ?
            nullptr :
            area_records[area_idx].clear ?
            game.sys_content.bmp_mission_clear :
            game.sys_content.bmp_mission_fail;
        if(!record_exists) {
            cur_medal = nullptr;
        } else {
            switch(area_ptr->mission.grading_mode) {
            case MISSION_GRADING_MODE_POINTS: {
                if(score >= area_ptr->mission.platinum_req) {
                    cur_medal = game.sys_content.bmp_medal_platinum;
                } else if(score >= area_ptr->mission.gold_req) {
                    cur_medal = game.sys_content.bmp_medal_gold;
                } else if(score >= area_ptr->mission.silver_req) {
                    cur_medal = game.sys_content.bmp_medal_silver;
                } else if(score >= area_ptr->mission.bronze_req) {
                    cur_medal = game.sys_content.bmp_medal_bronze;
                } else {
                    cur_medal = game.sys_content.bmp_medal_none;
                }
                break;
            } case MISSION_GRADING_MODE_GOAL: {
                if(area_records[area_idx].clear) {
                    cur_medal = game.sys_content.bmp_medal_platinum;
                }
                break;
            } case MISSION_GRADING_MODE_PARTICIPATION: {
                cur_medal = game.sys_content.bmp_medal_platinum;
                break;
            }
            }
        }
        record_date_text->text = area_records[area_idx].date;
    }
    
    //Now fill in the mission specs.
    if(area_type == AREA_TYPE_MISSION) {
        specs_name_text->text = area_ptr->name;
        mission_data &mission = area_ptr->mission;
        goal_text->text =
            game.mission_goals[mission.goal]->
            get_player_description(&mission);
            
        for(size_t f = 0; f < game.mission_fail_conds.size(); f++) {
            if(has_flag(mission.fail_conditions, get_idx_bitmask(f))) {
                mission_fail* cond = game.mission_fail_conds[f];
                add_bullet(
                    fail_list,
                    cond->get_player_description(&mission)
                );
            }
        }
        
        if(mission.fail_conditions == 0) {
            add_bullet(fail_list, "(None)");
        }
        
        switch(mission.grading_mode) {
        case MISSION_GRADING_MODE_POINTS: {
            add_bullet(
                grading_list,
                "Your medal depends on your score:"
            );
            add_bullet(
                grading_list,
                "    Platinum: " + i2s(mission.platinum_req) + "+ points."
            );
            add_bullet(
                grading_list,
                "    Gold: " + i2s(mission.gold_req) + "+ points."
            );
            add_bullet(
                grading_list,
                "    Silver: " + i2s(mission.silver_req) + "+ points."
            );
            add_bullet(
                grading_list,
                "    Bronze: " + i2s(mission.bronze_req) + "+ points."
            );
            vector<string> score_notes;
            for(size_t c = 0; c < game.mission_score_criteria.size(); c++) {
                mission_score_criterion* c_ptr =
                    game.mission_score_criteria[c];
                int mult = c_ptr->get_multiplier(&mission);
                if(mult != 0) {
                    score_notes.push_back(
                        "    " + c_ptr->get_name() + " x " + i2s(mult) + "."
                    );
                }
            }
            if(!score_notes.empty()) {
                add_bullet(
                    grading_list,
                    "Your score is calculated like so:"
                );
                for(size_t s = 0; s < score_notes.size(); s++) {
                    add_bullet(grading_list, score_notes[s]);
                }
            } else {
                add_bullet(
                    grading_list,
                    "In this mission, your score will always be 0."
                );
            }
            vector<string> loss_notes;
            for(size_t c = 0; c < game.mission_score_criteria.size(); c++) {
                mission_score_criterion* c_ptr =
                    game.mission_score_criteria[c];
                if(
                    has_flag(
                        mission.point_loss_data,
                        get_idx_bitmask(c)
                    )
                ) {
                    loss_notes.push_back("    " + c_ptr->get_name());
                }
            }
            if(!loss_notes.empty()) {
                add_bullet(
                    grading_list,
                    "If you fail, you'll lose your score for:"
                );
                for(size_t l = 0; l < loss_notes.size(); l++) {
                    add_bullet(grading_list, loss_notes[l]);
                }
            }
            if(!mission.maker_record_date.empty()) {
                add_bullet(
                    grading_list,
                    "Maker's record: " + i2s(mission.maker_record) +
                    " (" + mission.maker_record_date + ")"
                );
            }
            break;
        }
        case MISSION_GRADING_MODE_GOAL: {
            add_bullet(
                grading_list,
                "You get a platinum medal if you clear the goal."
            );
            add_bullet(
                grading_list,
                "You get no medal if you fail."
            );
            break;
        }
        case MISSION_GRADING_MODE_PARTICIPATION: {
            add_bullet(
                grading_list,
                "You get a platinum medal just by playing the mission."
            );
            break;
        }
        }
    }
    
    animate_info_and_specs();
}


/**
 * @brief Draws the area menu.
 */
void area_menu_state::do_drawing() {
    al_clear_to_color(COLOR_BLACK);
    draw_bitmap(
        bmp_menu_bg, point(game.win_w * 0.5, game.win_h * 0.5),
        point(game.win_w, game.win_h), 0, map_gray(64)
    );
    
    gui.draw();
    
    draw_mouse_cursor(GAME::CURSOR_STANDARD_COLOR);
}


/**
 * @brief Ticks time by one frame of logic.
 */
void area_menu_state::do_logic() {
    vector<player_action> player_actions = game.controls.new_frame();
    if(!game.fade_mgr.is_fading()) {
        for(size_t a = 0; a < player_actions.size(); a++) {
            gui.handle_player_action(player_actions[a]);
        }
    }
    
    gui.tick(game.delta_t);
    
    game.fade_mgr.tick(game.delta_t);
}


/**
 * @brief Returns the name of this state.
 *
 * @return The name.
 */
string area_menu_state::get_name() const {
    return "area menu";
}


/**
 * @brief Handles Allegro events.
 *
 * @param ev Event to handle.
 */
void area_menu_state::handle_allegro_event(ALLEGRO_EVENT &ev) {
    if(game.fade_mgr.is_fading()) return;
    
    gui.handle_event(ev);
}


/**
 * @brief Initializes the area info page GUI items.
 */
void area_menu_state::init_gui_info_page() {
    gui.register_coords("info_name",    36,  6, 68,  8);
    gui.register_coords("subtitle",     36, 16, 68,  8);
    gui.register_coords("thumbnail",    85, 14, 26, 24);
    gui.register_coords("description",  50, 40, 96, 24);
    gui.register_coords("record_label", 50, 56, 96,  4);
    gui.register_coords("record_info",  50, 62, 36,  4);
    gui.register_coords("record_stamp", 20, 65, 20, 14);
    gui.register_coords("record_medal", 80, 65, 20, 14);
    gui.register_coords("record_date",  50, 66, 28,  4);
    gui.register_coords("difficulty",   50, 79, 96,  6);
    gui.register_coords("tags",         50, 87, 96,  6);
    gui.register_coords("maker",        28, 95, 52,  6);
    gui.register_coords("version",      76, 95, 44,  6);
    gui.read_coords(
        game.content.gui_defs.list[AREA_MENU::INFO_GUI_FILE_NAME].get_child_by_name("positions")
    );
    
    if(!game.content.areas.list[area_type].empty()) {
    
        //Name text.
        info_name_text =
            new text_gui_item("", game.sys_content.fnt_area_name, COLOR_GOLD);
        info_box->add_child(info_name_text);
        gui.add_item(info_name_text, "info_name");
        
        //Subtitle text.
        subtitle_text = new text_gui_item("", game.sys_content.fnt_area_name);
        info_box->add_child(subtitle_text);
        gui.add_item(subtitle_text, "subtitle");
        
        //Thumbnail.
        gui_item* thumb_item = new gui_item();
        thumb_item->on_draw =
        [this] (const point & center, const point & size) {
            //Make it a square.
            point final_size(
                std::min(size.x, size.y),
                std::min(size.x, size.y)
            );
            //Align it to the top-right corner.
            point final_center(
                (center.x + size.x / 2.0f) - final_size.x / 2.0f,
                (center.y - size.y / 2.0f) + final_size.y / 2.0f
            );
            if(cur_thumb) {
                draw_bitmap(cur_thumb, final_center, final_size - 4.0f);
            }
            draw_textured_box(
                final_center, final_size, game.sys_content.bmp_frame_box,
                COLOR_TRANSPARENT_WHITE
            );
        };
        info_box->add_child(thumb_item);
        gui.add_item(thumb_item, "thumbnail");
        
        //Description text.
        description_text =
            new text_gui_item(
            "", game.sys_content.fnt_standard, COLOR_WHITE, ALLEGRO_ALIGN_LEFT
        );
        description_text->line_wrap = true;
        info_box->add_child(description_text);
        gui.add_item(description_text, "description");
        
        if(area_type == AREA_TYPE_MISSION) {
            //Record label.
            text_gui_item* record_label_text =
                new text_gui_item("Record:", game.sys_content.fnt_standard);
            info_box->add_child(record_label_text);
            gui.add_item(record_label_text, "record_label");
            
            //Record info.
            record_info_text =
                new text_gui_item("", game.sys_content.fnt_standard);
            info_box->add_child(record_info_text);
            gui.add_item(record_info_text, "record_info");
            
            //Record stamp.
            gui_item* record_stamp_item = new gui_item();
            record_stamp_item->on_draw =
            [this] (const point & center, const point & size) {
                if(cur_stamp) {
                    draw_bitmap_in_box(
                        cur_stamp, center, size, true
                    );
                }
            };
            info_box->add_child(record_stamp_item);
            gui.add_item(record_stamp_item, "record_stamp");
            
            //Record medal.
            gui_item* record_medal_item = new gui_item();
            record_medal_item->on_draw =
            [this] (const point & center, const point & size) {
                if(cur_medal) {
                    draw_bitmap_in_box(
                        cur_medal, center, size, true
                    );
                }
            };
            info_box->add_child(record_medal_item);
            gui.add_item(record_medal_item, "record_medal");
            
            //Record date.
            record_date_text =
                new text_gui_item(
                "", game.sys_content.fnt_slim, al_map_rgb(128, 128, 128)
            );
            info_box->add_child(record_date_text);
            gui.add_item(record_date_text, "record_date");
        }
        
        //Difficulty text.
        difficulty_text =
            new text_gui_item(
            "", game.sys_content.fnt_standard, COLOR_WHITE, ALLEGRO_ALIGN_LEFT
        );
        info_box->add_child(difficulty_text);
        gui.add_item(difficulty_text, "difficulty");
        
        //Tags text.
        tags_text =
            new text_gui_item(
            "", game.sys_content.fnt_standard, COLOR_WHITE, ALLEGRO_ALIGN_LEFT
        );
        info_box->add_child(tags_text);
        gui.add_item(tags_text, "tags");
        
        //Maker text.
        maker_text =
            new text_gui_item(
            "", game.sys_content.fnt_standard, COLOR_WHITE, ALLEGRO_ALIGN_LEFT
        );
        info_box->add_child(maker_text);
        gui.add_item(maker_text, "maker");
        
        //Version text.
        version_text =
            new text_gui_item(
            "", game.sys_content.fnt_standard, COLOR_WHITE, ALLEGRO_ALIGN_RIGHT
        );
        info_box->add_child(version_text);
        gui.add_item(version_text, "version");
        
    }
}


/**
 * @brief Initializes the main GUI items.
 */
void area_menu_state::init_gui_main() {
    gui.register_coords("back",          12,  5, 20,  6);
    gui.register_coords("back_input",     3,  7,  4,  4);
    gui.register_coords("header",        40,  5, 32,  6);
    gui.register_coords("list",          20, 51, 36, 82);
    gui.register_coords("list_scroll",   40, 51,  2, 82);
    gui.register_coords("view_toggle",   74,  5, 32,  6);
    gui.register_coords("info_box",      70, 51, 56, 82);
    gui.register_coords("specs_box",     70, 51, 56, 82);
    gui.register_coords("random",        95,  5,  6,  6);
    gui.register_coords("tooltip",       50, 96, 96,  4);
    gui.register_coords("no_areas_text", 50, 50, 96, 10);
    
    gui.read_coords(
        game.content.gui_defs.list[AREA_MENU::GUI_FILE_NAME].get_child_by_name("positions")
    );
    
    //Back button.
    gui.back_item =
        new button_gui_item("Back", game.sys_content.fnt_standard);
    gui.back_item->on_activate =
    [this] (const point &) {
        leave();
    };
    gui.back_item->on_get_tooltip =
    [] () { return "Return to the main menu."; };
    gui.add_item(gui.back_item, "back");
    
    //Back input icon.
    gui_add_back_input_icon(&gui);
    
    //Header text.
    text_gui_item* header_text =
        new text_gui_item(
        "PICK AN AREA:",
        game.sys_content.fnt_area_name, COLOR_TRANSPARENT_WHITE, ALLEGRO_ALIGN_LEFT
    );
    gui.add_item(header_text, "header");
    
    if(!game.content.areas.list[area_type].empty()) {
    
        //Area list box.
        list_box = new list_gui_item();
        gui.add_item(list_box, "list");
        
        //Area list scrollbar.
        scroll_gui_item* list_scroll = new scroll_gui_item();
        list_scroll->list_item = list_box;
        gui.add_item(list_scroll, "list_scroll");
        
        //Items for the various areas.
        for(size_t a = 0; a < game.content.areas.list[area_type].size(); a++) {
            area_data* area_ptr = game.content.areas.list[area_type][a];
            const float BUTTON_HEIGHT = 0.09f;
            const float center_y = 0.045f + a * 0.10f;
            
            //Area button.
            button_gui_item* area_button =
                new button_gui_item(area_ptr->name, game.sys_content.fnt_standard);
            area_button->center =
                point(
                    area_type == AREA_TYPE_MISSION ? 0.40f : 0.50f,
                    center_y
                );
            area_button->size =
                point(
                    area_type == AREA_TYPE_MISSION ? 0.80f : 1.00f,
                    BUTTON_HEIGHT
                );
            area_button->on_activate =
            [this, area_ptr] (const point &) {
                game.states.gameplay->path_of_area_to_load = area_ptr->manifest->path;
                game.fade_mgr.start_fade(false, [] () {
                    game.change_state(game.states.gameplay);
                });
            };
            area_button->on_selected =
            [this, a] () { change_info(a); };
            area_button->on_get_tooltip =
            [area_ptr] () { return "Play " + area_ptr->name + "."; };
            list_box->add_child(area_button);
            gui.add_item(area_button);
            area_buttons.push_back(area_button);
            if(!first_area_button) {
                first_area_button = area_button;
            }
            
            if(area_type == AREA_TYPE_MISSION) {
                //Stamp item.
                gui_item* stamp_item = new gui_item();
                stamp_item->center =
                    point(0.85f, center_y - (BUTTON_HEIGHT * 0.15f));
                stamp_item->size =
                    point(0.12f, BUTTON_HEIGHT * 0.60f);
                stamp_item->on_draw =
                [this, a] (const point & center, const point & size) {
                    if(area_records[a].clear) {
                        draw_bitmap_in_box(
                            game.sys_content.bmp_mission_clear,
                            center, size, true
                        );
                    }
                };
                list_box->add_child(stamp_item);
                gui.add_item(stamp_item);
                
                //Medal item.
                gui_item* medal_item = new gui_item();
                medal_item->center =
                    point(0.95f, center_y + (BUTTON_HEIGHT * 0.15f));
                medal_item->size =
                    point(0.12f, BUTTON_HEIGHT * 0.60f);
                medal_item->on_draw =
                [this, area_ptr, a] (const point & center, const point & size) {
                    ALLEGRO_BITMAP* medal_bmp = nullptr;
                    switch(area_ptr->mission.grading_mode) {
                    case MISSION_GRADING_MODE_POINTS: {
                        int score = area_records[a].score;
                        if(score >= area_ptr->mission.platinum_req) {
                            medal_bmp = game.sys_content.bmp_medal_platinum;
                        } else if(score >= area_ptr->mission.gold_req) {
                            medal_bmp = game.sys_content.bmp_medal_gold;
                        } else if(score >= area_ptr->mission.silver_req) {
                            medal_bmp = game.sys_content.bmp_medal_silver;
                        } else if(score >= area_ptr->mission.bronze_req) {
                            medal_bmp = game.sys_content.bmp_medal_bronze;
                        }
                        break;
                    } case MISSION_GRADING_MODE_GOAL: {
                        if(area_records[a].clear) {
                            medal_bmp = game.sys_content.bmp_medal_platinum;
                        }
                        break;
                    } case MISSION_GRADING_MODE_PARTICIPATION: {
                        medal_bmp = game.sys_content.bmp_medal_platinum;
                    }
                    }
                    
                    if(medal_bmp) {
                        draw_bitmap_in_box(
                            medal_bmp, center, size, true
                        );
                    }
                };
                list_box->add_child(medal_item);
                gui.add_item(medal_item);
            }
        }
        
        //Info box item.
        info_box = new gui_item();
        info_box->on_draw =
        [] (const point & center, const point & size) {
            draw_textured_box(
                center, size, game.sys_content.bmp_frame_box,
                COLOR_TRANSPARENT_WHITE
            );
        };
        gui.add_item(info_box, "info_box");
        
        //Random button.
        button_gui_item* random_button =
            new button_gui_item("", game.sys_content.fnt_standard);
        random_button->on_draw =
        [random_button] (const point & center, const point & size) {
            draw_button(
                center, size, "", game.sys_content.fnt_standard, COLOR_WHITE,
                random_button->selected
            );
            draw_bitmap_in_box(
                game.sys_content.bmp_random,
                center, size - 8, true
            );
        };
        random_button->on_activate =
        [this] (const point &) {
            size_t area_idx = game.rng.i(0, (int) (area_buttons.size() - 1));
            area_buttons[area_idx]->on_activate(point());
        };
        random_button->on_get_tooltip =
        [] () { return "Pick a random area."; };
        gui.add_item(random_button, "random");
        
        if(area_type == AREA_TYPE_MISSION) {
            //View toggle button.
            button_gui_item* view_toggle_button =
                new button_gui_item(
                "Show mission specs",
                game.sys_content.fnt_standard
            );
            view_toggle_button->on_activate =
            [this, view_toggle_button] (const point &) {
                gui_item* box_to_show = nullptr;
                gui_item* box_to_hide = nullptr;
                if(show_mission_specs) {
                    box_to_show = info_box;
                    box_to_hide = specs_box;
                    show_mission_specs = false;
                    view_toggle_button->text = "Show mission specs";
                } else {
                    box_to_show = specs_box;
                    box_to_hide = info_box;
                    show_mission_specs = true;
                    view_toggle_button->text = "Show standard info";
                }
                box_to_show->visible = true;
                box_to_show->responsive = true;
                box_to_hide->visible = false;
                box_to_hide->responsive = false;
                animate_info_and_specs();
            };
            view_toggle_button->on_get_tooltip =
            [] () {
                return "Toggles between basic area info and mission specs.";
            };
            gui.add_item(view_toggle_button, "view_toggle");
            
            //Specs box item.
            specs_box = new gui_item();
            specs_box->on_draw =
            [] (const point & center, const point & size) {
                draw_textured_box(
                    center, size, game.sys_content.bmp_frame_box,
                    COLOR_TRANSPARENT_WHITE
                );
            };
            gui.add_item(specs_box, "specs_box");
            
        }
        
    } else {
    
        //No areas found text.
        text_gui_item* no_areas_text =
            new text_gui_item(
            "No areas found! Try making your own in the area editor!",
            game.sys_content.fnt_standard
        );
        gui.add_item(no_areas_text, "no_areas_text");
        
    }
    
    //Tooltip text.
    tooltip_gui_item* tooltip_text =
        new tooltip_gui_item(&gui);
    gui.add_item(tooltip_text, "tooltip");
}


/**
 * @brief Initializes the mission specs page GUI items.
 */
void area_menu_state::init_gui_specs_page() {
    gui.register_coords("specs_name",     50,  5, 96,  6);
    gui.register_coords("goal_header",    50, 13, 96,  6);
    gui.register_coords("goal",           50, 21, 96,  6);
    gui.register_coords("fail_header",    50, 29, 96,  6);
    gui.register_coords("fail_list",      47, 48, 90, 28);
    gui.register_coords("fail_scroll",    96, 48,  4, 28);
    gui.register_coords("grading_header", 50, 67, 96,  6);
    gui.register_coords("grading_list",   47, 85, 90, 26);
    gui.register_coords("grading_scroll", 96, 85,  4, 26);
    gui.read_coords(
        game.content.gui_defs.list[AREA_MENU::SPECS_GUI_FILE_NAME].get_child_by_name("positions")
    );
    
    if(!game.content.areas.list[area_type].empty()) {
    
        //Name text.
        specs_name_text =
            new text_gui_item("", game.sys_content.fnt_area_name, COLOR_GOLD);
        specs_box->add_child(specs_name_text);
        gui.add_item(specs_name_text, "specs_name");
        
        //Goal header text.
        text_gui_item* goal_header_text =
            new text_gui_item("Goal", game.sys_content.fnt_area_name);
        specs_box->add_child(goal_header_text);
        gui.add_item(goal_header_text, "goal_header");
        
        //Goal explanation text.
        goal_text =
            new text_gui_item("", game.sys_content.fnt_standard);
        specs_box->add_child(goal_text);
        gui.add_item(goal_text, "goal");
        
        //Fail conditions header text.
        text_gui_item* fail_header_text =
            new text_gui_item("Fail conditions", game.sys_content.fnt_area_name);
        specs_box->add_child(fail_header_text);
        gui.add_item(fail_header_text, "fail_header");
        
        //Fail condition explanation list.
        fail_list = new list_gui_item();
        specs_box->add_child(fail_list);
        gui.add_item(fail_list, "fail_list");
        
        //Fail condition explanation scrollbar.
        scroll_gui_item* fail_scroll = new scroll_gui_item();
        fail_scroll->list_item = fail_list;
        specs_box->add_child(fail_scroll);
        gui.add_item(fail_scroll, "fail_scroll");
        
        //Grading header text.
        text_gui_item* grading_header_text =
            new text_gui_item("Grading", game.sys_content.fnt_area_name);
        specs_box->add_child(grading_header_text);
        gui.add_item(grading_header_text, "grading_header");
        
        //Grading explanation list.
        grading_list = new list_gui_item();
        specs_box->add_child(grading_list);
        gui.add_item(grading_list, "grading_list");
        
        //Grading explanation scrollbar.
        scroll_gui_item* grading_scroll = new scroll_gui_item();
        grading_scroll->list_item = grading_list;
        specs_box->add_child(grading_scroll);
        gui.add_item(grading_scroll, "grading_scroll");
    }
}


/**
 * @brief Leaves the area menu and goes into the main menu.
 */
void area_menu_state::leave() {
    game.fade_mgr.start_fade(false, [] () {
        game.states.main_menu->page_to_load = MAIN_MENU_PAGE_PLAY;
        game.change_state(game.states.main_menu);
    });
}


/**
 * @brief Loads the area menu into memory.
 */
void area_menu_state::load() {
    bmp_menu_bg = nullptr;
    first_area_button = nullptr;
    cur_area_idx = INVALID;
    cur_thumb = nullptr;
    cur_stamp = nullptr;
    cur_medal = nullptr;
    show_mission_specs = false;
    
    //Game content.
    game.content.reload_packs();
    game.content.load_all(
    vector<CONTENT_TYPE> {
        CONTENT_TYPE_AREA,
        CONTENT_TYPE_GUI,
    },
    CONTENT_LOAD_LEVEL_BASIC
    );
    
    //Mission records.
    if(area_type == AREA_TYPE_MISSION) {
        data_node mission_records;
        mission_records.load_file(FILE_PATHS_FROM_ROOT::MISSION_RECORDS, true, false, true);
        
        for(size_t a = 0; a < game.content.areas.list[AREA_TYPE_MISSION].size(); a++) {
            area_data* area_ptr = game.content.areas.list[AREA_TYPE_MISSION][a];
            mission_record record;
            
            load_area_mission_record(&mission_records, area_ptr, record);
            
            area_records.push_back(record);
        }
    }
    
    bmp_menu_bg = game.content.bitmaps.list.get(game.sys_content_names.bmp_main_menu);
    
    init_gui_main();
    init_gui_info_page();
    if(area_type == AREA_TYPE_MISSION && !game.content.areas.list[AREA_TYPE_MISSION].empty()) {
        init_gui_specs_page();
        specs_box->visible = false;
        specs_box->responsive = false;
    }
    if(first_area_button) {
        gui.set_selected_item(first_area_button, true);
    }
    
    //Finishing touches.
    game.audio.set_current_song(game.sys_content_names.sng_menus);
    game.fade_mgr.start_fade(true, nullptr);
    
}


/**
 * @brief Unloads the area menu from memory.
 */
void area_menu_state::unload() {

    //Resources.
    game.content.bitmaps.list.free(bmp_menu_bg);
    bmp_menu_bg = nullptr;
    
    //Menu items.
    gui.destroy();
    
    //Misc
    area_buttons.clear();
    area_records.clear();
    
    cur_thumb = nullptr;
    cur_stamp = nullptr;
    cur_medal = nullptr;
    
    game.content.unload_all(
    vector<CONTENT_TYPE> {
        CONTENT_TYPE_GUI,
        CONTENT_TYPE_AREA,
    }
    );
    
}
