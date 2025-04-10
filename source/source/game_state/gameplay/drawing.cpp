/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Main gameplay drawing functions.
 */

#include <algorithm>

#include "gameplay.h"

#include "../../content/mob/group_task.h"
#include "../../content/mob/pile.h"
#include "../../content/mob/scale.h"
#include "../../core/drawing.h"
#include "../../core/misc_functions.h"
#include "../../core/game.h"
#include "../../util/allegro_utils.h"
#include "../../util/general_utils.h"
#include "../../util/string_utils.h"


#pragma warning(disable: 4701)


/**
 * @brief Does the drawing for the main game loop.
 *
 * @param bmp_output If not nullptr, draw the area onto this.
 * @param bmp_transform Transformation to use when drawing to a bitmap.
 * @param bmp_settings Settings to use when drawing to a bitmap.
 */
void GameplayState::do_game_drawing(
    ALLEGRO_BITMAP* bmp_output, const ALLEGRO_TRANSFORM* bmp_transform,
    const MakerTools::AreaImageSettings &bmp_settings
) {

    /*  ***************************************
      *** |  |                           |  | ***
    ***** |__|          DRAWING          |__| *****
      ***  \/                             \/  ***
        ***************************************/
    
    ALLEGRO_TRANSFORM old_world_to_screen_transform;
    int blend_old_op, blend_old_src, blend_old_dst,
        blend_old_aop, blend_old_asrc, blend_old_adst;
        
    if(bmp_output) {
        old_world_to_screen_transform = game.world_to_screen_transform;
        game.world_to_screen_transform = *bmp_transform;
        al_set_target_bitmap(bmp_output);
        al_get_separate_blender(
            &blend_old_op, &blend_old_src, &blend_old_dst,
            &blend_old_aop, &blend_old_asrc, &blend_old_adst
        );
        al_set_separate_blender(
            ALLEGRO_ADD, ALLEGRO_ALPHA,
            ALLEGRO_INVERSE_ALPHA, ALLEGRO_ADD,
            ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA
        );
    }
    
    al_clear_to_color(game.cur_area_data->bg_color);
    
    //Layer 1 -- Background.
    if(game.perf_mon) {
        game.perf_mon->start_measurement("Drawing -- Background");
    }
    draw_background(bmp_output);
    if(game.perf_mon) {
        game.perf_mon->finish_measurement();
    }
    
    //Layer 2 -- World components.
    if(game.perf_mon) {
        game.perf_mon->start_measurement("Drawing -- World");
    }
    al_use_transform(&game.world_to_screen_transform);
    draw_world_components(bmp_output);
    if(game.perf_mon) {
        game.perf_mon->finish_measurement();
    }
    
    //Layer 3 -- In-game text.
    if(game.perf_mon) {
        game.perf_mon->start_measurement("Drawing -- In-game text");
    }
    if(!bmp_output && game.maker_tools.hud) {
        draw_ingame_text();
    }
    if(game.perf_mon) {
        game.perf_mon->finish_measurement();
    }
    
    //Layer 4 -- Precipitation.
    if(game.perf_mon) {
        game.perf_mon->start_measurement("Drawing -- precipitation");
    }
    if(!bmp_output) {
        draw_precipitation();
    }
    if(game.perf_mon) {
        game.perf_mon->finish_measurement();
    }
    
    //Layer 5 -- Tree shadows.
    if(game.perf_mon) {
        game.perf_mon->start_measurement("Drawing -- Tree shadows");
    }
    if(!(bmp_output && !bmp_settings.shadows)) {
        draw_tree_shadows();
    }
    if(game.perf_mon) {
        game.perf_mon->finish_measurement();
    }
    
    //Finish dumping to a bitmap image here.
    if(bmp_output) {
        al_set_separate_blender(
            blend_old_op, blend_old_src, blend_old_dst,
            blend_old_aop, blend_old_asrc, blend_old_adst
        );
        game.world_to_screen_transform = old_world_to_screen_transform;
        al_set_target_backbuffer(game.display);
        return;
    }
    
    //Layer 6 -- Lighting filter.
    if(game.perf_mon) {
        game.perf_mon->start_measurement("Drawing -- Lighting");
    }
    draw_lighting_filter();
    if(game.perf_mon) {
        game.perf_mon->finish_measurement();
    }
    
    //Layer 7 -- Leader cursor.
    al_use_transform(&game.world_to_screen_transform);
    ALLEGRO_COLOR cursor_color = game.config.aesthetic_gen.no_pikmin_color;
    if(closest_group_member[BUBBLE_RELATION_CURRENT]) {
        cursor_color =
            closest_group_member[BUBBLE_RELATION_CURRENT]->type->main_color;
    }
    if(cur_leader_ptr && game.maker_tools.hud) {
        cursor_color =
            change_color_lighting(cursor_color, cursor_height_diff_light);
        draw_leader_cursor(cursor_color);
    }
    
    //Layer 8 -- HUD.
    al_use_transform(&game.identity_transform);
    
    if(game.perf_mon) {
        game.perf_mon->start_measurement("Drawing -- HUD");
    }
    
    if(game.maker_tools.hud) {
        hud->gui.draw();
        
        draw_big_msg();
        
        if(msg_box) {
            draw_gameplay_message_box();
        } else if(onion_menu) {
            draw_onion_menu();
        } else if(pause_menu) {
            draw_pause_menu();
        } else {
            draw_mouse_cursor(cursor_color);
        }
    }
    
    if(game.perf_mon) {
        game.perf_mon->finish_measurement();
    }
    
    //Layer 9 -- System stuff.
    if(game.maker_tools.hud) {
        draw_system_stuff();
        
        if(area_title_fade_timer.time_left > 0) {
            draw_loading_screen(
                game.cur_area_data->name,
                get_subtitle_or_mission_goal(
                    game.cur_area_data->subtitle,
                    game.cur_area_data->type,
                    game.cur_area_data->mission.goal
                ),
                area_title_fade_timer.get_ratio_left()
            );
        }
        
    }
    
    draw_debug_tools();
}


#pragma warning(default: 4701)


/**
 * @brief Draws the area background.
 *
 * @param bmp_output If not nullptr, draw the background onto this.
 */
void GameplayState::draw_background(ALLEGRO_BITMAP* bmp_output) {
    if(!game.cur_area_data->bg_bmp) return;
    
    ALLEGRO_VERTEX bg_v[4];
    for(unsigned char v = 0; v < 4; v++) {
        bg_v[v].color = COLOR_WHITE;
        bg_v[v].z = 0;
    }
    
    //Not gonna lie, this uses some fancy-shmancy numbers.
    //I mostly got here via trial and error.
    //I apologize if you're trying to understand what it means.
    int bmp_w = bmp_output ? al_get_bitmap_width(bmp_output) : game.win_w;
    int bmp_h = bmp_output ? al_get_bitmap_height(bmp_output) : game.win_h;
    float zoom_to_use = bmp_output ? 0.5 : game.cam.zoom;
    Point final_zoom(
        bmp_w * 0.5 * game.cur_area_data->bg_dist / zoom_to_use,
        bmp_h * 0.5 * game.cur_area_data->bg_dist / zoom_to_use
    );
    
    bg_v[0].x =
        0;
    bg_v[0].y =
        0;
    bg_v[0].u =
        (game.cam.pos.x - final_zoom.x) / game.cur_area_data->bg_bmp_zoom;
    bg_v[0].v =
        (game.cam.pos.y - final_zoom.y) / game.cur_area_data->bg_bmp_zoom;
    bg_v[1].x =
        bmp_w;
    bg_v[1].y =
        0;
    bg_v[1].u =
        (game.cam.pos.x + final_zoom.x) / game.cur_area_data->bg_bmp_zoom;
    bg_v[1].v =
        (game.cam.pos.y - final_zoom.y) / game.cur_area_data->bg_bmp_zoom;
    bg_v[2].x =
        bmp_w;
    bg_v[2].y =
        bmp_h;
    bg_v[2].u =
        (game.cam.pos.x + final_zoom.x) / game.cur_area_data->bg_bmp_zoom;
    bg_v[2].v =
        (game.cam.pos.y + final_zoom.y) / game.cur_area_data->bg_bmp_zoom;
    bg_v[3].x =
        0;
    bg_v[3].y =
        bmp_h;
    bg_v[3].u =
        (game.cam.pos.x - final_zoom.x) / game.cur_area_data->bg_bmp_zoom;
    bg_v[3].v =
        (game.cam.pos.y + final_zoom.y) / game.cur_area_data->bg_bmp_zoom;
        
    al_draw_prim(
        bg_v, nullptr, game.cur_area_data->bg_bmp,
        0, 4, ALLEGRO_PRIM_TRIANGLE_FAN
    );
}


/**
 * @brief Draws the current big message, if any.
 */
void GameplayState::draw_big_msg() {
    switch(cur_big_msg) {
    case BIG_MESSAGE_NONE: {
        return;
        
    } case BIG_MESSAGE_READY: {
        const float TEXT_W = game.win_w * 0.60f;
        const float TEXT_INITIAL_HEIGHT = 0.10;
        const float TEXT_VARIATION_DUR = 0.08f;
        const float TEXT_START_T = 0.15f;
        const float TEXT_MOVE_MID_T = 0.30f;
        const float TEXT_PAUSE_T = 0.60f;
        const float TEXT_SHRINK_T = 0.95f;
        const float t = big_msg_time / GAMEPLAY::BIG_MSG_READY_DUR;
        
        KeyframeInterpolator<float> ki_y(game.win_h * (-0.2f));
        ki_y.add(TEXT_START_T, game.win_h * (-0.2f));
        ki_y.add(TEXT_MOVE_MID_T, game.win_h * 0.40f, EASE_METHOD_IN);
        ki_y.add(TEXT_PAUSE_T, game.win_h / 2.0f, EASE_METHOD_OUT_ELASTIC);
        ki_y.add(TEXT_SHRINK_T, game.win_h / 2.0f);
        KeyframeInterpolator<float> ki_h(TEXT_INITIAL_HEIGHT);
        ki_h.add(TEXT_SHRINK_T, TEXT_INITIAL_HEIGHT * 1.4f);
        ki_h.add(1.0f, 0.0f, EASE_METHOD_IN);
        
        for(size_t c = 0; c < GAMEPLAY::BIG_MSG_READY_TEXT.size(); c++) {
            float char_ratio =
                c / ((float) GAMEPLAY::BIG_MSG_READY_TEXT.size() - 1);
            char_ratio = 1.0f - char_ratio;
            float x_offset = (TEXT_W / 2.0f) - (TEXT_W * char_ratio);
            float y = ki_y.get(t + char_ratio * TEXT_VARIATION_DUR);
            draw_text(
                string(1, GAMEPLAY::BIG_MSG_READY_TEXT[c]),
                game.sys_content.fnt_area_name,
                Point((game.win_w / 2.0f) + x_offset, y),
                Point(LARGE_FLOAT, game.win_h * ki_h.get(t)), COLOR_GOLD
            );
        }
        break;
        
    } case BIG_MESSAGE_GO: {

        const float TEXT_GROW_STOP_T = 0.10f;
        const float t = big_msg_time / GAMEPLAY::BIG_MSG_GO_DUR;
        
        KeyframeInterpolator<float> ki_h(0.0f);
        ki_h.add(TEXT_GROW_STOP_T, 0.20f, EASE_METHOD_OUT_ELASTIC);
        ki_h.add(1.0f, 0.22f);
        KeyframeInterpolator<float> ki_a(1.0f);
        ki_a.add(TEXT_GROW_STOP_T, 1.0f);
        ki_a.add(1.0f, 0.0f);
        
        draw_text(
            GAMEPLAY::BIG_MSG_GO_TEXT,
            game.sys_content.fnt_area_name,
            Point(game.win_w / 2.0f, game.win_h / 2.0f),
            Point(LARGE_FLOAT, game.win_h * ki_h.get(t)),
            change_alpha(COLOR_GOLD, 255 * ki_a.get(t))
        );
        break;
        
    } case BIG_MESSAGE_MISSION_CLEAR:
    case BIG_MESSAGE_MISSION_FAILED: {
        const string &TEXT =
            cur_big_msg == BIG_MESSAGE_MISSION_CLEAR ?
            GAMEPLAY::BIG_MSG_MISSION_CLEAR_TEXT :
            GAMEPLAY::BIG_MSG_MISSION_FAILED_TEXT;
        const float TEXT_W = game.win_w * 0.80f;
        const float TEXT_INITIAL_HEIGHT = 0.05f;
        const float TEXT_VARIATION_DUR = 0.08f;
        const float TEXT_MOVE_MID_T = 0.30f;
        const float TEXT_PAUSE_T = 0.50f;
        const float TEXT_FADE_T = 0.90f;
        const float t =
            cur_big_msg == BIG_MESSAGE_MISSION_CLEAR ?
            (big_msg_time / GAMEPLAY::BIG_MSG_MISSION_CLEAR_DUR) :
            (big_msg_time / GAMEPLAY::BIG_MSG_MISSION_FAILED_DUR);
            
        KeyframeInterpolator<float> ki_y(game.win_h * (-0.2f));
        ki_y.add(TEXT_MOVE_MID_T, game.win_h * 0.40f, EASE_METHOD_IN);
        ki_y.add(TEXT_PAUSE_T, game.win_h / 2.0f, EASE_METHOD_OUT_ELASTIC);
        KeyframeInterpolator<float> ki_h(TEXT_INITIAL_HEIGHT);
        ki_h.add(1.0f, TEXT_INITIAL_HEIGHT * 1.4f, EASE_METHOD_IN);
        KeyframeInterpolator<float> ki_a(1.0f);
        ki_a.add(TEXT_FADE_T, 1.0f);
        ki_a.add(1.0f, 0.0f);
        
        float alpha = ki_a.get(t);
        
        for(size_t c = 0; c < TEXT.size(); c++) {
            float char_ratio = c / ((float) TEXT.size() - 1);
            char_ratio = 1.0f - char_ratio;
            float x_offset = (TEXT_W / 2.0f) - (TEXT_W * char_ratio);
            float y = ki_y.get(t + char_ratio * TEXT_VARIATION_DUR);
            
            draw_text(
                string(1, TEXT[c]), game.sys_content.fnt_area_name,
                Point((game.win_w / 2.0f) + x_offset, y),
                Point(LARGE_FLOAT, game.win_h * ki_h.get(t)),
                change_alpha(COLOR_GOLD, 255 * alpha)
            );
        }
        break;
        
    }
    }
}


/**
 * @brief Draws any debug visualization tools useful for debugging.
 */
void GameplayState::draw_debug_tools() {
    //Tests using Dear ImGui.
    /*
    ImGui::GetIO().MouseDrawCursor = true;
    //GUI logic goes here.
    */
    
    //Raw analog stick viewer.
    /*
    const float RAW_STICK_VIEWER_X = 8;
    const float RAW_STICK_VIEWER_Y = 8;
    const float RAW_STICK_VIEWER_SIZE = 100;
    
    point raw_stick_coords;
    raw_stick_coords.x = game.controls.mgr.rawSticks[0][0][0];
    raw_stick_coords.y = game.controls.mgr.rawSticks[0][0][1];
    float raw_stick_angle;
    float raw_stick_mag;
    coordinates_to_angle(
        raw_stick_coords, &raw_stick_angle, &raw_stick_mag
    );
    al_draw_filled_rectangle(
        RAW_STICK_VIEWER_X,
        RAW_STICK_VIEWER_Y,
        RAW_STICK_VIEWER_X + RAW_STICK_VIEWER_SIZE,
        RAW_STICK_VIEWER_Y + RAW_STICK_VIEWER_SIZE,
        al_map_rgba(0, 0, 0, 200)
    );
    al_draw_circle(
        RAW_STICK_VIEWER_X + RAW_STICK_VIEWER_SIZE / 2.0f,
        RAW_STICK_VIEWER_Y + RAW_STICK_VIEWER_SIZE / 2.0f,
        RAW_STICK_VIEWER_SIZE / 2.0f,
        raw_stick_mag >= 0.99f ?
        al_map_rgba(240, 64, 64, 200) :
        al_map_rgba(240, 240, 240, 200),
        1
    );
    al_draw_line(
        RAW_STICK_VIEWER_X,
        RAW_STICK_VIEWER_Y + RAW_STICK_VIEWER_SIZE / 2.0f,
        RAW_STICK_VIEWER_X + RAW_STICK_VIEWER_SIZE,
        RAW_STICK_VIEWER_Y + RAW_STICK_VIEWER_SIZE / 2.0f,
        fabs(raw_stick_coords.y) <= 0.01f ?
        al_map_rgba(240, 64, 64, 200) :
        al_map_rgba(240, 240, 240, 200),
        1
    );
    al_draw_line(
        RAW_STICK_VIEWER_X + RAW_STICK_VIEWER_SIZE / 2.0f,
        RAW_STICK_VIEWER_Y,
        RAW_STICK_VIEWER_X + RAW_STICK_VIEWER_SIZE / 2.0f,
        RAW_STICK_VIEWER_Y + RAW_STICK_VIEWER_SIZE,
        fabs(raw_stick_coords.x) <= 0.01f ?
        al_map_rgba(240, 64, 64, 200) :
        al_map_rgba(240, 240, 240, 200),
        1
    );
    point raw_draw_coords =
        raw_stick_coords * RAW_STICK_VIEWER_SIZE / 2.0f;
    al_draw_filled_circle(
        RAW_STICK_VIEWER_X + RAW_STICK_VIEWER_SIZE / 2.0f +
        raw_draw_coords.x,
        RAW_STICK_VIEWER_Y + RAW_STICK_VIEWER_SIZE / 2.0f +
        raw_draw_coords.y,
        3.5f, al_map_rgb(255, 64, 64)
    );
    al_draw_filled_rectangle(
        RAW_STICK_VIEWER_X,
        RAW_STICK_VIEWER_Y + RAW_STICK_VIEWER_SIZE + 1,
        RAW_STICK_VIEWER_X + RAW_STICK_VIEWER_SIZE,
        RAW_STICK_VIEWER_Y + RAW_STICK_VIEWER_SIZE + 18,
        al_map_rgba(0, 0, 0, 200)
    );
    al_draw_text(
        game.sys_content.fnt_builtin,
        al_map_rgb(255, 64, 64),
        RAW_STICK_VIEWER_X, RAW_STICK_VIEWER_Y + RAW_STICK_VIEWER_SIZE + 1,
        ALLEGRO_ALIGN_LEFT,
        (
            box_string(
                (raw_stick_coords.x >= 0.0f ? " " : "") +
                f2s(raw_stick_coords.x), 6
            ) + " " + box_string(
                (raw_stick_coords.y >= 0.0f ? " " : "") +
                f2s(raw_stick_coords.y), 6
            )
        ).c_str()
    );
    al_draw_text(
        game.sys_content.fnt_builtin,
        al_map_rgb(255, 64, 64),
        RAW_STICK_VIEWER_X, RAW_STICK_VIEWER_Y + RAW_STICK_VIEWER_SIZE + 1 + 8,
        ALLEGRO_ALIGN_LEFT,
        (
            box_string(
                (raw_stick_angle >= 0.0f ? " " : "") +
                f2s(raw_stick_angle), 6
            ) + " " + box_string(
                (raw_stick_mag >= 0.0f ? " " : "") +
                f2s(raw_stick_mag), 6
            )
        ).c_str()
    );
    */
    
    //Clean analog stick viewer.
    /*
    const float CLEAN_STICK_VIEWER_X = 116;
    const float CLEAN_STICK_VIEWER_Y = 8;
    const float CLEAN_STICK_VIEWER_SIZE = 100;
    
    point clean_stick_coords;
    clean_stick_coords.x =
        game.controls.get_player_action_type_value(PLAYER_ACTION_TYPE_RIGHT) -
        game.controls.get_player_action_type_value(PLAYER_ACTION_TYPE_LEFT);
    clean_stick_coords.y =
        game.controls.get_player_action_type_value(PLAYER_ACTION_TYPE_DOWN) -
        game.controls.get_player_action_type_value(PLAYER_ACTION_TYPE_UP);
    float clean_stick_angle;
    float clean_stick_mag;
    coordinates_to_angle(
        clean_stick_coords, &clean_stick_angle, &clean_stick_mag
    );
    al_draw_filled_rectangle(
        CLEAN_STICK_VIEWER_X,
        CLEAN_STICK_VIEWER_Y,
        CLEAN_STICK_VIEWER_X + CLEAN_STICK_VIEWER_SIZE,
        CLEAN_STICK_VIEWER_Y + CLEAN_STICK_VIEWER_SIZE,
        al_map_rgba(0, 0, 0, 200)
    );
    al_draw_circle(
        CLEAN_STICK_VIEWER_X + CLEAN_STICK_VIEWER_SIZE / 2.0f,
        CLEAN_STICK_VIEWER_Y + CLEAN_STICK_VIEWER_SIZE / 2.0f,
        CLEAN_STICK_VIEWER_SIZE / 2.0f,
        clean_stick_mag >= 0.99f ?
        al_map_rgba(240, 64, 64, 200) :
        al_map_rgba(240, 240, 240, 200),
        1
    );
    al_draw_line(
        CLEAN_STICK_VIEWER_X,
        CLEAN_STICK_VIEWER_Y + CLEAN_STICK_VIEWER_SIZE / 2.0f,
        CLEAN_STICK_VIEWER_X + CLEAN_STICK_VIEWER_SIZE,
        CLEAN_STICK_VIEWER_Y + CLEAN_STICK_VIEWER_SIZE / 2.0f,
        fabs(clean_stick_coords.y) <= 0.01f ?
        al_map_rgba(240, 64, 64, 200) :
        al_map_rgba(240, 240, 240, 200),
        1
    );
    al_draw_line(
        CLEAN_STICK_VIEWER_X + CLEAN_STICK_VIEWER_SIZE / 2.0f,
        CLEAN_STICK_VIEWER_Y,
        CLEAN_STICK_VIEWER_X + CLEAN_STICK_VIEWER_SIZE / 2.0f,
        CLEAN_STICK_VIEWER_Y + CLEAN_STICK_VIEWER_SIZE,
        fabs(clean_stick_coords.x) <= 0.01f ?
        al_map_rgba(240, 64, 64, 200) :
        al_map_rgba(240, 240, 240, 200),
        1
    );
    point clean_draw_coords =
        clean_stick_coords * CLEAN_STICK_VIEWER_SIZE / 2.0f;
    al_draw_filled_circle(
        CLEAN_STICK_VIEWER_X + CLEAN_STICK_VIEWER_SIZE / 2.0f +
        clean_draw_coords.x,
        CLEAN_STICK_VIEWER_Y + CLEAN_STICK_VIEWER_SIZE / 2.0f +
        clean_draw_coords.y,
        3.5f, al_map_rgb(255, 64, 64)
    );
    al_draw_filled_rectangle(
        CLEAN_STICK_VIEWER_X,
        CLEAN_STICK_VIEWER_Y + CLEAN_STICK_VIEWER_SIZE + 1,
        CLEAN_STICK_VIEWER_X + CLEAN_STICK_VIEWER_SIZE,
        CLEAN_STICK_VIEWER_Y + CLEAN_STICK_VIEWER_SIZE + 18,
        al_map_rgba(0, 0, 0, 200)
    );
    al_draw_text(
        game.sys_content.fnt_builtin,
        al_map_rgb(255, 64, 64),
        CLEAN_STICK_VIEWER_X,
        CLEAN_STICK_VIEWER_Y + CLEAN_STICK_VIEWER_SIZE + 1,
        ALLEGRO_ALIGN_LEFT,
        (
            box_string(
                (clean_stick_coords.x >= 0.0f ? " " : "") +
                f2s(clean_stick_coords.x), 6
            ) + " " + box_string(
                (clean_stick_coords.y >= 0.0f ? " " : "") +
                f2s(clean_stick_coords.y), 6
            )
        ).c_str()
    );
    al_draw_text(
        game.sys_content.fnt_builtin,
        al_map_rgb(255, 64, 64),
        CLEAN_STICK_VIEWER_X, CLEAN_STICK_VIEWER_Y + CLEAN_STICK_VIEWER_SIZE +
        1 + 8,
        ALLEGRO_ALIGN_LEFT,
        (
            box_string(
                (clean_stick_angle >= 0.0f ? " " : "") +
                f2s(clean_stick_angle), 6
            ) + " " + box_string(
                (clean_stick_mag >= 0.0f ? " " : "") +
                f2s(clean_stick_mag), 6
            )
        ).c_str()
    );
    */
    
    //Group stuff.
    /*
    al_use_transform(&game.world_to_screen_transform);
    for(size_t m = 0; m < cur_leader_ptr->group->members.size(); m++) {
        point offset = cur_leader_ptr->group->get_spot_offset(m);
        al_draw_filled_circle(
            cur_leader_ptr->group->anchor.x + offset.x,
            cur_leader_ptr->group->anchor.y + offset.y,
            3.0f,
            al_map_rgba(0, 0, 0, 192)
        );
    }
    al_draw_circle(
        cur_leader_ptr->group->anchor.x,
        cur_leader_ptr->group->anchor.y,
        3.0f,
        cur_leader_ptr->group->mode == Group::MODE_SHUFFLE ?
        al_map_rgba(0, 255, 0, 192) :
        cur_leader_ptr->group->mode == Group::MODE_FOLLOW_BACK ?
        al_map_rgba(255, 255, 0, 192) :
        al_map_rgba(255, 0, 0, 192),
        2.0f
    );
    
    point group_mid_point =
        cur_leader_ptr->group->anchor +
        rotate_point(
            point(cur_leader_ptr->group->radius, 0.0f),
            cur_leader_ptr->group->anchor_angle
        );
    al_draw_filled_circle(
        group_mid_point.x,
        group_mid_point.y,
        3.0f,
        al_map_rgb(0, 0, 255)
    );
    */
}


/**
 * @brief Draws the in-game text.
 */
void GameplayState::draw_ingame_text() {
    //Mob things.
    size_t n_mobs = mobs.all.size();
    for(size_t m = 0; m < n_mobs; m++) {
        Mob* mob_ptr = mobs.all[m];
        
        //Fractions and health.
        if(mob_ptr->health_wheel) {
            mob_ptr->health_wheel->draw();
        }
        if(mob_ptr->fraction) {
            mob_ptr->fraction->draw();
        }
        
        //Maker tool -- draw hitboxes.
        if(game.maker_tools.hitboxes) {
            Sprite* s;
            mob_ptr->get_sprite_data(&s, nullptr, nullptr);
            if(s) {
                for(size_t h = 0; h < s->hitboxes.size(); h++) {
                    Hitbox* h_ptr = &s->hitboxes[h];
                    ALLEGRO_COLOR hc;
                    switch(h_ptr->type) {
                    case HITBOX_TYPE_NORMAL: {
                        hc = al_map_rgba(0, 128, 0, 192); //Green.
                        break;
                    } case HITBOX_TYPE_ATTACK: {
                        hc = al_map_rgba(128, 0, 0, 192); //Red.
                        break;
                    } case HITBOX_TYPE_DISABLED: {
                        hc = al_map_rgba(128, 128, 0, 192); //Yellow.
                        break;
                    } default:
                        hc = COLOR_BLACK;
                        break;
                    }
                    Point p =
                        mob_ptr->pos + rotate_point(h_ptr->pos, mob_ptr->angle);
                    al_draw_filled_circle(p.x, p.y, h_ptr->radius, hc);
                }
            }
        }
        
        //Maker tool -- draw collision.
        if(game.maker_tools.collision) {
            if(mob_ptr->type->pushes_with_hitboxes) {
                Sprite* s;
                mob_ptr->get_sprite_data(&s, nullptr, nullptr);
                if(s) {
                    for(size_t h = 0; h < s->hitboxes.size(); h++) {
                        Hitbox* h_ptr = &s->hitboxes[h];
                        Point p =
                            mob_ptr->pos +
                            rotate_point(h_ptr->pos, mob_ptr->angle);
                        al_draw_circle(
                            p.x, p.y,
                            h_ptr->radius, COLOR_WHITE, 1
                        );
                    }
                }
            } else if(mob_ptr->rectangular_dim.x != 0) {
                Point tl(
                    -mob_ptr->rectangular_dim.x / 2.0f,
                    -mob_ptr->rectangular_dim.y / 2.0f
                );
                Point br(
                    mob_ptr->rectangular_dim.x / 2.0f,
                    mob_ptr->rectangular_dim.y / 2.0f
                );
                vector<Point> rect_vertices {
                    rotate_point(tl, mob_ptr->angle) +
                    mob_ptr->pos,
                    rotate_point(Point(tl.x, br.y),  mob_ptr->angle) +
                    mob_ptr->pos,
                    rotate_point(br, mob_ptr->angle) +
                    mob_ptr->pos,
                    rotate_point(Point(br.x, tl.y), mob_ptr->angle) +
                    mob_ptr->pos
                };
                float vertices[] {
                    rect_vertices[0].x,
                    rect_vertices[0].y,
                    rect_vertices[1].x,
                    rect_vertices[1].y,
                    rect_vertices[2].x,
                    rect_vertices[2].y,
                    rect_vertices[3].x,
                    rect_vertices[3].y
                };
                
                al_draw_polygon(vertices, 4, 0, COLOR_WHITE, 1, 10);
            } else {
                al_draw_circle(
                    mob_ptr->pos.x, mob_ptr->pos.y,
                    mob_ptr->radius, COLOR_WHITE, 1
                );
            }
        }
    }
    
    //Maker tool -- draw path info.
    if(
        game.maker_tools.info_lock &&
        game.maker_tools.path_info &&
        game.maker_tools.info_lock->path_info
    ) {
        Path* path = game.maker_tools.info_lock->path_info;
        Point target_pos =
            has_flag(path->settings.flags, PATH_FOLLOW_FLAG_FOLLOW_MOB) ?
            path->settings.target_mob->pos :
            path->settings.target_point;
            
        if(!path->path.empty()) {
        
            //Faint lines for the entire path.
            for(size_t s = 0; s < path->path.size() - 1; s++) {
                bool is_blocked = false;
                PathLink* l_ptr = path->path[s]->get_link(path->path[s + 1]);
                auto l_it = path_mgr.obstructions.find(l_ptr);
                if(l_it != path_mgr.obstructions.end()) {
                    is_blocked = !l_it->second.empty();
                }
                
                al_draw_line(
                    path->path[s]->pos.x,
                    path->path[s]->pos.y,
                    path->path[s + 1]->pos.x,
                    path->path[s + 1]->pos.y,
                    is_blocked ?
                    al_map_rgba(200, 0, 0, 150) :
                    al_map_rgba(0, 0, 200, 150),
                    2.0f
                );
            }
            
            //Colored circles for the first and last stops.
            al_draw_filled_circle(
                path->path[0]->pos.x,
                path->path[0]->pos.y,
                16.0f,
                al_map_rgba(192, 0, 0, 200)
            );
            al_draw_filled_circle(
                path->path.back()->pos.x,
                path->path.back()->pos.y,
                16.0f,
                al_map_rgba(0, 192, 0, 200)
            );
            
        }
        
        if(
            path->result == PATH_RESULT_DIRECT ||
            path->result == PATH_RESULT_DIRECT_NO_STOPS ||
            path->cur_path_stop_idx == path->path.size()
        ) {
            bool is_blocked = path->block_reason != PATH_BLOCK_REASON_NONE;
            //Line directly to the target.
            al_draw_line(
                game.maker_tools.info_lock->pos.x,
                game.maker_tools.info_lock->pos.y,
                target_pos.x,
                target_pos.y,
                is_blocked ?
                al_map_rgba(255, 0, 0, 200) :
                al_map_rgba(0, 0, 255, 200),
                4.0f
            );
        } else if(path->cur_path_stop_idx < path->path.size()) {
            bool is_blocked = path->block_reason != PATH_BLOCK_REASON_NONE;
            //Line to the next stop, and circle for the next stop in blue.
            al_draw_line(
                game.maker_tools.info_lock->pos.x,
                game.maker_tools.info_lock->pos.y,
                path->path[path->cur_path_stop_idx]->pos.x,
                path->path[path->cur_path_stop_idx]->pos.y,
                is_blocked ?
                al_map_rgba(255, 0, 0, 200) :
                al_map_rgba(0, 0, 255, 200),
                4.0f
            );
            al_draw_filled_circle(
                path->path[path->cur_path_stop_idx]->pos.x,
                path->path[path->cur_path_stop_idx]->pos.y,
                10.0f,
                is_blocked ?
                al_map_rgba(192, 0, 0, 200) :
                al_map_rgba(0, 0, 192, 200)
            );
        }
        
        //Square on the target spot, and target distance.
        al_draw_filled_rectangle(
            target_pos.x - 8.0f,
            target_pos.y - 8.0f,
            target_pos.x + 8.0f,
            target_pos.y + 8.0f,
            al_map_rgba(0, 192, 0, 200)
        );
        al_draw_circle(
            target_pos.x,
            target_pos.y,
            path->settings.final_target_distance,
            al_map_rgba(0, 255, 0, 200),
            1.0f
        );
        
        //Diamonds for faked starts and ends.
        if(has_flag(path->settings.flags, PATH_FOLLOW_FLAG_FAKED_START)) {
            draw_filled_diamond(
                path->settings.faked_start, 8, al_map_rgba(255, 0, 0, 200)
            );
        }
        if(has_flag(path->settings.flags, PATH_FOLLOW_FLAG_FAKED_END)) {
            draw_filled_diamond(
                path->settings.faked_end, 8, al_map_rgba(0, 255, 0, 200)
            );
        }
    }
    
    notification.draw();
}


/**
 * @brief Draws the leader's cursor and associated effects.
 *
 * @param color Color to tint it by.
 */
void GameplayState::draw_leader_cursor(const ALLEGRO_COLOR &color) {
    if(!cur_leader_ptr) return;
    
    size_t n_arrows = cur_leader_ptr->swarm_arrows.size();
    for(size_t a = 0; a < n_arrows; a++) {
        Point pos(
            cos(swarm_angle) * cur_leader_ptr->swarm_arrows[a],
            sin(swarm_angle) * cur_leader_ptr->swarm_arrows[a]
        );
        float alpha =
            64 + std::min(
                191,
                (int) (
                    191 *
                    (cur_leader_ptr->swarm_arrows[a] /
                     (game.config.rules.cursor_max_dist * 0.4))
                )
            );
        draw_bitmap(
            game.sys_content.bmp_swarm_arrow,
            cur_leader_ptr->pos + pos,
            Point(
                16 * (1 + cur_leader_ptr->swarm_arrows[a] /
                      game.config.rules.cursor_max_dist),
                -1
            ),
            swarm_angle,
            map_alpha(alpha)
        );
    }
    
    size_t n_rings = whistle.rings.size();
    float cursor_angle =
        get_angle(cur_leader_ptr->pos, leader_cursor_w);
    float cursor_distance =
        Distance(cur_leader_ptr->pos, leader_cursor_w).to_float();
    for(size_t r = 0; r < n_rings; r++) {
        Point pos(
            cur_leader_ptr->pos.x + cos(cursor_angle) * whistle.rings[r],
            cur_leader_ptr->pos.y + sin(cursor_angle) * whistle.rings[r]
        );
        float ring_to_whistle_distance = cursor_distance - whistle.rings[r];
        float scale =
            interpolate_number(
                ring_to_whistle_distance,
                0, cursor_distance,
                whistle.radius * 2, 0
            );
        float alpha =
            interpolate_number(
                ring_to_whistle_distance,
                0, cursor_distance,
                0, 100
            );
        unsigned char n = whistle.ring_colors[r];
        draw_bitmap(
            game.sys_content.bmp_bright_ring,
            pos,
            Point(scale),
            0.0f,
            al_map_rgba(
                WHISTLE::RING_COLORS[n][0],
                WHISTLE::RING_COLORS[n][1],
                WHISTLE::RING_COLORS[n][2],
                alpha
            )
        );
    }
    
    if(whistle.radius > 0 || whistle.fade_timer.time_left > 0.0f) {
        al_draw_filled_circle(
            whistle.center.x, whistle.center.y,
            whistle.radius,
            al_map_rgba(48, 128, 120, 64)
        );
        
        unsigned char n_dots = 16 * WHISTLE::N_DOT_COLORS;
        for(unsigned char d = 0; d < WHISTLE::N_DOT_COLORS; d++) {
            for(unsigned char d2 = 0; d2 < 16; d2++) {
                unsigned char current_dot = d2 * WHISTLE::N_DOT_COLORS + d;
                float angle =
                    TAU / n_dots *
                    current_dot -
                    WHISTLE::DOT_SPIN_SPEED * area_time_passed;
                    
                Point dot_pos(
                    whistle.center.x +
                    cos(angle) * whistle.dot_radius[d],
                    whistle.center.y +
                    sin(angle) * whistle.dot_radius[d]
                );
                
                ALLEGRO_COLOR dot_color =
                    al_map_rgb(
                        WHISTLE::DOT_COLORS[d][0],
                        WHISTLE::DOT_COLORS[d][1],
                        WHISTLE::DOT_COLORS[d][2]
                    );
                unsigned char dot_alpha = 255;
                if(whistle.fade_timer.time_left > 0.0f) {
                    dot_alpha = 255 * whistle.fade_timer.get_ratio_left();
                }
                
                draw_bitmap(
                    game.sys_content.bmp_bright_circle,
                    dot_pos, Point(5.0f),
                    0.0f, change_alpha(dot_color, dot_alpha)
                );
            }
        }
    }
    
    //Leader cursor.
    Point bmp_cursor_size = get_bitmap_dimensions(game.sys_content.bmp_cursor);
    
    draw_bitmap(
        game.sys_content.bmp_cursor,
        leader_cursor_w,
        bmp_cursor_size / 2.0f,
        cursor_angle,
        change_color_lighting(
            color,
            cursor_height_diff_light
        )
    );
    
    //Throw preview.
    draw_throw_preview();
    
    //Standby type count.
    size_t n_standby_pikmin = 0;
    if(cur_leader_ptr->group->cur_standby_type) {
        for(
            size_t m = 0; m < cur_leader_ptr->group->members.size(); m++
        ) {
            Mob* m_ptr = cur_leader_ptr->group->members[m];
            if(
                m_ptr->subgroup_type_ptr ==
                cur_leader_ptr->group->cur_standby_type
            ) {
                n_standby_pikmin++;
            }
        }
    }
    
    al_use_transform(&game.identity_transform);
    
    float count_offset =
        std::max(bmp_cursor_size.x, bmp_cursor_size.y) * 0.18f * game.cam.zoom;
        
    if(n_standby_pikmin > 0) {
        draw_text(
            i2s(n_standby_pikmin), game.sys_content.fnt_cursor_counter,
            leader_cursor_s +
            Point(count_offset),
            Point(LARGE_FLOAT, game.win_h * 0.02f), color,
            ALLEGRO_ALIGN_LEFT, V_ALIGN_MODE_TOP
        );
    }
    
    al_use_transform(&game.world_to_screen_transform);
}


/**
 * @brief Draws the full-screen effects that will represent lighting.
 */
void GameplayState::draw_lighting_filter() {
    al_use_transform(&game.identity_transform);
    
    //Draw the fog effect.
    ALLEGRO_COLOR fog_c =
        game.cur_area_data->weather_condition.get_fog_color();
    if(fog_c.a > 0) {
        //Start by drawing the central fog fade out effect.
        
        Point fog_top_left =
            game.cam.pos -
            Point(
                game.cur_area_data->weather_condition.fog_far,
                game.cur_area_data->weather_condition.fog_far
            );
        Point fog_bottom_right =
            game.cam.pos +
            Point(
                game.cur_area_data->weather_condition.fog_far,
                game.cur_area_data->weather_condition.fog_far
            );
        al_transform_coordinates(
            &game.world_to_screen_transform,
            &fog_top_left.x, &fog_top_left.y
        );
        al_transform_coordinates(
            &game.world_to_screen_transform,
            &fog_bottom_right.x, &fog_bottom_right.y
        );
        
        if(bmp_fog) {
            draw_bitmap(
                bmp_fog,
                (fog_top_left + fog_bottom_right) / 2,
                (fog_bottom_right - fog_top_left),
                0, fog_c
            );
        }
        
        //Now draw the fully opaque fog around the central fade.
        //Top-left and top-center.
        al_draw_filled_rectangle(
            0, 0,
            fog_bottom_right.x, fog_top_left.y,
            fog_c
        );
        //Top-right and center-right.
        al_draw_filled_rectangle(
            fog_bottom_right.x, 0,
            game.win_w, fog_bottom_right.y,
            fog_c
        );
        //Bottom-right and bottom-center.
        al_draw_filled_rectangle(
            fog_top_left.x, fog_bottom_right.y,
            game.win_w, game.win_h,
            fog_c
        );
        //Bottom-left and center-left.
        al_draw_filled_rectangle(
            0, fog_top_left.y,
            fog_top_left.x, game.win_h,
            fog_c
        );
        
    }
    
    //Draw the daylight.
    ALLEGRO_COLOR daylight_c =
        game.cur_area_data->weather_condition.get_daylight_color();
    if(daylight_c.a > 0) {
        al_draw_filled_rectangle(0, 0, game.win_w, game.win_h, daylight_c);
    }
    
    //Draw the blackout effect.
    unsigned char blackout_s =
        game.cur_area_data->weather_condition.get_blackout_strength();
    if(blackout_s > 0) {
        //First, we'll create the lightmap.
        //This is inverted (white = darkness, black = light), because we'll
        //apply it to the screen using a subtraction operation.
        al_set_target_bitmap(lightmap_bmp);
        
        //For starters, the whole screen is dark (white in the map).
        al_clear_to_color(map_gray(blackout_s));
        
        int old_op, old_src, old_dst, old_aop, old_asrc, old_adst;
        al_get_separate_blender(
            &old_op, &old_src, &old_dst, &old_aop, &old_asrc, &old_adst
        );
        al_set_separate_blender(
            ALLEGRO_DEST_MINUS_SRC, ALLEGRO_ONE, ALLEGRO_ONE,
            ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE
        );
        
        //Then, find out spotlights, and draw
        //their lights on the map (as black).
        al_hold_bitmap_drawing(true);
        for(size_t m = 0; m < mobs.all.size(); m++) {
            Mob* m_ptr = mobs.all[m];
            if(
                has_flag(m_ptr->flags, MOB_FLAG_HIDDEN) ||
                m_ptr->type->blackout_radius == 0.0f
            ) {
                continue;
            }
            
            Point pos = m_ptr->pos;
            al_transform_coordinates(
                &game.world_to_screen_transform,
                &pos.x, &pos.y
            );
            float radius = 4.0f * game.cam.zoom;
            
            if(m_ptr->type->blackout_radius > 0.0f) {
                radius *= m_ptr->type->blackout_radius;
            } else {
                radius *= m_ptr->radius;
            }
            
            al_draw_scaled_bitmap(
                game.sys_content.bmp_spotlight,
                0, 0, 64, 64,
                pos.x - radius, pos.y - radius,
                radius * 2.0, radius * 2.0,
                0
            );
        }
        al_hold_bitmap_drawing(false);
        
        //Now, simply darken the screen using the map.
        al_set_target_backbuffer(game.display);
        
        al_draw_bitmap(lightmap_bmp, 0, 0, 0);
        
        al_set_separate_blender(
            old_op, old_src, old_dst, old_aop, old_asrc, old_adst
        );
        
    }
    
}


/**
 * @brief Draws a gameplay message box.
 */
void GameplayState::draw_gameplay_message_box() {
    //Mouse cursor.
    draw_mouse_cursor(GAME::CURSOR_STANDARD_COLOR);
    
    al_use_transform(&game.identity_transform);
    
    //Transition things.
    float transition_ratio =
        msg_box->transition_in ?
        msg_box->transition_timer / GAMEPLAY::MENU_ENTRY_HUD_MOVE_TIME :
        (1 - msg_box->transition_timer / GAMEPLAY::MENU_EXIT_HUD_MOVE_TIME);
    int line_height = al_get_font_line_height(game.sys_content.fnt_standard);
    float box_height = line_height * 4;
    float offset =
        box_height * ease(EASE_METHOD_IN, transition_ratio);
        
    //Draw a rectangle to darken gameplay.
    al_draw_filled_rectangle(
        0.0f, 0.0f,
        game.win_w, game.win_h,
        al_map_rgba(0, 0, 0, 64 * (1 - transition_ratio))
    );
    
    //Draw the message box proper.
    draw_textured_box(
        Point(
            game.win_w / 2,
            game.win_h - (box_height / 2.0f) - 4 + offset
        ),
        Point(game.win_w - 16, box_height),
        game.sys_content.bmp_bubble_box
    );
    
    //Draw the speaker's icon, if any.
    if(msg_box->speaker_icon) {
        draw_bitmap(
            msg_box->speaker_icon,
            Point(
                40,
                game.win_h - box_height - 16 + offset
            ),
            Point(48.0f)
        );
        draw_bitmap(
            hud->bmp_bubble,
            Point(
                40,
                game.win_h - box_height - 16 + offset
            ),
            Point(64.0f)
        );
    }
    
    //Draw the button to advance, if it's time.
    draw_player_input_source_icon(
        game.sys_content.fnt_slim,
        game.controls.find_bind(PLAYER_ACTION_TYPE_THROW).inputSource,
        true,
        Point(
            game.win_w - (GAMEPLAY_MSG_BOX::MARGIN + GAMEPLAY_MSG_BOX::PADDING + 8.0f),
            game.win_h - (GAMEPLAY_MSG_BOX::MARGIN + GAMEPLAY_MSG_BOX::PADDING + 8.0f) +
            offset
        ),
        Point(32.0f),
        msg_box->advance_button_alpha * 255
    );
    
    //Draw the message's text.
    size_t token_idx = 0;
    for(size_t l = 0; l < 3; l++) {
        size_t line_idx = msg_box->cur_section * 3 + l;
        if(line_idx >= msg_box->tokens_per_line.size()) {
            break;
        }
        
        //Figure out what scaling is necessary, if any.
        unsigned int total_width = 0;
        float x_scale = 1.0f;
        for(size_t t = 0; t < msg_box->tokens_per_line[line_idx].size(); t++) {
            total_width += msg_box->tokens_per_line[line_idx][t].width;
        }
        const float max_text_width = (GAMEPLAY_MSG_BOX::MARGIN + GAMEPLAY_MSG_BOX::PADDING) * 2;
        if(total_width > game.win_w - max_text_width) {
            x_scale = (game.win_w - max_text_width) / total_width;
        }
        
        float caret =
            GAMEPLAY_MSG_BOX::MARGIN + GAMEPLAY_MSG_BOX::PADDING;
        float start_y =
            game.win_h - line_height * 4 + GAMEPLAY_MSG_BOX::PADDING + offset;
            
        for(size_t t = 0; t < msg_box->tokens_per_line[line_idx].size(); t++) {
            token_idx++;
            if(token_idx >= msg_box->cur_token) break;
            StringToken &cur_token = msg_box->tokens_per_line[line_idx][t];
            
            float x = caret;
            float y = start_y + line_height * l;
            unsigned char alpha = 255;
            float this_token_anim_time;
            
            //Change the token's position and alpha, if it needs animating.
            //First, check for the typing animation.
            if(token_idx >= msg_box->skipped_at_token) {
                this_token_anim_time = msg_box->total_skip_anim_time;
            } else {
                this_token_anim_time =
                    msg_box->total_token_anim_time -
                    ((token_idx + 1) * game.config.aesthetic_gen.g_msg_ch_interval);
            }
            if(
                this_token_anim_time > 0 &&
                this_token_anim_time < GAMEPLAY_MSG_BOX::TOKEN_ANIM_DURATION
            ) {
                float ratio =
                    this_token_anim_time / GAMEPLAY_MSG_BOX::TOKEN_ANIM_DURATION;
                x +=
                    GAMEPLAY_MSG_BOX::TOKEN_ANIM_X_AMOUNT *
                    ease(EASE_METHOD_UP_AND_DOWN_ELASTIC, ratio);
                y +=
                    GAMEPLAY_MSG_BOX::TOKEN_ANIM_Y_AMOUNT *
                    ease(EASE_METHOD_UP_AND_DOWN_ELASTIC, ratio);
                alpha = ratio * 255;
            }
            
            //Now, for the swiping animation.
            if(msg_box->swipe_timer > 0.0f) {
                float ratio =
                    1 - (msg_box->swipe_timer / GAMEPLAY_MSG_BOX::TOKEN_SWIPE_DURATION);
                x += GAMEPLAY_MSG_BOX::TOKEN_SWIPE_X_AMOUNT * ratio;
                y += GAMEPLAY_MSG_BOX::TOKEN_SWIPE_Y_AMOUNT * ratio;
                alpha = std::max(0, (signed int) (alpha - ratio * 255));
            }
            
            //Actually draw it now.
            float token_final_width = cur_token.width * x_scale;
            switch(cur_token.type) {
            case STRING_TOKEN_CHAR: {
                draw_text(
                    cur_token.content, game.sys_content.fnt_standard,
                    Point(x, y),
                    Point(token_final_width, LARGE_FLOAT),
                    map_alpha(alpha),
                    ALLEGRO_ALIGN_LEFT, V_ALIGN_MODE_TOP, 0,
                    Point(x_scale, 1.0f)
                );
                break;
            }
            case STRING_TOKEN_CONTROL_BIND: {
                draw_player_input_source_icon(
                    game.sys_content.fnt_slim,
                    game.controls.find_bind(cur_token.content).inputSource,
                    true,
                    Point(
                        x + token_final_width / 2.0f,
                        y + line_height / 2.0f
                    ),
                    Point(token_final_width, line_height)
                );
                break;
            }
            default: {
                break;
            }
            }
            caret += token_final_width;
        }
    }
}


/**
 * @brief Draws the current Onion menu.
 */
void GameplayState::draw_onion_menu() {
    al_draw_filled_rectangle(
        0, 0, game.win_w, game.win_h,
        al_map_rgba(24, 64, 60, 220 * onion_menu->bg_alpha_mult)
    );
    
    onion_menu->gui.draw();
    
    draw_mouse_cursor(GAME::CURSOR_STANDARD_COLOR);
}


/**
 * @brief Draws the current pause menu.
 */
void GameplayState::draw_pause_menu() {
    al_draw_filled_rectangle(
        0, 0, game.win_w, game.win_h,
        al_map_rgba(24, 64, 60, 200 * pause_menu->bg_alpha_mult)
    );
    
    pause_menu->draw();
    
    draw_mouse_cursor(GAME::CURSOR_STANDARD_COLOR);
}


/**
 * @brief Draws the precipitation.
 */
void GameplayState::draw_precipitation() {
    if(
        game.cur_area_data->weather_condition.precipitation_type !=
        PRECIPITATION_TYPE_NONE
    ) {
        size_t n_precipitation_particles = precipitation.size();
        for(size_t p = 0; p < n_precipitation_particles; p++) {
            al_draw_filled_circle(
                precipitation[p].x, precipitation[p].y,
                3, COLOR_WHITE
            );
        }
    }
}


/**
 * @brief Draws system stuff.
 */
void GameplayState::draw_system_stuff() {
    if(!game.maker_tools.info_print_text.empty()) {
        float alpha_mult = 1;
        if(
            game.maker_tools.info_print_timer.time_left <
            game.maker_tools.info_print_fade_duration
        ) {
            alpha_mult =
                game.maker_tools.info_print_timer.time_left /
                game.maker_tools.info_print_fade_duration;
        }
        
        size_t n_lines =
            split(game.maker_tools.info_print_text, "\n", true).size();
        int fh = al_get_font_line_height(game.sys_content.fnt_builtin);
        //We add n_lines - 1 because there is a 1px gap between each line.
        int total_height = (int) n_lines * fh + (int) (n_lines - 1);
        
        al_draw_filled_rectangle(
            0, 0, game.win_w, total_height + 16,
            al_map_rgba(0, 0, 0, 96 * alpha_mult)
        );
        draw_text_lines(
            game.maker_tools.info_print_text,
            game.sys_content.fnt_builtin,
            Point(8.0f),
            Point(LARGE_FLOAT),
            al_map_rgba(255, 255, 255, 128 * alpha_mult),
            ALLEGRO_ALIGN_LEFT, V_ALIGN_MODE_TOP, TEXT_SETTING_FLAG_CANT_GROW
        );
    }
    
    if(game.show_system_info && !game.framerate_history.empty()) {
        //Draw the framerate chart.
        al_draw_filled_rectangle(
            game.win_w - GAME::FRAMERATE_HISTORY_SIZE, 0,
            game.win_w, 100,
            al_map_rgba(0, 0, 0, 192)
        );
        double chart_min = 1.0f; //1 FPS.
        double chart_max =
            game.options.advanced.target_fps + game.options.advanced.target_fps * 0.05f;
        for(size_t f = 0; f < game.framerate_history.size(); f++) {
            float fps =
                std::min(
                    (float) (1.0f / game.framerate_history[f]),
                    (float) game.options.advanced.target_fps
                );
            float fps_y =
                interpolate_number(
                    fps,
                    chart_min, chart_max,
                    0, 100
                );
            al_draw_line(
                game.win_w - GAME::FRAMERATE_HISTORY_SIZE + f + 0.5, 0,
                game.win_w - GAME::FRAMERATE_HISTORY_SIZE + f + 0.5, fps_y,
                al_map_rgba(24, 96, 192, 192), 1
            );
        }
        float target_fps_y =
            interpolate_number(
                game.options.advanced.target_fps,
                chart_min, chart_max,
                0, 100
            );
        al_draw_line(
            game.win_w - GAME::FRAMERATE_HISTORY_SIZE, target_fps_y,
            game.win_w, target_fps_y,
            al_map_rgba(128, 224, 128, 48), 1
        );
    }
}


/**
 * @brief Draws a leader's throw preview.
 */
void GameplayState::draw_throw_preview() {
    if(!cur_leader_ptr) return;
    
    ALLEGRO_VERTEX vertexes[16];
    
    if(!cur_leader_ptr->throwee) {
        //Just draw a simple line and leave.
        unsigned char n_vertexes =
            get_throw_preview_vertexes(
                vertexes, 0.0f, 1.0f,
                cur_leader_ptr->pos, throw_dest,
                change_alpha(
                    game.config.aesthetic_gen.no_pikmin_color,
                    GAMEPLAY::PREVIEW_OPACITY / 2.0f
                ),
                0.0f, 1.0f, false
            );
            
        for(unsigned char v = 0; v < n_vertexes; v += 4) {
            al_draw_prim(
                vertexes, nullptr, nullptr,
                v, v + 4, ALLEGRO_PRIM_TRIANGLE_FAN
            );
        }
        
        return;
    }
    
    //Check which edges exist near the throw.
    set<Edge*> candidate_edges;
    
    game.cur_area_data->bmap.get_edges_in_region(
        Point(
            std::min(cur_leader_ptr->pos.x, throw_dest.x),
            std::min(cur_leader_ptr->pos.y, throw_dest.y)
        ),
        Point(
            std::max(cur_leader_ptr->pos.x, throw_dest.x),
            std::max(cur_leader_ptr->pos.y, throw_dest.y)
        ),
        candidate_edges
    );
    
    float wall_collision_r = 2.0f;
    bool wall_is_blocking_sector = false;
    Distance leader_to_dest_dist(
        cur_leader_ptr->pos, throw_dest
    );
    float throw_h_angle = 0.0f;
    float throw_v_angle = 0.0f;
    float throw_speed = 0.0f;
    float throw_h_speed = 0.0f;
    coordinates_to_angle(
        cur_leader_ptr->throwee_speed, &throw_h_angle, &throw_h_speed
    );
    coordinates_to_angle(
        Point(throw_h_speed, cur_leader_ptr->throwee_speed_z),
        &throw_v_angle, &throw_speed
    );
    float texture_offset =
        fmod(
            area_time_passed * GAMEPLAY::PREVIEW_TEXTURE_TIME_MULT,
            al_get_bitmap_width(game.sys_content.bmp_throw_preview) *
            GAMEPLAY::PREVIEW_TEXTURE_SCALE
        );
        
    //For each edge, check if it crosses the throw line.
    for(Edge* e : candidate_edges) {
        if(!e->sectors[0] || !e->sectors[1]) {
            continue;
        }
        
        float r = 0.0f;
        if(
            !line_segs_intersect(
                cur_leader_ptr->pos, throw_dest,
                v2p(e->vertexes[0]), v2p(e->vertexes[1]),
                &r, nullptr
            )
        ) {
            //No collision.
            continue;
        }
        
        //If this is a blocking sector then yeah, collision.
        if(
            (
                e->sectors[0]->type == SECTOR_TYPE_BLOCKING ||
                e->sectors[1]->type == SECTOR_TYPE_BLOCKING
            ) &&
            r < wall_collision_r
        ) {
            wall_collision_r = r;
            wall_is_blocking_sector = true;
            continue;
        }
        
        //Otherwise, let's check for walls.
        
        if(e->sectors[0]->z == e->sectors[1]->z) {
            //Edges where both sectors have the same height have no wall.
            continue;
        }
        
        //Calculate the throwee's vertical position at that point.
        float edge_z = std::max(e->sectors[0]->z, e->sectors[1]->z);
        float x_at_edge =
            leader_to_dest_dist.to_float() * r;
        float y_at_edge =
            tan(throw_v_angle) * x_at_edge -
            (
                -MOB::GRAVITY_ADDER /
                (
                    2 * throw_speed * throw_speed *
                    cos(throw_v_angle) * cos(throw_v_angle)
                )
            ) * x_at_edge * x_at_edge;
        y_at_edge += cur_leader_ptr->z;
        
        //If the throwee would hit the wall at these coordinates, collision.
        if(edge_z >= y_at_edge && r < wall_collision_r) {
            wall_collision_r = r;
            wall_is_blocking_sector = false;
        }
    }
    
    /*
     * Time to draw. There are three possible scenarios.
     * 1. Nothing interrupts the throw, so we can draw directly from
     *   the leader to the throw destination.
     * 2. The throwee could never reach because it's too high, so draw the
     *   line colliding against the edge.
     * 3. The throwee will collide against a wall, but can theoretically reach
     *   the target, since it's within the height limit. After the wall
     *   collision, its trajectory is unpredictable.
     */
    
    if(wall_collision_r > 1.0f) {
        //No collision. Free throw.
        
        unsigned char n_vertexes =
            get_throw_preview_vertexes(
                vertexes, 0.0f, 1.0f,
                cur_leader_ptr->pos, throw_dest,
                change_alpha(
                    cur_leader_ptr->throwee->type->main_color,
                    GAMEPLAY::PREVIEW_OPACITY
                ),
                texture_offset, GAMEPLAY::PREVIEW_TEXTURE_SCALE, true
            );
            
        for(unsigned char v = 0; v < n_vertexes; v += 4) {
            al_draw_prim(
                vertexes, nullptr, game.sys_content.bmp_throw_preview,
                v, v + 4, ALLEGRO_PRIM_TRIANGLE_FAN
            );
        }
        
    } else {
        //Wall collision.
        
        Point collision_point(
            cur_leader_ptr->pos.x +
            (throw_dest.x - cur_leader_ptr->pos.x) *
            wall_collision_r,
            cur_leader_ptr->pos.y +
            (throw_dest.y - cur_leader_ptr->pos.y) *
            wall_collision_r
        );
        
        if(!cur_leader_ptr->throwee_can_reach || wall_is_blocking_sector) {
            //It's impossible to reach.
            
            unsigned char n_vertexes =
                get_throw_preview_vertexes(
                    vertexes, 0.0f, wall_collision_r,
                    cur_leader_ptr->pos, throw_dest,
                    change_alpha(
                        cur_leader_ptr->throwee->type->main_color,
                        GAMEPLAY::PREVIEW_OPACITY
                    ),
                    texture_offset, GAMEPLAY::PREVIEW_TEXTURE_SCALE, true
                );
                
            for(unsigned char v = 0; v < n_vertexes; v += 4) {
                al_draw_prim(
                    vertexes, nullptr, game.sys_content.bmp_throw_preview,
                    v, v + 4, ALLEGRO_PRIM_TRIANGLE_FAN
                );
            }
            
            draw_bitmap(
                game.sys_content.bmp_throw_invalid,
                collision_point, Point(32.0f), throw_h_angle,
                change_alpha(
                    cur_leader_ptr->throwee->type->main_color,
                    GAMEPLAY::PREVIEW_OPACITY
                )
            );
            
        } else {
            //Trajectory is unknown after collision. Can theoretically reach.
            
            unsigned char n_vertexes =
                get_throw_preview_vertexes(
                    vertexes, 0.0f, wall_collision_r,
                    cur_leader_ptr->pos, throw_dest,
                    change_alpha(
                        cur_leader_ptr->throwee->type->main_color,
                        GAMEPLAY::COLLISION_OPACITY
                    ),
                    texture_offset, GAMEPLAY::PREVIEW_TEXTURE_SCALE, true
                );
                
            for(unsigned char v = 0; v < n_vertexes; v += 4) {
                al_draw_prim(
                    vertexes, nullptr, game.sys_content.bmp_throw_preview,
                    v, v + 4, ALLEGRO_PRIM_TRIANGLE_FAN
                );
            }
            
            n_vertexes =
                get_throw_preview_vertexes(
                    vertexes, wall_collision_r, 1.0f,
                    cur_leader_ptr->pos, throw_dest,
                    change_alpha(
                        cur_leader_ptr->throwee->type->main_color,
                        GAMEPLAY::PREVIEW_OPACITY
                    ),
                    0.0f, 1.0f, true
                );
                
            for(unsigned char v = 0; v < n_vertexes; v += 4) {
                al_draw_prim(
                    vertexes, nullptr, game.sys_content.bmp_throw_preview_dashed,
                    v, v + 4, ALLEGRO_PRIM_TRIANGLE_FAN
                );
            }
            
            draw_bitmap(
                game.sys_content.bmp_throw_invalid,
                collision_point, Point(16.0f), throw_h_angle,
                change_alpha(
                    cur_leader_ptr->throwee->type->main_color,
                    GAMEPLAY::PREVIEW_OPACITY
                )
            );
            
        }
    }
    
}


/**
 * @brief Draws the current area and mobs to a bitmap and returns it.
 *
 * @param settings What settings to use.
 * @return The bitmap.
 */
ALLEGRO_BITMAP* GameplayState::draw_to_bitmap(
    const MakerTools::AreaImageSettings &settings
) {
    //First, get the full dimensions of the map.
    Point min_coords(FLT_MAX, FLT_MAX);
    Point max_coords(-FLT_MAX, -FLT_MAX);
    
    for(size_t v = 0; v < game.cur_area_data->vertexes.size(); v++) {
        Vertex* v_ptr = game.cur_area_data->vertexes[v];
        update_min_max_coords(
            min_coords, max_coords, v2p(v_ptr)
        );
    }
    
    //Figure out the scale that will fit on the image.
    float area_w = max_coords.x - min_coords.x + settings.padding;
    float area_h = max_coords.y - min_coords.y + settings.padding;
    float final_bmp_w = settings.size;
    float final_bmp_h = settings.size;
    float scale;
    
    if(area_w > area_h) {
        scale = settings.size / area_w;
        final_bmp_h *= area_h / area_w;
    } else {
        scale = settings.size / area_h;
        final_bmp_w *= area_w / area_h;
    }
    
    //Create the bitmap.
    ALLEGRO_BITMAP* bmp = al_create_bitmap(final_bmp_w, final_bmp_h);
    
    ALLEGRO_TRANSFORM t;
    al_identity_transform(&t);
    al_translate_transform(
        &t,
        -min_coords.x + settings.padding / 2.0f,
        -min_coords.y + settings.padding / 2.0f
    );
    al_scale_transform(&t, scale, scale);
    
    //Begin drawing!
    do_game_drawing(bmp, &t);
    
    return bmp;
}


/**
 * @brief Draws tree shadows.
 */
void GameplayState::draw_tree_shadows() {
    for(size_t s = 0; s < game.cur_area_data->tree_shadows.size(); s++) {
        TreeShadow* s_ptr = game.cur_area_data->tree_shadows[s];
        
        unsigned char alpha =
            (
                (s_ptr->alpha / 255.0) *
                game.cur_area_data->weather_condition.get_sun_strength()
            ) * 255;
            
        draw_bitmap(
            s_ptr->bitmap,
            Point(
                s_ptr->center.x + GAMEPLAY::TREE_SHADOW_SWAY_AMOUNT*
                cos(GAMEPLAY::TREE_SHADOW_SWAY_SPEED * area_time_passed) *
                s_ptr->sway.x,
                s_ptr->center.y + GAMEPLAY::TREE_SHADOW_SWAY_AMOUNT*
                sin(GAMEPLAY::TREE_SHADOW_SWAY_SPEED * area_time_passed) *
                s_ptr->sway.y
            ),
            s_ptr->size,
            s_ptr->angle, map_alpha(alpha)
        );
    }
}


/**
 * @brief Draws the components that make up the game world:
 * layout, objects, etc.
 *
 * @param bmp_output If not nullptr, draw the area onto this.
 */
void GameplayState::draw_world_components(ALLEGRO_BITMAP* bmp_output) {
    ALLEGRO_BITMAP* custom_wall_offset_effect_buffer = nullptr;
    ALLEGRO_BITMAP* custom_liquid_limit_effect_buffer = nullptr;
    if(!bmp_output) {
        update_offset_effect_buffer(
            game.cam.box[0], game.cam.box[1],
            game.liquid_limit_effect_caches,
            game.liquid_limit_effect_buffer,
            true
        );
        update_offset_effect_buffer(
            game.cam.box[0], game.cam.box[1],
            game.wall_smoothing_effect_caches,
            game.wall_offset_effect_buffer,
            true
        );
        update_offset_effect_buffer(
            game.cam.box[0], game.cam.box[1],
            game.wall_shadow_effect_caches,
            game.wall_offset_effect_buffer,
            false
        );
        
    } else {
        custom_liquid_limit_effect_buffer =
            al_create_bitmap(
                al_get_bitmap_width(bmp_output),
                al_get_bitmap_height(bmp_output)
            );
        custom_wall_offset_effect_buffer =
            al_create_bitmap(
                al_get_bitmap_width(bmp_output),
                al_get_bitmap_height(bmp_output)
            );
        update_offset_effect_buffer(
            Point(-FLT_MAX), Point(FLT_MAX),
            game.liquid_limit_effect_caches,
            custom_liquid_limit_effect_buffer,
            true
        );
        update_offset_effect_buffer(
            Point(-FLT_MAX), Point(FLT_MAX),
            game.wall_smoothing_effect_caches,
            custom_wall_offset_effect_buffer,
            true
        );
        update_offset_effect_buffer(
            Point(-FLT_MAX), Point(FLT_MAX),
            game.wall_shadow_effect_caches,
            custom_wall_offset_effect_buffer,
            false
        );
        
    }
    
    vector<WorldComponent> components;
    //Let's reserve some space. We might need more or less,
    //but this is a nice estimate.
    components.reserve(
        game.cur_area_data->sectors.size() + //Sectors.
        mobs.all.size() + //Mob shadows.
        mobs.all.size() + //Mobs.
        particles.get_count() //Particles.
    );
    
    //Sectors.
    for(size_t s = 0; s < game.cur_area_data->sectors.size(); s++) {
        Sector* s_ptr = game.cur_area_data->sectors[s];
        
        if(
            !bmp_output &&
            !rectangles_intersect(
                s_ptr->bbox[0], s_ptr->bbox[1],
                game.cam.box[0], game.cam.box[1]
            )
        ) {
            //Off-camera.
            continue;
        }
        
        WorldComponent c;
        c.sector_ptr = s_ptr;
        c.z = s_ptr->z;
        components.push_back(c);
    }
    
    //Particles.
    particles.fill_component_list(components, game.cam.box[0], game.cam.box[1]);
    
    //Mobs.
    for(size_t m = 0; m < mobs.all.size(); m++) {
        Mob* mob_ptr = mobs.all[m];
        
        if(!bmp_output && mob_ptr->is_off_camera()) {
            //Off-camera.
            continue;
        }
        
        if(has_flag(mob_ptr->flags, MOB_FLAG_HIDDEN)) continue;
        if(mob_ptr->is_stored_inside_mob()) continue;
        
        //Shadows.
        if(
            mob_ptr->type->casts_shadow &&
            !has_flag(mob_ptr->flags, MOB_FLAG_SHADOW_INVISIBLE)
        ) {
            WorldComponent c;
            c.mob_shadow_ptr = mob_ptr;
            if(mob_ptr->standing_on_mob) {
                c.z =
                    mob_ptr->standing_on_mob->z +
                    mob_ptr->standing_on_mob->get_drawing_height();
            } else {
                c.z = mob_ptr->ground_sector->z;
            }
            c.z += mob_ptr->get_drawing_height() - 1;
            components.push_back(c);
        }
        
        //Limbs.
        if(mob_ptr->parent && mob_ptr->parent->limb_anim.anim_db) {
            unsigned char method = mob_ptr->parent->limb_draw_method;
            WorldComponent c;
            c.mob_limb_ptr = mob_ptr;
            
            switch(method) {
            case LIMB_DRAW_METHOD_BELOW_BOTH: {
                c.z = std::min(mob_ptr->z, mob_ptr->parent->m->z);
                break;
            } case LIMB_DRAW_METHOD_BELOW_CHILD: {
                c.z = mob_ptr->z;
                break;
            } case LIMB_DRAW_METHOD_BELOW_PARENT: {
                c.z = mob_ptr->parent->m->z;
                break;
            } case LIMB_DRAW_METHOD_ABOVE_PARENT: {
                c.z =
                    mob_ptr->parent->m->z +
                    mob_ptr->parent->m->get_drawing_height() +
                    0.001;
                break;
            } case LIMB_DRAW_METHOD_ABOVE_CHILD: {
                c.z = mob_ptr->z + mob_ptr->get_drawing_height() + 0.001;
                break;
            } case LIMB_DRAW_METHOD_ABOVE_BOTH: {
                c.z =
                    std::max(
                        mob_ptr->parent->m->z +
                        mob_ptr->parent->m->get_drawing_height() +
                        0.001,
                        mob_ptr->z + mob_ptr->get_drawing_height() +
                        0.001
                    );
                break;
            }
            }
            
            components.push_back(c);
        }
        
        //The mob proper.
        WorldComponent c;
        c.mob_ptr = mob_ptr;
        c.z = mob_ptr->z + mob_ptr->get_drawing_height();
        if(mob_ptr->holder.m && mob_ptr->holder.force_above_holder) {
            c.z += mob_ptr->holder.m->get_drawing_height() + 1;
        }
        components.push_back(c);
    }
    
    //Time to draw!
    for(size_t c = 0; c < components.size(); c++) {
        components[c].idx = c;
    }
    
    sort(
        components.begin(), components.end(),
    [] (const WorldComponent & c1, const WorldComponent & c2) -> bool {
        if(c1.z == c2.z) {
            return c1.idx < c2.idx;
        }
        return c1.z < c2.z;
    }
    );
    
    float mob_shadow_stretch = 0;
    
    if(day_minutes < 60 * 5 || day_minutes > 60 * 20) {
        mob_shadow_stretch = 1;
    } else if(day_minutes < 60 * 12) {
        mob_shadow_stretch = 1 - ((day_minutes - 60 * 5) / (60 * 12 - 60 * 5));
    } else {
        mob_shadow_stretch = (day_minutes - 60 * 12) / (60 * 20 - 60 * 12);
    }
    
    for(size_t c = 0; c < components.size(); c++) {
        WorldComponent* c_ptr = &components[c];
        
        if(c_ptr->sector_ptr) {
        
            bool has_liquid = false;
            for(size_t h = 0; h < c_ptr->sector_ptr->hazards.size(); h++) {
                if(c_ptr->sector_ptr->hazards[h]->associated_liquid) {
                    draw_liquid(
                        c_ptr->sector_ptr,
                        c_ptr->sector_ptr->hazards[h]->associated_liquid,
                        Point(),
                        1.0f,
                        area_time_passed
                    );
                    has_liquid = true;
                    break;
                }
            }
            if(!has_liquid) {
                draw_sector_texture(c_ptr->sector_ptr, Point(), 1.0f, 1.0f);
            }
            float liquid_opacity_mult = 1.0f;
            if(c_ptr->sector_ptr->draining_liquid) {
                liquid_opacity_mult =
                    c_ptr->sector_ptr->liquid_drain_left /
                    GEOMETRY::LIQUID_DRAIN_DURATION;
            }
            draw_sector_edge_offsets(
                c_ptr->sector_ptr,
                bmp_output ?
                custom_liquid_limit_effect_buffer :
                game.liquid_limit_effect_buffer,
                liquid_opacity_mult
            );
            draw_sector_edge_offsets(
                c_ptr->sector_ptr,
                bmp_output ?
                custom_wall_offset_effect_buffer :
                game.wall_offset_effect_buffer,
                1.0f
            );
            
        } else if(c_ptr->mob_shadow_ptr) {
        
            float delta_z = 0;
            if(!c_ptr->mob_shadow_ptr->standing_on_mob) {
                delta_z =
                    c_ptr->mob_shadow_ptr->z -
                    c_ptr->mob_shadow_ptr->ground_sector->z;
            }
            draw_mob_shadow(
                c_ptr->mob_shadow_ptr,
                delta_z,
                mob_shadow_stretch
            );
            
        } else if(c_ptr->mob_limb_ptr) {
        
            if(!has_flag(c_ptr->mob_limb_ptr->flags, MOB_FLAG_HIDDEN)) {
                c_ptr->mob_limb_ptr->draw_limb();
            }
            
        } else if(c_ptr->mob_ptr) {
        
            if(!has_flag(c_ptr->mob_ptr->flags, MOB_FLAG_HIDDEN)) {
                c_ptr->mob_ptr->draw_mob();
                if(c_ptr->mob_ptr->type->draw_mob_callback) {
                    c_ptr->mob_ptr->type->draw_mob_callback(c_ptr->mob_ptr);
                }
            }
            
        } else if(c_ptr->particle_ptr) {
        
            c_ptr->particle_ptr->draw();
            
        }
    }
    
    if(bmp_output) {
        al_destroy_bitmap(custom_wall_offset_effect_buffer);
    }
}
