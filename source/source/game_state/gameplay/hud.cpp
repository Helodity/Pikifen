/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * In-game HUD classes and functions.
 */

#include <algorithm>

#include "../../core/drawing.h"
#include "../../core/misc_functions.h"
#include "../../core/game.h"
#include "../../util/allegro_utils.h"
#include "../../util/string_utils.h"
#include "gameplay.h"


using DrawInfo = GuiItem::DrawInfo;


namespace HUD {

//Dampen the mission goal indicator's movement by this much.
const float GOAL_INDICATOR_SMOOTHNESS_MULT = 5.5f;

//Name of the GUI information file.
const string GUI_FILE_NAME = "gameplay";

//How long the leader swap juice animation lasts for.
const float LEADER_SWAP_JUICE_DURATION = 0.7f;

//Standard mission score medal icon scale.
const float MEDAL_ICON_SCALE = 1.5f;

//Multiply time by this much to get the right scale animation amount.
const float MEDAL_ICON_SCALE_MULT = 0.3f;

//Multiply time by this much to get the right scale animation speed.
const float MEDAL_ICON_SCALE_TIME_MULT = 4.0f;

//Dampen the mission score indicator's movement by this much.
const float SCORE_INDICATOR_SMOOTHNESS_MULT = 5.5f;

//How many points to show before and after the mission score ruler flapper.
const int SCORE_RULER_RANGE = 125;

//How long the spray swap juice animation lasts for.
const float SPRAY_SWAP_JUICE_DURATION = 0.7f;

//How long the standby swap juice animation lasts for.
const float STANDBY_SWAP_JUICE_DURATION = 0.5f;

//The Sun Meter's sun spins these many radians per second.
const float SUN_METER_SUN_SPIN_SPEED = 0.5f;

//Speed at which previously-unnecessary items fade in, in alpha per second.
const float UNNECESSARY_ITEMS_FADE_IN_SPEED = 2.5f;

//Delay before unnecessary items start fading out.
const float UNNECESSARY_ITEMS_FADE_OUT_DELAY = 2.5f;

//Speed at which unnecessary items fade out, in alpha per second.
const float UNNECESSARY_ITEMS_FADE_OUT_SPEED = 0.5f;

}


/**
 * @brief Constructs a new HUD struct object.
 */
Hud::Hud() :
    leader_icon_mgr(&gui),
    leader_health_mgr(&gui),
    standby_icon_mgr(&gui),
    spray_icon_mgr(&gui) {
    
    DataNode* hud_file_node = &game.content.gui_defs.list[HUD::GUI_FILE_NAME];
    
    gui.register_coords("time",                          0,    0,  0,  0);
    gui.register_coords("day_bubble",                    0,    0,  0,  0);
    gui.register_coords("day_number",                    0,    0,  0,  0);
    gui.register_coords("leader_1_icon",                 7,   90,  8, 10);
    gui.register_coords("leader_2_icon",                 6,   80,  5,  9);
    gui.register_coords("leader_3_icon",                 6, 71.5,  5,  7);
    gui.register_coords("leader_1_health",              16,   90,  8, 10);
    gui.register_coords("leader_2_health",              12,   80,  5,  9);
    gui.register_coords("leader_3_health",              12, 71.5,  5,  7);
    gui.register_coords("leader_next_input",            4,   83,  3,  3);
    gui.register_coords("standby_icon",                 50,   91,  8, 10);
    gui.register_coords("standby_amount",               50,   97,  8,  4);
    gui.register_coords("standby_bubble",                0,    0,  0,  0);
    gui.register_coords("standby_maturity_icon",        54,   88,  4,  8);
    gui.register_coords("standby_next_icon",            58,   93,  6,  8);
    gui.register_coords("standby_next_input",           60,   96,  3,  3);
    gui.register_coords("standby_prev_icon",            42,   93,  6,  8);
    gui.register_coords("standby_prev_input",           40,   96,  3,  3);
    gui.register_coords("group_amount",                 73,   91, 15, 14);
    gui.register_coords("group_bubble",                 73,   91, 15, 14);
    gui.register_coords("field_amount",                 91,   91, 15, 14);
    gui.register_coords("field_bubble",                 91,   91, 15, 14);
    gui.register_coords("total_amount",                  0,    0,  0,  0);
    gui.register_coords("total_bubble",                  0,    0,  0,  0);
    gui.register_coords("counters_x",                    0,    0,  0,  0);
    gui.register_coords("counters_slash_1",             82,   91,  4,  8);
    gui.register_coords("counters_slash_2",              0,    0,  0,  0);
    gui.register_coords("counters_slash_3",              0,    0,  0,  0);
    gui.register_coords("spray_1_icon",                  6,   36,  4,  7);
    gui.register_coords("spray_1_amount",               13,   37, 10,  5);
    gui.register_coords("spray_1_input",                 4,   39,  3,  3);
    gui.register_coords("spray_2_icon",                  6,   52,  4,  7);
    gui.register_coords("spray_2_amount",               13,   53, 10,  5);
    gui.register_coords("spray_2_input",                 4,   55,  3,  3);
    gui.register_coords("spray_prev_icon",               6,   48,  3,  5);
    gui.register_coords("spray_prev_input",              4,   51,  4,  4);
    gui.register_coords("spray_next_icon",              12,   48,  3,  5);
    gui.register_coords("spray_next_input",             14,   51,  4,  4);
    gui.register_coords("mission_goal_bubble",          18,    8, 32, 12);
    gui.register_coords("mission_goal_cur_label",      9.5, 11.5, 13,  3);
    gui.register_coords("mission_goal_cur",            9.5,  6.5, 13,  7);
    gui.register_coords("mission_goal_req_label",     26.5, 11.5, 13,  3);
    gui.register_coords("mission_goal_req",           26.5,  6.5, 13,  7);
    gui.register_coords("mission_goal_slash",           18,  6.5,  4,  7);
    gui.register_coords("mission_goal_name",            18,    8, 30, 10);
    gui.register_coords("mission_score_bubble",         18,   20, 32, 10);
    gui.register_coords("mission_score_score_label",   7.5,   22,  9,  4);
    gui.register_coords("mission_score_points",         18, 21.5, 10,  5);
    gui.register_coords("mission_score_points_label", 28.5,   22,  9,  4);
    gui.register_coords("mission_score_ruler",          18,   17, 30,  2);
    gui.register_coords("mission_fail_1_bubble",        82,    8, 32, 12);
    gui.register_coords("mission_fail_1_cur_label",   73.5, 11.5, 13,  3);
    gui.register_coords("mission_fail_1_cur",         73.5,  6.5, 13,  7);
    gui.register_coords("mission_fail_1_req_label",   90.5, 11.5, 13,  3);
    gui.register_coords("mission_fail_1_req",         90.5,  6.5, 13,  7);
    gui.register_coords("mission_fail_1_slash",         82,  6.5,  4,  7);
    gui.register_coords("mission_fail_1_name",          82,    8, 30, 10);
    gui.register_coords("mission_fail_2_bubble",        82,   20, 32, 10);
    gui.register_coords("mission_fail_2_cur_label",   73.5, 22.5, 13,  3);
    gui.register_coords("mission_fail_2_cur",         73.5, 18.5, 13,  5);
    gui.register_coords("mission_fail_2_req_label",   90.5, 22.5, 13,  3);
    gui.register_coords("mission_fail_2_req",         90.5, 18.5, 13,  5);
    gui.register_coords("mission_fail_2_slash",         82, 18.5,  4,  5);
    gui.register_coords("mission_fail_2_name",          82,   20, 30,  8);
    gui.read_coords(hud_file_node->getChildByName("positions"));
    
    //Leader health and icons.
    for(size_t l = 0; l < 3; l++) {
    
        //Icon.
        GuiItem* leader_icon = new GuiItem();
        leader_icon->on_draw =
        [this, l] (const DrawInfo & draw) {
            LeaderIconBubble icon;
            DrawInfo final_draw;
            game.states.gameplay->hud->leader_icon_mgr.get_drawing_info(
                l, &icon, &final_draw
            );
            
            if(!icon.bmp) return;
            
            al_draw_filled_circle(
                final_draw.center.x, final_draw.center.y,
                std::min(final_draw.size.x, final_draw.size.y) / 2.0f,
                change_alpha(
                    icon.color,
                    128
                )
            );
            draw_bitmap_in_box(
                icon.bmp,
                final_draw.center, final_draw.size, true
            );
            draw_bitmap_in_box(
                bmp_bubble,
                final_draw.center, final_draw.size, true
            );
        };
        gui.add_item(leader_icon, "leader_" + i2s(l + 1) + "_icon");
        leader_icon_mgr.register_bubble(l, leader_icon);
        
        
        //Health wheel.
        GuiItem* leader_health = new GuiItem();
        leader_health->on_draw =
        [this, l] (const DrawInfo & draw) {
            LeaderHealthBubble health;
            DrawInfo final_draw;
            game.states.gameplay->hud->leader_health_mgr.get_drawing_info(
                l, &health, &final_draw
            );
            
            if(health.ratio <= 0.0f) return;
            
            if(health.caution_timer > 0.0f) {
                float caution_ring_scale =
                    interpolate_number(
                        health.caution_timer,
                        0.0f, LEADER::HEALTH_CAUTION_RING_DURATION,
                        1.0f, 2.0f
                    );
                unsigned char caution_ring_alpha =
                    health.caution_timer <
                    LEADER::HEALTH_CAUTION_RING_DURATION / 2.0f ?
                    interpolate_number(
                        health.caution_timer,
                        0.0f, LEADER::HEALTH_CAUTION_RING_DURATION / 2.0f,
                        0.0f, 192
                    ) :
                    interpolate_number(
                        health.caution_timer,
                        LEADER::HEALTH_CAUTION_RING_DURATION / 2.0f,
                        LEADER::HEALTH_CAUTION_RING_DURATION,
                        192, 0
                    );
                float caution_ring_size =
                    std::min(final_draw.size.x, final_draw.size.y) * caution_ring_scale;
                    
                draw_bitmap(
                    game.sys_content.bmp_bright_ring,
                    final_draw.center,
                    Point(caution_ring_size),
                    0.0f,
                    al_map_rgba(255, 0, 0, caution_ring_alpha)
                );
            }
            
            draw_health(
                final_draw.center,
                health.ratio,
                1.0f,
                std::min(final_draw.size.x, final_draw.size.y) * 0.47f,
                true
            );
            draw_bitmap_in_box(
                bmp_hard_bubble,
                final_draw.center,
                final_draw.size,
                true
            );
        };
        gui.add_item(leader_health, "leader_" + i2s(l + 1) + "_health");
        leader_health_mgr.register_bubble(l, leader_health);
        
    }
    
    
    //Next leader input.
    GuiItem* leader_next_input = new GuiItem();
    leader_next_input->on_draw =
    [this] (const DrawInfo & draw) {
        if(!game.options.misc.show_hud_input_icons) return;
        if(game.states.gameplay->available_leaders.size() < 2) return;
        const PlayerInputSource &s =
            game.controls.find_bind(PLAYER_ACTION_TYPE_NEXT_LEADER).
            inputSource;
        if(s.type == INPUT_SOURCE_TYPE_NONE) return;
        draw_player_input_source_icon(
            game.sys_content.fnt_slim, s, true, draw.center, draw.size
        );
    };
    gui.add_item(leader_next_input, "leader_next_input");
    
    
    //Sun Meter.
    GuiItem* sun_meter = new GuiItem();
    sun_meter->on_draw =
    [this, sun_meter] (const DrawInfo & draw) {
        unsigned char n_hours =
            (
                game.config.misc.day_minutes_end -
                game.config.misc.day_minutes_start
            ) / 60.0f;
        float day_length =
            game.config.misc.day_minutes_end - game.config.misc.day_minutes_start;
        float day_passed_ratio =
            (
                game.states.gameplay->day_minutes -
                game.config.misc.day_minutes_start
            ) /
            (float) (day_length);
        float sun_radius = draw.size.y / 2.0;
        float first_dot_x = (draw.center.x - draw.size.x / 2.0) + sun_radius;
        float last_dot_x = (draw.center.x + draw.size.x / 2.0) - sun_radius;
        float dots_y = draw.center.y;
        //Width, from the center of the first dot to the center of the last.
        float dots_span = last_dot_x - first_dot_x;
        float dot_interval = dots_span / (float) n_hours;
        float sun_meter_sun_angle =
            game.states.gameplay->area_time_passed *
            HUD::SUN_METER_SUN_SPIN_SPEED;
            
        //Larger bubbles at the start, middle and end of the meter.
        al_hold_bitmap_drawing(true);
        draw_bitmap(
            bmp_hard_bubble,
            Point(first_dot_x + dots_span * 0.0, dots_y),
            Point(sun_radius * 0.9)
        );
        draw_bitmap(
            bmp_hard_bubble,
            Point(first_dot_x + dots_span * 0.5, dots_y),
            Point(sun_radius * 0.9)
        );
        draw_bitmap(
            bmp_hard_bubble,
            Point(first_dot_x + dots_span * 1.0, dots_y),
            Point(sun_radius * 0.9)
        );
        
        for(unsigned char h = 0; h < n_hours + 1; h++) {
            draw_bitmap(
                bmp_hard_bubble,
                Point(first_dot_x + h * dot_interval, dots_y),
                Point(sun_radius * 0.6)
            );
        }
        al_hold_bitmap_drawing(false);
        
        Point sun_size =
            Point(sun_radius * 1.5) +
            sun_meter->get_juice_value();
        //Static sun.
        draw_bitmap(
            bmp_sun,
            Point(first_dot_x + day_passed_ratio * dots_span, dots_y),
            sun_size
        );
        //Spinning sun.
        draw_bitmap(
            bmp_sun,
            Point(first_dot_x + day_passed_ratio * dots_span, dots_y),
            sun_size,
            sun_meter_sun_angle
        );
        //Bubble in front the sun.
        draw_bitmap(
            bmp_hard_bubble,
            Point(first_dot_x + day_passed_ratio * dots_span, dots_y),
            sun_size,
            0.0f,
            al_map_rgb(255, 192, 128)
        );
    };
    sun_meter->on_tick =
    [this, sun_meter] (float delta_t) {
        float day_length =
            game.config.misc.day_minutes_end - game.config.misc.day_minutes_start;
        float pre_tick_day_minutes =
            game.states.gameplay->day_minutes -
            game.cur_area_data->day_time_speed * delta_t / 60.0f;
        float post_tick_day_minutes =
            game.states.gameplay->day_minutes;
        const float checkpoints[3] = {0.25f, 0.50f, 0.75f};
        for(unsigned char c = 0; c < 3; c++) {
            float checkpoint =
                game.config.misc.day_minutes_start + day_length * checkpoints[c];
            if(
                pre_tick_day_minutes < checkpoint &&
                post_tick_day_minutes >= checkpoint
            ) {
                sun_meter->start_juice_animation(
                    GuiItem::JUICE_TYPE_GROW_ICON
                );
                break;
            }
        }
    };
    gui.add_item(sun_meter, "time");
    
    
    //Day number bubble.
    GuiItem* day_bubble = new GuiItem();
    day_bubble->on_draw =
    [this] (const DrawInfo & draw) {
        draw_bitmap_in_box(bmp_day_bubble, draw.center, draw.size, true);
    };
    gui.add_item(day_bubble, "day_bubble");
    
    
    //Day number text.
    GuiItem* day_nr = new GuiItem();
    day_nr->on_draw =
    [this] (const DrawInfo & draw) {
        draw_text(
            i2s(game.states.gameplay->day),
            game.sys_content.fnt_counter, draw.center,
            Point(draw.size.x * 0.70f, draw.size.y * 0.50f)
        );
    };
    gui.add_item(day_nr, "day_number");
    
    
    //Standby group member icon.
    GuiItem* standby_icon = new GuiItem();
    standby_icon->on_draw =
    [this] (const DrawInfo & draw) {
        game.states.gameplay->hud->draw_standby_icon(BUBBLE_RELATION_CURRENT);
    };
    gui.add_item(standby_icon, "standby_icon");
    standby_icon_mgr.register_bubble(BUBBLE_RELATION_CURRENT, standby_icon);
    
    
    //Next standby subgroup icon.
    GuiItem* standby_next_icon = new GuiItem();
    standby_next_icon->on_draw =
    [this] (const DrawInfo & draw) {
        game.states.gameplay->hud->draw_standby_icon(BUBBLE_RELATION_NEXT);
    };
    gui.add_item(standby_next_icon, "standby_next_icon");
    standby_icon_mgr.register_bubble(BUBBLE_RELATION_NEXT, standby_next_icon);
    
    
    //Next standby subgroup input.
    GuiItem* standby_next_input = new GuiItem();
    standby_next_input->on_draw =
    [this] (const DrawInfo & draw) {
        if(!game.options.misc.show_hud_input_icons) return;
        if(!game.states.gameplay->cur_leader_ptr) return;
        SubgroupType* next_type;
        game.states.gameplay->cur_leader_ptr->group->get_next_standby_type(
            false, &next_type
        );
        if(
            next_type ==
            game.states.gameplay->cur_leader_ptr->group->cur_standby_type
        ) {
            return;
        }
        const PlayerInputSource &s =
            game.controls.find_bind(PLAYER_ACTION_TYPE_NEXT_TYPE).
            inputSource;
        if(s.type == INPUT_SOURCE_TYPE_NONE) return;
        draw_player_input_source_icon(
            game.sys_content.fnt_slim, s, true, draw.center, draw.size,
            game.states.gameplay->hud->standby_items_opacity * 255
        );
    };
    gui.add_item(standby_next_input, "standby_next_input");
    
    
    //Previous standby subgroup icon.
    GuiItem* standby_prev_icon = new GuiItem();
    standby_prev_icon->on_draw =
    [this] (const DrawInfo & draw) {
        game.states.gameplay->hud->draw_standby_icon(BUBBLE_RELATION_PREVIOUS);
    };
    gui.add_item(standby_prev_icon, "standby_prev_icon");
    standby_icon_mgr.register_bubble(BUBBLE_RELATION_PREVIOUS, standby_prev_icon);
    
    
    //Previous standby subgroup input.
    GuiItem* standby_prev_input = new GuiItem();
    standby_prev_input->on_draw =
    [this] (const DrawInfo & draw) {
        if(!game.options.misc.show_hud_input_icons) return;
        if(!game.states.gameplay->cur_leader_ptr) return;
        SubgroupType* prev_type;
        game.states.gameplay->cur_leader_ptr->group->get_next_standby_type(
            true, &prev_type
        );
        SubgroupType* next_type;
        game.states.gameplay->cur_leader_ptr->group->get_next_standby_type(
            false, &next_type
        );
        if(
            prev_type ==
            game.states.gameplay->cur_leader_ptr->group->cur_standby_type ||
            prev_type == next_type
        ) {
            return;
        }
        const PlayerInputSource s =
            game.controls.find_bind(PLAYER_ACTION_TYPE_PREV_TYPE).
            inputSource;
        if(s.type == INPUT_SOURCE_TYPE_NONE) return;
        draw_player_input_source_icon(
            game.sys_content.fnt_slim, s, true, draw.center, draw.size,
            game.states.gameplay->hud->standby_items_opacity * 255
        );
    };
    gui.add_item(standby_prev_input, "standby_prev_input");
    
    
    //Standby group member maturity.
    GuiItem* standby_maturity_icon = new GuiItem();
    standby_maturity_icon->on_draw =
    [this, standby_maturity_icon] (const DrawInfo & draw) {
        //Standby group member preparations.
        Leader* l_ptr = game.states.gameplay->cur_leader_ptr;
        if(!l_ptr || !l_ptr->group) return;
        
        ALLEGRO_BITMAP* standby_mat_bmp = nullptr;
        Mob* closest =
            game.states.gameplay->closest_group_member[BUBBLE_RELATION_CURRENT];
            
        if(l_ptr->group->cur_standby_type && closest) {
            SUBGROUP_TYPE_CATEGORY c =
                l_ptr->group->cur_standby_type->get_category();
                
            switch(c) {
            case SUBGROUP_TYPE_CATEGORY_PIKMIN: {
                Pikmin* p_ptr =
                    dynamic_cast<Pikmin*>(closest);
                standby_mat_bmp =
                    p_ptr->pik_type->bmp_maturity_icon[p_ptr->maturity];
                break;
                
            } default: {
                break;
                
            }
            }
        }
        
        ALLEGRO_COLOR color =
            map_alpha(game.states.gameplay->hud->standby_items_opacity * 255);
            
        if(standby_mat_bmp) {
            draw_bitmap_in_box(
                standby_mat_bmp, draw.center,
                (draw.size * 0.8) + standby_maturity_icon->get_juice_value(),
                true,
                0.0f, color
            );
            draw_bitmap_in_box(
                bmp_bubble, draw.center,
                draw.size + standby_maturity_icon->get_juice_value(),
                true, 0.0f, color
            );
        }
        
        if(
            l_ptr->group->cur_standby_type != prev_standby_type ||
            standby_mat_bmp != prev_maturity_icon
        ) {
            standby_maturity_icon->start_juice_animation(
                GuiItem::JUICE_TYPE_GROW_ICON
            );
            prev_standby_type = l_ptr->group->cur_standby_type;
            prev_maturity_icon = standby_mat_bmp;
        }
    };
    gui.add_item(standby_maturity_icon, "standby_maturity_icon");
    
    
    //Standby subgroup member amount bubble.
    GuiItem* standby_bubble = new GuiItem();
    standby_bubble->on_draw =
    [this] (const DrawInfo & draw) {
        draw_bitmap(
            bmp_counter_bubble_standby,
            draw.center, draw.size,
            0.0f,
            map_alpha(game.states.gameplay->hud->standby_items_opacity * 255)
        );
    };
    gui.add_item(standby_bubble, "standby_bubble");
    
    
    //Standby subgroup member amount.
    standby_amount = new GuiItem();
    standby_amount->on_draw =
    [this] (const DrawInfo & draw) {
        size_t n_standby_pikmin = 0;
        Leader* l_ptr = game.states.gameplay->cur_leader_ptr;
        
        if(l_ptr && l_ptr->group->cur_standby_type) {
            for(size_t m = 0; m < l_ptr->group->members.size(); m++) {
                Mob* m_ptr = l_ptr->group->members[m];
                if(m_ptr->subgroup_type_ptr == l_ptr->group->cur_standby_type) {
                    n_standby_pikmin++;
                }
            }
        }
        
        if(n_standby_pikmin != standby_count_nr) {
            standby_amount->start_juice_animation(
                GuiItem::JUICE_TYPE_GROW_TEXT_ELASTIC_HIGH
            );
            standby_count_nr = n_standby_pikmin;
        }
        
        draw_text(
            i2s(n_standby_pikmin), game.sys_content.fnt_counter,
            draw.center, draw.size,
            map_alpha(game.states.gameplay->hud->standby_items_opacity * 255),
            ALLEGRO_ALIGN_CENTER, V_ALIGN_MODE_CENTER, 0,
            Point(1.0f + standby_amount->get_juice_value())
        );
    };
    gui.add_item(standby_amount, "standby_amount");
    
    
    //Group Pikmin amount bubble.
    GuiItem* group_bubble = new GuiItem();
    group_bubble->on_draw =
    [this] (const DrawInfo & draw) {
        if(!game.states.gameplay->cur_leader_ptr) return;
        draw_bitmap(
            bmp_counter_bubble_group,
            draw.center, draw.size
        );
    };
    gui.add_item(group_bubble, "group_bubble");
    
    
    //Group Pikmin amount.
    group_amount = new GuiItem();
    group_amount->on_draw =
    [this] (const DrawInfo & draw) {
        if(!game.states.gameplay->cur_leader_ptr) return;
        size_t cur_amount = game.states.gameplay->get_amount_of_group_pikmin();
        
        if(cur_amount != group_count_nr) {
            group_amount->start_juice_animation(
                GuiItem::JUICE_TYPE_GROW_TEXT_ELASTIC_HIGH
            );
            group_count_nr = cur_amount;
        }
        
        draw_text(
            i2s(cur_amount), game.sys_content.fnt_counter,
            draw.center, Point(draw.size.x * 0.70f, draw.size.y * 0.50f), COLOR_WHITE,
            ALLEGRO_ALIGN_CENTER, V_ALIGN_MODE_CENTER, 0,
            Point(1.0f + group_amount->get_juice_value())
        );
    };
    gui.add_item(group_amount, "group_amount");
    
    
    //Field Pikmin amount bubble.
    GuiItem* field_bubble = new GuiItem();
    field_bubble->on_draw =
    [this] (const DrawInfo & draw) {
        draw_bitmap(
            bmp_counter_bubble_field,
            draw.center, draw.size
        );
    };
    gui.add_item(field_bubble, "field_bubble");
    
    
    //Field Pikmin amount.
    field_amount = new GuiItem();
    field_amount->on_draw =
    [this] (const DrawInfo & draw) {
        size_t cur_amount = game.states.gameplay->get_amount_of_field_pikmin();
        
        if(cur_amount != field_count_nr) {
            field_amount->start_juice_animation(
                GuiItem::JUICE_TYPE_GROW_TEXT_ELASTIC_HIGH
            );
            field_count_nr = cur_amount;
        }
        
        draw_text(
            i2s(cur_amount), game.sys_content.fnt_counter,
            draw.center, Point(draw.size.x * 0.70f, draw.size.y * 0.50f), COLOR_WHITE,
            ALLEGRO_ALIGN_CENTER, V_ALIGN_MODE_CENTER, 0,
            Point(1.0f + field_amount->get_juice_value())
        );
    };
    gui.add_item(field_amount, "field_amount");
    
    
    //Total Pikmin amount bubble.
    GuiItem* total_bubble = new GuiItem();
    total_bubble->on_draw =
    [this] (const DrawInfo & draw) {
        draw_bitmap(
            bmp_counter_bubble_total,
            draw.center, draw.size
        );
    };
    gui.add_item(total_bubble, "total_bubble");
    
    
    //Total Pikmin amount.
    total_amount = new GuiItem();
    total_amount->on_draw =
    [this] (const DrawInfo & draw) {
        size_t cur_amount = game.states.gameplay->get_amount_of_total_pikmin();
        
        if(cur_amount != total_count_nr) {
            total_amount->start_juice_animation(
                GuiItem::JUICE_TYPE_GROW_TEXT_ELASTIC_HIGH
            );
            total_count_nr = cur_amount;
        }
        
        draw_text(
            i2s(total_count_nr), game.sys_content.fnt_counter,
            draw.center, Point(draw.size.x * 0.70f, draw.size.y * 0.50f), COLOR_WHITE,
            ALLEGRO_ALIGN_CENTER, V_ALIGN_MODE_CENTER, 0,
            Point(1.0f + total_amount->get_juice_value())
        );
    };
    gui.add_item(total_amount, "total_amount");
    
    
    //Pikmin counter "x".
    GuiItem* counters_x = new GuiItem();
    counters_x->on_draw =
    [this] (const DrawInfo & draw) {
        draw_text(
            "x", game.sys_content.fnt_counter, draw.center, draw.size,
            map_alpha(game.states.gameplay->hud->standby_items_opacity * 255)
        );
    };
    gui.add_item(counters_x, "counters_x");
    
    
    //Pikmin counter slashes.
    for(size_t s = 0; s < 3; s++) {
        GuiItem* counter_slash = new GuiItem();
        counter_slash->on_draw =
        [this] (const DrawInfo & draw) {
            if(!game.states.gameplay->cur_leader_ptr) return;
            draw_text(
                "/", game.sys_content.fnt_counter, draw.center, draw.size
            );
        };
        gui.add_item(counter_slash, "counters_slash_" + i2s(s + 1));
    }
    
    
    //Spray 1 icon.
    GuiItem* spray_1_icon = new GuiItem();
    spray_1_icon->on_draw =
    [this] (const DrawInfo & draw) {
        draw_spray_icon(BUBBLE_RELATION_CURRENT);
    };
    gui.add_item(spray_1_icon, "spray_1_icon");
    spray_icon_mgr.register_bubble(BUBBLE_RELATION_CURRENT, spray_1_icon);
    
    
    //Spray 1 amount.
    spray_1_amount = new GuiItem();
    spray_1_amount->on_draw =
    [this] (const DrawInfo & draw) {
        if(!game.states.gameplay->cur_leader_ptr) return;
        
        size_t top_spray_idx = INVALID;
        if(game.content.spray_types.list.size() > 2) {
            top_spray_idx = game.states.gameplay->selected_spray;
        } else if(!game.content.spray_types.list.empty() && game.content.spray_types.list.size() <= 2) {
            top_spray_idx = 0;
        }
        if(top_spray_idx == INVALID) return;
        
        draw_text(
            "x" +
            i2s(game.states.gameplay->spray_stats[top_spray_idx].nr_sprays),
            game.sys_content.fnt_counter,
            Point(draw.center.x - draw.size.x / 2.0, draw.center.y), draw.size,
            map_alpha(game.states.gameplay->hud->spray_items_opacity * 255),
            ALLEGRO_ALIGN_LEFT, V_ALIGN_MODE_CENTER, 0,
            Point(1.0f + spray_1_amount->get_juice_value())
        );
    };
    gui.add_item(spray_1_amount, "spray_1_amount");
    
    
    //Spray 1 input.
    GuiItem* spray_1_input = new GuiItem();
    spray_1_input->on_draw =
    [this] (const DrawInfo & draw) {
        if(!game.options.misc.show_hud_input_icons) return;
        if(!game.states.gameplay->cur_leader_ptr) return;
        
        size_t top_spray_idx = INVALID;
        if(game.content.spray_types.list.size() > 2) {
            top_spray_idx = game.states.gameplay->selected_spray;
        } else if(!game.content.spray_types.list.empty() && game.content.spray_types.list.size() <= 2) {
            top_spray_idx = 0;
        }
        if(top_spray_idx == INVALID) return;
        if(game.states.gameplay->spray_stats[top_spray_idx].nr_sprays == 0) {
            return;
        }
        
        PlayerInputSource s;
        if(
            game.content.spray_types.list.size() > 2
        ) {
            s =
                game.controls.find_bind(PLAYER_ACTION_TYPE_USE_SPRAY).
                inputSource;
        } else if(
            !game.content.spray_types.list.empty() &&
            game.content.spray_types.list.size() <= 2
        ) {
            s =
                game.controls.find_bind(PLAYER_ACTION_TYPE_USE_SPRAY_1).
                inputSource;
        }
        if(s.type == INPUT_SOURCE_TYPE_NONE) return;
        
        draw_player_input_source_icon(
            game.sys_content.fnt_slim, s, true, draw.center, draw.size,
            game.states.gameplay->hud->spray_items_opacity * 255
        );
    };
    gui.add_item(spray_1_input, "spray_1_input");
    
    
    //Spray 2 icon.
    GuiItem* spray_2_icon = new GuiItem();
    spray_2_icon->on_draw =
    [this] (const DrawInfo & draw) {
        if(!game.states.gameplay->cur_leader_ptr) return;
        
        size_t bottom_spray_idx = INVALID;
        if(game.content.spray_types.list.size() == 2) {
            bottom_spray_idx = 1;
        }
        if(bottom_spray_idx == INVALID) return;
        
        draw_bitmap_in_box(
            game.config.misc.spray_order[bottom_spray_idx]->bmp_spray,
            draw.center, draw.size, true,
            0.0f,
            map_alpha(game.states.gameplay->hud->spray_items_opacity * 255)
        );
    };
    gui.add_item(spray_2_icon, "spray_2_icon");
    
    
    //Spray 2 amount.
    spray_2_amount = new GuiItem();
    spray_2_amount->on_draw =
    [this] (const DrawInfo & draw) {
        if(!game.states.gameplay->cur_leader_ptr) return;
        
        size_t bottom_spray_idx = INVALID;
        if(game.content.spray_types.list.size() == 2) {
            bottom_spray_idx = 1;
        }
        if(bottom_spray_idx == INVALID) return;
        
        draw_text(
            "x" +
            i2s(game.states.gameplay->spray_stats[bottom_spray_idx].nr_sprays),
            game.sys_content.fnt_counter,
            Point(draw.center.x - draw.size.x / 2.0, draw.center.y), draw.size,
            map_alpha(game.states.gameplay->hud->spray_items_opacity * 255),
            ALLEGRO_ALIGN_LEFT, V_ALIGN_MODE_CENTER, 0,
            Point(1.0f + spray_2_amount->get_juice_value())
        );
    };
    gui.add_item(spray_2_amount, "spray_2_amount");
    
    
    //Spray 2 input.
    GuiItem* spray_2_input = new GuiItem();
    spray_2_input->on_draw =
    [this] (const DrawInfo & draw) {
        if(!game.options.misc.show_hud_input_icons) return;
        if(!game.states.gameplay->cur_leader_ptr) return;
        
        size_t bottom_spray_idx = INVALID;
        if(game.content.spray_types.list.size() == 2) {
            bottom_spray_idx = 1;
        }
        if(bottom_spray_idx == INVALID) return;
        if(game.states.gameplay->spray_stats[bottom_spray_idx].nr_sprays == 0) {
            return;
        }
        
        PlayerInputSource s;
        if(game.content.spray_types.list.size() == 2) {
            s =
                game.controls.find_bind(PLAYER_ACTION_TYPE_USE_SPRAY_2).
                inputSource;
        }
        if(s.type == INPUT_SOURCE_TYPE_NONE) return;
        
        draw_player_input_source_icon(
            game.sys_content.fnt_slim, s, true, draw.center, draw.size,
            game.states.gameplay->hud->spray_items_opacity * 255
        );
    };
    gui.add_item(spray_2_input, "spray_2_input");
    
    
    //Previous spray icon.
    GuiItem* prev_spray_icon = new GuiItem();
    prev_spray_icon->on_draw =
    [this] (const DrawInfo & draw) {
        draw_spray_icon(BUBBLE_RELATION_PREVIOUS);
    };
    gui.add_item(prev_spray_icon, "spray_prev_icon");
    spray_icon_mgr.register_bubble(BUBBLE_RELATION_PREVIOUS, prev_spray_icon);
    
    
    //Previous spray input.
    GuiItem* prev_spray_input = new GuiItem();
    prev_spray_input->on_draw =
    [this] (const DrawInfo & draw) {
        if(!game.options.misc.show_hud_input_icons) return;
        if(!game.states.gameplay->cur_leader_ptr) return;
        
        size_t prev_spray_idx = INVALID;
        if(game.content.spray_types.list.size() >= 3) {
            prev_spray_idx =
                sum_and_wrap(
                    (int) game.states.gameplay->selected_spray,
                    -1,
                    (int) game.content.spray_types.list.size()
                );
        }
        if(prev_spray_idx == INVALID) return;
        
        PlayerInputSource s;
        if(game.content.spray_types.list.size() >= 3) {
            s =
                game.controls.find_bind(PLAYER_ACTION_TYPE_PREV_SPRAY).
                inputSource;
        }
        if(s.type == INPUT_SOURCE_TYPE_NONE) return;
        
        draw_player_input_source_icon(
            game.sys_content.fnt_slim, s, true, draw.center, draw.size,
            game.states.gameplay->hud->spray_items_opacity * 255
        );
    };
    gui.add_item(prev_spray_input, "spray_prev_input");
    
    
    //Next spray icon.
    GuiItem* next_spray_icon = new GuiItem();
    next_spray_icon->on_draw =
    [this] (const DrawInfo & draw) {
        draw_spray_icon(BUBBLE_RELATION_NEXT);
    };
    gui.add_item(next_spray_icon, "spray_next_icon");
    spray_icon_mgr.register_bubble(BUBBLE_RELATION_NEXT, next_spray_icon);
    
    
    //Next spray input.
    GuiItem* next_spray_input = new GuiItem();
    next_spray_input->on_draw =
    [this] (const DrawInfo & draw) {
        if(!game.options.misc.show_hud_input_icons) return;
        if(!game.states.gameplay->cur_leader_ptr) return;
        
        size_t next_spray_idx = INVALID;
        if(game.content.spray_types.list.size() >= 3) {
            next_spray_idx =
                sum_and_wrap(
                    (int) game.states.gameplay->selected_spray,
                    1,
                    (int) game.content.spray_types.list.size()
                );
        }
        if(next_spray_idx == INVALID) return;
        
        PlayerInputSource s;
        if(game.content.spray_types.list.size() >= 3) {
            s =
                game.controls.find_bind(PLAYER_ACTION_TYPE_NEXT_SPRAY).
                inputSource;
        }
        if(s.type == INPUT_SOURCE_TYPE_NONE) return;
        
        draw_player_input_source_icon(
            game.sys_content.fnt_slim, s, true, draw.center, draw.size,
            game.states.gameplay->hud->spray_items_opacity * 255
        );
    };
    gui.add_item(next_spray_input, "spray_next_input");
    
    
    if(game.cur_area_data->type == AREA_TYPE_MISSION) {
    
        //Mission goal bubble.
        GuiItem* mission_goal_bubble = new GuiItem();
        mission_goal_bubble->on_draw =
        [this] (const DrawInfo & draw) {
            int cx = 0;
            int cy = 0;
            int cw = 0;
            int ch = 0;
            al_get_clipping_rectangle(&cx, &cy, &cw, &ch);
            set_combined_clipping_rectangles(
                cx, cy, cw, ch,
                draw.center.x - draw.size.x / 2.0f,
                draw.center.y - draw.size.y / 2.0f,
                draw.size.x * game.states.gameplay->goal_indicator_ratio + 1,
                draw.size.y
            );
            draw_filled_rounded_rectangle(
                draw.center, draw.size, 20.0f, al_map_rgba(86, 149, 50, 160)
            );
            set_combined_clipping_rectangles(
                cx, cy, cw, ch,
                draw.center.x - draw.size.x / 2.0f +
                draw.size.x * game.states.gameplay->goal_indicator_ratio,
                draw.center.y - draw.size.y / 2.0f,
                draw.size.x * (1 - game.states.gameplay->goal_indicator_ratio),
                draw.size.y
            );
            draw_filled_rounded_rectangle(
                draw.center, draw.size, 20.0f, al_map_rgba(34, 102, 102, 80)
            );
            al_set_clipping_rectangle(cx, cy, cw, ch);
            draw_textured_box(
                draw.center, draw.size, game.sys_content.bmp_bubble_box,
                al_map_rgba(255, 255, 255, 200)
            );
        };
        gui.add_item(mission_goal_bubble, "mission_goal_bubble");
        
        
        string goal_cur_label_text =
            game.mission_goals[game.cur_area_data->mission.goal]->
            get_hud_label();
            
        if(!goal_cur_label_text.empty()) {
            //Mission goal current label.
            GuiItem* mission_goal_cur_label = new GuiItem();
            mission_goal_cur_label->on_draw =
                [this, goal_cur_label_text]
            (const DrawInfo & draw) {
                draw_text(
                    goal_cur_label_text, game.sys_content.fnt_standard,
                    draw.center, draw.size, al_map_rgba(255, 255, 255, 128)
                );
            };
            gui.add_item(mission_goal_cur_label, "mission_goal_cur_label");
            
            
            //Mission goal current.
            GuiItem* mission_goal_cur = new GuiItem();
            mission_goal_cur->on_draw =
                [this, mission_goal_cur]
            (const DrawInfo & draw) {
                int value =
                    game.mission_goals[game.cur_area_data->mission.goal]->
                    get_cur_amount(game.states.gameplay);
                string text;
                if(
                    game.cur_area_data->mission.goal ==
                    MISSION_GOAL_TIMED_SURVIVAL
                ) {
                    text = time_to_str2(value, ":", "");
                } else {
                    text = i2s(value);
                }
                float juicy_grow_amount =
                    mission_goal_cur->get_juice_value();
                draw_text(
                    text, game.sys_content.fnt_counter, draw.center, draw.size,
                    COLOR_WHITE, ALLEGRO_ALIGN_CENTER, V_ALIGN_MODE_CENTER, 0,
                    Point(1.0 + juicy_grow_amount)
                );
            };
            gui.add_item(mission_goal_cur, "mission_goal_cur");
            game.states.gameplay->mission_goal_cur_text = mission_goal_cur;
            
            
            //Mission goal requirement label.
            GuiItem* mission_goal_req_label = new GuiItem();
            mission_goal_req_label->on_draw =
            [this] (const DrawInfo & draw) {
                draw_text(
                    "Goal", game.sys_content.fnt_standard, draw.center, draw.size,
                    al_map_rgba(255, 255, 255, 128)
                );
            };
            gui.add_item(mission_goal_req_label, "mission_goal_req_label");
            
            
            //Mission goal requirement.
            GuiItem* mission_goal_req = new GuiItem();
            mission_goal_req->on_draw =
            [this] (const DrawInfo & draw) {
                int value =
                    game.mission_goals[game.cur_area_data->mission.goal]->
                    get_req_amount(game.states.gameplay);
                string text;
                if(
                    game.cur_area_data->mission.goal ==
                    MISSION_GOAL_TIMED_SURVIVAL
                ) {
                    text = time_to_str2(value, ":", "");
                } else {
                    text = i2s(value);
                }
                draw_text(
                    text, game.sys_content.fnt_counter, draw.center, draw.size
                );
            };
            gui.add_item(mission_goal_req, "mission_goal_req");
            
            
            //Mission goal slash.
            GuiItem* mission_goal_slash = new GuiItem();
            mission_goal_slash->on_draw =
            [this] (const DrawInfo & draw) {
                draw_text(
                    "/", game.sys_content.fnt_counter, draw.center, draw.size
                );
            };
            gui.add_item(mission_goal_slash, "mission_goal_slash");
            
        } else {
        
            //Mission goal name text.
            GuiItem* mission_goal_name = new GuiItem();
            mission_goal_name->on_draw =
            [this] (const DrawInfo & draw) {
                draw_text(
                    game.mission_goals[game.cur_area_data->mission.goal]->
                    get_name(), game.sys_content.fnt_standard,
                    draw.center, draw.size, al_map_rgba(255, 255, 255, 128)
                );
            };
            gui.add_item(mission_goal_name, "mission_goal_name");
            
        }
        
    }
    
    if(
        game.cur_area_data->type == AREA_TYPE_MISSION &&
        game.cur_area_data->mission.grading_mode == MISSION_GRADING_MODE_POINTS &&
        game.cur_area_data->mission.point_hud_data != 0
    ) {
    
        //Mission score bubble.
        GuiItem* mission_score_bubble = new GuiItem();
        mission_score_bubble->on_draw =
        [this] (const DrawInfo & draw) {
            draw_filled_rounded_rectangle(
                draw.center, draw.size, 20.0f, al_map_rgba(86, 149, 50, 160)
            );
            draw_textured_box(
                draw.center, draw.size, game.sys_content.bmp_bubble_box,
                al_map_rgba(255, 255, 255, 200)
            );
        };
        gui.add_item(mission_score_bubble, "mission_score_bubble");
        
        
        //Mission score "score" label.
        GuiItem* mission_score_score_label = new GuiItem();
        mission_score_score_label->on_draw =
        [this] (const DrawInfo & draw) {
            draw_text(
                "Score:", game.sys_content.fnt_standard,
                Point(draw.center.x + draw.size.x / 2.0f, draw.center.y), draw.size,
                al_map_rgba(255, 255, 255, 128), ALLEGRO_ALIGN_RIGHT
            );
        };
        gui.add_item(mission_score_score_label, "mission_score_score_label");
        
        
        //Mission score points.
        GuiItem* mission_score_points = new GuiItem();
        mission_score_points->on_draw =
            [this, mission_score_points]
        (const DrawInfo & draw) {
            float juicy_grow_amount = mission_score_points->get_juice_value();
            draw_text(
                i2s(game.states.gameplay->mission_score),
                game.sys_content.fnt_counter, draw.center, draw.size, COLOR_WHITE,
                ALLEGRO_ALIGN_CENTER, V_ALIGN_MODE_CENTER, 0,
                Point(1.0 + juicy_grow_amount)
            );
        };
        gui.add_item(mission_score_points, "mission_score_points");
        game.states.gameplay->mission_score_cur_text = mission_score_points;
        
        
        //Mission score "points" label.
        GuiItem* mission_score_points_label = new GuiItem();
        mission_score_points_label->on_draw =
        [this] (const DrawInfo & draw) {
            draw_text(
                "pts", game.sys_content.fnt_standard,
                Point(draw.center.x + draw.size.x / 2.0f, draw.center.y), draw.size,
                al_map_rgba(255, 255, 255, 128), ALLEGRO_ALIGN_RIGHT
            );
        };
        gui.add_item(mission_score_points_label, "mission_score_points_label");
        
        
        //Mission score ruler.
        GuiItem* mission_score_ruler = new GuiItem();
        mission_score_ruler->on_draw =
        [this] (const DrawInfo & draw) {
            //Setup.
            const float ruler_start_value =
                game.states.gameplay->score_indicator -
                HUD::SCORE_RULER_RANGE / 2.0f;
            const float ruler_end_value =
                game.states.gameplay->score_indicator +
                HUD::SCORE_RULER_RANGE / 2.0f;
            const float ruler_scale =
                draw.size.x / (float) HUD::SCORE_RULER_RANGE;
            const float ruler_start_x = draw.center.x - draw.size.x / 2.0f;
            const float ruler_end_x = draw.center.x + draw.size.x / 2.0f;
            
            const float seg_limits[] = {
                std::min(ruler_start_value, 0.0f),
                0,
                (float) game.cur_area_data->mission.bronze_req,
                (float) game.cur_area_data->mission.silver_req,
                (float) game.cur_area_data->mission.gold_req,
                (float) game.cur_area_data->mission.platinum_req,
                std::max(
                    ruler_end_value,
                    (float) game.cur_area_data->mission.platinum_req
                )
            };
            const ALLEGRO_COLOR seg_colors_top[] = {
                al_map_rgba(152, 160, 152, 128), //Negatives.
                al_map_rgb(158, 166, 158),       //No medal.
                al_map_rgb(203, 117, 37),        //Bronze.
                al_map_rgb(223, 227, 209),       //Silver.
                al_map_rgb(235, 209, 59),        //Gold.
                al_map_rgb(158, 222, 211)        //Platinum.
            };
            const ALLEGRO_COLOR seg_colors_bottom[] = {
                al_map_rgba(152, 160, 152, 128), //Negatives.
                al_map_rgb(119, 128, 118),       //No medal.
                al_map_rgb(131, 52, 18),         //Bronze.
                al_map_rgb(160, 154, 127),       //Silver.
                al_map_rgb(173, 127, 24),        //Gold.
                al_map_rgb(79, 172, 153)         //Platinum.
            };
            ALLEGRO_BITMAP* seg_icons[] = {
                nullptr,
                nullptr,
                game.sys_content.bmp_medal_bronze,
                game.sys_content.bmp_medal_silver,
                game.sys_content.bmp_medal_gold,
                game.sys_content.bmp_medal_platinum
            };
            
            //Draw each segment (no medal, bronze, etc.).
            for(int s = 0; s < 6; s++) {
                float seg_start_value = seg_limits[s];
                float seg_end_value = seg_limits[s + 1];
                if(seg_end_value < ruler_start_value) continue;
                if(seg_start_value > ruler_end_value) continue;
                float seg_start_x =
                    draw.center.x -
                    (game.states.gameplay->score_indicator - seg_start_value) *
                    ruler_scale;
                float seg_end_x =
                    draw.center.x +
                    (seg_end_value - game.states.gameplay->score_indicator) *
                    ruler_scale;
                seg_start_x = std::max(seg_start_x, ruler_start_x);
                seg_end_x = std::min(ruler_end_x, seg_end_x);
                
                ALLEGRO_VERTEX vertexes[4];
                for(unsigned char v = 0; v < 4; v++) {
                    vertexes[v].z = 0.0f;
                }
                vertexes[0].x = seg_start_x;
                vertexes[0].y = draw.center.y - draw.size.y / 2.0f;
                vertexes[0].color = seg_colors_top[s];
                vertexes[1].x = seg_start_x;
                vertexes[1].y = draw.center.y + draw.size.y / 2.0f;
                vertexes[1].color = seg_colors_bottom[s];
                vertexes[2].x = seg_end_x;
                vertexes[2].y = draw.center.y + draw.size.y / 2.0f;
                vertexes[2].color = seg_colors_bottom[s];
                vertexes[3].x = seg_end_x;
                vertexes[3].y = draw.center.y - draw.size.y / 2.0f;
                vertexes[3].color = seg_colors_top[s];
                al_draw_prim(
                    vertexes, nullptr, nullptr, 0, 4, ALLEGRO_PRIM_TRIANGLE_FAN
                );
            }
            
            //Draw the markings.
            for(
                float m = floor(ruler_start_value / 25.0f) * 25.0f;
                m <= ruler_end_value;
                m += 25.0f
            ) {
                if(m < 0.0f || m < ruler_start_value) continue;
                float marking_x =
                    draw.center.x -
                    (game.states.gameplay->score_indicator - m) *
                    ruler_scale;
                float marking_length =
                    fmod(m, 100) == 0 ?
                    draw.size.y * 0.7f :
                    fmod(m, 50) == 0 ?
                    draw.size.y * 0.4f :
                    draw.size.y * 0.1f;
                al_draw_filled_triangle(
                    marking_x,
                    draw.center.y - draw.size.y / 2.0f + marking_length,
                    marking_x + 2.0f,
                    draw.center.y - draw.size.y / 2.0f,
                    marking_x - 2.0f,
                    draw.center.y - draw.size.y / 2.0f,
                    al_map_rgb(100, 110, 180)
                );
            }
            
            //Draw the medal icons.
            int cur_seg = 0;
            int last_passed_seg = 0;
            float cur_medal_scale =
                HUD::MEDAL_ICON_SCALE +
                sin(
                    game.states.gameplay->area_time_passed *
                    HUD::MEDAL_ICON_SCALE_TIME_MULT
                ) *
                HUD::MEDAL_ICON_SCALE_MULT;
            for(int s = 0; s < 6; s++) {
                float seg_start_value = seg_limits[s];
                if(seg_start_value <= game.states.gameplay->score_indicator) {
                    cur_seg = s;
                }
                if(seg_start_value <= ruler_start_value) {
                    last_passed_seg = s;
                }
            }
            for(int s = 0; s < 6; s++) {
                if(!seg_icons[s]) continue;
                float seg_start_value = seg_limits[s];
                if(seg_start_value < ruler_start_value) continue;
                float seg_start_x =
                    draw.center.x -
                    (game.states.gameplay->score_indicator - seg_start_value) *
                    ruler_scale;
                float icon_x = seg_start_x;
                unsigned char icon_alpha = 255;
                float icon_scale = HUD::MEDAL_ICON_SCALE;
                if(cur_seg == s) {
                    icon_scale = cur_medal_scale;
                }
                if(seg_start_value > ruler_end_value) {
                    icon_x = ruler_end_x;
                    icon_alpha = 128;
                }
                draw_bitmap(
                    seg_icons[s],
                    Point(icon_x, draw.center.y),
                    Point(-1, draw.size.y * icon_scale),
                    0,
                    al_map_rgba(255, 255, 255, icon_alpha)
                );
                if(seg_start_value > ruler_end_value) {
                    //If we found the first icon that goes past the ruler's end,
                    //then we shouldn't draw the other ones that come after.
                    break;
                }
            }
            if(seg_icons[last_passed_seg] && last_passed_seg == cur_seg) {
                draw_bitmap(
                    seg_icons[last_passed_seg],
                    Point(ruler_start_x, draw.center.y),
                    Point(-1, draw.size.y * cur_medal_scale)
                );
            }
            
            //Draw the flapper.
            al_draw_filled_triangle(
                draw.center.x, draw.center.y + draw.size.y / 2.0f,
                draw.center.x, draw.center.y,
                draw.center.x + (draw.size.y * 0.4), draw.center.y + draw.size.y / 2.0f,
                al_map_rgb(105, 161, 105)
            );
            al_draw_filled_triangle(
                draw.center.x, draw.center.y + draw.size.y / 2.0f,
                draw.center.x, draw.center.y,
                draw.center.x - (draw.size.y * 0.4), draw.center.y + draw.size.y / 2.0f,
                al_map_rgb(124, 191, 124)
            );
        };
        gui.add_item(mission_score_ruler, "mission_score_ruler");
        
    }
    
    if(
        game.cur_area_data->type == AREA_TYPE_MISSION &&
        game.cur_area_data->mission.fail_hud_primary_cond != INVALID
    ) {
        create_mission_fail_cond_items(true);
    }
    if(
        game.cur_area_data->type == AREA_TYPE_MISSION &&
        game.cur_area_data->mission.fail_hud_secondary_cond != INVALID
    ) {
        create_mission_fail_cond_items(false);
    }
    
    
    DataNode* bitmaps_node = hud_file_node->getChildByName("files");
    
#define loader(var, name) \
    var = \
          game.content.bitmaps.list.get( \
                                         bitmaps_node->getChildByName(name)->value, \
                                         bitmaps_node->getChildByName(name) \
                                       );
    
    loader(bmp_bubble,                 "bubble");
    loader(bmp_counter_bubble_field,   "counter_bubble_field");
    loader(bmp_counter_bubble_group,   "counter_bubble_group");
    loader(bmp_counter_bubble_standby, "counter_bubble_standby");
    loader(bmp_counter_bubble_total,   "counter_bubble_total");
    loader(bmp_day_bubble,             "day_bubble");
    loader(bmp_distant_pikmin_marker,  "distant_pikmin_marker");
    loader(bmp_hard_bubble,            "hard_bubble");
    loader(bmp_no_pikmin_bubble,       "no_pikmin_bubble");
    loader(bmp_sun,                    "sun");
    
#undef loader
    
    leader_icon_mgr.move_method = HUD_BUBBLE_MOVE_METHOD_CIRCLE;
    leader_icon_mgr.transition_duration = HUD::LEADER_SWAP_JUICE_DURATION;
    leader_health_mgr.move_method = HUD_BUBBLE_MOVE_METHOD_CIRCLE;
    leader_health_mgr.transition_duration = HUD::LEADER_SWAP_JUICE_DURATION;
    standby_icon_mgr.move_method = HUD_BUBBLE_MOVE_METHOD_STRAIGHT;
    standby_icon_mgr.transition_duration = HUD::STANDBY_SWAP_JUICE_DURATION;
    spray_icon_mgr.move_method = HUD_BUBBLE_MOVE_METHOD_STRAIGHT;
    spray_icon_mgr.transition_duration = HUD::SPRAY_SWAP_JUICE_DURATION;
    
}


/**
 * @brief Destroys the HUD struct object.
 */
Hud::~Hud() {
    game.content.bitmaps.list.free(bmp_bubble);
    game.content.bitmaps.list.free(bmp_counter_bubble_field);
    game.content.bitmaps.list.free(bmp_counter_bubble_group);
    game.content.bitmaps.list.free(bmp_counter_bubble_standby);
    game.content.bitmaps.list.free(bmp_counter_bubble_total);
    game.content.bitmaps.list.free(bmp_day_bubble);
    game.content.bitmaps.list.free(bmp_distant_pikmin_marker);
    game.content.bitmaps.list.free(bmp_hard_bubble);
    game.content.bitmaps.list.free(bmp_no_pikmin_bubble);
    game.content.bitmaps.list.free(bmp_sun);
    
}


/**
 * @brief Creates either the primary or the secondary mission fail condition
 * HUD items.
 *
 * @param primary True if it's the primary HUD item,
 * false if it's the secondary.
 */
void Hud::create_mission_fail_cond_items(bool primary) {
    MISSION_FAIL_COND cond =
        primary ?
        (MISSION_FAIL_COND)
        game.cur_area_data->mission.fail_hud_primary_cond :
        (MISSION_FAIL_COND)
        game.cur_area_data->mission.fail_hud_secondary_cond;
        
    //Mission fail condition bubble.
    GuiItem* mission_fail_bubble = new GuiItem();
    mission_fail_bubble->on_draw =
    [this, primary] (const DrawInfo & draw) {
        int cx = 0;
        int cy = 0;
        int cw = 0;
        int ch = 0;
        float ratio =
            primary ?
            game.states.gameplay->fail_1_indicator_ratio :
            game.states.gameplay->fail_2_indicator_ratio;
        al_get_clipping_rectangle(&cx, &cy, &cw, &ch);
        set_combined_clipping_rectangles(
            cx, cy, cw, ch,
            draw.center.x - draw.size.x / 2.0f,
            draw.center.y - draw.size.y / 2.0f,
            draw.size.x * ratio + 1,
            draw.size.y
        );
        draw_filled_rounded_rectangle(
            draw.center, draw.size, 20.0f, al_map_rgba(149, 80, 50, 160)
        );
        set_combined_clipping_rectangles(
            cx, cy, cw, ch,
            draw.center.x - draw.size.x / 2.0f +
            draw.size.x * ratio,
            draw.center.y - draw.size.y / 2.0f,
            draw.size.x * (1 - ratio),
            draw.size.y
        );
        draw_filled_rounded_rectangle(
            draw.center, draw.size, 20.0f, al_map_rgba(149, 130, 50, 160)
        );
        al_set_clipping_rectangle(cx, cy, cw, ch);
        draw_textured_box(
            draw.center, draw.size, game.sys_content.bmp_bubble_box,
            al_map_rgba(255, 255, 255, 200)
        );
    };
    gui.add_item(
        mission_fail_bubble,
        primary ?
        "mission_fail_1_bubble" :
        "mission_fail_2_bubble"
    );
    
    
    if(game.mission_fail_conds[cond]->has_hud_content()) {
    
        //Mission fail condition current label.
        GuiItem* mission_fail_cur_label = new GuiItem();
        mission_fail_cur_label->on_draw =
        [this, cond] (const DrawInfo & draw) {
            draw_text(
                game.mission_fail_conds[cond]->
                get_hud_label(game.states.gameplay),
                game.sys_content.fnt_standard, draw.center, draw.size,
                al_map_rgba(255, 255, 255, 128)
            );
        };
        gui.add_item(
            mission_fail_cur_label,
            primary ?
            "mission_fail_1_cur_label" :
            "mission_fail_2_cur_label"
        );
        
        
        //Mission fail condition current.
        GuiItem* mission_fail_cur = new GuiItem();
        mission_fail_cur->on_draw =
            [this, cond, mission_fail_cur]
        (const DrawInfo & draw) {
            int value =
                game.mission_fail_conds[cond]->
                get_cur_amount(game.states.gameplay);
            string text;
            if(cond == MISSION_FAIL_COND_TIME_LIMIT) {
                text = time_to_str2(value, ":", "");
            } else {
                text = i2s(value);
            }
            float juicy_grow_amount = mission_fail_cur->get_juice_value();
            draw_text(
                text, game.sys_content.fnt_counter, draw.center, draw.size,
                COLOR_WHITE, ALLEGRO_ALIGN_CENTER, V_ALIGN_MODE_CENTER, 0,
                Point(1.0 + juicy_grow_amount)
            );
        };
        gui.add_item(
            mission_fail_cur,
            primary ?
            "mission_fail_1_cur" :
            "mission_fail_2_cur"
        );
        if(primary) {
            game.states.gameplay->mission_fail_1_cur_text = mission_fail_cur;
        } else {
            game.states.gameplay->mission_fail_2_cur_text = mission_fail_cur;
        }
        
        
        //Mission fail condition requirement label.
        GuiItem* mission_fail_req_label = new GuiItem();
        mission_fail_req_label->on_draw =
            [this]
        (const DrawInfo & draw) {
            draw_text(
                "Fail", game.sys_content.fnt_standard, draw.center, draw.size,
                al_map_rgba(255, 255, 255, 128)
            );
        };
        gui.add_item(
            mission_fail_req_label,
            primary ?
            "mission_fail_1_req_label" :
            "mission_fail_2_req_label"
        );
        
        
        //Mission fail condition requirement.
        GuiItem* mission_fail_req = new GuiItem();
        mission_fail_req->on_draw =
        [this, cond] (const DrawInfo & draw) {
            int value =
                game.mission_fail_conds[cond]->
                get_req_amount(game.states.gameplay);
            string text;
            if(cond == MISSION_FAIL_COND_TIME_LIMIT) {
                text = time_to_str2(value, ":", "");
            } else {
                text = i2s(value);
            }
            draw_text(
                text, game.sys_content.fnt_counter, draw.center, draw.size
            );
        };
        gui.add_item(
            mission_fail_req,
            primary ?
            "mission_fail_1_req" :
            "mission_fail_2_req"
        );
        
        
        //Mission primary fail condition slash.
        GuiItem* mission_fail_slash = new GuiItem();
        mission_fail_slash->on_draw =
        [this] (const DrawInfo & draw) {
            draw_text(
                "/", game.sys_content.fnt_counter, draw.center, draw.size
            );
        };
        gui.add_item(
            mission_fail_slash,
            primary ?
            "mission_fail_1_slash" :
            "mission_fail_2_slash"
        );
        
    } else {
    
        //Mission fail condition name text.
        GuiItem* mission_fail_name = new GuiItem();
        mission_fail_name->on_draw =
        [this, cond] (const DrawInfo & draw) {
            draw_text(
                "Fail: " +
                game.mission_fail_conds[cond]->get_name(),
                game.sys_content.fnt_standard, draw.center, draw.size,
                al_map_rgba(255, 255, 255, 128)
            );
        };
        gui.add_item(
            mission_fail_name,
            primary ?
            "mission_fail_1_name" :
            "mission_fail_2_name"
        );
        
    }
}


/**
 * @brief Code to draw a spray icon with. This does not apply to the
 * second spray.
 *
 * @param which Which spray icon to draw -- the previous type's,
 * the current type's, or the next type's.
 */
void Hud::draw_spray_icon(BUBBLE_RELATION which) {
    if(!game.states.gameplay->cur_leader_ptr) return;
    
    DrawInfo draw;
    ALLEGRO_BITMAP* icon;
    game.states.gameplay->hud->spray_icon_mgr.get_drawing_info(
        which, &icon, &draw
    );
    
    if(!icon) return;
    draw_bitmap_in_box(
        icon, draw.center, draw.size, true, 0.0f,
        map_alpha(game.states.gameplay->hud->spray_items_opacity * 255)
    );
}


/**
 * @brief Code to draw a standby icon with.
 *
 * @param which Which standby icon to draw -- the previous type's,
 * the current type's, or the next type's.
 */
void Hud::draw_standby_icon(BUBBLE_RELATION which) {
    DrawInfo draw;
    ALLEGRO_BITMAP* icon;
    game.states.gameplay->hud->standby_icon_mgr.get_drawing_info(
        which, &icon, &draw
    );
    
    if(!icon) return;
    
    ALLEGRO_COLOR color =
        map_alpha(game.states.gameplay->hud->standby_items_opacity * 255);
        
    draw_bitmap_in_box(icon, draw.center, draw.size * 0.8, true, 0.0f, color);
    
    if(
        game.states.gameplay->closest_group_member_distant &&
        which == BUBBLE_RELATION_CURRENT
    ) {
        draw_bitmap_in_box(
            bmp_distant_pikmin_marker,
            draw.center,
            draw.size * 0.8,
            true,
            0.0f, color
        );
    }
    
    draw_bitmap_in_box(bmp_bubble, draw.center, draw.size, true, 0.0f, color);
}


/**
 * @brief Ticks time by one frame of logic.
 *
 * @param delta_t How long the frame's tick is, in seconds.
 */
void Hud::tick(float delta_t) {
    //Update leader bubbles.
    for(size_t l = 0; l < 3; l++) {
        Leader* l_ptr = nullptr;
        if(l < game.states.gameplay->available_leaders.size()) {
            size_t l_idx =
                (size_t) sum_and_wrap(
                    (int) game.states.gameplay->cur_leader_idx,
                    (int) l,
                    (int) game.states.gameplay->available_leaders.size()
                );
            l_ptr = game.states.gameplay->available_leaders[l_idx];
        }
        
        LeaderIconBubble icon;
        icon.bmp = nullptr;
        icon.color = COLOR_EMPTY;
        if(l_ptr) {
            icon.bmp = l_ptr->lea_type->bmp_icon;
            icon.color = l_ptr->lea_type->main_color;
        }
        
        leader_icon_mgr.update(l, l_ptr, icon);
        
        LeaderHealthBubble health;
        health.ratio = 0.0f;
        health.caution_timer = 0.0f;
        if(l_ptr) {
            health.ratio = l_ptr->health_wheel_visible_ratio;
            health.caution_timer = l_ptr->health_wheel_caution_timer;
        }
        leader_health_mgr.update(l, l_ptr, health);
    }
    leader_icon_mgr.tick(delta_t);
    leader_health_mgr.tick(delta_t);
    
    //Update standby bubbles.
    for(unsigned char s = 0; s < 3; s++) {
    
        ALLEGRO_BITMAP* icon = nullptr;
        Leader* cur_leader_ptr = game.states.gameplay->cur_leader_ptr;
        Mob* member = game.states.gameplay->closest_group_member[s];
        SubgroupType* type = nullptr;
        
        if(cur_leader_ptr) {
            switch(s) {
            case BUBBLE_RELATION_PREVIOUS: {
                SubgroupType* prev_type;
                cur_leader_ptr->group->get_next_standby_type(true, &prev_type);
                SubgroupType* next_type;
                cur_leader_ptr->group->get_next_standby_type(false, &next_type);
                if(
                    prev_type != cur_leader_ptr->group->cur_standby_type &&
                    prev_type != next_type
                ) {
                    type = prev_type;
                }
                break;
            }
            case BUBBLE_RELATION_CURRENT: {
                type = cur_leader_ptr->group->cur_standby_type;
                break;
            }
            case BUBBLE_RELATION_NEXT: {
                SubgroupType* next_type;
                cur_leader_ptr->group->get_next_standby_type(false, &next_type);
                if(next_type != cur_leader_ptr->group->cur_standby_type) {
                    type = next_type;
                }
                break;
            }
            }
        }
        
        if(cur_leader_ptr && type && member) {
            SUBGROUP_TYPE_CATEGORY cat = type->get_category();
            
            switch(cat) {
            case SUBGROUP_TYPE_CATEGORY_LEADER: {
                Leader* l_ptr = dynamic_cast<Leader*>(member);
                icon = l_ptr->lea_type->bmp_icon;
                break;
            } default: {
                icon = type->get_icon();
                break;
            }
            }
        }
        
        if(!icon && s == BUBBLE_RELATION_CURRENT) {
            icon = bmp_no_pikmin_bubble;
        }
        
        standby_icon_mgr.update(s, type, icon);
    }
    standby_icon_mgr.tick(delta_t);
    
    //Update spray bubbles.
    size_t top_spray_idx = INVALID;
    if(game.content.spray_types.list.size() > 2) {
        top_spray_idx = game.states.gameplay->selected_spray;
    } else if(!game.content.spray_types.list.empty() && game.content.spray_types.list.size() <= 2) {
        top_spray_idx = 0;
    }
    spray_icon_mgr.update(
        BUBBLE_RELATION_CURRENT,
        top_spray_idx == INVALID ? nullptr :
        &game.states.gameplay->spray_stats[top_spray_idx],
        top_spray_idx == INVALID ? nullptr :
        game.config.misc.spray_order[top_spray_idx]->bmp_spray
    );
    
    size_t prev_spray_idx = INVALID;
    if(game.content.spray_types.list.size() >= 3) {
        prev_spray_idx =
            sum_and_wrap(
                (int) game.states.gameplay->selected_spray,
                -1,
                (int) game.content.spray_types.list.size()
            );
    }
    spray_icon_mgr.update(
        BUBBLE_RELATION_PREVIOUS,
        prev_spray_idx == INVALID ? nullptr :
        &game.states.gameplay->spray_stats[prev_spray_idx],
        prev_spray_idx == INVALID ? nullptr :
        game.config.misc.spray_order[prev_spray_idx]->bmp_spray
    );
    
    size_t next_spray_idx = INVALID;
    if(game.content.spray_types.list.size() >= 3) {
        next_spray_idx =
            sum_and_wrap(
                (int) game.states.gameplay->selected_spray,
                1,
                (int) game.content.spray_types.list.size()
            );
    }
    spray_icon_mgr.update(
        BUBBLE_RELATION_NEXT,
        next_spray_idx == INVALID ? nullptr :
        &game.states.gameplay->spray_stats[next_spray_idx],
        next_spray_idx == INVALID ? nullptr :
        game.config.misc.spray_order[next_spray_idx]->bmp_spray
    );
    
    spray_icon_mgr.tick(delta_t);
    
    //Update the standby items opacity.
    if(
        !game.states.gameplay->cur_leader_ptr ||
        game.states.gameplay->cur_leader_ptr->group->members.empty()
    ) {
        if(standby_items_fade_timer > 0.0f) {
            standby_items_fade_timer -= delta_t;
        } else {
            standby_items_opacity -=
                HUD::UNNECESSARY_ITEMS_FADE_OUT_SPEED * delta_t;
        }
    } else {
        standby_items_fade_timer =
            HUD::UNNECESSARY_ITEMS_FADE_OUT_DELAY;
        standby_items_opacity +=
            HUD::UNNECESSARY_ITEMS_FADE_IN_SPEED * delta_t;
    }
    standby_items_opacity = std::clamp(standby_items_opacity, 0.0f, 1.0f);
    
    //Update the spray items opacity.
    size_t total_sprays = 0;
    for(size_t s = 0; s < game.states.gameplay->spray_stats.size(); s++) {
        total_sprays +=
            game.states.gameplay->spray_stats[s].nr_sprays;
    }
    if(total_sprays == 0) {
        if(spray_items_fade_timer > 0.0f) {
            spray_items_fade_timer -= delta_t;
        } else {
            spray_items_opacity -=
                HUD::UNNECESSARY_ITEMS_FADE_OUT_SPEED * delta_t;
        }
    } else {
        spray_items_fade_timer =
            HUD::UNNECESSARY_ITEMS_FADE_OUT_DELAY;
        spray_items_opacity +=
            HUD::UNNECESSARY_ITEMS_FADE_IN_SPEED * delta_t;
    }
    spray_items_opacity = std::clamp(spray_items_opacity, 0.0f, 1.0f);
    
    //Tick the GUI items proper.
    gui.tick(game.delta_t);
}
