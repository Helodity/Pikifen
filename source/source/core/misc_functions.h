/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Globally used functions.
 */

#pragma once

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_native_dialog.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>

#include "../core/game.h"
#include "../content/area/area.h"
#include "../content/area/sector.h"
#include "../content/mob/leader.h"
#include "../content/mob/onion.h"
#include "../content/mob/pikmin.h"
#include "../content/other/gui.h"
#include "../content/other/mob_script.h"
#include "../game_state/editor.h"
#include "../lib/data_file/data_file.h"
#include "controls_mediator.h"


//A custom-made assertion.
#define engineAssert(expr, message) \
    if(!(expr)) { \
        string info = "\"" #expr "\", in "; \
        info += __FUNCTION__; \
        info += " ("; \
        info += __FILE__; \
        info += ":"; \
        info += std::to_string((long long) (__LINE__)); \
        info += "). Extra info: "; \
        info += (message); \
        crash("Assert", info, 1); \
    }

//Returns the task range for whether the Pikmin is idling or being C-sticked.
#define taskRange(p) \
    (((p)->followingGroup == curLeaderPtr && swarmMagnitude) ? \
     game.config.pikmin.swarmTaskRange : game.config.pikmin.idleTaskRange)


/**
 * @brief Function that checks if an edge should use a given edge offset effect.
 *
 * The first parameter is the edge to check.
 * The second parameter is where the affected sector gets returned to.
 * The third parameter is where the unaffected sector gets returned to.
 * Returns whether it should receive the effect.
 */
typedef bool (*offset_effect_checker_t)(Edge*, Sector**, Sector**);

/**
 * @brief Function that returns an edge's edge offset effect color.
 *
 * The first parameter is the edge to check.
 * Returns the color.
 */
typedef ALLEGRO_COLOR (*offset_effect_color_getter_t)(Edge*);

/**
 * @brief Function that returns an edge's edge offset effect length.
 *
 * The first parameter is the edge to check.
 * Returns the length.
 */
typedef float (*offset_effect_length_getter_t)(Edge*);



bool areaWallsBetween(
    const Point &p1, const Point &p2,
    float ignore_walls_below_z = -FLT_MAX, bool* out_impassable_walls = nullptr
);
void clearAreaTextures();
void crash(const string &reason, const string &info, int exit_status);
bool doesEdgeHaveLedgeSmoothing(
    Edge* e_ptr, Sector** out_affected_sector, Sector** out_unaffected_sector
);
bool doesEdgeHaveLiquidLimit(
    Edge* e_ptr, Sector** out_affected_sector, Sector** out_unaffected_sector
);
bool doesEdgeHaveWallShadow(
    Edge* e_ptr, Sector** out_affected_sector, Sector** out_unaffected_sector
);
void drawEdgeOffsetOnBuffer(
    const vector<EdgeOffsetCache> &caches, size_t e_idx
);
Mob* getClosestMobToCursor(bool must_have_health = false);
void getEdgeOffsetEdgeInfo(
    Edge* e_ptr, Vertex* end_vertex, unsigned char end_idx,
    float edge_process_angle,
    offset_effect_checker_t checker,
    offset_effect_length_getter_t length_getter,
    offset_effect_color_getter_t color_getter,
    float* out_angle, float* out_length, ALLEGRO_COLOR* out_color,
    float* out_elbow_angle, float* out_elbow_length
);
void getEdgeOffsetIntersection(
    const Edge* e1, const Edge* e2, const Vertex* common_vertex,
    float base_shadow_angle1, float base_shadow_angle2,
    float shadow_length,
    float* out_angle, float* out_length
);
ALLEGRO_COLOR getLedgeSmoothingColor(Edge* e_ptr);
ALLEGRO_COLOR getLiquidLimitColor(Edge* e_ptr);
float getLedgeSmoothingLength(Edge* e_ptr);
float getLiquidLimitLength(Edge* e_ptr);
string getMissionRecordEntryName(Area* area_ptr);
void getNextEdge(
    Vertex* v_ptr, float pivot_angle, bool clockwise,
    const Edge* ignore, Edge** out_edge, float* out_angle, float* out_diff
);
Mob* getNextMobNearCursor(
    Mob* pivot, bool must_have_health = false
);
void getNextOffsetEffectEdge(
    Vertex* v_ptr, float pivot_angle, bool clockwise,
    const Edge* ignore, offset_effect_checker_t edge_checker,
    Edge** out_edge, float* out_angle, float* out_diff,
    float* out_base_shadow_angle,
    bool* out_shadow_cw
);
string getSubtitleOrMissionGoal(
    const string &subtitle, const AREA_TYPE area_type,
    const MISSION_GOAL goal
);
unsigned char getThrowPreviewVertexes(
    ALLEGRO_VERTEX* vertexes,
    float start, float end,
    const Point &leader_pos, const Point &cursor_pos,
    const ALLEGRO_COLOR &color,
    float u_offset, float u_scale,
    bool vary_thickness
);
map<string, string> getVarMap(const string &vars_string);
string getEngineVersionString();
ALLEGRO_COLOR getWallShadowColor(Edge* e_ptr);
float getWallShadowLength(Edge* e_ptr);
vector<std::pair<int, string> > getWeatherTable(DataNode* node);
void guiAddBackInputIcon(
    GuiManager* gui, const string &item_name = "back_input"
);
bool monoCombo(
    const string &label, int* current_item, const vector<string> &items,
    int popup_max_height_in_items = -1
);
bool monoCombo(
    const string &label, string* current_item, const vector<string> &items,
    int popup_max_height_in_items = -1
);
bool monoCombo(
    const string &label, string* current_item,
    const vector<string> &item_internal_values,
    const vector<string> &item_display_names,
    int popup_max_height_in_items = -1
);
bool monoButton(
    const char* label, const ImVec2 &size = ImVec2(0, 0)
);
bool monoInputText(
    const char* label, string* str, ImGuiInputTextFlags flags = 0,
    ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr
);
bool monoInputTextWithHint(
    const char* label, const char* hint, string* str,
    ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr,
    void* user_data = nullptr
);
bool monoListBox(
    const string &label, int* current_item, const vector<string> &items,
    int height_in_items = -1
);
bool monoSelectable(
    const char* label, bool selected = false, ImGuiSelectableFlags flags = 0,
    const ImVec2 &size = ImVec2(0, 0)
);
bool monoSelectable(
    const char* label, bool* p_selected, ImGuiSelectableFlags flags = 0,
    const ImVec2 &size = ImVec2(0, 0)
);
bool openManual(const string &page);
void printInfo(
    const string &text,
    float total_duration = 5.0f,
    float fade_duration = 3.0f
);
void reportFatalError(const string &s, const DataNode* dn = nullptr);
void saveMakerTools();
void saveOptions();
void saveScreenshot();
void saveStatistics();
void setStringTokenWidths(
    vector<StringToken> &tokens,
    const ALLEGRO_FONT* text_font, const ALLEGRO_FONT* control_font,
    float max_control_bitmap_height = 0, bool control_condensed = false
);
void signalHandler(int signum);
void spewPikminSeed(
    const Point pos, float z, PikminType* pik_type,
    float angle, float horizontal_speed, float vertical_speed
);
vector<vector<StringToken> > splitLongStringWithTokens(
    const vector<StringToken> &tokens, int max_width
);
ParticleGenerator standardParticleGenSetup(
    const string &internal_name, Mob* target_mob
);
void startGameplayMessage(const string &text, ALLEGRO_BITMAP* speaker_bmp);
vector<StringToken> tokenizeString(const string &s);
string unescapeString(const string &s);
void updateOffsetEffectBuffer(
    const Point &cam_tl, const Point &cam_br,
    const vector<EdgeOffsetCache> &caches, ALLEGRO_BITMAP* buffer,
    bool clear_first
);
void updateOffsetEffectCaches (
    vector<EdgeOffsetCache> &caches,
    const unordered_set<Vertex*> &vertexes_to_update,
    offset_effect_checker_t checker,
    offset_effect_length_getter_t length_getter,
    offset_effect_color_getter_t color_getter
);
Point v2p(const Vertex* v);



/**
 * @brief Goes through all keyframes in a keyframe interpolator, and lets you
 * adjust the value in each one, by running the "predicate" function for each.
 *
 * @tparam t Value type for the interpolator.
 * @param interpolator Interpolator to adjust.
 * @param predicate Function whose argument is the original value at that
 * keyframe, and whose return value is the new value.
 * @return Whether the operation succeeded.
 */
template<typename t>
bool adjustKeyframeInterpolatorValues(
    KeyframeInterpolator<t> &interpolator,
    std::function<t(const t &)> predicate
) {
    bool result = false;
    size_t n_keyframes = interpolator.getKeyframeCount();
    for(size_t k = 0; k < n_keyframes; k++) {
        const auto &orig_keyframe = interpolator.getKeyframe(k);
        interpolator.setKeyframeValue(k, predicate(orig_keyframe.second));
        result = true;
    }
    return result;
}


/**
 * @brief Processes a Dear ImGui text widget, but sets the font to be
 * monospaced.
 *
 * @tparam args_t Function argument type.
 * @param args Function arguments to pass to ImGui::Text().
 */
template <typename ...args_t>
void monoText(args_t && ...args) {
    ImGui::PushFont(game.sysContent.fntDearImGuiMonospace);
    ImGui::Text(std::forward<args_t>(args)...);
    ImGui::PopFont();
}
