/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Data loading and unloading functions.
 */

#include <algorithm>

#include <allegro5/allegro_ttf.h>

#include "load.h"

#include "../content/other/spike_damage.h"
#include "../util/allegro_utils.h"
#include "../util/general_utils.h"
#include "../util/string_utils.h"
#include "const.h"
#include "drawing.h"
#include "game.h"
#include "init.h"
#include "misc_functions.h"


using std::make_pair;
using std::set;


/**
 * @brief Loads a mission's record.
 *
 * @param file File data node to load from.
 * @param area_ptr The area's data.
 * @param record Record object to fill.
 */
void load_area_mission_record(
    DataNode* file, Area* area_ptr, MissionRecord &record
) {
    string mission_record_entry_name =
        get_mission_record_entry_name(area_ptr);
        
    vector<string> record_parts =
        split(
            file->getChildByName(
                mission_record_entry_name
            )->value,
            ";", true
        );
        
    if(record_parts.size() == 3) {
        record.clear = record_parts[0] == "1";
        record.score = s2i(record_parts[1]);
        record.date = record_parts[2];
    }
}


/**
 * @brief Loads an audio stream from the game's content.
 *
 * @param file_path Name of the file to load.
 * @param node If not nullptr, blame this data node if the file
 * doesn't exist.
 * @param report_errors Only issues errors if this is true.
 * @return The stream.
 */
ALLEGRO_AUDIO_STREAM* load_audio_stream(
    const string &file_path, DataNode* node, bool report_errors
) {
    ALLEGRO_AUDIO_STREAM* stream =
        al_load_audio_stream((file_path).c_str(), 4, 2048);
        
    if(!stream && report_errors) {
        game.errors.report(
            "Could not open audio stream file \"" + file_path + "\"!",
            node
        );
    }
    
    return stream;
}


/**
 * @brief Loads a bitmap from the game's content.
 *
 * @param path Path to the bitmap file.
 * @param node If present, it will be used to report errors, if any.
 * @param report_error If false, omits error reporting.
 * @param error_bmp_on_error If true, returns the error bitmap in the case of an
 * error. Otherwise, returns nullptr.
 * @param error_bmp_on_empty If true, returns the error bitmap in the case of an
 * empty file name. Otherwise, returns nullptr.
 * @param path_from_root Normally, files are fetched from the images folder.
 * If this parameter is true, the path starts from the game's root.
 * @return The bitmap.
 */
ALLEGRO_BITMAP* load_bmp(
    const string &path, DataNode* node,
    bool report_error, bool error_bmp_on_error,
    bool error_bmp_on_empty
) {
    if(path.empty()) {
        if(error_bmp_on_empty) {
            return game.bmp_error;
        } else {
            return nullptr;
        }
    }
    
    ALLEGRO_BITMAP* b =
        al_load_bitmap((path).c_str());
        
    if(!b) {
        if(report_error) {
            game.errors.report(
                "Could not open image \"" + path + "\"!",
                node
            );
        }
        if(error_bmp_on_error) {
            b = game.bmp_error;
        }
    }
    
    return b;
}


/**
 * @brief Loads a data file from the game's content.
 *
 * @param file_path Path to the file, relative to the program root folder.
 */
DataNode load_data_file(const string &file_path) {
    DataNode n = DataNode(file_path);
    if(!n.fileWasOpened) {
        game.errors.report(
            "Could not open data file \"" + file_path + "\"!"
        );
    }
    
    return n;
}


/**
 * @brief Loads a font from the disk. If it's a bitmap it'll load it from
 * the bitmap and map the characters according to the ranges provided.
 * If it's a font file, it'll just load it directly.
 *
 * @param path Path to the file.
 * @param n Number of Unicode ranges in the bitmap, if it's a bitmap.
 * @param ranges "n" pairs of first and last Unicode point to map glyphs to
 * for each range, if it's a bitmap.
 * @param size Font size, if it's a font file.
 */
ALLEGRO_FONT* load_font(
    const string &path, int n, const int ranges[], int size
) {
    const string &final_path =
        game.content.bitmaps.manifests[path].path;
        
    ALLEGRO_FONT* result = nullptr;
    
    //First, try to load it as a TTF font.
    result =
        al_load_ttf_font(final_path.c_str(), size, ALLEGRO_TTF_NO_KERNING);
        
    if(result) return result;
    
    //Now try as a bitmap.
    ALLEGRO_BITMAP* bmp = load_bmp(final_path);
    result = al_grab_font_from_bitmap(bmp, n, ranges);
    al_destroy_bitmap(bmp);
    
    return result;
}


/**
 * @brief Loads the game's fonts.
 */
void load_fonts() {
    const int STANDARD_FONT_RANGES_SIZE = 2;
    const int STANDARD_FONT_RANGES[STANDARD_FONT_RANGES_SIZE] = {
        0x0020, 0x007E, //ASCII
        /*0x00A0, 0x00A1, //Non-breaking space and inverted !
        0x00BF, 0x00FF, //Inverted ? and European vowels and such*/
    };
    
    const int COUNTER_FONT_RANGES_SIZE = 6;
    const int COUNTER_FONT_RANGES[COUNTER_FONT_RANGES_SIZE] = {
        0x002D, 0x0039, //Dash, dot, slash, numbers
        0x003A, 0x003A, //Colon
        0x0078, 0x0078, //Lowercase x
    };
    
    const int JUST_NUMBERS_FONT_RANGES_SIZE = 2;
    const int JUST_NUMBERS_FONT_RANGES[JUST_NUMBERS_FONT_RANGES_SIZE] = {
        0x0030, 0x0039, //0 to 9
    };
    
    const int VALUE_FONT_RANGES_SIZE = 6;
    const int VALUE_FONT_RANGES[VALUE_FONT_RANGES_SIZE] = {
        0x0024, 0x0024, //Dollar sign
        0x002D, 0x002D, //Dash
        0x0030, 0x0039, //Numbers
    };
    
    //We can't load the fonts directly because we want to set the ranges.
    //So we load them into bitmaps first.
    
    //Area name font.
    game.sys_content.fnt_area_name =
        load_font(
            game.sys_content_names.fnt_area_name,
            STANDARD_FONT_RANGES_SIZE / 2, STANDARD_FONT_RANGES,
            34
        );
        
    //Built-in font.
    game.sys_content.fnt_builtin = al_create_builtin_font();
    
    //Counter font.
    game.sys_content.fnt_counter =
        load_font(
            game.sys_content_names.fnt_counter,
            COUNTER_FONT_RANGES_SIZE / 2, COUNTER_FONT_RANGES,
            32
        );
        
    //Cursor counter font.
    game.sys_content.fnt_cursor_counter =
        load_font(
            game.sys_content_names.fnt_cursor_counter,
            JUST_NUMBERS_FONT_RANGES_SIZE / 2, JUST_NUMBERS_FONT_RANGES,
            16
        );
        
    //Slim font.
    game.sys_content.fnt_slim =
        load_font(
            game.sys_content_names.fnt_slim,
            STANDARD_FONT_RANGES_SIZE / 2, STANDARD_FONT_RANGES,
            22
        );
        
    //Standard font.
    game.sys_content.fnt_standard =
        load_font(
            game.sys_content_names.fnt_standard,
            STANDARD_FONT_RANGES_SIZE / 2, STANDARD_FONT_RANGES,
            22
        );
        
    //Value font.
    game.sys_content.fnt_value =
        load_font(
            game.sys_content_names.fnt_value,
            VALUE_FONT_RANGES_SIZE / 2, VALUE_FONT_RANGES,
            16
        );
}


/**
 * @brief Loads the maker tools from the tool config file.
 */
void load_maker_tools() {
    DataNode file(FILE_PATHS_FROM_ROOT::MAKER_TOOLS);
    if(!file.fileWasOpened) return;
    game.maker_tools.load_from_data_node(&file);
}


/**
 * @brief Loads miscellaneous fixed graphics.
 */
void load_misc_graphics() {
    //Icon.
    game.sys_content.bmp_icon = game.content.bitmaps.list.get(game.sys_content_names.bmp_icon);
    al_set_display_icon(game.display, game.sys_content.bmp_icon);
    
    //Graphics.
    game.sys_content.bmp_menu_icons =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_menu_icons);
    game.sys_content.bmp_bright_circle =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_bright_circle);
    game.sys_content.bmp_bright_ring =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_bright_ring);
    game.sys_content.bmp_bubble_box =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_bubble_box);
    game.sys_content.bmp_button_box =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_button_box);
    game.sys_content.bmp_checkbox_check =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_checkbox_check);
    game.sys_content.bmp_checkbox_no_check =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_checkbox_no_check);
    game.sys_content.bmp_cursor =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_cursor);
    game.sys_content.bmp_discord_icon =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_discord_icon);
    game.sys_content.bmp_enemy_spirit =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_enemy_spirit);
    game.sys_content.bmp_focus_box =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_focus_box);
    game.sys_content.bmp_frame_box =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_frame_box);
    game.sys_content.bmp_github_icon =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_github_icon);
    game.sys_content.bmp_hard_bubble =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_hard_bubble);
    game.sys_content.bmp_idle_glow =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_idle_glow);
    game.sys_content.bmp_key_box =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_key_box);
    game.sys_content.bmp_leader_silhouette_side =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_leader_silhouette_side);
    game.sys_content.bmp_leader_silhouette_top =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_leader_silhouette_top);
    game.sys_content.bmp_medal_bronze =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_medal_bronze);
    game.sys_content.bmp_medal_gold =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_medal_gold);
    game.sys_content.bmp_medal_none =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_medal_none);
    game.sys_content.bmp_medal_platinum =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_medal_platinum);
    game.sys_content.bmp_medal_silver =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_medal_silver);
    game.sys_content.bmp_menu_icons =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_menu_icons);
    game.sys_content.bmp_mission_clear =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_mission_clear);
    game.sys_content.bmp_mission_fail =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_mission_fail);
    game.sys_content.bmp_more =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_more);
    game.sys_content.bmp_mouse_cursor =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_mouse_cursor);
    game.sys_content.bmp_notification =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_notification);
    game.sys_content.bmp_pikmin_spirit =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_pikmin_spirit);
    game.sys_content.bmp_player_input_icons =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_player_input_icons);
    game.sys_content.bmp_random =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_random);
    game.sys_content.bmp_rock =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_rock);
    game.sys_content.bmp_shadow =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_shadow);
    game.sys_content.bmp_shadow_square =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_shadow_square);
    game.sys_content.bmp_smack =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_smack);
    game.sys_content.bmp_smoke =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_smoke);
    game.sys_content.bmp_sparkle =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_sparkle);
    game.sys_content.bmp_spotlight =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_spotlight);
    game.sys_content.bmp_swarm_arrow =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_swarm_arrow);
    game.sys_content.bmp_throw_invalid =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_throw_invalid);
    game.sys_content.bmp_throw_preview =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_throw_preview);
    game.sys_content.bmp_throw_preview_dashed =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_throw_preview_dashed);
    game.sys_content.bmp_wave_ring =
        game.content.bitmaps.list.get(game.sys_content_names.bmp_wave_ring);
}


/**
 * @brief Loads miscellaneous fixed sound effects.
 */
void load_misc_sounds() {
    game.audio.init(
        game.options.audio.master_vol,
        game.options.audio.gameplay_sound_vol,
        game.options.audio.music_vol,
        game.options.audio.ambiance_sound_vol,
        game.options.audio.ui_sound_vol
    );
    
    //Sound effects.
    game.sys_content.sound_attack =
        game.content.sounds.list.get(game.sys_content_names.sound_attack);
    game.sys_content.sound_camera =
        game.content.sounds.list.get(game.sys_content_names.sound_camera);
    game.sys_content.sound_menu_activate =
        game.content.sounds.list.get(game.sys_content_names.sound_menu_activate);
    game.sys_content.sound_menu_back =
        game.content.sounds.list.get(game.sys_content_names.sound_menu_back);
    game.sys_content.sound_menu_select =
        game.content.sounds.list.get(game.sys_content_names.sound_menu_select);
    game.sys_content.sound_switch_pikmin =
        game.content.sounds.list.get(game.sys_content_names.sound_switch_pikmin);
}


/**
 * @brief Loads the player's options.
 */
void load_options() {
    DataNode file = DataNode(FILE_PATHS_FROM_ROOT::OPTIONS);
    if(!file.fileWasOpened) return;
    
    //Init game controllers.
    game.controller_numbers.clear();
    int n_joysticks = al_get_num_joysticks();
    for(int j = 0; j < n_joysticks; j++) {
        game.controller_numbers[al_get_joystick(j)] = j;
    }
    
    //Read the main options.
    game.options.load_from_data_node(&file);
    
    //Final setup.
    game.win_fullscreen = game.options.graphics.intended_win_fullscreen;
    game.win_w = game.options.graphics.intended_win_w;
    game.win_h = game.options.graphics.intended_win_h;
    
    ControlsManagerOptions controls_mgr_options;
    controls_mgr_options.stickMinDeadzone =
        game.options.advanced.joystick_min_deadzone;
    controls_mgr_options.stickMaxDeadzone =
        game.options.advanced.joystick_max_deadzone;
    game.controls.set_options(controls_mgr_options);
}


/**
 * @brief Loads an audio sample from the game's content.
 *
 * @param path Path to the file to load.
 * @param node If not nullptr, blame this data node if the file
 * doesn't exist.
 * @param report_errors Only issues errors if this is true.
 * @return The sample.
 */
ALLEGRO_SAMPLE* load_sample(
    const string &path, DataNode* node, bool report_errors
) {
    ALLEGRO_SAMPLE* sample = al_load_sample((path).c_str());
    
    if(!sample && report_errors) {
        game.errors.report(
            "Could not open audio file \"" + path + "\"!",
            node
        );
    }
    
    return sample;
}


/**
 * @brief Loads the engine's lifetime statistics.
 */
void load_statistics() {
    DataNode stats_file;
    stats_file.loadFile(FILE_PATHS_FROM_ROOT::STATISTICS, true, false, true);
    if(!stats_file.fileWasOpened) return;
    
    Statistics &s = game.statistics;
    
    ReaderSetter rs(&stats_file);
    rs.set("startups",               s.startups);
    rs.set("runtime",                s.runtime);
    rs.set("gameplay_time",          s.gameplay_time);
    rs.set("area_entries",           s.area_entries);
    rs.set("pikmin_births",          s.pikmin_births);
    rs.set("pikmin_deaths",          s.pikmin_deaths);
    rs.set("pikmin_eaten",           s.pikmin_eaten);
    rs.set("pikmin_hazard_deaths",   s.pikmin_hazard_deaths);
    rs.set("pikmin_blooms",          s.pikmin_blooms);
    rs.set("pikmin_saved",           s.pikmin_saved);
    rs.set("enemy_deaths",           s.enemy_deaths);
    rs.set("pikmin_thrown",          s.pikmin_thrown);
    rs.set("whistle_uses",           s.whistle_uses);
    rs.set("distance_walked",        s.distance_walked);
    rs.set("leader_damage_suffered", s.leader_damage_suffered);
    rs.set("punch_damage_caused",    s.punch_damage_caused);
    rs.set("leader_kos",             s.leader_kos);
    rs.set("sprays_used",            s.sprays_used);
}


/**
 * @brief Unloads miscellaneous graphics, sounds, and other resources.
 */
void unload_misc_resources() {
    //Graphics.
    game.content.bitmaps.list.free(game.sys_content.bmp_bright_circle);
    game.content.bitmaps.list.free(game.sys_content.bmp_bright_ring);
    game.content.bitmaps.list.free(game.sys_content.bmp_bubble_box);
    game.content.bitmaps.list.free(game.sys_content.bmp_button_box);
    game.content.bitmaps.list.free(game.sys_content.bmp_checkbox_check);
    game.content.bitmaps.list.free(game.sys_content.bmp_checkbox_no_check);
    game.content.bitmaps.list.free(game.sys_content.bmp_cursor);
    game.content.bitmaps.list.free(game.sys_content.bmp_discord_icon);
    game.content.bitmaps.list.free(game.sys_content.bmp_enemy_spirit);
    game.content.bitmaps.list.free(game.sys_content.bmp_focus_box);
    game.content.bitmaps.list.free(game.sys_content.bmp_frame_box);
    game.content.bitmaps.list.free(game.sys_content.bmp_github_icon);
    game.content.bitmaps.list.free(game.sys_content.bmp_hard_bubble);
    game.content.bitmaps.list.free(game.sys_content.bmp_icon);
    game.content.bitmaps.list.free(game.sys_content.bmp_idle_glow);
    game.content.bitmaps.list.free(game.sys_content.bmp_key_box);
    game.content.bitmaps.list.free(game.sys_content.bmp_leader_silhouette_side);
    game.content.bitmaps.list.free(game.sys_content.bmp_leader_silhouette_top);
    game.content.bitmaps.list.free(game.sys_content.bmp_medal_bronze);
    game.content.bitmaps.list.free(game.sys_content.bmp_medal_gold);
    game.content.bitmaps.list.free(game.sys_content.bmp_medal_none);
    game.content.bitmaps.list.free(game.sys_content.bmp_medal_platinum);
    game.content.bitmaps.list.free(game.sys_content.bmp_medal_silver);
    game.content.bitmaps.list.free(game.sys_content.bmp_menu_icons);
    game.content.bitmaps.list.free(game.sys_content.bmp_mission_clear);
    game.content.bitmaps.list.free(game.sys_content.bmp_mission_fail);
    game.content.bitmaps.list.free(game.sys_content.bmp_more);
    game.content.bitmaps.list.free(game.sys_content.bmp_mouse_cursor);
    game.content.bitmaps.list.free(game.sys_content.bmp_notification);
    game.content.bitmaps.list.free(game.sys_content.bmp_pikmin_spirit);
    game.content.bitmaps.list.free(game.sys_content.bmp_player_input_icons);
    game.content.bitmaps.list.free(game.sys_content.bmp_random);
    game.content.bitmaps.list.free(game.sys_content.bmp_rock);
    game.content.bitmaps.list.free(game.sys_content.bmp_shadow);
    game.content.bitmaps.list.free(game.sys_content.bmp_shadow_square);
    game.content.bitmaps.list.free(game.sys_content.bmp_smack);
    game.content.bitmaps.list.free(game.sys_content.bmp_smoke);
    game.content.bitmaps.list.free(game.sys_content.bmp_sparkle);
    game.content.bitmaps.list.free(game.sys_content.bmp_spotlight);
    game.content.bitmaps.list.free(game.sys_content.bmp_swarm_arrow);
    game.content.bitmaps.list.free(game.sys_content.bmp_throw_invalid);
    game.content.bitmaps.list.free(game.sys_content.bmp_throw_preview);
    game.content.bitmaps.list.free(game.sys_content.bmp_throw_preview_dashed);
    game.content.bitmaps.list.free(game.sys_content.bmp_wave_ring);
    
    //Fonts.
    al_destroy_font(game.sys_content.fnt_area_name);
    al_destroy_font(game.sys_content.fnt_counter);
    al_destroy_font(game.sys_content.fnt_cursor_counter);
    al_destroy_font(game.sys_content.fnt_slim);
    al_destroy_font(game.sys_content.fnt_standard);
    al_destroy_font(game.sys_content.fnt_value);
    
    //Sounds effects.
    game.content.sounds.list.free(game.sys_content.sound_attack);
    game.content.sounds.list.free(game.sys_content.sound_camera);
    game.content.sounds.list.free(game.sys_content.sound_menu_activate);
    game.content.sounds.list.free(game.sys_content.sound_menu_back);
    game.content.sounds.list.free(game.sys_content.sound_menu_select);
    game.content.sounds.list.free(game.sys_content.sound_switch_pikmin);
}
