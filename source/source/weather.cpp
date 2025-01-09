/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Weather class and weather-related functions.
 */

#include <algorithm>

#include "weather.h"

#include "functions.h"
#include "game.h"
#include "utils/allegro_utils.h"
#include "utils/math_utils.h"
#include "utils/string_utils.h"


/**
 * @brief Constructs a new weather object.
 */
weather::weather() {
}


/**
 * @brief Constructs a new weather object.
 *
 * @param n Its name.
 * @param dl Daylight information table.
 * @param ss Sun strength information table.
 * @param bs Blackout strength information table.
 * @param pt Precipitation type.
 */
weather::weather(
    const string &n, const vector<std::pair<int, ALLEGRO_COLOR> > &dl,
    const vector<std::pair<int, unsigned char> > &ss,
    const vector<std::pair<int, unsigned char> > &bs
) :
    daylight(dl),
    sun_strength(ss),
    blackout_strength(bs) {
    
    name = n;
}


/**
 * @brief Returns the blackout effect's strength for the current time.
 *
 * @return The blackout strength.
 */
unsigned char weather::get_blackout_strength() {
    float ratio;
    unsigned char strength1;
    unsigned char strength2;
    bool success =
        get_table_values(
            blackout_strength, game.states.gameplay->day_minutes,
            &ratio, &strength1, &strength2
        );
        
    if(success) {
        return interpolate_number(ratio, 0.0f, 1.0f, strength1, strength2);
    } else {
        return 0;
    }
}


/**
 * @brief Returns the daylight color for the current time.
 *
 * @return The daylight color.
 */
ALLEGRO_COLOR weather::get_daylight_color() {
    float ratio;
    ALLEGRO_COLOR color1;
    ALLEGRO_COLOR color2;
    bool success =
        get_table_values(
            daylight, game.states.gameplay->day_minutes,
            &ratio, &color1, &color2
        );
        
    if(success) {
        return interpolate_color(ratio, 0.0f, 1.0f, color1, color2);
    } else {
        return al_map_rgba(255, 255, 255, 0);
    }
}


/**
 * @brief Returns the fog color for the current time.
 *
 * @return The fog color.
 */
ALLEGRO_COLOR weather::get_fog_color() {
    float ratio;
    ALLEGRO_COLOR color1;
    ALLEGRO_COLOR color2;
    bool success =
        get_table_values(
            fog_color, game.states.gameplay->day_minutes,
            &ratio, &color1, &color2
        );
        
    if(success) {
        return interpolate_color(ratio, 0.0f, 1.0f, color1, color2);
    } else {
        return al_map_rgba(255, 255, 255, 0);
    }
}


/**
 * @brief Returns the sun strength for the current time, in the range 0 - 1.
 *
 * @return The sun strength.
 */
float weather::get_sun_strength() {
    float ratio;
    unsigned char strength1;
    unsigned char strength2;
    bool success =
        get_table_values(
            sun_strength, game.states.gameplay->day_minutes,
            &ratio, &strength1, &strength2
        );
        
    if(success) {
        return
            interpolate_number(ratio, 0.0f, 1.0f, strength1, strength2) /
            255.0f;
    } else {
        return 1.0f;
    }
}


/**
 * @brief Loads weather data from a data node.
 *
 * @param node Data node to load from.
 */
void weather::load_from_data_node(data_node* node) {
    //Content metadata.
    load_metadata_from_data_node(node);
    
    //Standard data.
    reader_setter rs(node);
    
    data_node* particle_gen_node = nullptr;
    string particle_gen_str;
    
    rs.set("fog_near", fog_near);
    rs.set("fog_far", fog_far);
    rs.set("particle_generator", particle_gen_str, &particle_gen_node);
    
    fog_near = std::max(fog_near, 0.0f);
    fog_far = std::max(fog_far, fog_near);
    
    //Lighting.
    vector<std::pair<int, string> > lighting_table =
        get_weather_table(node->get_child_by_name("lighting"));
        
    for(size_t p = 0; p < lighting_table.size(); p++) {
        daylight.push_back(
            std::make_pair(
                lighting_table[p].first,
                s2c(lighting_table[p].second)
            )
        );
    }
    
    //Sun's strength.
    vector<std::pair<int, string> > sun_strength_table =
        get_weather_table(node->get_child_by_name("sun_strength"));
        
    for(size_t p = 0; p < sun_strength_table.size(); p++) {
        sun_strength.push_back(
            std::make_pair(
                sun_strength_table[p].first,
                s2i(sun_strength_table[p].second)
            )
        );
    }
    
    //Blackout effect's strength.
    vector<std::pair<int, string> > blackout_strength_table =
        get_weather_table(
            node->get_child_by_name("blackout_strength")
        );
        
    for(size_t p = 0; p < blackout_strength_table.size(); p++) {
        blackout_strength.push_back(
            std::make_pair(
                blackout_strength_table[p].first,
                s2i(blackout_strength_table[p].second)
            )
        );
    }
    
    //Fog.
    vector<std::pair<int, string> > fog_color_table =
        get_weather_table(
            node->get_child_by_name("fog_color")
        );
    for(size_t p = 0; p < fog_color_table.size(); p++) {
        fog_color.push_back(
            std::make_pair(
                fog_color_table[p].first,
                s2c(fog_color_table[p].second)
            )
        );
    }

    //Precipitation
    if(particle_gen_node) {
        if(
            game.content.custom_particle_gen.list.find(particle_gen_str) ==
            game.content.custom_particle_gen.list.end()
        ) {
            game.errors.report(
                "Unknown particle generator \"" +
                particle_gen_str + "\"!", particle_gen_node
            );
        } else {
            has_precipitation = true;
            precipitation_generator =
                &game.content.custom_particle_gen.list[particle_gen_str];
        }
    }
}
