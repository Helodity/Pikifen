/*
 * Copyright (c) Andr� 'Espyo' Silva 2014.
 * The following source file belongs to the open-source project
 * Pikmin fangame engine. Please read the included README file
 * for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Header for the drawing-related functions.
 */

#ifndef DRAWING_INCLUDED
#define DRAWING_INCLUDED

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>

#include "controls.h"
#include "functions.h"

void do_drawing();

void draw_control(const ALLEGRO_FONT* const font, const control_info c, const float x, const float y, const float max_w, const float max_h);
void draw_compressed_text(const ALLEGRO_FONT* const font, const ALLEGRO_COLOR color, const float x, const float y, const int flags, const unsigned char valign, const float max_w, const float max_h, const string text);
void draw_fraction(const float cx, const float cy, const unsigned int current, const unsigned int needed, const ALLEGRO_COLOR color);
void draw_health(const float cx, const float cy, const unsigned int health, const unsigned int max_health, const float radius = 20, const bool just_chart = false);
void draw_sector(sector* s_ptr, const float x, const float y);
void draw_shadow(const float cx, const float cy, const float size, const float delta_z, const float shadow_stretch);
void draw_sprite(ALLEGRO_BITMAP* bmp, const float cx, const float cy, const float w, const float h, const float angle = 0, const ALLEGRO_COLOR tint = al_map_rgb(255, 255, 255));
void draw_text_lines(const ALLEGRO_FONT* const f, const ALLEGRO_COLOR c, const float x, const float y, const int fl, const unsigned char va, const string text);

#endif //ifndef DRAWING_INCLUDED