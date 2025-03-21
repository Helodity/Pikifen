/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Game options class and related functions.
 */

#include <algorithm>

#include "options.h"

#include "../util/allegro_utils.h"
#include "../util/string_utils.h"
#include "game.h"
#include "misc_functions.h"
#include "misc_structs.h"


namespace OPTIONS {

//Default value for the ambiance sound volume.
const float DEF_AMBIANCE_SOUND_VOLUME = 1.0f;

//Default value for the animation editor background texture.
const char* DEF_ANIM_EDITOR_BG_TEXTURE = "";

//Default value for the area editor advanced mode setting.
const bool DEF_AREA_EDITOR_ADVANCED_MODE = false;

//Default value for the area editor backup interval.
const float DEF_AREA_EDITOR_BACKUP_INTERVAL = 120.0f;

//Default value for the area editor grid interval.
const float DEF_AREA_EDITOR_GRID_INTERVAL = 32.0f;

//Default value for the area editor selection transformation widget.
const bool DEF_AREA_EDITOR_SEL_TRANS = false;

//Default value for whether to show a circular sector's info in the area editor.
const bool DEF_AREA_EDITOR_SHOW_CIRCULAR_INFO = true;

//Default value for whether to show an edge's length in the area editor.
const bool DEF_AREA_EDITOR_SHOW_EDGE_LENGTH = true;

//Default value for whether to show a path link's length in the area editor.
const bool DEF_AREA_EDITOR_SHOW_PATH_LINK_LENGTH = true;

//Default value for whether to show a mob's territory in the area editor.
const bool DEF_AREA_EDITOR_SHOW_TERRITORY = false;

//Default value for the area editor snap mode.
const AreaEditor::SNAP_MODE DEF_AREA_EDITOR_SNAP_MODE =
    AreaEditor::SNAP_MODE_GRID;
    
//Default value for the area editor snap threshold.
const size_t DEF_AREA_EDITOR_SNAP_THRESHOLD = 80;

//Default value for the area editor undo limit.
const size_t DEF_AREA_EDITOR_UNDO_LIMIT = 20;

//Default value for the area editor view mode.
const AreaEditor::VIEW_MODE DEF_AREA_EDITOR_VIEW_MODE =
    AreaEditor::VIEW_MODE_TEXTURES;
    
//Default value for the auto-throw mode.
const AUTO_THROW_MODE DEF_AUTO_THROW_MODE = AUTO_THROW_MODE_OFF;

//Default value for the cursor camera weight.
const float DEF_CURSOR_CAM_WEIGHT = 0.0f;

//Default value for the cursor speed.
const float DEF_CURSOR_SPEED = 500.0f;

//Default value for the cursor trail.
const bool DEF_DRAW_CURSOR_TRAIL = true;

//Default value for the editor highlights.
const ALLEGRO_COLOR DEF_EDITOR_HIGHLIGHT_COLOR = { 1.0f, 1.0f, 1.0f, 1.0f };

//Default value for whether the middle mouse button pans in editors.
const bool DEF_EDITOR_MMB_PAN = false;

//Default value for the editor mouse drag threshold.
const float DEF_EDITOR_MOUSE_DRAG_THRESHOLD = 4;

//Default value for the editor primary color.
const ALLEGRO_COLOR DEF_EDITOR_PRIMARY_COLOR = {0.05f, 0.05f, 0.05f, 1.0f};

//Default value for the editor secondary color.
const ALLEGRO_COLOR DEF_EDITOR_SECONDARY_COLOR = {0.19f, 0.47f, 0.78f, 1.0f};

//Default value for whether to show tooltips in editors.
const bool DEF_EDITOR_SHOW_TOOLTIPS = true;

//Default value for the editor text color.
const ALLEGRO_COLOR DEF_EDITOR_TEXT_COLOR = {1.0f, 1.0f, 1.0f, 1.0f};

//Default value for whether to use custom styles in editors.
const bool DEF_EDITOR_USE_CUSTOM_STYLE = false;

//Default value for whether the player is an engine developer.
const bool DEF_ENGINE_DEVELOPER = false;

//Default value for gameplay sound effects volume.
const float DEF_GAMEPLAY_SOUND_VOLUME = 1.0f;

//Default value for the GUI editor grid interval.
const float DEF_GUI_EDITOR_GRID_INTERVAL = 2.5f;

//Default value for the area editor grid interval.
const float DEF_PARTICLE_EDITOR_GRID_INTERVAL = 32.0f;

//Default value for the GUI editor snap mode.
const bool DEF_GUI_EDITOR_SNAP = true;

//Default value for the joystick maximum deadzone.
const float DEF_JOYSTICK_MAX_DEADZONE = 0.9f;

//Default value for the joystick minimum deadzone.
const float DEF_JOYSTICK_MIN_DEADZONE = 0.2f;

//Default value for the pause menu leaving confirmation mode.
const LEAVING_CONFIRMATION_MODE DEF_LEAVING_CONFIRMATION_MODE =
    LEAVING_CONFIRMATION_MODE_ALWAYS;
    
//Default value for the master sound volume.
const float DEF_MASTER_VOLUME = 0.8f;

//Default value for the maximum amount of particles.
const size_t DEF_MAX_PARTICLES = 1000;

//Default value for whether mipmaps are enabled.
const bool DEF_MIPMAPS_ENABLED = true;

//Default value for whether the mouse moves the cursor, for each player.
const bool DEF_MOUSE_MOVES_CURSOR[MAX_PLAYERS] = {true, false, false, false};

//Default value for the music volume.
const float DEF_MUSIC_VOLUME = 1.0f;

//Default value for the particle editor background texture.
const char* DEF_PARTICLE_EDITOR_BG_TEXTURE = "";

//Default value for whether to show player input icons on the HUD.
const bool DEF_SHOW_HUD_INPUT_ICONS = true;

//Default value for whether to use smooth scaling.
const bool DEF_SMOOTH_SCALING = true;

//Default value for the default target framerate.
const unsigned int DEF_TARGET_FPS = 60;

//Default value for whether to use true fullscreen.
const bool DEF_TRUE_FULLSCREEN = false;

//Default value for UI sound effects volume.
const float DEF_UI_SOUND_VOLUME = 1.0f;

//Default value for whether to use the window position hack.
const bool DEF_WINDOW_POSITION_HACK = false;

//Default value for whether to use fullscreen.
const bool DEF_WIN_FULLSCREEN = false;

//Default value for the window height.
const unsigned int DEF_WIN_H = 768;

//Default value for the window width.
const unsigned int DEF_WIN_W = 1024;

//Default value for the middle zoom level.
const float DEF_ZOOM_MID_LEVEL = 1.4f;

}


/**
 * @brief Loads the player's options from a file.
 *
 * @param file File to read from.
 */
void Options::load(DataNode* file) {
    ReaderSetter rs(file);
    
    /* Load control binds. Format of a bind:
     * "p<player>_<action>=<possible control 1>;<possible control 2>;<...>"
     * Format of a possible control:
     * "<input type>_<parameters, underscore separated>"
     * Input types:
     * "k" (keyboard key), "mb" (mouse button),
     * "mwu" (mouse wheel up), "mwd" (down),
     * "mwl" (left), "mwr" (right), "jb" (joystick button),
     * "jap" (joystick axis, positive), "jan" (joystick axis, negative).
     * The parameters are the key/button number, controller number,
     * controller stick and axis, etc.
     * Check the constructor of control_info for more information.
     */
    game.controls.binds().clear();
    const vector<PlayerActionType> &player_action_types =
        game.controls.get_all_player_action_types();
    for(unsigned char p = 0; p < MAX_PLAYERS; p++) {
        for(size_t a = 0; a < player_action_types.size(); a++) {
            string internal_name = player_action_types[a].internal_name;
            if(internal_name.empty()) continue;
            DataNode* control_node =
                file->getChildByName("p" + i2s(p + 1) + "_" + internal_name);
            vector<string> possible_controls =
                semicolon_list_to_vector(control_node->value);
                
            for(size_t c = 0; c < possible_controls.size(); c++) {
                PlayerInput input =
                    game.controls.str_to_input(possible_controls[c]);
                if(input.type == INPUT_TYPE_NONE) continue;
                ControlBind new_bind;
                new_bind.actionTypeId = player_action_types[a].id;
                new_bind.playerNr = p;
                new_bind.input = input;
                game.controls.binds().push_back(new_bind);
            }
        }
    }
    
    for(unsigned char p = 0; p < MAX_PLAYERS; p++) {
        rs.set(
            "p" + i2s((p + 1)) + "_mouse_moves_cursor",
            mouse_moves_cursor[p]
        );
    }
    
    //Opened tree nodes in editors.
    editor_open_nodes.clear();
    vector<string> open_nodes_vector =
        semicolon_list_to_vector(
            file->getChildByName("editor_open_nodes")->value
        );
    for(size_t n = 0; n < open_nodes_vector.size(); n++) {
        editor_open_nodes[open_nodes_vector[n]] = true;
    }
    
    //Other options.
    string resolution_str;
    unsigned char editor_snap_mode_c;
    unsigned char editor_view_mode_c;
    unsigned char auto_throw_mode_c;
    unsigned char leaving_confirmation_mode_c;
    string pack_load_order_str;
    string packs_disabled_str;
    
    rs.set("ambiance_sound_volume", ambiance_sound_volume);
    rs.set("anim_editor_bg_path", anim_editor_bg_path);
    rs.set("area_editor_advanced_mode", area_editor_advanced_mode);
    rs.set("area_editor_backup_interval", area_editor_backup_interval);
    rs.set("area_editor_grid_interval", area_editor_grid_interval);
    rs.set("area_editor_selection_transformation", area_editor_sel_trans);
    rs.set("area_editor_show_circular_info", area_editor_show_circular_info);
    rs.set("area_editor_show_edge_length", area_editor_show_edge_length);
    rs.set(
        "area_editor_show_path_link_length",
        area_editor_show_path_link_length
    );
    rs.set("area_editor_show_territory", area_editor_show_territory);
    rs.set("area_editor_snap_mode", editor_snap_mode_c);
    rs.set("area_editor_snap_threshold", area_editor_snap_threshold);
    rs.set("area_editor_undo_limit", area_editor_undo_limit);
    rs.set("area_editor_view_mode", editor_view_mode_c);
    rs.set("auto_throw_mode", auto_throw_mode_c);
    rs.set("cursor_cam_weight", cursor_cam_weight);
    rs.set("cursor_speed", cursor_speed);
    rs.set("draw_cursor_trail", draw_cursor_trail);
    rs.set("editor_highlight_color", editor_highlight_color);
    rs.set("editor_mmb_pan", editor_mmb_pan);
    rs.set("editor_mouse_drag_threshold", editor_mouse_drag_threshold);
    rs.set("editor_primary_color", editor_primary_color);
    rs.set("editor_secondary_color", editor_secondary_color);
    rs.set("editor_show_tooltips", editor_show_tooltips);
    rs.set("editor_text_color", editor_text_color);
    rs.set("editor_use_custom_style", editor_use_custom_style);
    rs.set("engine_developer", engine_developer);
    rs.set("gameplay_sound_volume", gameplay_sound_volume);
    rs.set("fps", target_fps);
    rs.set("fullscreen", intended_win_fullscreen);
    rs.set("gui_editor_grid_interval", gui_editor_grid_interval);
    rs.set("gui_editor_snap", gui_editor_snap);
    rs.set("joystick_min_deadzone", joystick_min_deadzone);
    rs.set("joystick_max_deadzone", joystick_max_deadzone);
    rs.set("leaving_confirmation_mode", leaving_confirmation_mode_c);
    rs.set("master_volume", master_volume);
    rs.set("max_particles", max_particles);
    rs.set("middle_zoom_level", zoom_mid_level);
    rs.set("mipmaps", mipmaps_enabled);
    rs.set("music_volume", music_volume);
    rs.set("pack_order", pack_load_order_str);
    rs.set("packs_disabled", packs_disabled_str);
    rs.set("particle_editor_bg_path", particle_editor_bg_path);
    rs.set("particle_editor_grid_interval", particle_editor_grid_interval);
    rs.set("resolution", resolution_str);
    rs.set("smooth_scaling", smooth_scaling);
    rs.set("show_hud_input_icons", show_hud_input_icons);
    rs.set("true_fullscreen", true_fullscreen);
    rs.set("ui_sound_volume", ui_sound_volume);
    rs.set("window_position_hack", window_position_hack);
    
    auto_throw_mode =
        (AUTO_THROW_MODE)
        std::min(
            auto_throw_mode_c,
            (unsigned char) (N_AUTO_THROW_MODES - 1)
        );
    area_editor_snap_mode =
        (AreaEditor::SNAP_MODE)
        std::min(
            editor_snap_mode_c,
            (unsigned char) (AreaEditor::N_SNAP_MODES - 1)
        );
    area_editor_view_mode =
        (AreaEditor::VIEW_MODE)
        std::min(
            editor_view_mode_c,
            (unsigned char) (AreaEditor::N_VIEW_MODES - 1)
        );
    leaving_confirmation_mode =
        (LEAVING_CONFIRMATION_MODE)
        std::min(
            leaving_confirmation_mode_c,
            (unsigned char) (N_LEAVING_CONFIRMATION_MODES - 1)
        );
    target_fps = std::max(1, target_fps);
    
    if(joystick_min_deadzone > joystick_max_deadzone) {
        std::swap(joystick_min_deadzone, joystick_max_deadzone);
    }
    if(joystick_min_deadzone == joystick_max_deadzone) {
        joystick_min_deadzone -= 0.1;
        joystick_max_deadzone += 0.1;
    }
    joystick_min_deadzone = clamp(joystick_min_deadzone, 0.0f, 1.0f);
    joystick_max_deadzone = clamp(joystick_max_deadzone, 0.0f, 1.0f);
    
    vector<string> resolution_parts = split(resolution_str);
    if(resolution_parts.size() >= 2) {
        intended_win_w = std::max(1, s2i(resolution_parts[0]));
        intended_win_h = std::max(1, s2i(resolution_parts[1]));
    }
    
    ambiance_sound_volume = clamp(ambiance_sound_volume, 0.0f, 1.0f);
    gameplay_sound_volume = clamp(gameplay_sound_volume, 0.0f, 1.0f);
    master_volume = clamp(master_volume, 0.0f, 1.0f);
    music_volume = clamp(music_volume, 0.0f, 1.0f);
    ui_sound_volume = clamp(ui_sound_volume, 0.0f, 1.0f);
    
    //Force the editor styles to be opaque, otherwise there can be problems.
    editor_primary_color.a = 1.0f;
    editor_secondary_color.a = 1.0f;
    editor_text_color.a = 1.0f;
    editor_highlight_color.a = 1.0f;
    
    pack_order = semicolon_list_to_vector(pack_load_order_str);
    packs_disabled = semicolon_list_to_vector(packs_disabled_str);
}


/**
 * @brief Saves the player's options into a file.
 *
 * @param file File to write to.
 */
void Options::save(DataNode* file) const {
    //First, group the controls by action and player.
    map<string, string> grouped_controls;
    
    const vector<PlayerActionType> &player_action_types =
        game.controls.get_all_player_action_types();
    for(unsigned char p = 0; p < MAX_PLAYERS; p++) {
        string prefix = "p" + i2s((p + 1)) + "_";
        for(size_t b = 0; b < player_action_types.size(); b++) {
            string internal_name = player_action_types[b].internal_name;
            if(internal_name.empty()) continue;
            grouped_controls[prefix + internal_name].clear();
        }
    }
    
    //Write down their input strings.
    const vector<ControlBind> &all_binds = game.controls.binds();
    for(unsigned char p = 0; p < MAX_PLAYERS; p++) {
        for(size_t b = 0; b < all_binds.size(); b++) {
            if(all_binds[b].playerNr != p) continue;
            string name = "p" + i2s(p + 1) + "_";
            
            for(size_t a = 0; a < player_action_types.size(); a++) {
                if(player_action_types[a].internal_name.empty()) continue;
                
                if(all_binds[b].actionTypeId == player_action_types[a].id) {
                    name += player_action_types[a].internal_name;
                    break;
                }
            }
            
            grouped_controls[name] +=
                game.controls.input_to_str(all_binds[b].input) + ";";
        }
    }
    
    //Save controls.
    for(auto &c : grouped_controls) {
        //Remove the final character, which is always an extra semicolon.
        if(c.second.size()) c.second.erase(c.second.size() - 1);
        
        file->add(new DataNode(c.first, c.second));
    }
    
    for(unsigned char p = 0; p < MAX_PLAYERS; p++) {
        file->add(
            new DataNode(
                "p" + i2s((p + 1)) + "_mouse_moves_cursor",
                b2s(mouse_moves_cursor[p])
            )
        );
    }
    
    //Figure out the value for the editor tree node preferences.
    vector<string> open_nodes_strs;
    for(auto &n : editor_open_nodes) {
        if(n.second) open_nodes_strs.push_back(n.first);
    }
    string open_nodes_str = join(open_nodes_strs, ";");
    
    //Other options.
    string pack_load_order_str = join(pack_order, ";");
    string packs_disabled_str = join(packs_disabled, ";");
    
    file->add(
        new DataNode(
            "ambiance_sound_volume",
            f2s(ambiance_sound_volume)
        )
    );
    file->add(
        new DataNode(
            "anim_editor_bg_path",
            anim_editor_bg_path
        )
    );
    file->add(
        new DataNode(
            "area_editor_advanced_mode",
            b2s(area_editor_advanced_mode)
        )
    );
    file->add(
        new DataNode(
            "area_editor_backup_interval",
            f2s(area_editor_backup_interval)
        )
    );
    file->add(
        new DataNode(
            "area_editor_grid_interval",
            i2s(area_editor_grid_interval)
        )
    );
    file->add(
        new DataNode(
            "area_editor_selection_transformation",
            b2s(area_editor_sel_trans)
        )
    );
    file->add(
        new DataNode(
            "area_editor_show_circular_info",
            b2s(area_editor_show_circular_info)
        )
    );
    file->add(
        new DataNode(
            "area_editor_show_edge_length",
            b2s(area_editor_show_edge_length)
        )
    );
    file->add(
        new DataNode(
            "area_editor_show_path_link_length",
            b2s(area_editor_show_path_link_length)
        )
    );
    file->add(
        new DataNode(
            "area_editor_show_territory",
            b2s(area_editor_show_territory)
        )
    );
    file->add(
        new DataNode(
            "area_editor_snap_mode",
            i2s(area_editor_snap_mode)
        )
    );
    file->add(
        new DataNode(
            "area_editor_snap_threshold",
            i2s(area_editor_snap_threshold)
        )
    );
    file->add(
        new DataNode(
            "area_editor_undo_limit",
            i2s(area_editor_undo_limit)
        )
    );
    file->add(
        new DataNode(
            "area_editor_view_mode",
            i2s(area_editor_view_mode)
        )
    );
    file->add(
        new DataNode(
            "auto_throw_mode",
            i2s(auto_throw_mode)
        )
    );
    file->add(
        new DataNode(
            "cursor_cam_weight",
            f2s(cursor_cam_weight)
        )
    );
    file->add(
        new DataNode(
            "cursor_speed",
            f2s(cursor_speed)
        )
    );
    file->add(
        new DataNode(
            "draw_cursor_trail",
            b2s(draw_cursor_trail)
        )
    );
    file->add(
        new DataNode(
            "editor_highlight_color",
            c2s(editor_highlight_color)
        )
    );
    file->add(
        new DataNode(
            "editor_mmb_pan",
            b2s(editor_mmb_pan)
        )
    );
    file->add(
        new DataNode(
            "editor_mouse_drag_threshold",
            i2s(editor_mouse_drag_threshold)
        )
    );
    file->add(
        new DataNode(
            "editor_open_nodes",
            open_nodes_str
        )
    );
    file->add(
        new DataNode(
            "editor_primary_color",
            c2s(editor_primary_color)
        )
    );
    file->add(
        new DataNode(
            "editor_secondary_color",
            c2s(editor_secondary_color)
        )
    );
    file->add(
        new DataNode(
            "editor_show_tooltips",
            b2s(editor_show_tooltips)
        )
    );
    file->add(
        new DataNode(
            "editor_text_color",
            c2s(editor_text_color)
        )
    );
    file->add(
        new DataNode(
            "editor_use_custom_style",
            b2s(editor_use_custom_style)
        )
    );
    file->add(
        new DataNode(
            "engine_developer",
            b2s(engine_developer)
        )
    );
    file->add(
        new DataNode(
            "fps",
            i2s(target_fps)
        )
    );
    file->add(
        new DataNode(
            "fullscreen",
            b2s(intended_win_fullscreen)
        )
    );
    file->add(
        new DataNode(
            "gameplay_sound_volume",
            f2s(gameplay_sound_volume)
        )
    );
    file->add(
        new DataNode(
            "gui_editor_grid_interval",
            f2s(gui_editor_grid_interval)
        )
    );
    file->add(
        new DataNode(
            "gui_editor_snap",
            b2s(gui_editor_snap)
        )
    );
    file->add(
        new DataNode(
            "joystick_max_deadzone",
            f2s(joystick_max_deadzone)
        )
    );
    file->add(
        new DataNode(
            "joystick_min_deadzone",
            f2s(joystick_min_deadzone)
        )
    );
    file->add(
        new DataNode(
            "leaving_confirmation_mode",
            i2s(leaving_confirmation_mode)
        )
    );
    file->add(
        new DataNode(
            "master_volume",
            f2s(master_volume)
        )
    );
    file->add(
        new DataNode(
            "max_particles",
            i2s(max_particles)
        )
    );
    file->add(
        new DataNode(
            "middle_zoom_level",
            f2s(zoom_mid_level)
        )
    );
    file->add(
        new DataNode(
            "mipmaps",
            b2s(mipmaps_enabled)
        )
    );
    file->add(
        new DataNode(
            "music_volume",
            f2s(music_volume)
        )
    );
    file->add(
        new DataNode(
            "pack_order",
            pack_load_order_str
        )
    );
    file->add(
        new DataNode(
            "packs_disabled",
            packs_disabled_str
        )
    );
    file->add(
        new DataNode(
            "particle_editor_grid_interval",
            i2s(particle_editor_grid_interval)
        )
    );
    file->add(
        new DataNode(
            "particle_editor_bg_path",
            particle_editor_bg_path
        )
    );
    file->add(
        new DataNode(
            "resolution",
            i2s(intended_win_w) + " " +
            i2s(intended_win_h)
        )
    );
    file->add(
        new DataNode(
            "smooth_scaling",
            b2s(smooth_scaling)
        )
    );
    file->add(
        new DataNode(
            "show_hud_input_icons",
            b2s(show_hud_input_icons)
        )
    );
    file->add(
        new DataNode(
            "true_fullscreen",
            b2s(true_fullscreen)
        )
    );
    file->add(
        new DataNode(
            "ui_sound_volume",
            f2s(ui_sound_volume)
        )
    );
    file->add(
        new DataNode(
            "window_position_hack",
            b2s(window_position_hack)
        )
    );
}
