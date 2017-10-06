/*
 * Copyright (c) Andre 'Espyo' Silva 2013-2017.
 * The following source file belongs to the open-source project
 * Pikmin fangame engine. Please read the included
 * README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Area editor event handler function.
 */

#include <allegro5/allegro.h>

#include "area_editor.h"
#include "../functions.h"
#include "../vars.h"

/* ----------------------------------------------------------------------------
 * Handles an Allegro event for control-related things.
 */
void area_editor::handle_controls(const ALLEGRO_EVENT &ev) {
    //TODO
    
    if(fade_mgr.is_fading()) return;
    
    gui->handle_event(ev);
    
    if(
        ev.type == ALLEGRO_EVENT_MOUSE_AXES ||
        ev.type == ALLEGRO_EVENT_MOUSE_WARPED ||
        ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN ||
        ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP
    ) {
        handle_mouse_update(ev);
    }
    
    if(
        ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN &&
        !is_mouse_in_gui(mouse_cursor_s)
    ) {
    
        if(ev.mouse.button == 1) {
            holding_m1 = true;
        } else if(ev.mouse.button == 2) {
            holding_m2 = true;
        } else if(ev.mouse.button == 3) {
            holding_m3 = true;
        }
        
        mouse_drag_start = point(ev.mouse.x, ev.mouse.y);
        mouse_drag_confirmed = false;
        
        gui->lose_focus();
        is_gui_focused = false;
        
        if(ev.mouse.button == last_mouse_click && double_click_time > 0) {
            if(ev.mouse.button == 1) {
                handle_lmb_double_click(ev);
            } else if(ev.mouse.button == 2) {
                handle_rmb_double_click(ev);
            } else if(ev.mouse.button == 3) {
                handle_mmb_double_click(ev);
            }
            
            double_click_time = 0;
            
        } else {
            if(ev.mouse.button == 1) {
                handle_lmb_down(ev);
            } else if(ev.mouse.button == 2) {
                handle_rmb_down(ev);
            } else if(ev.mouse.button == 3) {
                handle_mmb_down(ev);
            }
            
            last_mouse_click = ev.mouse.button;
            double_click_time = DOUBLE_CLICK_TIMEOUT;
        }
        
        
    } else if(
        ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN &&
        is_mouse_in_gui(mouse_cursor_s)
    ) {
        is_gui_focused = true;
        
    } else if(
        ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP
    ) {
        if(ev.mouse.button == 1) {
            holding_m1 = false;
            handle_lmb_up(ev);
        } else if(ev.mouse.button == 2) {
            holding_m2 = false;
            handle_rmb_up(ev);
        } else if(ev.mouse.button == 3) {
            holding_m3 = false;
            handle_mmb_up(ev);
        }
        
    } else if(
        ev.type == ALLEGRO_EVENT_MOUSE_AXES ||
        ev.type == ALLEGRO_EVENT_MOUSE_WARPED
    ) {
        if(
            fabs(ev.mouse.x - mouse_drag_start.x) >= MOUSE_DRAG_CONFIRM_RANGE ||
            fabs(ev.mouse.y - mouse_drag_start.y) >= MOUSE_DRAG_CONFIRM_RANGE
        ) {
            mouse_drag_confirmed = true;
        }
        
        if(mouse_drag_confirmed) {
            if(holding_m1) {
                handle_lmb_drag(ev);
            }
            if(holding_m2) {
                handle_rmb_drag(ev);
            }
            if(holding_m3) {
                handle_mmb_drag(ev);
            }
        }
        if(
            (ev.mouse.dz != 0 || ev.mouse.dw != 0) &&
            !is_mouse_in_gui(mouse_cursor_s)
        ) {
            handle_mouse_wheel(ev);
        }
        
    } else if(ev.type == ALLEGRO_EVENT_KEY_DOWN) {
        if(
            ev.keyboard.keycode == ALLEGRO_KEY_LSHIFT ||
            ev.keyboard.keycode == ALLEGRO_KEY_RSHIFT
        ) {
            is_shift_pressed = true;
            
        } else if(
            ev.keyboard.keycode == ALLEGRO_KEY_LCTRL ||
            ev.keyboard.keycode == ALLEGRO_KEY_RCTRL ||
            ev.keyboard.keycode == ALLEGRO_KEY_COMMAND
        ) {
            is_ctrl_pressed = true;
            
        }
        
        if(!is_gui_focused) {
            handle_key_down(ev);
        }
        
    } else if(ev.type == ALLEGRO_EVENT_KEY_UP) {
        if(
            ev.keyboard.keycode == ALLEGRO_KEY_LSHIFT ||
            ev.keyboard.keycode == ALLEGRO_KEY_RSHIFT
        ) {
            is_shift_pressed = false;
            
        } else if(
            ev.keyboard.keycode == ALLEGRO_KEY_LCTRL ||
            ev.keyboard.keycode == ALLEGRO_KEY_RCTRL ||
            ev.keyboard.keycode == ALLEGRO_KEY_COMMAND
        ) {
            is_ctrl_pressed = false;
            
        }
        
        if(!is_gui_focused) {
            handle_key_up(ev);
        }
        
    } else if(ev.type == ALLEGRO_EVENT_KEY_CHAR) {
        if(!is_gui_focused) {
            handle_key_char(ev);
        }
        
    }
}


/* ----------------------------------------------------------------------------
 * Handles a key being "char"-typed.
 */
void area_editor::handle_key_char(const ALLEGRO_EVENT &ev) {
    if(ev.keyboard.keycode == ALLEGRO_KEY_LEFT) {
        cam_pos.x -= DEF_GRID_INTERVAL / cam_zoom;
        
    } else if(ev.keyboard.keycode == ALLEGRO_KEY_RIGHT) {
        cam_pos.x += DEF_GRID_INTERVAL / cam_zoom;
        
    } else if(ev.keyboard.keycode == ALLEGRO_KEY_UP) {
        cam_pos.y -= DEF_GRID_INTERVAL / cam_zoom;
        
    } else if(ev.keyboard.keycode == ALLEGRO_KEY_DOWN) {
        cam_pos.y += DEF_GRID_INTERVAL / cam_zoom;
        
    } else if(ev.keyboard.keycode == ALLEGRO_KEY_MINUS) {
        zoom(cam_zoom - (cam_zoom * KEYBOARD_CAM_ZOOM), false);
        
    } else if(ev.keyboard.keycode == ALLEGRO_KEY_EQUALS) {
        zoom(cam_zoom + (cam_zoom * KEYBOARD_CAM_ZOOM), false);
        
    }
}


/* ----------------------------------------------------------------------------
 * Handles a key being pressed down.
 */
void area_editor::handle_key_down(const ALLEGRO_EVENT &ev) {
    if(ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE && !selecting) {
        selected_edges.clear();
        selected_sectors.clear();
        selected_vertexes.clear();
        selected_mobs.clear();
        selected_path_stops.clear();
        
    } else if(ev.keyboard.keycode == ALLEGRO_KEY_A && is_ctrl_pressed) {
    
        if(
            state == EDITOR_STATE_LAYOUT ||
            state == EDITOR_STATE_ASA ||
            state == EDITOR_STATE_ASB
        ) {
            selected_edges.insert(
                cur_area_data.edges.begin(), cur_area_data.edges.end()
            );
            selected_sectors.insert(
                cur_area_data.sectors.begin(), cur_area_data.sectors.end()
            );
            selected_vertexes.insert(
                cur_area_data.vertexes.begin(), cur_area_data.vertexes.end()
            );
            
        } else if(state == EDITOR_STATE_MOBS) {
            selected_mobs.insert(
                cur_area_data.mob_generators.begin(),
                cur_area_data.mob_generators.end()
            );
            
        } else if(state == EDITOR_STATE_PATHS) {
            selected_path_stops.insert(
                cur_area_data.path_stops.begin(),
                cur_area_data.path_stops.end()
            );
        }
        
    } else if(ev.keyboard.keycode == ALLEGRO_KEY_HOME) {
        bool got_something = false;
        point min_coords, max_coords;
        
        for(size_t v = 0; v < cur_area_data.vertexes.size(); ++v) {
            vertex* v_ptr = cur_area_data.vertexes[v];
            if(v_ptr->x < min_coords.x || !got_something) {
                min_coords.x = v_ptr->x;
            }
            if(v_ptr->y < min_coords.y || !got_something) {
                min_coords.y = v_ptr->y;
            }
            if(v_ptr->x > max_coords.x || !got_something) {
                max_coords.x = v_ptr->x;
            }
            if(v_ptr->y > max_coords.y || !got_something) {
                max_coords.y = v_ptr->y;
            }
            got_something = true;
        }
        
        for(size_t m = 0; m < cur_area_data.mob_generators.size(); ++m) {
            mob_gen* m_ptr = cur_area_data.mob_generators[m];
            if(m_ptr->pos.x < min_coords.x || !got_something) {
                min_coords.x = m_ptr->pos.x;
            }
            if(m_ptr->pos.y < min_coords.y || !got_something) {
                min_coords.y = m_ptr->pos.y;
            }
            if(m_ptr->pos.x > max_coords.x || !got_something) {
                max_coords.x = m_ptr->pos.x;
            }
            if(m_ptr->pos.y > max_coords.y || !got_something) {
                max_coords.y = m_ptr->pos.y;
            }
            got_something = true;
        }
        
        for(size_t s = 0; s < cur_area_data.path_stops.size(); ++s) {
            path_stop* s_ptr = cur_area_data.path_stops[s];
            if(s_ptr->pos.x < min_coords.x || !got_something) {
                min_coords.x = s_ptr->pos.x;
            }
            if(s_ptr->pos.y < min_coords.y || !got_something) {
                min_coords.y = s_ptr->pos.y;
            }
            if(s_ptr->pos.x > max_coords.x || !got_something) {
                max_coords.x = s_ptr->pos.x;
            }
            if(s_ptr->pos.y > max_coords.y || !got_something) {
                max_coords.y = s_ptr->pos.y;
            }
            got_something = true;
        }
        
        if(!got_something) return;
        
        //Place the camera in the center.
        cam_pos = (min_coords + max_coords) / 2.0f;
        
        //Reset the transformations, first.
        zoom(1.0, false);
        
        //Transform the points so we can know, at 1.0 zoom, where in the screen
        //the points would be, so we can know how much to scale by such that
        //they would align with the actual screen space.
        al_transform_coordinates(
            &world_to_screen_transform,
            &min_coords.x, &min_coords.y
        );
        al_transform_coordinates(
            &world_to_screen_transform,
            &max_coords.x, &max_coords.y
        );
        float w_ratio = (gui_x - 8) / (max_coords.x - min_coords.x);
        float h_ratio = (scr_h - 48 - 8) / (max_coords.y - min_coords.y);
        zoom(min(w_ratio, h_ratio), false);
    }
}


/* ----------------------------------------------------------------------------
 * Handles a key being released.
 */
void area_editor::handle_key_up(const ALLEGRO_EVENT &ev) {
    //TODO
}


/* ----------------------------------------------------------------------------
 * Handles the left mouse button being double-clicked.
 */
void area_editor::handle_lmb_double_click(const ALLEGRO_EVENT &ev) {
    //TODO
    handle_lmb_down(ev);
}


/* ----------------------------------------------------------------------------
 * Handles the left mouse button being pressed down.
 */
void area_editor::handle_lmb_down(const ALLEGRO_EVENT &ev) {
    //TODO
    if(
        state == EDITOR_STATE_LAYOUT ||
        state == EDITOR_STATE_ASA ||
        state == EDITOR_STATE_ASB
    ) {
    
        selected_vertexes.clear();
        selected_edges.clear();
        selected_sectors.clear();
        selecting = false;
        bool start_new_selection = true;
        
        if(!is_shift_pressed) {
        
            vertex* clicked_vertex = get_vertex_under_mouse();
            edge* clicked_edge = NULL;
            sector* clicked_sector = NULL;
            bool selected_something = false;
            
            if(clicked_vertex) {
                selected_vertexes.insert(clicked_vertex);
                selected_something = true;
            }
            
            if(!selected_something) {
                clicked_edge = get_edge_under_mouse();
                if(clicked_edge) {
                    selected_edges.insert(clicked_edge);
                    selected_something = true;
                }
            }
            
            if(!selected_something) {
                clicked_sector = get_sector_under_mouse();
                if(clicked_sector) {
                    selected_sectors.insert(clicked_sector);
                    selected_something = true;
                }
            }
            
            if(clicked_vertex || clicked_edge || clicked_sector) {
                start_new_selection = false;
            }
            
        }
        
        if(start_new_selection) {
            selecting = true;
            selection_start = mouse_cursor_w;
            selection_end = mouse_cursor_w;
        }
        
        selection_homogenized = false;
        sector_to_gui();
        asa_to_gui();
        asb_to_gui();
        
    } else if(state == EDITOR_STATE_MOBS) {
    
        selected_mobs.clear();
        selecting = false;
        bool start_new_selection = true;
        
        if(!is_shift_pressed) {
        
            mob_gen* clicked_mob = get_mob_under_mouse();
            if(clicked_mob) {
                selected_mobs.insert(clicked_mob);
                start_new_selection = false;
            }
            
        }
        
        if(start_new_selection) {
            selecting = true;
            selection_start = mouse_cursor_w;
            selection_end = mouse_cursor_w;
        }
        
        selection_homogenized = false;
        mob_to_gui();
        
    } else if(state == EDITOR_STATE_PATHS) {
    
        selected_path_stops.clear();
        selecting = false;
        bool start_new_selection = true;
        
        if(!is_shift_pressed) {
        
            path_stop* clicked_stop = get_path_stop_under_mouse();
            if(clicked_stop) {
                selected_path_stops.insert(clicked_stop);
                start_new_selection = false;
            }
            
        }
        
        if(start_new_selection) {
            selecting = true;
            selection_start = mouse_cursor_w;
            selection_end = mouse_cursor_w;
        }
        
        path_to_gui();
        
    }
}


/* ----------------------------------------------------------------------------
 * Handles the left mouse button being dragged.
 */
void area_editor::handle_lmb_drag(const ALLEGRO_EVENT &ev) {
    //TODO
    if(selecting) {
    
        float selection_x1 = min(selection_start.x, selection_end.x);
        float selection_x2 = max(selection_start.x, selection_end.x);
        float selection_y1 = min(selection_start.y, selection_end.y);
        float selection_y2 = max(selection_start.y, selection_end.y);
        selection_end = mouse_cursor_w;
        
        if(
            state == EDITOR_STATE_LAYOUT ||
            state == EDITOR_STATE_ASA ||
            state == EDITOR_STATE_ASB
        ) {
        
            selected_vertexes.clear();
            selected_edges.clear();
            selected_sectors.clear();
            
            for(size_t v = 0; v < cur_area_data.vertexes.size(); ++v) {
                vertex* v_ptr = cur_area_data.vertexes[v];
                
                if(
                    v_ptr->x >= selection_x1 &&
                    v_ptr->x <= selection_x2 &&
                    v_ptr->y >= selection_y1 &&
                    v_ptr->y <= selection_y2
                ) {
                    selected_vertexes.insert(v_ptr);
                }
            }
            
            for(size_t e = 0; e < cur_area_data.edges.size(); ++e) {
                edge* e_ptr = cur_area_data.edges[e];
                
                if(
                    e_ptr->vertexes[0]->x >= selection_x1 &&
                    e_ptr->vertexes[0]->x <= selection_x2 &&
                    e_ptr->vertexes[0]->y >= selection_y1 &&
                    e_ptr->vertexes[0]->y <= selection_y2 &&
                    e_ptr->vertexes[1]->x >= selection_x1 &&
                    e_ptr->vertexes[1]->x <= selection_x2 &&
                    e_ptr->vertexes[1]->y >= selection_y1 &&
                    e_ptr->vertexes[1]->y <= selection_y2
                ) {
                    selected_edges.insert(e_ptr);
                }
            }
            
            for(size_t s = 0; s < cur_area_data.sectors.size(); ++s) {
                sector* s_ptr = cur_area_data.sectors[s];
                bool valid_sector = true;
                
                for(size_t e = 0; e < s_ptr->edges.size(); ++e) {
                    edge* e_ptr = s_ptr->edges[e];
                    
                    if(
                        e_ptr->vertexes[0]->x < selection_x1 ||
                        e_ptr->vertexes[0]->x > selection_x2 ||
                        e_ptr->vertexes[0]->y < selection_y1 ||
                        e_ptr->vertexes[0]->y > selection_y2 ||
                        e_ptr->vertexes[1]->x < selection_x1 ||
                        e_ptr->vertexes[1]->x > selection_x2 ||
                        e_ptr->vertexes[1]->y < selection_y1 ||
                        e_ptr->vertexes[1]->y > selection_y2
                    ) {
                        valid_sector = false;
                        break;
                    }
                }
                
                if(valid_sector) {
                    selected_sectors.insert(s_ptr);
                }
            }
            
            selection_homogenized = false;
            sector_to_gui();
            asa_to_gui();
            asb_to_gui();
            
        } else if(state == EDITOR_STATE_MOBS) {
        
            selected_mobs.clear();
            
            for(size_t m = 0; m < cur_area_data.mob_generators.size(); ++m) {
                mob_gen* m_ptr = cur_area_data.mob_generators[m];
                float radius = get_mob_gen_radius(m_ptr);
                
                if(
                    m_ptr->pos.x - radius >= selection_x1 &&
                    m_ptr->pos.x + radius <= selection_x2 &&
                    m_ptr->pos.y - radius >= selection_y1 &&
                    m_ptr->pos.y + radius <= selection_y2
                ) {
                    selected_mobs.insert(m_ptr);
                }
            }
            
            selection_homogenized = false;
            mob_to_gui();
            
        } else if(state == EDITOR_STATE_PATHS) {
        
            selected_path_stops.clear();
            
            for(size_t s = 0; s < cur_area_data.path_stops.size(); ++s) {
                path_stop* s_ptr = cur_area_data.path_stops[s];
                
                if(
                    s_ptr->pos.x - PATH_STOP_RADIUS >= selection_x1 &&
                    s_ptr->pos.x + PATH_STOP_RADIUS <= selection_x2 &&
                    s_ptr->pos.y - PATH_STOP_RADIUS >= selection_y1 &&
                    s_ptr->pos.y + PATH_STOP_RADIUS <= selection_y2
                ) {
                    selected_path_stops.insert(s_ptr);
                }
            }
            
            path_to_gui();
            
        }
    }
}


/* ----------------------------------------------------------------------------
 * Handles the left mouse button being released.
 */
void area_editor::handle_lmb_up(const ALLEGRO_EVENT &ev) {
    //TODO
    selecting = false;
}


/* ----------------------------------------------------------------------------
 * Handles the middle mouse button being double-clicked.
 */
void area_editor::handle_mmb_double_click(const ALLEGRO_EVENT &ev) {
    cam_pos.x = 0;
    cam_pos.y = 0;
}


/* ----------------------------------------------------------------------------
 * Handles the middle mouse button being pressed down.
 */
void area_editor::handle_mmb_down(const ALLEGRO_EVENT &ev) {
    zoom(1.0f);
}


/* ----------------------------------------------------------------------------
 * Handles the middle mouse button being dragged.
 */
void area_editor::handle_mmb_drag(const ALLEGRO_EVENT &ev) {
}


/* ----------------------------------------------------------------------------
 * Handles the middle mouse button being released.
 */
void area_editor::handle_mmb_up(const ALLEGRO_EVENT &ev) {
}


/* ----------------------------------------------------------------------------
 * Handles the mouse coordinates being updated.
 */
void area_editor::handle_mouse_update(const ALLEGRO_EVENT &ev) {
    mouse_cursor_s.x = ev.mouse.x;
    mouse_cursor_s.y = ev.mouse.y;
    mouse_cursor_w = mouse_cursor_s;
    al_transform_coordinates(
        &screen_to_world_transform,
        &mouse_cursor_w.x, &mouse_cursor_w.y
    );
    
    if(!is_mouse_in_gui(mouse_cursor_s)) {
        lbl_status_bar->text =
            "(" + i2s(mouse_cursor_w.x) + "," + i2s(mouse_cursor_w.y) + ")";
    } else {
        lafi::widget* wum =
            gui->get_widget_under_mouse(mouse_cursor_s.x, mouse_cursor_s.y);
        if(wum) {
            lbl_status_bar->text = wum->description;
        }
    }
}


/* ----------------------------------------------------------------------------
 * Handles the mouse wheel being moved.
 */
void area_editor::handle_mouse_wheel(const ALLEGRO_EVENT &ev) {
    zoom(cam_zoom + (cam_zoom * ev.mouse.dz * 0.1));
}


/* ----------------------------------------------------------------------------
 * Handles the right mouse button being double-clicked.
 */
void area_editor::handle_rmb_double_click(const ALLEGRO_EVENT &ev) {
    //TODO
}


/* ----------------------------------------------------------------------------
 * Handles the right mouse button being pressed down.
 */
void area_editor::handle_rmb_down(const ALLEGRO_EVENT &ev) {
    //TODO
}


/* ----------------------------------------------------------------------------
 * Handles the right mouse button being dragged.
 */
void area_editor::handle_rmb_drag(const ALLEGRO_EVENT &ev) {
    cam_pos.x -= ev.mouse.dx / cam_zoom;
    cam_pos.y -= ev.mouse.dy / cam_zoom;
}


/* ----------------------------------------------------------------------------
 * Handles the right mouse button being released.
 */
void area_editor::handle_rmb_up(const ALLEGRO_EVENT &ev) {
    //TODO
}
