/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Options menu state class and options menu state-related functions.
 */

#include <algorithm>

#include "menus.h"

#include "../drawing.h"
#include "../functions.h"
#include "../game.h"
#include "../load.h"
#include "../options.h"
#include "../utils/string_utils.h"


namespace OPTIONS_MENU {
//Auto-throw preset names.
const string AUTO_THROW_PRESET_NAMES[] =
{"Off", "Hold button", "Button toggles"};
//Auto-throw preset values.
const AUTO_THROW_MODES AUTO_THROW_PRESETS[] =
{AUTO_THROW_OFF, AUTO_THROW_HOLD, AUTO_THROW_TOGGLE};
//Cursor speed preset names.
const string CURSOR_SPEED_PRESET_NAMES[] =
{"Very slow", "Slow", "Medium", "Fast", "Very fast"};
//Cursor speed preset values.
const float CURSOR_SPEED_PRESETS[] =
{250.0f, 350.0f, 500.0f, 700.0f, 1000.0f};
//Path to the GUI information file.
const string GUI_FILE_PATH = GUI_FOLDER_PATH + "/Options_menu.txt";
//Leaving conformation preset names.
const string LEAVING_CONFIRMATION_PRESET_NAMES[] =
{"Always", "After 1min", "Never"};
//Leaving conformation preset values.
const LEAVING_CONFIRMATION_MODES LEAVING_CONFIRMATION_PRESETS[] = {
    LEAVING_CONFIRMATION_ALWAYS,
    LEAVING_CONFIRMATION_1_MIN,
    LEAVING_CONFIRMATION_NEVER
};
//Auto-throw preset amount.
const unsigned char N_AUTO_THROW_PRESETS = 3;
//Cursor speed preset amount.
const unsigned char N_CURSOR_SPEED_PRESETS = 5;
//Leaving confirmation preset amount.
const unsigned char N_LEAVING_CONFIRMATION_PRESETS = 3;
}


/* ----------------------------------------------------------------------------
 * Creates an "options menu" state.
 */
options_menu_state::options_menu_state() :
    game_state(),
    bmp_menu_bg(nullptr),
    auto_throw_picker(nullptr),
    resolution_picker(nullptr),
    cursor_speed_picker(nullptr),
    warning_text(nullptr) {
    
    //Let's fill in the list of preset resolutions. For that, we'll get
    //the display modes fetched by Allegro. These are usually nice round
    //resolutions, and they work on fullscreen mode.
    int n_modes = al_get_num_display_modes();
    for(int d = 0; d < n_modes; ++d) {
        ALLEGRO_DISPLAY_MODE d_info;
        if(!al_get_display_mode(d, &d_info)) continue;
        if(d_info.width < SMALLEST_WIN_WIDTH) continue;
        if(d_info.height < SMALLEST_WIN_HEIGHT) continue;
        resolution_presets.push_back(
            std::make_pair(d_info.width, d_info.height)
        );
    }
    
    //In case things go wrong, at least add these presets.
    resolution_presets.push_back(
        std::make_pair(OPTIONS::DEF_WIN_W, OPTIONS::DEF_WIN_H)
    );
    resolution_presets.push_back(
        std::make_pair(SMALLEST_WIN_WIDTH, SMALLEST_WIN_HEIGHT)
    );
    
    //Sort the list.
    sort(
        resolution_presets.begin(), resolution_presets.end(),
    [] (std::pair<int, int> p1, std::pair<int, int> p2) -> bool {
        if(p1.first == p2.first) {
            return p1.second < p2.second;
        }
        return p1.first < p2.first;
    }
    );
    
    //Remove any duplicates.
    for(size_t p = 0; p < resolution_presets.size() - 1;) {
        if(resolution_presets[p] == resolution_presets[p + 1]) {
            resolution_presets.erase(resolution_presets.begin() + (p + 1));
        } else {
            ++p;
        }
    }
}


/* ----------------------------------------------------------------------------
 * Changes to the next auto-throw mode preset in the list.
 * step:
 *   How much to move forward in the list.
 */
void options_menu_state::change_auto_throw(const signed int step) {
    size_t cur_auto_throw_idx = get_auto_throw_idx();
    
    if(cur_auto_throw_idx == INVALID) {
        cur_auto_throw_idx = 0;
    } else {
        cur_auto_throw_idx =
            sum_and_wrap(
                (int) cur_auto_throw_idx, step,
                OPTIONS_MENU::N_AUTO_THROW_PRESETS
            );
    }
    
    game.options.auto_throw_mode =
        OPTIONS_MENU::AUTO_THROW_PRESETS[cur_auto_throw_idx];
        
    auto_throw_picker->cur_option_idx = cur_auto_throw_idx;
    auto_throw_picker->start_juice_animation(
        gui_item::JUICE_TYPE_GROW_TEXT_ELASTIC_MEDIUM
    );
    update();
}


/* ----------------------------------------------------------------------------
 * Changes to the next cursor speed preset in the list.
 * step:
 *   How much to move forward in the list.
 */
void options_menu_state::change_cursor_speed(const signed int step) {
    size_t cur_cursor_speed_idx = get_cursor_speed_idx();
    
    if(cur_cursor_speed_idx == INVALID) {
        cur_cursor_speed_idx = 0;
    } else {
        cur_cursor_speed_idx =
            sum_and_wrap(
                (int) cur_cursor_speed_idx, step,
                OPTIONS_MENU::N_CURSOR_SPEED_PRESETS
            );
    }
    
    game.options.cursor_speed =
        OPTIONS_MENU::CURSOR_SPEED_PRESETS[cur_cursor_speed_idx];
        
    cursor_speed_picker->cur_option_idx = cur_cursor_speed_idx;
    cursor_speed_picker->start_juice_animation(
        gui_item::JUICE_TYPE_GROW_TEXT_ELASTIC_MEDIUM
    );
    update();
}


/* ----------------------------------------------------------------------------
 * Changes to the next leaving confirmation mode preset in the list.
 * step:
 *   How much to move forward in the list.
 */
void options_menu_state::change_leaving_confirmation(const signed int step) {
    size_t cur_leaving_conf_idx = get_leaving_confirmation_idx();
    
    if(cur_leaving_conf_idx == INVALID) {
        cur_leaving_conf_idx = 0;
    } else {
        cur_leaving_conf_idx =
            sum_and_wrap(
                (int) cur_leaving_conf_idx, step,
                OPTIONS_MENU::N_LEAVING_CONFIRMATION_PRESETS
            );
    }
    
    game.options.leaving_confirmation_mode =
        OPTIONS_MENU::LEAVING_CONFIRMATION_PRESETS[cur_leaving_conf_idx];
        
    leaving_confirmation_picker->cur_option_idx = cur_leaving_conf_idx;
    leaving_confirmation_picker->start_juice_animation(
        gui_item::JUICE_TYPE_GROW_TEXT_ELASTIC_MEDIUM
    );
    update();
}


/* ----------------------------------------------------------------------------
 * Changes to the next resolution preset in the list.
 * step:
 *   How much to move forward in the list.
 */
void options_menu_state::change_resolution(const signed int step) {
    size_t cur_resolution_idx = get_resolution_idx();
    if(cur_resolution_idx == INVALID) {
        cur_resolution_idx = 0;
    } else {
        cur_resolution_idx =
            sum_and_wrap(
                (int) cur_resolution_idx, step,
                resolution_presets.size()
            );
    }
    
    game.options.intended_win_w = resolution_presets[cur_resolution_idx].first;
    game.options.intended_win_h = resolution_presets[cur_resolution_idx].second;
    
    trigger_restart_warning();
    resolution_picker->cur_option_idx = cur_resolution_idx;
    resolution_picker->start_juice_animation(
        gui_item::JUICE_TYPE_GROW_TEXT_ELASTIC_MEDIUM
    );
    update();
}


/* ----------------------------------------------------------------------------
 * Draws the options menu.
 */
void options_menu_state::do_drawing() {
    al_clear_to_color(COLOR_BLACK);
    
    draw_bitmap(
        bmp_menu_bg, point(game.win_w * 0.5, game.win_h * 0.5),
        point(game.win_w, game.win_h), 0, map_gray(64)
    );
    
    gui.draw();
    
    game.fade_mgr.draw();
    
    al_flip_display();
}


/* ----------------------------------------------------------------------------
 * Ticks one frame's worth of logic.
 */
void options_menu_state::do_logic() {
    game.fade_mgr.tick(game.delta_t);
    
    gui.tick(game.delta_t);
}


/* ----------------------------------------------------------------------------
 * Returns the current auto-throw option's index, or INVALID if not found.
 */
size_t options_menu_state::get_auto_throw_idx() const {
    for(size_t m = 0; m < OPTIONS_MENU::N_AUTO_THROW_PRESETS; ++m) {
        if(
            game.options.auto_throw_mode ==
            OPTIONS_MENU::AUTO_THROW_PRESETS[m]
        ) {
            return m;
        }
    }
    
    return INVALID;
}


/* ----------------------------------------------------------------------------
 * Returns the current cursor speed option's index, or INVALID if not found.
 */
size_t options_menu_state::get_cursor_speed_idx() const {
    for(size_t s = 0; s < OPTIONS_MENU::N_CURSOR_SPEED_PRESETS; ++s) {
        if(
            game.options.cursor_speed ==
            OPTIONS_MENU::CURSOR_SPEED_PRESETS[s]
        ) {
            return s;
        }
    }
    
    return INVALID;
}


/* ----------------------------------------------------------------------------
 * Returns the current leaving confirmation option's index,
 * or INVALID if not found.
 */
size_t options_menu_state::get_leaving_confirmation_idx() const {
    for(size_t m = 0; m < OPTIONS_MENU::N_LEAVING_CONFIRMATION_PRESETS; ++m) {
        if(
            game.options.leaving_confirmation_mode ==
            OPTIONS_MENU::LEAVING_CONFIRMATION_PRESETS[m]
        ) {
            return m;
        }
    }
    
    return INVALID;
}


/* ----------------------------------------------------------------------------
 * Returns the name of this state.
 */
string options_menu_state::get_name() const {
    return "options menu";
}


/* ----------------------------------------------------------------------------
 * Returns the current resolution option's index, or INVALID if custom.
 */
size_t options_menu_state::get_resolution_idx() const {
    for(size_t r = 0; r < resolution_presets.size(); ++r) {
        if(
            game.options.intended_win_w == resolution_presets[r].first &&
            game.options.intended_win_h == resolution_presets[r].second
        ) {
            return r;
        }
    }
    
    return INVALID;
}


/* ----------------------------------------------------------------------------
 * Goes to the controls menu.
 */
void options_menu_state::go_to_controls() {
    game.fade_mgr.start_fade(false, [] () {
        game.change_state(game.states.controls_menu);
    });
}


/* ----------------------------------------------------------------------------
 * Handles Allegro events.
 * ev:
 *   Event to handle.
 */
void options_menu_state::handle_allegro_event(ALLEGRO_EVENT &ev) {
    if(game.fade_mgr.is_fading()) return;
    
    gui.handle_event(ev);
}


/* ----------------------------------------------------------------------------
 * Leaves the options menu and goes to the main menu.
 */
void options_menu_state::leave() {
    game.fade_mgr.start_fade(false, [] () {
        game.change_state(game.states.main_menu);
    });
    save_options();
}


/* ----------------------------------------------------------------------------
 * Loads the options menu into memory.
 */
void options_menu_state::load() {
    //Resources.
    bmp_menu_bg = load_bmp(game.asset_file_names.main_menu);
    
    //Menu items.
    gui.register_coords("back",                 14,  8, 20, 8);
    gui.register_coords("fullscreen",           26, 20, 44, 8);
    gui.register_coords("resolution",           74, 20, 44, 8);
    gui.register_coords("cursor_speed",         50, 34, 44, 8);
    gui.register_coords("auto_throw",           50, 44, 44, 8);
    gui.register_coords("show_hud_controls",    50, 54, 44, 8);
    gui.register_coords("controls",             50, 74, 44, 8);
    gui.register_coords("leaving_confirmation", 50, 64, 44, 8);
    gui.register_coords("advanced",             86, 84, 24, 8);
    gui.register_coords("tooltip",              50, 95, 96, 7);
    gui.register_coords("restart_warning",      62,  5, 72, 6);
    gui.read_coords(
        data_node(OPTIONS_MENU::GUI_FILE_PATH).get_child_by_name("positions")
    );
    
    //Back button.
    gui.back_item =
        new button_gui_item("Back", game.fonts.standard);
    gui.back_item->on_activate =
    [this] (const point &) {
        leave();
    };
    gui.back_item->on_get_tooltip =
    [] () { return "Return to the main menu."; };
    gui.add_item(gui.back_item, "back");
    
    //Fullscreen checkbox.
    check_gui_item* fullscreen_check =
        new check_gui_item(
        &game.options.intended_win_fullscreen,
        "Fullscreen", game.fonts.standard
    );
    fullscreen_check->on_activate =
    [this, fullscreen_check] (const point &) {
        game.options.intended_win_fullscreen =
            !game.options.intended_win_fullscreen;
        fullscreen_check->start_juice_animation(
            gui_item::JUICE_TYPE_GROW_TEXT_ELASTIC_MEDIUM
        );
        trigger_restart_warning();
    };
    fullscreen_check->on_get_tooltip =
    [] () {
        return
            "Show the game in fullscreen, or in a window? Default: " +
            b2s(OPTIONS::DEF_WIN_FULLSCREEN) + ".";
    };
    gui.add_item(fullscreen_check, "fullscreen");
    
    //Resolution picker.
    resolution_picker =
        new picker_gui_item(
        "Resolution: ", "", resolution_presets.size(), get_resolution_idx()
    );
    resolution_picker->on_previous =
    [this] () {
        change_resolution(-1);
    };
    resolution_picker->on_next =
    [this] () {
        change_resolution(1);
    };
    resolution_picker->on_get_tooltip =
    [] () {
        return
            "The game's width and height. Default: " +
            i2s(OPTIONS::DEF_WIN_W) + "x" +
            i2s(OPTIONS::DEF_WIN_H) + ".";
    };
    gui.add_item(resolution_picker, "resolution");
    
    //Cursor speed.
    cursor_speed_picker =
        new picker_gui_item(
        "Cursor speed: ", "",
        OPTIONS_MENU::N_CURSOR_SPEED_PRESETS, get_cursor_speed_idx()
    );
    cursor_speed_picker->on_previous =
    [this] () {
        change_cursor_speed(-1);
    };
    cursor_speed_picker->on_next =
    [this] () {
        change_cursor_speed(1);
    };
    cursor_speed_picker->on_get_tooltip =
    [] () {
        size_t idx = 0;
        for(; idx < OPTIONS_MENU::N_CURSOR_SPEED_PRESETS; ++idx) {
            if(
                OPTIONS_MENU::CURSOR_SPEED_PRESETS[idx] ==
                OPTIONS::DEF_CURSOR_SPEED
            ) {
                break;
            }
        }
        
        return
            "Cursor speed, when controlling without a mouse. Default: " +
            OPTIONS_MENU::CURSOR_SPEED_PRESET_NAMES[idx] + ".";
    };
    gui.add_item(cursor_speed_picker, "cursor_speed");
    
    //Auto-throw mode.
    auto_throw_picker =
        new picker_gui_item(
        "Auto-throw: ", "",
        OPTIONS_MENU::N_AUTO_THROW_PRESETS, get_auto_throw_idx()
    );
    auto_throw_picker->on_previous =
    [this] () {
        change_auto_throw(-1);
    };
    auto_throw_picker->on_next =
    [this] () {
        change_auto_throw(1);
    };
    auto_throw_picker->on_get_tooltip =
    [] () -> string {
        size_t idx = 0;
        for(; idx < OPTIONS_MENU::N_AUTO_THROW_PRESETS; ++idx) {
            if(
                OPTIONS_MENU::AUTO_THROW_PRESETS[idx] ==
                OPTIONS::DEF_AUTO_THROW_MODE
            ) {
                break;
            }
        }
        
        string s;
        switch(game.options.auto_throw_mode) {
        case AUTO_THROW_OFF: {
            s = "Pikmin are only thrown when you release the button.";
            break;
        }
        case AUTO_THROW_HOLD: {
            s = "Auto-throw Pikmin periodically as long as "
                "the button is held.";
            break;
        }
        case AUTO_THROW_TOGGLE: {
            s = "Press once to auto-throw Pikmin periodically, and again "
                "to stop.";
            break;
        }
        default: {
            return "";
        }
        }
        return
            s + " Default: " +
            OPTIONS_MENU::AUTO_THROW_PRESET_NAMES[idx] + ".";
    };
    gui.add_item(auto_throw_picker, "auto_throw");
    
    //Show HUD controls checkbox.
    check_gui_item* show_hud_controls_check =
        new check_gui_item(
        &game.options.show_hud_controls,
        "Show controls on HUD", game.fonts.standard
    );
    show_hud_controls_check->on_get_tooltip =
    [] () {
        return
            "Show icons of the controls near relevant HUD items? Default: " +
            b2s(OPTIONS::DEF_SHOW_HUD_CONTROLS) + ".";
    };
    gui.add_item(show_hud_controls_check, "show_hud_controls");
    
    //Leaving confirmation mode.
    leaving_confirmation_picker =
        new picker_gui_item(
        "Leaving confirmation: ", "",
        OPTIONS_MENU::N_LEAVING_CONFIRMATION_PRESETS,
        get_leaving_confirmation_idx()
    );
    leaving_confirmation_picker->on_previous =
    [this] () {
        change_leaving_confirmation(-1);
    };
    leaving_confirmation_picker->on_next =
    [this] () {
        change_leaving_confirmation(1);
    };
    leaving_confirmation_picker->on_get_tooltip =
    [] () -> string {
        size_t idx = 0;
        for(; idx < OPTIONS_MENU::N_LEAVING_CONFIRMATION_PRESETS; ++idx) {
            if(
                OPTIONS_MENU::LEAVING_CONFIRMATION_PRESETS[idx] ==
                OPTIONS::DEF_LEAVING_CONFIRMATION_MODE
            ) {
                break;
            }
        }
        
        string s;
        switch(game.options.leaving_confirmation_mode) {
        case LEAVING_CONFIRMATION_NEVER: {
            s = "When leaving from the pause menu, never ask to confirm.";
            break;
        }
        case LEAVING_CONFIRMATION_1_MIN: {
            s = "When leaving from the pause menu, only ask to confirm "
                "if one minute has passed.";
            break;
        }
        case LEAVING_CONFIRMATION_ALWAYS: {
            s = "When leaving from the pause menu, always ask to confirm.";
            break;
        }
        default: {
            return "";
        }
        }
        return
            s + " Default: " +
            OPTIONS_MENU::LEAVING_CONFIRMATION_PRESET_NAMES[idx] + ".";
    };
    gui.add_item(leaving_confirmation_picker, "leaving_confirmation");
    
    //Controls button.
    button_gui_item* controls_button =
        new button_gui_item("Edit controls...", game.fonts.standard);
    controls_button->on_activate =
    [this] (const point &) {
        go_to_controls();
    };
    controls_button->on_get_tooltip =
    [] () { return "Choose what buttons do what."; };
    gui.add_item(controls_button, "controls");
    
    //Advanced bullet point.
    bullet_point_gui_item* advanced_bullet =
        new bullet_point_gui_item("Advanced...", game.fonts.standard);
    advanced_bullet->on_get_tooltip =
    [] () {
        return
            "For more advanced options, check out the "
            "manual in the game's folder.";
    };
    gui.add_item(advanced_bullet, "advanced");
    
    //Tooltip text.
    tooltip_gui_item* tooltip_text =
        new tooltip_gui_item(&gui);
    gui.add_item(tooltip_text, "tooltip");
    
    //Warning text.
    warning_text =
        new text_gui_item(
        "Please restart for the changes to take effect.",
        game.fonts.standard, COLOR_WHITE, ALLEGRO_ALIGN_RIGHT
    );
    warning_text->visible = false;
    gui.add_item(warning_text, "restart_warning");
    
    //Finishing touches.
    game.fade_mgr.start_fade(true, nullptr);
    gui.set_selected_item(gui.back_item);
    update();
    
}


/* ----------------------------------------------------------------------------
 * Triggers the restart warning at the bottom of the screen.
 */
void options_menu_state::trigger_restart_warning() {
    if(!warning_text->visible) {
        warning_text->visible = true;
        warning_text->start_juice_animation(
            gui_item::JUICE_TYPE_GROW_TEXT_ELASTIC_MEDIUM
        );
    }
}


/* ----------------------------------------------------------------------------
 * Unloads the options menu from memory.
 */
void options_menu_state::unload() {

    //Resources.
    al_destroy_bitmap(bmp_menu_bg);
    
    //Menu items.
    gui.destroy();
}


/* ----------------------------------------------------------------------------
 * Updates the contents of the options menu.
 */
void options_menu_state::update() {
    //Resolution.
    size_t cur_resolution_idx = INVALID;
    
    for(size_t r = 0; r < resolution_presets.size(); ++r) {
        if(
            game.options.intended_win_w == resolution_presets[r].first &&
            game.options.intended_win_h == resolution_presets[r].second
        ) {
            cur_resolution_idx = r;
            break;
        }
    }
    
    resolution_picker->option =
        i2s(game.options.intended_win_w) + "x" +
        i2s(game.options.intended_win_h) +
        (cur_resolution_idx == INVALID ? " (custom)" : "");
        
    //Cursor speed.
    size_t cur_cursor_speed_idx = INVALID;
    
    for(size_t s = 0; s < OPTIONS_MENU::N_CURSOR_SPEED_PRESETS; ++s) {
        if(
            game.options.cursor_speed ==
            OPTIONS_MENU::CURSOR_SPEED_PRESETS[s]
        ) {
            cur_cursor_speed_idx = s;
            break;
        }
    }
    
    cursor_speed_picker->option =
        cur_cursor_speed_idx == INVALID ?
        i2s(game.options.cursor_speed) + " (custom)" :
        OPTIONS_MENU::CURSOR_SPEED_PRESET_NAMES[cur_cursor_speed_idx];
        
    //Auto-throw.
    size_t cur_auto_throw_idx = INVALID;
    
    for(size_t m = 0; m < OPTIONS_MENU::N_AUTO_THROW_PRESETS; ++m) {
        if(
            game.options.auto_throw_mode ==
            OPTIONS_MENU::AUTO_THROW_PRESETS[m]
        ) {
            cur_auto_throw_idx = m;
            break;
        }
    }
    
    auto_throw_picker->option =
        OPTIONS_MENU::AUTO_THROW_PRESET_NAMES[cur_auto_throw_idx];
        
    //Leaving confirmation.
    size_t cur_leaving_conf_idx = INVALID;
    
    for(size_t m = 0; m < OPTIONS_MENU::N_LEAVING_CONFIRMATION_PRESETS; ++m) {
        if(
            game.options.leaving_confirmation_mode ==
            OPTIONS_MENU::LEAVING_CONFIRMATION_PRESETS[m]
        ) {
            cur_leaving_conf_idx = m;
            break;
        }
    }
    
    leaving_confirmation_picker->option =
        OPTIONS_MENU::LEAVING_CONFIRMATION_PRESET_NAMES[
         cur_leaving_conf_idx
     ];
}
