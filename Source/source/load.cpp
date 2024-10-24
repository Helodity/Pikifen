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

#include "const.h"
#include "drawing.h"
#include "functions.h"
#include "game.h"
#include "init.h"
#include "spike_damage.h"
#include "utils/allegro_utils.h"
#include "utils/general_utils.h"
#include "utils/string_utils.h"


using std::set;


/**
 * @brief Loads a mission's record.
 *
 * @param file File data node to load from.
 * @param area_name Name of the area.
 * @param area_subtitle Area subtitle, or mission goal if none.
 * @param area_maker Area maker.
 * @param area_version Area version.
 * @param record Record object to fill.
 */
void load_area_mission_record(
    data_node* file,
    const string &area_name, const string &area_subtitle,
    const string &area_maker, const string &area_version,
    mission_record &record
) {
    string mission_record_entry_name =
        area_name + ";" +
        area_subtitle + ";" +
        area_maker + ";" +
        area_version;
        
    vector<string> record_parts =
        split(
            file->get_child_by_name(
                mission_record_entry_name
            )->value,
            ";"
        );
        
    if(record_parts.size() == 3) {
        record.clear = record_parts[0] == "1";
        record.score = s2i(record_parts[1]);
        record.date = record_parts[2];
    }
}


/**
 * @brief Loads asset file names.
 */
void load_asset_file_names() {
    data_node file(SYSTEM_ASSET_FILE_NAMES_FILE_PATH);
    
    game.asset_file_names.load(&file);
}


/**
 * @brief Loads an audio stream from the game's content.
 *
 * @param file_name Name of the file to load.
 * @param node If not nullptr, blame this data node if the file
 * doesn't exist.
 * @param report_errors Only issues errors if this is true.
 * @return The stream.
 */
ALLEGRO_AUDIO_STREAM* load_audio_stream(
    const string &file_name, data_node* node, bool report_errors
) {
    ALLEGRO_AUDIO_STREAM* stream =
        al_load_audio_stream(
            (AUDIO_TRACK_FOLDER_PATH + "/" + file_name).c_str(),
            4, 2048
        );
        
    if(!stream && report_errors) {
        game.errors.report(
            "Could not open audio stream file \"" + file_name + "\"!",
            node
        );
    }
    
    return stream;
}


/**
 * @brief Loads a bitmap from the game's content.
 *
 * @param file_name File name of the bitmap.
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
    const string &file_name, data_node* node,
    bool report_error, bool error_bmp_on_error,
    bool error_bmp_on_empty, bool path_from_root
) {
    if(file_name.empty()) {
        if(error_bmp_on_empty) {
            return game.bmp_error;
        } else {
            return nullptr;
        }
    }
    
    string base_dir = (path_from_root ? "" : (GRAPHICS_FOLDER_PATH + "/"));
    ALLEGRO_BITMAP* b =
        al_load_bitmap((base_dir + file_name).c_str());
        
    if(!b) {
        if(report_error) {
            game.errors.report(
                "Could not open image \"" + file_name + "\"!",
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
data_node load_data_file(const string &file_path) {
    data_node n = data_node(file_path);
    if(!n.file_was_opened) {
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
 * @param file_name Name of the file in the graphics folder.
 * @param n Number of Unicode ranges in the bitmap, if it's a bitmap.
 * @param ranges "n" pairs of first and last Unicode point to map glyphs to
 * for each range, if it's a bitmap.
 * @param size Font size, if it's a font file.
 */
ALLEGRO_FONT* load_font(
    const string &file_name, int n, const int ranges[], int size
) {
    string full_path = GRAPHICS_FOLDER_PATH + "/" + file_name;
    ALLEGRO_FONT* result = nullptr;
    
    //First, try to load it as a TTF font.
    result =
        al_load_ttf_font(full_path.c_str(), size, ALLEGRO_TTF_NO_KERNING);
        
    if(result) return result;
    
    //Now try as a bitmap.
    ALLEGRO_BITMAP* bmp = load_bmp(file_name);
    result = al_grab_font_from_bitmap(bmp, n, ranges);
    al_destroy_bitmap(bmp);
    
    return result;
}


/**
 * @brief Loads the game's fonts.
 */
void load_fonts() {
    const int STANDARD_FONT_RANGES_SIZE = 2;
    const int standard_font_ranges[STANDARD_FONT_RANGES_SIZE] = {
        0x0020, 0x007E, //ASCII
        /*0x00A0, 0x00A1, //Non-breaking space and inverted !
        0x00BF, 0x00FF, //Inverted ? and European vowels and such*/
    };
    
    const int COUNTER_FONT_RANGES_SIZE = 6;
    const int counter_font_ranges[COUNTER_FONT_RANGES_SIZE] = {
        0x002D, 0x0039, //Dash, dot, slash, numbers
        0x003A, 0x003A, //Colon
        0x0078, 0x0078, //Lowercase x
    };
    
    const int JUST_NUMBERS_FONT_RANGES_SIZE = 2;
    const int just_numbers_font_ranges[JUST_NUMBERS_FONT_RANGES_SIZE] = {
        0x0030, 0x0039, //0 to 9
    };
    
    const int VALUE_FONT_RANGES_SIZE = 6;
    const int value_font_ranges[VALUE_FONT_RANGES_SIZE] = {
        0x0024, 0x0024, //Dollar sign
        0x002D, 0x002D, //Dash
        0x0030, 0x0039, //Numbers
    };
    
    //We can't load the fonts directly because we want to set the ranges.
    //So we load them into bitmaps first.
    
    //Area name font.
    game.sys_assets.fnt_area_name =
        load_font(
            game.asset_file_names.fnt_area_name,
            STANDARD_FONT_RANGES_SIZE / 2, standard_font_ranges,
            34
        );
        
    //Built-in font.
    game.sys_assets.fnt_builtin = al_create_builtin_font();
    
    //Counter font.
    game.sys_assets.fnt_counter =
        load_font(
            game.asset_file_names.fnt_counter,
            COUNTER_FONT_RANGES_SIZE / 2, counter_font_ranges,
            32
        );
        
    //Cursor counter font.
    game.sys_assets.fnt_cursor_counter =
        load_font(
            game.asset_file_names.fnt_cursor_counter,
            JUST_NUMBERS_FONT_RANGES_SIZE / 2, just_numbers_font_ranges,
            16
        );
        
    //Slim font.
    game.sys_assets.fnt_slim =
        load_font(
            game.asset_file_names.fnt_slim,
            STANDARD_FONT_RANGES_SIZE / 2, standard_font_ranges,
            22
        );
        
    //Standard font.
    game.sys_assets.fnt_standard =
        load_font(
            game.asset_file_names.fnt_standard,
            STANDARD_FONT_RANGES_SIZE / 2, standard_font_ranges,
            22
        );
        
    //Value font.
    game.sys_assets.fnt_value =
        load_font(
            game.asset_file_names.fnt_value,
            VALUE_FONT_RANGES_SIZE / 2, value_font_ranges,
            16
        );
}


/**
 * @brief Loads the game's configuration file.
 */
void load_game_config() {
    data_node file = load_data_file(CONFIG_FILE);
    
    game.config.load(&file);
    
    al_set_window_title(
        game.display,
        game.config.name.empty() ? "Pikifen" : game.config.name.c_str()
    );
}


/**
 * @brief Loads the maker tools from the tool config file.
 */
void load_maker_tools() {
    data_node file(MAKER_TOOLS_FILE_PATH);
    
    if(!file.file_was_opened) return;
    
    game.maker_tools.enabled = s2b(file.get_child_by_name("enabled")->value);
    
    for(unsigned char k = 0; k < 20; k++) {
        string tool_name;
        if(k < 10) {
            //The first ten indexes are the F2 - F11 keys.
            tool_name = file.get_child_by_name("f" + i2s(k + 2))->value;
        } else {
            //The second ten indexes are the 0 - 9 keys.
            tool_name = file.get_child_by_name(i2s(k - 10))->value;
        }
        
        for(size_t t = 0; t < N_MAKER_TOOLS; t++) {
            if(tool_name == MAKER_TOOLS::NAMES[t]) {
                game.maker_tools.keys[k] = (MAKER_TOOL_TYPE) t;
            }
        }
    }
    
    reader_setter rs(&file);
    
    data_node* mob_hurting_percentage_node = nullptr;
    
    rs.set("area_image_mobs", game.maker_tools.area_image_mobs);
    rs.set("area_image_padding", game.maker_tools.area_image_padding);
    rs.set("area_image_shadows", game.maker_tools.area_image_shadows);
    rs.set("area_image_size", game.maker_tools.area_image_size);
    rs.set("change_speed_multiplier", game.maker_tools.change_speed_mult);
    rs.set(
        "mob_hurting_percentage", game.maker_tools.mob_hurting_ratio,
        &mob_hurting_percentage_node
    );
    rs.set("auto_start_option", game.maker_tools.auto_start_option);
    rs.set("auto_start_mode", game.maker_tools.auto_start_mode);
    rs.set("performance_monitor", game.maker_tools.use_perf_mon);
    
    if(mob_hurting_percentage_node) {
        game.maker_tools.mob_hurting_ratio /= 100.0;
    }
}


/**
 * @brief Loads miscellaneous fixed graphics.
 */
void load_misc_graphics() {
    //Icon.
    game.sys_assets.bmp_icon = game.bitmaps.get(game.asset_file_names.bmp_icon);
    al_set_display_icon(game.display, game.sys_assets.bmp_icon);
    
    //Graphics.
    game.sys_assets.bmp_menu_icons =
        game.bitmaps.get(game.asset_file_names.bmp_menu_icons);
    game.sys_assets.bmp_bright_circle =
        game.bitmaps.get(game.asset_file_names.bmp_bright_circle);
    game.sys_assets.bmp_bright_ring =
        game.bitmaps.get(game.asset_file_names.bmp_bright_ring);
    game.sys_assets.bmp_bubble_box =
        game.bitmaps.get(game.asset_file_names.bmp_bubble_box);
    game.sys_assets.bmp_button_box =
        game.bitmaps.get(game.asset_file_names.bmp_button_box);
    game.sys_assets.bmp_checkbox_check =
        game.bitmaps.get(game.asset_file_names.bmp_checkbox_check);
    game.sys_assets.bmp_checkbox_no_check =
        game.bitmaps.get(game.asset_file_names.bmp_checkbox_no_check);
    game.sys_assets.bmp_cursor =
        game.bitmaps.get(game.asset_file_names.bmp_cursor);
    game.sys_assets.bmp_enemy_spirit =
        game.bitmaps.get(game.asset_file_names.bmp_enemy_spirit);
    game.sys_assets.bmp_focus_box =
        game.bitmaps.get(game.asset_file_names.bmp_focus_box);
    game.sys_assets.bmp_frame_box =
        game.bitmaps.get(game.asset_file_names.bmp_frame_box);
    game.sys_assets.bmp_hard_bubble =
        game.bitmaps.get(game.asset_file_names.bmp_hard_bubble);
    game.sys_assets.bmp_idle_glow =
        game.bitmaps.get(game.asset_file_names.bmp_idle_glow);
    game.sys_assets.bmp_key_box =
        game.bitmaps.get(game.asset_file_names.bmp_key_box);
    game.sys_assets.bmp_leader_silhouette_side =
        game.bitmaps.get(game.asset_file_names.bmp_leader_silhouette_side);
    game.sys_assets.bmp_leader_silhouette_top =
        game.bitmaps.get(game.asset_file_names.bmp_leader_silhouette_top);
    game.sys_assets.bmp_medal_bronze =
        game.bitmaps.get(game.asset_file_names.bmp_medal_bronze);
    game.sys_assets.bmp_medal_gold =
        game.bitmaps.get(game.asset_file_names.bmp_medal_gold);
    game.sys_assets.bmp_medal_none =
        game.bitmaps.get(game.asset_file_names.bmp_medal_none);
    game.sys_assets.bmp_medal_platinum =
        game.bitmaps.get(game.asset_file_names.bmp_medal_platinum);
    game.sys_assets.bmp_medal_silver =
        game.bitmaps.get(game.asset_file_names.bmp_medal_silver);
    game.sys_assets.bmp_menu_icons =
        game.bitmaps.get(game.asset_file_names.bmp_menu_icons);
    game.sys_assets.bmp_mission_clear =
        game.bitmaps.get(game.asset_file_names.bmp_mission_clear);
    game.sys_assets.bmp_mission_fail =
        game.bitmaps.get(game.asset_file_names.bmp_mission_fail);
    game.sys_assets.bmp_more =
        game.bitmaps.get(game.asset_file_names.bmp_more);
    game.sys_assets.bmp_mouse_cursor =
        game.bitmaps.get(game.asset_file_names.bmp_mouse_cursor);
    game.sys_assets.bmp_notification =
        game.bitmaps.get(game.asset_file_names.bmp_notification);
    game.sys_assets.bmp_pikmin_spirit =
        game.bitmaps.get(game.asset_file_names.bmp_pikmin_spirit);
    game.sys_assets.bmp_player_input_icons =
        game.bitmaps.get(game.asset_file_names.bmp_player_input_icons);
    game.sys_assets.bmp_random =
        game.bitmaps.get(game.asset_file_names.bmp_random);
    game.sys_assets.bmp_rock =
        game.bitmaps.get(game.asset_file_names.bmp_rock);
    game.sys_assets.bmp_shadow =
        game.bitmaps.get(game.asset_file_names.bmp_shadow);
    game.sys_assets.bmp_shadow_square =
        game.bitmaps.get(game.asset_file_names.bmp_shadow_square);
    game.sys_assets.bmp_smack =
        game.bitmaps.get(game.asset_file_names.bmp_smack);
    game.sys_assets.bmp_smoke =
        game.bitmaps.get(game.asset_file_names.bmp_smoke);
    game.sys_assets.bmp_sparkle =
        game.bitmaps.get(game.asset_file_names.bmp_sparkle);
    game.sys_assets.bmp_spotlight =
        game.bitmaps.get(game.asset_file_names.bmp_spotlight);
    game.sys_assets.bmp_swarm_arrow =
        game.bitmaps.get(game.asset_file_names.bmp_swarm_arrow);
    game.sys_assets.bmp_throw_invalid =
        game.bitmaps.get(game.asset_file_names.bmp_throw_invalid);
    game.sys_assets.bmp_throw_preview =
        game.bitmaps.get(game.asset_file_names.bmp_throw_preview);
    game.sys_assets.bmp_throw_preview_dashed =
        game.bitmaps.get(game.asset_file_names.bmp_throw_preview_dashed);
    game.sys_assets.bmp_wave_ring =
        game.bitmaps.get(game.asset_file_names.bmp_wave_ring);
}


/**
 * @brief Loads miscellaneous fixed sound effects.
 */
void load_misc_sounds() {
    game.audio.init(
        game.options.master_volume,
        game.options.world_sfx_volume,
        game.options.music_volume,
        game.options.ambiance_volume,
        game.options.ui_sfx_volume
    );
    
    //Sound effects.
    game.sys_assets.sfx_attack =
        game.audio.samples.get(game.asset_file_names.sfx_attack);
    game.sys_assets.sfx_camera =
        game.audio.samples.get(game.asset_file_names.sfx_camera);
    game.sys_assets.sfx_menu_activate =
        game.audio.samples.get(game.asset_file_names.sfx_menu_activate);
    game.sys_assets.sfx_menu_back =
        game.audio.samples.get(game.asset_file_names.sfx_menu_back);
    game.sys_assets.sfx_menu_select =
        game.audio.samples.get(game.asset_file_names.sfx_menu_select);
    game.sys_assets.sfx_pluck =
        game.audio.samples.get(game.asset_file_names.sfx_pluck);
    game.sys_assets.sfx_throw =
        game.audio.samples.get(game.asset_file_names.sfx_throw);
    game.sys_assets.sfx_spray =
        game.audio.samples.get(game.asset_file_names.sfx_spray);
    game.sys_assets.sfx_switch_pikmin =
        game.audio.samples.get(game.asset_file_names.sfx_switch_pikmin);
}


/**
 * @brief Loads the player's options.
 */
void load_options() {
    data_node file = data_node(OPTIONS_FILE_PATH);
    if(!file.file_was_opened) return;
    
    //Init game controllers.
    game.controller_numbers.clear();
    int n_joysticks = al_get_num_joysticks();
    for(int j = 0; j < n_joysticks; j++) {
        game.controller_numbers[al_get_joystick(j)] = j;
    }
    
    //Read the main options.
    game.options.load(&file);
    
    game.win_fullscreen = game.options.intended_win_fullscreen;
    game.win_w = game.options.intended_win_w;
    game.win_h = game.options.intended_win_h;
    
    //Set up the editor histories.
    reader_setter rs(&file);
    
    game.states.animation_ed->history.clear();
    for(size_t h = 0; h < game.states.animation_ed->get_history_size(); h++) {
        game.states.animation_ed->history.push_back("");
        rs.set(
            game.states.animation_ed->get_history_option_prefix() +
            i2s(h + 1),
            game.states.animation_ed->history[h]
        );
    }
    game.states.area_ed->history.clear();
    for(size_t h = 0; h < game.states.area_ed->get_history_size(); h++) {
        game.states.area_ed->history.push_back("");
        rs.set(
            game.states.area_ed->get_history_option_prefix() +
            i2s(h + 1),
            game.states.area_ed->history[h]
        );
    }
    game.states.gui_ed->history.clear();
    for(size_t h = 0; h < game.states.gui_ed->get_history_size(); h++) {
        game.states.gui_ed->history.push_back("");
        rs.set(
            game.states.gui_ed->get_history_option_prefix() +
            i2s(h + 1),
            game.states.gui_ed->history[h]
        );
    }
    
    //Final setup.
    controls_manager_options controls_mgr_options;
    controls_mgr_options.stick_min_deadzone =
        game.options.joystick_min_deadzone;
    controls_mgr_options.stick_max_deadzone =
        game.options.joystick_max_deadzone;
    game.controls.set_options(controls_mgr_options);
}


/**
 * @brief Loads an audio sample from the game's content.
 *
 * @param file_name Name of the file to load.
 * @param node If not nullptr, blame this data node if the file
 * doesn't exist.
 * @param report_errors Only issues errors if this is true.
 * @return The sample.
 */
ALLEGRO_SAMPLE* load_sample(
    const string &file_name, data_node* node, bool report_errors
) {
    ALLEGRO_SAMPLE* sample =
        al_load_sample((AUDIO_SOUNDS_FOLDER_PATH + "/" + file_name).c_str());
        
    if(!sample && report_errors) {
        game.errors.report(
            "Could not open audio file \"" + file_name + "\"!",
            node
        );
    }
    
    return sample;
}


/**
 * @brief Loads the songs.
 */
void load_songs() {
    vector<string> song_files =
        folder_to_vector(AUDIO_SONG_FOLDER_PATH, false);
        
    for(size_t s = 0; s < song_files.size(); s++) {
        string path = AUDIO_SONG_FOLDER_PATH + "/" + song_files[s];
        data_node file = load_data_file(path);
        if(!file.file_was_opened) continue;
        
        song new_song;
        new_song.path = path;
        new_song.load_from_data_node(&file);
        game.audio.songs[new_song.name] = new_song;
    }
}


/**
 * @brief Loads the engine's lifetime statistics.
 */
void load_statistics() {
    data_node stats_file;
    stats_file.load_file(STATISTICS_FILE_PATH, true, false, true);
    if(!stats_file.file_was_opened) return;
    
    statistics_t &s = game.statistics;
    
    reader_setter rs(&stats_file);
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
 * @brief Loads the animations that are used system-wide.
 */
void load_system_animations() {
    data_node system_animations_file =
        load_data_file(SYSTEM_ANIMATIONS_FILE_PATH);
        
    init_single_animation(
        &system_animations_file,
        "leader_damage_sparks", game.sys_assets.spark_animation
    );
}


/**
 * @brief Unloads miscellaneous graphics, sounds, and other resources.
 */
void unload_misc_resources() {
    //Graphics.
    game.bitmaps.free(game.sys_assets.bmp_bright_circle);
    game.bitmaps.free(game.sys_assets.bmp_bright_ring);
    game.bitmaps.free(game.sys_assets.bmp_bubble_box);
    game.bitmaps.free(game.sys_assets.bmp_button_box);
    game.bitmaps.free(game.sys_assets.bmp_checkbox_check);
    game.bitmaps.free(game.sys_assets.bmp_checkbox_no_check);
    game.bitmaps.free(game.sys_assets.bmp_cursor);
    game.bitmaps.free(game.sys_assets.bmp_enemy_spirit);
    game.bitmaps.free(game.sys_assets.bmp_focus_box);
    game.bitmaps.free(game.sys_assets.bmp_frame_box);
    game.bitmaps.free(game.sys_assets.bmp_hard_bubble);
    game.bitmaps.free(game.sys_assets.bmp_icon);
    game.bitmaps.free(game.sys_assets.bmp_idle_glow);
    game.bitmaps.free(game.sys_assets.bmp_key_box);
    game.bitmaps.free(game.sys_assets.bmp_leader_silhouette_side);
    game.bitmaps.free(game.sys_assets.bmp_leader_silhouette_top);
    game.bitmaps.free(game.sys_assets.bmp_medal_bronze);
    game.bitmaps.free(game.sys_assets.bmp_medal_gold);
    game.bitmaps.free(game.sys_assets.bmp_medal_none);
    game.bitmaps.free(game.sys_assets.bmp_medal_platinum);
    game.bitmaps.free(game.sys_assets.bmp_medal_silver);
    game.bitmaps.free(game.sys_assets.bmp_menu_icons);
    game.bitmaps.free(game.sys_assets.bmp_mission_clear);
    game.bitmaps.free(game.sys_assets.bmp_mission_fail);
    game.bitmaps.free(game.sys_assets.bmp_more);
    game.bitmaps.free(game.sys_assets.bmp_mouse_cursor);
    game.bitmaps.free(game.sys_assets.bmp_notification);
    game.bitmaps.free(game.sys_assets.bmp_pikmin_spirit);
    game.bitmaps.free(game.sys_assets.bmp_player_input_icons);
    game.bitmaps.free(game.sys_assets.bmp_random);
    game.bitmaps.free(game.sys_assets.bmp_rock);
    game.bitmaps.free(game.sys_assets.bmp_shadow);
    game.bitmaps.free(game.sys_assets.bmp_shadow_square);
    game.bitmaps.free(game.sys_assets.bmp_smack);
    game.bitmaps.free(game.sys_assets.bmp_smoke);
    game.bitmaps.free(game.sys_assets.bmp_sparkle);
    game.bitmaps.free(game.sys_assets.bmp_spotlight);
    game.bitmaps.free(game.sys_assets.bmp_swarm_arrow);
    game.bitmaps.free(game.sys_assets.bmp_throw_invalid);
    game.bitmaps.free(game.sys_assets.bmp_throw_preview);
    game.bitmaps.free(game.sys_assets.bmp_throw_preview_dashed);
    game.bitmaps.free(game.sys_assets.bmp_wave_ring);
    
    //Fonts.
    al_destroy_font(game.sys_assets.fnt_area_name);
    al_destroy_font(game.sys_assets.fnt_counter);
    al_destroy_font(game.sys_assets.fnt_cursor_counter);
    al_destroy_font(game.sys_assets.fnt_slim);
    al_destroy_font(game.sys_assets.fnt_standard);
    al_destroy_font(game.sys_assets.fnt_value);
    
    //Sounds effects.
    game.audio.samples.free(game.sys_assets.sfx_attack);
    game.audio.samples.free(game.sys_assets.sfx_camera);
    game.audio.samples.free(game.sys_assets.sfx_menu_activate);
    game.audio.samples.free(game.sys_assets.sfx_menu_back);
    game.audio.samples.free(game.sys_assets.sfx_menu_select);
    game.audio.samples.free(game.sys_assets.sfx_spray);
    game.audio.samples.free(game.sys_assets.sfx_switch_pikmin);
    game.audio.samples.free(game.sys_assets.sfx_throw);
}


/**
 * @brief Unloads loaded songs from memory.
 */
void unload_songs() {
    for(auto &s : game.audio.songs) {
        game.audio.streams.free(s.second.main_track);
        for(auto &t : s.second.mix_tracks) {
            game.audio.streams.free(t.second);
        }
    }
    game.audio.songs.clear();
}
