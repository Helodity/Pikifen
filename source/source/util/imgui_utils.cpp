/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Dear ImGui-related utility functions.
 * These don't contain logic specific to the Pikifen project.
 */

#include <algorithm>
#include <cmath>

#include "imgui_utils.h"

#include "allegro_utils.h"


/**
 * @brief Adjusts the hue, saturation, and value of a given Dear ImGui color.
 *
 * @param color Color to edit.
 * @param h_delta Hue amount (0-1) to add or subtract.
 * @param s_delta Saturation amount (0-1) to add or subtract.
 * @param v_delta Value amount (0-1) to add or subtract.
 */
void ImGui::AdjustColorHSV(
    ImVec4 &color, float h_delta, float s_delta, float v_delta
) {
    float h, s, v;
    ImGui::ColorConvertRGBtoHSV(color.x, color.y, color.z, h, s, v);
    h += h_delta;
    s += s_delta;
    v += v_delta;
    ImGui::ColorConvertHSVtoRGB(h, s, v, color.x, color.y, color.z);
}


/**
 * @brief Wrapper for creating a Dear ImGui combo box widget, but
 * using a vector of strings for the list of items.
 *
 * @param label Combo widget label.
 * @param current_item Index number of the current selected item. -1 means none.
 * @param items List of items.
 * @param popup_max_height_in_items Maximum height of the popup,
 * in number of items.
 * @return Whether the value was changed.
 */
bool ImGui::Combo(
    const string &label, int* current_item, const vector<string> &items,
    int popup_max_height_in_items
) {
    string items_str;
    for(size_t i = 0; i < items.size(); i++) {
        items_str += items[i] + '\0';
    }
    
    return
        ImGui::Combo(
            label.c_str(), current_item, items_str.c_str(),
            popup_max_height_in_items
        );
}


/**
 * @brief Wrapper for creating a Dear ImGui combo box widget, but
 * using a string to control the selection,
 * as well as a vector of strings for the list of items.
 *
 * @param label Combo widget label.
 * @param current_item Name of the current selected item.
 * @param items List of items.
 * @param popup_max_height_in_items Maximum height of the popup,
 * in number of items.
 * @return Whether the value was changed.
 */
bool ImGui::Combo(
    const string &label, string* current_item, const vector<string> &items,
    int popup_max_height_in_items
) {

    string items_str;
    int item_idx = -1;
    for(size_t i = 0; i < items.size(); i++) {
        items_str += items[i] + '\0';
        if(*current_item == items[i]) {
            item_idx = (int) i;
        }
    }
    
    bool result =
        ImGui::Combo(
            label.c_str(), &item_idx, items_str.c_str(),
            popup_max_height_in_items
        );
        
    if(item_idx >= 0 && item_idx < (int) items.size()) {
        *current_item = items[item_idx];
    } else {
        *current_item = "";
    }
    
    return result;
}


/**
 * @brief Wrapper for creating a Dear ImGui combo box widget, but
 * using a string to control the selection,
 * as well as two vector of strings for the list of items, one with
 * the internal values of each item, another with the names to display.
 *
 * @param label Combo widget label.
 * @param current_item Internal value of the current selected item.
 * @param item_internal_values List of internal values for each item.
 * @param item_display_names List of names to show the user for each item.
 * @param popup_max_height_in_items Maximum height of the popup,
 * in number of items.
 * @return Whether the value was changed.
 */
bool ImGui::Combo(
    const string &label, string* current_item,
    const vector<string> &item_internal_values,
    const vector<string> &item_display_names,
    int popup_max_height_in_items
) {
    int current_item_idx = -1;
    for(size_t i = 0; i < item_internal_values.size(); i++) {
        if(item_internal_values[i] == *current_item) {
            current_item_idx = (int) i;
            break;
        }
    }
    
    bool result =
        ImGui::Combo(
            label, &current_item_idx, item_display_names,
            popup_max_height_in_items
        );
        
    if(current_item_idx == -1) {
        current_item->clear();
    } else {
        *current_item = item_internal_values[current_item_idx];
    }
    
    return result;
}


/**
 * @brief Creates two Dear ImGui drag int widgets, one that sets the
 * number of minutes, one that sets the number of seconds.
 * Though with some arguments, this can be changed to hours and minutes.
 *
 * @param label Widget label.
 * @param total_amount Time in the total amount of seconds.
 * Or minutes, or whatever the lowest unit represent is.
 * @param format1 String to write in front of the first component's value.
 * @param format2 String to write in front of the second component's value.
 * @param limit1 Maximum value for the first component.
 * @param limit2 Maximum value for the second component.
 * @return Whether either value was changed.
 */
bool ImGui::DragTime2(
    const string &label, int* total_amount,
    const string &format1, const string &format2,
    int limit1, int limit2
) {
    int part1 = floor(*total_amount / 60.0f);
    int part2 = *total_amount % 60;
    
    ImGui::BeginGroup();
    ImGui::PushID(label.c_str());
    
    //Part 1 (hours or minutes) value.
    ImGui::SetNextItemWidth(80);
    ImGui::PushID(1);
    string format = "%02d" + format1;
    bool result =
        ImGui::DragInt("", &part1, 0.1f, 0, limit1, format.c_str());
    part1 = std::max(0, part1);
    part1 = std::min(part1, limit1);
    ImGui::PopID();
    
    //Part 2 (seconds or minutes) value.
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80);
    ImGui::PushID(2);
    format = "%02d" + format2;
    result |=
        ImGui::DragInt(label.c_str(), &part2, 0.1f, 0, limit2, format.c_str());
    part2 = std::max(0, part2);
    part2 = std::min(part2, limit2);
    ImGui::PopID();
    
    ImGui::PopID();
    ImGui::EndGroup();
    
    *total_amount = part1 * 60 + part2;
    
    return result;
}


/**
 * @brief Makes it so Dear ImGui focuses on the next input text widget.
 *
 * @param condition Only focus if this boolean is true. After setting the focus,
 * this boolean is set to false. This is done so that Dear ImGui only focuses
 * when you want, like when the dialog is first shown, instead of doing it
 * every frame.
 */
void ImGui::FocusOnInputText(bool &condition) {
    if(!ImGui::IsAnyItemActive() && condition) {
        ImGui::SetKeyboardFocusHere();
        condition = false;
    }
}


/**
 * @brief Wrapper for creating a Dear ImGui combo image widget, but
 * using Allegro bitmaps.
 *
 * @param bitmap Bitmap to show on the button.
 * @param bitmap_size Width and height of the bitmap.
 * @param uv0 UV coordinates of the top-left coordinate.
 * @param uv1 UV coordinates of the bottom-right coordinate.
 * @param tint_col Tint color.
 * @param border_col Border color.
 * @return Whether the button was pressed.
 */
void ImGui::Image(
    ALLEGRO_BITMAP* bitmap, const Point &bitmap_size,
    const Point &uv0, const Point &uv1,
    const ALLEGRO_COLOR &tint_col, const ALLEGRO_COLOR &border_col
) {
    ImGui::Image(
        (ImTextureID) (intptr_t) bitmap,
        ImVec2(bitmap_size.x, bitmap_size.y),
        ImVec2(uv0.x, uv0.y),
        ImVec2(uv1.x, uv1.y),
        ImVec4(tint_col.r, tint_col.g, tint_col.b, tint_col.a),
        ImVec4(border_col.r, border_col.g, border_col.b, border_col.a)
    );
}


/**
 * @brief Wrapper for creating a Dear ImGui image button widget, but
 * using Allegro bitmaps.
 *
 * @param str_id Button widget ID.
 * @param bitmap Bitmap to show on the button.
 * @param bitmap_size Size to display the bitmap at in the GUI.
 * @param uv0 UV coordinates of the top-left coordinate.
 * @param uv1 UV coordinates of the bottom-right coordinate.
 * @param bg_col Bitmap background color.
 * @param tint_col Bitmap tint color.
 * @return Whether the button was pressed.
 */
bool ImGui::ImageButton(
    const string &str_id, ALLEGRO_BITMAP* bitmap, const Point &bitmap_size,
    const Point &uv0, const Point &uv1,
    const ALLEGRO_COLOR &bg_col,
    const ALLEGRO_COLOR &tint_col
) {
    return
        ImGui::ImageButton(
            str_id.c_str(), (ImTextureID) (intptr_t) bitmap,
            ImVec2(bitmap_size.x, bitmap_size.y),
            ImVec2(uv0.x, uv0.y),
            ImVec2(uv1.x, uv1.y),
            ImVec4(bg_col.r, bg_col.g, bg_col.b, bg_col.a),
            ImVec4(tint_col.r, tint_col.g, tint_col.b, tint_col.a)
        );
}


/**
 * @brief Wrapper for creating a Dear ImGui image button widget, but
 * using Allegro bitmaps, and keeping the bitmap centered and in proportion,
 * while also allowing the button size to be specified.
 *
 * @param str_id Button widget ID.
 * @param bitmap Bitmap to show on the button.
 * @param max_bitmap_size Maximum size to display the bitmap at in the GUI.
 * @param button_size Size of the button.
 * @param bg_col Bitmap background color.
 * @param tint_col Bitmap tint color.
 * @return Whether the button was pressed.
 */
bool ImGui::ImageButtonOrganized(
    const string &str_id, ALLEGRO_BITMAP* bitmap,
    const Point &max_bitmap_size, const Point &button_size,
    const ALLEGRO_COLOR &bg_col, const ALLEGRO_COLOR &tint_col
) {
    Point final_bmp_size =
        resizeToBoxKeepingAspectRatio(
            getBitmapDimensions(bitmap), max_bitmap_size
        );
        
    Point padding = (button_size - final_bmp_size) / 2.0f;
    
    PushStyleVar(
        ImGuiStyleVar_FramePadding, ImVec2(padding.x, padding.y)
    );
    bool result = ImageButton(str_id, bitmap, final_bmp_size);
    PopStyleVar();
    
    return result;
}


/**
 * @brief Wrapper for creating a Dear ImGui image button widget, followed
 * by a text widget.
 *
 * @param id Button widget ID.
 * @param icon Icon to show on the button.
 * @param icon_size Size to display the bitmap at in the GUI.
 * @param button_padding Padding between the icon and button edges.
 * @param text The button's text.
 * @return Whether the button was pressed.
 */
bool ImGui::ImageButtonAndText(
    const string &id, ALLEGRO_BITMAP* icon, const Point &icon_size,
    float button_padding, const string &text
) {
    ImGui::BeginGroup();
    
    ImGui::PushStyleVar(
        ImGuiStyleVar_FramePadding, ImVec2(button_padding, button_padding)
    );
    bool result = ImGui::ImageButton(id, icon, icon_size);
    ImGui::PopStyleVar();
    
    float offset = (icon_size.y + button_padding * 2 - 16.0f) / 2.0f;
    offset -= 3.0f; //It's 3.0 too far, with the group + dummy approach.
    
    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::Dummy(ImVec2(0.0f, offset));
    ImGui::Text("%s", text.c_str());
    ImGui::EndGroup();
    
    ImGui::EndGroup();
    
    return result;
}


/**
 * @brief Wrapper for creating a Dear ImGui list box widget, but
 * using a vector of strings for the list of items.
 *
 * @param label ListBox widget label.
 * @param current_item Index number of the current selected item.
 * @param items List of items.
 * @param height_in_items Maximum height, in number of items.
 * @return Whether the value was changed.
 */
bool ImGui::ListBox(
    const string &label, int* current_item, const vector<string> &items,
    int height_in_items
) {
    //TODO check if items is empty
    const char** array = new const char* [items.size()];
    
    for(size_t i = 0; i < items.size(); i++) {
        array[i] = items[i].c_str();
    }
    
    return
        ImGui::ListBox(
            label.c_str(),
            current_item, array, (int) items.size(),
            height_in_items
        );
    //TODO free "array"
}


/**
 * @brief Resets some variables inside the ImGui namespace.
 */
void ImGui::Reset() {
    ImGuiIO &io = ImGui::GetIO();
    
    io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
    io.MouseWheel = 0.0f;
    io.MouseWheelH = 0.0f;
    for(size_t b = 0; b < IM_ARRAYSIZE(io.MouseDown); b++) {
        io.MouseDown[b] = false;
    }
    
    io.KeyCtrl = false;
    io.KeyShift = false;
    io.KeyAlt = false;
    io.KeySuper = false;
    
    io.AddKeyEvent(ImGuiKey_Escape, false);
    io.AddKeyEvent(ImGuiKey_LeftCtrl, false);
    io.AddKeyEvent(ImGuiKey_RightCtrl, false);
    io.AddKeyEvent(ImGuiKey_LeftShift, false);
    io.AddKeyEvent(ImGuiKey_RightShift, false);
    io.AddKeyEvent(ImGuiKey_LeftAlt, false);
    io.AddKeyEvent(ImGuiKey_RightAlt, false);
    io.AddKeyEvent(ImGuiKey_LeftSuper, false);
    io.AddKeyEvent(ImGuiKey_RightSuper, false);
    io.AddKeyEvent(ImGuiMod_Alt, false);
    io.AddKeyEvent(ImGuiMod_Ctrl, false);
    io.AddKeyEvent(ImGuiMod_Shift, false);
    io.AddKeyEvent(ImGuiMod_Super, false);
}


/**
 * @brief Prepares the "cursor X" so that the next widgets will be centered.
 *
 * @param upcoming_items_width Width of the items that will belong to this line.
 */
void ImGui::SetupCentering(int upcoming_items_width) {
    int window_width = ImGui::GetWindowSize().x;
    ImGui::SetCursorPosX((window_width - upcoming_items_width) * 0.5f);
}


/**
 * @brief Prepares the state of the GUI to either place the next button
 * on the same line, or to break to a new line if it wouldn't fit.
 *
 * @param next_button_width Width of the next button.
 * @param next_button_idx Index of the next button.
 * @param total_n_buttons Total amount of buttons.
 */
void ImGui::SetupButtonWrapping(
    int next_button_width, int next_button_idx, int total_n_buttons
) {
    float last_x2 =
        ImGui::GetItemRectMax().x;
    float next_x2 =
        last_x2 + ImGui::GetStyle().ItemSpacing.x + next_button_width;
    float window_x2 =
        GetCursorScreenPos().x + GetContentRegionAvail().x;
    if(next_button_idx < total_n_buttons && next_x2 < window_x2) {
        ImGui::SameLine();
    }
}


/**
 * @brief Places a dummy widget designed to space things vertically.
 */
void ImGui::Spacer() {
    ImGui::Dummy(ImVec2(0, 16));
}
