/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Animation database, animation, animation instance, frame,
 * and sprite classes, and animation-related functions.
 */

#include <algorithm>
#include <map>
#include <vector>

#include "animation.h"

#include "../../core/misc_functions.h"
#include "../../core/game.h"
#include "../../util/allegro_utils.h"
#include "../../util/string_utils.h"


using std::size_t;
using std::string;
using std::vector;


/**
 * @brief Constructs a new animation object.
 *
 * @param name Name, should be unique.
 * @param frames List of frames.
 * @param loop_frame Loop frame index.
 * @param hit_rate If this has an attack, this is the chance of hitting.
 * 0 - 100.
 */
Animation::Animation(
    const string &name, const vector<Frame> &frames,
    size_t loop_frame, unsigned char hit_rate
) :
    name(name),
    frames(frames),
    loop_frame(loop_frame),
    hit_rate(hit_rate) {
    
}


/**
 * @brief Constructs a new animation object.
 *
 * @param a2 The other animation.
 */
Animation::Animation(const Animation &a2) :
    name(a2.name),
    frames(a2.frames),
    loop_frame(a2.loop_frame),
    hit_rate(a2.hit_rate) {
}


/**
 * @brief Creates an animation by copying info from another animation.
 *
 * @param a2 The other animation.
 * @return The current object.
 */
Animation &Animation::operator=(const Animation &a2) {
    if(this != &a2) {
        name = a2.name;
        frames = a2.frames;
        loop_frame = a2.loop_frame;
        hit_rate = a2.hit_rate;
    }
    
    return *this;
}


/**
 * @brief Deletes one of the animation's frames.
 *
 * @param idx Frame index.
 */
void Animation::delete_frame(size_t idx) {
    if(idx == INVALID) return;
    
    if(idx < loop_frame) {
        //Let the loop frame stay the same.
        loop_frame--;
    }
    if(
        idx == loop_frame &&
        loop_frame == frames.size() - 1
    ) {
        //Stop the loop frame from going out of bounds.
        loop_frame--;
    }
    frames.erase(frames.begin() + idx);
}


/**
 * @brief Returns the total duration of the animation.
 *
 * @return The duration.
 */
float Animation::get_duration() {
    float duration = 0.0f;
    for(size_t f = 0; f < frames.size(); f++) {
        duration += frames[f].duration;
    }
    return duration;
}


/**
 * @brief Returns the frame index, and time within that frame,
 * that matches the specified time.
 *
 * @param t Time to check.
 * @param frame_idx The frame index is returned here.
 * @param frame_time The time within the frame is returned here.
 */
void Animation::get_frame_and_time(
    float t, size_t* frame_idx, float* frame_time
) {
    *frame_idx = 0;
    *frame_time = 0.0f;
    
    if(frames.empty() || t <= 0.0f) {
        return;
    }
    
    float duration_so_far = 0.0f;
    float prev_duration_so_far = 0.0f;
    size_t f = 0;
    for(f = 0; f < frames.size(); f++) {
        prev_duration_so_far = duration_so_far;
        duration_so_far += frames[f].duration;
        
        if(duration_so_far > t) {
            break;
        }
    }
    
    *frame_idx = clamp(f, 0, frames.size() - 1);
    *frame_time = t - prev_duration_so_far;
}


/**
 * @brief Returns the total duration of the loop segment of the animation.
 *
 * @return The duration.
 */
float Animation::get_loop_duration() {
    float duration = 0.0f;
    for(size_t f = loop_frame; f < frames.size(); f++) {
        duration += frames[f].duration;
    }
    return duration;
}


/**
 * @brief Returns the total time since the animation start, when given a frame
 * and the current time in the current frame.
 *
 * @param frame_idx Current frame index.
 * @param frame_time Time in the current frame.
 * @return The time.
 */
float Animation::get_time(size_t frame_idx, float frame_time) {
    if(frame_idx == INVALID) {
        return 0.0f;
    }
    if(frame_idx >= frames.size()) {
        return get_duration();
    }
    
    float cur_time = 0.0f;
    for(size_t f = 0; f < frame_idx; f++) {
        cur_time += frames[f].duration;
    }
    cur_time += frame_time;
    return cur_time;
}


/**
 * @brief Constructs a new animation database object.
 *
 * @param a List of animations.
 * @param s List of sprites.
 * @param b List of body parts.
 */
AnimationDatabase::AnimationDatabase(
    const vector<Animation*> &a, const vector<Sprite*> &s,
    const vector<BodyPart*> &b
) :
    animations(a),
    sprites(s),
    body_parts(b) {
    
}


/**
 * @brief Calculates the maximum distance that any of its hitbox can reach,
 * and stores it in the hitbox_span variable.
 */
void AnimationDatabase::calculate_hitbox_span() {
    hitbox_span = 0.0f;
    for(size_t s = 0; s < sprites.size(); s++) {
        Sprite* s_ptr = sprites[s];
        for(size_t h = 0; h < s_ptr->hitboxes.size(); h++) {
            Hitbox* h_ptr = &s_ptr->hitboxes[h];
            
            float d = Distance(Point(0.0f), h_ptr->pos).to_float();
            d += h_ptr->radius;
            hitbox_span = std::max(hitbox_span, d);
        }
    }
}


/**
 * @brief Enemies and such have a regular list of animations.
 * The only way to change these animations is through the script.
 * So animation control is done entirely through game data.
 * However, the animations for Pikmin, leaders, etc. are pre-named.
 * e.g.: The game wants there to be an "idle" animation,
 * a "walk" animation, etc.
 * Because we are NOT looking up with strings, if we want more than 20FPS,
 * we need a way to convert from a numeric index
 * (one that stands for walking, one for idling, etc.)
 * into the corresponding index on the animation file.
 * This is where this comes in.
 *
 * @param conversions A vector of size_t and strings.
 * The size_t is the hardcoded index (probably in some constant or enum).
 * The string is the name of the animation in the animation file.
 * @param file File from where these animations were loaded. Used to
 * report errors.
 */
void AnimationDatabase::create_conversions(
    const vector<std::pair<size_t, string> > &conversions,
    const DataNode* file
) {
    pre_named_conversions.clear();
    
    if(conversions.empty()) return;
    
    //First, find the highest index.
    size_t highest = conversions[0].first;
    for(size_t c = 1; c < conversions.size(); c++) {
        highest = std::max(highest, conversions[c].first);
    }
    
    pre_named_conversions.assign(highest + 1, INVALID);
    
    for(size_t c = 0; c < conversions.size(); c++) {
        size_t a_pos = find_animation(conversions[c].second);
        pre_named_conversions[conversions[c].first] = a_pos;
        if(a_pos == INVALID) {
            game.errors.report(
                "Animation \"" + conversions[c].second + "\" is required "
                "by the engine, but does not exist!", file
            );
        }
    }
}


/**
 * @brief Destroys an animation database and all of its content.
 */
void AnimationDatabase::destroy() {
    reset_metadata();
    for(size_t a = 0; a < animations.size(); a++) {
        delete animations[a];
    }
    for(size_t s = 0; s < sprites.size(); s++) {
        delete sprites[s];
    }
    for(size_t b = 0; b < body_parts.size(); b++) {
        delete body_parts[b];
    }
    animations.clear();
    sprites.clear();
    body_parts.clear();
}


/**
 * @brief Deletes a sprite, adjusting any animations that use it.
 *
 * @param idx Sprite index.
 */
void AnimationDatabase::delete_sprite(size_t idx) {
    for(size_t a = 0; a < animations.size(); a++) {
        Animation* a_ptr = animations[a];
        
        for(size_t f = 0; f < a_ptr->frames.size();) {
            if(a_ptr->frames[f].sprite_idx == idx) {
                a_ptr->delete_frame(f);
            } else {
                f++;
            }
        }
    }
    
    sprites.erase(sprites.begin() + idx);
    
    for(size_t a = 0; a < animations.size(); a++) {
        Animation* a_ptr = animations[a];
        for(size_t f = 0; f < a_ptr->frames.size(); f++) {
            Frame* f_ptr = &(a_ptr->frames[f]);
            f_ptr->sprite_idx = find_sprite(f_ptr->sprite_name);
        }
    }
}


/**
 * @brief Fills each frame's sound index cache variable, where applicable.
 *
 * @param mt_ptr Mob type with the sound data.
 */
void AnimationDatabase::fill_sound_idx_caches(MobType* mt_ptr) {
    for(size_t a = 0; a < animations.size(); a++) {
        Animation* a_ptr = animations[a];
        for(size_t f = 0; f < a_ptr->frames.size(); f++) {
            Frame* f_ptr = &a_ptr->frames[f];
            f_ptr->sound_idx = INVALID;
            if(f_ptr->sound.empty()) continue;
            
            for(size_t s = 0; s < mt_ptr->sounds.size(); s++) {
                if(mt_ptr->sounds[s].name == f_ptr->sound) {
                    f_ptr->sound_idx = s;
                }
            }
        }
    }
}


/**
 * @brief Returns the index of the specified animation.
 *
 * @param name Name of the animation to search for.
 * @return The index, or INVALID if not found.
 */
size_t AnimationDatabase::find_animation(const string &name) const {
    for(size_t a = 0; a < animations.size(); a++) {
        if(animations[a]->name == name) return a;
    }
    return INVALID;
}


/**
 * @brief Returns the index of the specified body part.
 *
 * @param name Name of the body part to search for.
 * @return The index, or INVALID if not found.
 */
size_t AnimationDatabase::find_body_part(const string &name) const {
    for(size_t b = 0; b < body_parts.size(); b++) {
        if(body_parts[b]->name == name) return b;
    }
    return INVALID;
}


/**
 * @brief Returns the index of the specified sprite.
 *
 * @param name Name of the sprite to search for.
 * @return The index, or INVALID if not found.
 */
size_t AnimationDatabase::find_sprite(const string &name) const {
    for(size_t s = 0; s < sprites.size(); s++) {
        if(sprites[s]->name == name) return s;
    }
    return INVALID;
}


/**
 * @brief Fixes the pointers for body parts.
 */
void AnimationDatabase::fix_body_part_pointers() {
    for(size_t s = 0; s < sprites.size(); s++) {
        Sprite* s_ptr = sprites[s];
        for(size_t h = 0; h < s_ptr->hitboxes.size(); h++) {
            Hitbox* h_ptr = &s_ptr->hitboxes[h];
            
            for(size_t b = 0; b < body_parts.size(); b++) {
                BodyPart* b_ptr = body_parts[b];
                if(b_ptr->name == h_ptr->body_part_name) {
                    h_ptr->body_part_idx = b;
                    h_ptr->body_part_ptr = b_ptr;
                    break;
                }
            }
        }
    }
}


/**
 * @brief Loads animation database data from a data node.
 *
 * @param node Data node to load from.
 */
void AnimationDatabase::load_from_data_node(DataNode* node) {
    //Content metadata.
    load_metadata_from_data_node(node);
    
    //Body parts.
    DataNode* body_parts_node = node->getChildByName("body_parts");
    size_t n_body_parts = body_parts_node->getNrOfChildren();
    for(size_t b = 0; b < n_body_parts; b++) {
    
        DataNode* body_part_node = body_parts_node->getChild(b);
        
        BodyPart* cur_body_part = new BodyPart(body_part_node->name);
        body_parts.push_back(cur_body_part);
    }
    
    //Sprites.
    DataNode* sprites_node = node->getChildByName("sprites");
    size_t n_sprites = sprites_node->getNrOfChildren();
    for(size_t s = 0; s < n_sprites; s++) {
    
        DataNode* sprite_node = sprites_node->getChild(s);
        vector<Hitbox> hitboxes;
        
        DataNode* hitboxes_node =
            sprite_node->getChildByName("hitboxes");
        size_t n_hitboxes = hitboxes_node->getNrOfChildren();
        
        for(size_t h = 0; h < n_hitboxes; h++) {
        
            DataNode* hitbox_node =
                hitboxes_node->getChild(h);
            Hitbox cur_hitbox = Hitbox();
            
            cur_hitbox.pos =
                s2p(
                    hitbox_node->getChildByName("coords")->value,
                    &cur_hitbox.z
                );
            cur_hitbox.height =
                s2f(hitbox_node->getChildByName("height")->value);
            cur_hitbox.radius =
                s2f(hitbox_node->getChildByName("radius")->value);
            cur_hitbox.body_part_name =
                hitbox_node->name;
            cur_hitbox.type =
                (HITBOX_TYPE)
                s2i(hitbox_node->getChildByName("type")->value);
            cur_hitbox.value =
                s2f(
                    hitbox_node->getChildByName("value")->value
                );
            cur_hitbox.can_pikmin_latch =
                s2b(
                    hitbox_node->getChildByName(
                        "can_pikmin_latch"
                    )->getValueOrDefault("false")
                );
            cur_hitbox.knockback_outward =
                s2b(
                    hitbox_node->getChildByName(
                        "knockback_outward"
                    )->getValueOrDefault("false")
                );
            cur_hitbox.knockback_angle =
                s2f(hitbox_node->getChildByName("knockback_angle")->value);
            cur_hitbox.knockback =
                s2f(
                    hitbox_node->getChildByName(
                        "knockback"
                    )->getValueOrDefault("0")
                );
            cur_hitbox.wither_chance =
                s2i(
                    hitbox_node->getChildByName("wither_chance")->value
                );
                
            DataNode* hazards_node =
                hitbox_node->getChildByName("hazards");
            cur_hitbox.hazards_str = hazards_node->value;
            vector<string> hazards_strs =
                semicolon_list_to_vector(cur_hitbox.hazards_str);
            for(size_t hs = 0; hs < hazards_strs.size(); hs++) {
                const string &hazard_name = hazards_strs[hs];
                if(game.content.hazards.list.find(hazard_name) == game.content.hazards.list.end()) {
                    game.errors.report(
                        "Unknown hazard \"" + hazard_name + "\"!",
                        hazards_node
                    );
                } else {
                    cur_hitbox.hazards.push_back(
                        &(game.content.hazards.list[hazard_name])
                    );
                }
            }
            
            
            hitboxes.push_back(cur_hitbox);
            
        }
        
        Sprite* new_s =
            new Sprite(
            sprite_node->name,
            nullptr,
            s2p(sprite_node->getChildByName("file_pos")->value),
            s2p(sprite_node->getChildByName("file_size")->value),
            hitboxes
        );
        sprites.push_back(new_s);
        
        new_s->offset = s2p(sprite_node->getChildByName("offset")->value);
        new_s->scale =
            s2p(
                sprite_node->getChildByName(
                    "scale"
                )->getValueOrDefault("1 1")
            );
        new_s->angle = s2f(sprite_node->getChildByName("angle")->value);
        new_s->tint =
            s2c(
                sprite_node->getChildByName("tint")->getValueOrDefault(
                    "255 255 255 255"
                )
            );
        new_s->bmp_name = sprite_node->getChildByName("file")->value;
        new_s->set_bitmap(
            new_s->bmp_name, new_s->bmp_pos, new_s->bmp_size,
            sprite_node->getChildByName("file")
        );
        new_s->top_visible =
            s2b(
                sprite_node->getChildByName("top_visible")->value
            );
        new_s->top_pos =
            s2p(sprite_node->getChildByName("top_pos")->value);
        new_s->top_size =
            s2p(sprite_node->getChildByName("top_size")->value);
        new_s->top_angle =
            s2f(
                sprite_node->getChildByName("top_angle")->value
            );
    }
    
    //Animations.
    DataNode* anims_node = node->getChildByName("animations");
    size_t n_anims = anims_node->getNrOfChildren();
    for(size_t a = 0; a < n_anims; a++) {
    
        DataNode* anim_node = anims_node->getChild(a);
        vector<Frame> frames;
        
        DataNode* frames_node =
            anim_node->getChildByName("frames");
        size_t n_frames =
            frames_node->getNrOfChildren();
            
        for(size_t f = 0; f < n_frames; f++) {
            DataNode* frame_node = frames_node->getChild(f);
            size_t s_pos = find_sprite(frame_node->name);
            string signal_str =
                frame_node->getChildByName("signal")->value;
            frames.push_back(
                Frame(
                    frame_node->name,
                    s_pos,
                    (s_pos == INVALID) ? nullptr : sprites[s_pos],
                    s2f(frame_node->getChildByName("duration")->value),
                    s2b(frame_node->getChildByName("interpolate")->value),
                    frame_node->getChildByName("sound")->value,
                    (signal_str.empty() ? INVALID : s2i(signal_str))
                )
            );
        }
        
        animations.push_back(
            new Animation(
                anim_node->name,
                frames,
                s2i(anim_node->getChildByName("loop_frame")->value),
                s2i(
                    anim_node->getChildByName(
                        "hit_rate"
                    )->getValueOrDefault("100")
                )
            )
        );
    }
    
    //Finish up.
    fix_body_part_pointers();
    calculate_hitbox_span();
}


/**
 * @brief Saves the animation database data to a data node.
 *
 * @param node Data node to save to.
 * @param save_top_data Whether to save the Pikmin top's data.
 */
void AnimationDatabase::save_to_data_node(
    DataNode* node, bool save_top_data
) {
    //Content metadata.
    save_metadata_to_data_node(node);
    
    //Animations.
    DataNode* animations_node = node->addNew("animations");
    for(size_t a = 0; a < animations.size(); a++) {

        //Animation.
        Animation* a_ptr = animations[a];
        DataNode* anim_node = animations_node->addNew(a_ptr->name);
        GetterWriter agw(anim_node);
        
        if(a_ptr->loop_frame > 0) {
            agw.get("loop_frame", a_ptr->loop_frame);
        }
        if(a_ptr->hit_rate != 100) {
            agw.get("hit_rate", a_ptr->hit_rate);
        }

        //Frames.
        DataNode* frames_node = anim_node->addNew("frames");
        for(size_t f = 0; f < a_ptr->frames.size(); f++) {

            //Frame.
            Frame* f_ptr = &a_ptr->frames[f];
            DataNode* frame_node = frames_node->addNew(f_ptr->sprite_name);
            GetterWriter fgw(frame_node);
            
            fgw.get("duration", f_ptr->duration);
            if(f_ptr->interpolate) {
                fgw.get("interpolate", f_ptr->interpolate);
            }
            if(f_ptr->signal != INVALID) {
                fgw.get("signal", f_ptr->signal);
            }
            if(!f_ptr->sound.empty() && f_ptr->sound != NONE_OPTION) {
                fgw.get("sound", f_ptr->sound);
            }
        }
    }
    
    //Sprites.
    DataNode* sprites_node = node->addNew("sprites");
    for(size_t s = 0; s < sprites.size(); s++) {

        //Sprite.
        Sprite* s_ptr = sprites[s];
        DataNode* sprite_node = sprites_node->addNew(sprites[s]->name);
        GetterWriter sgw(sprite_node);
        
        sgw.get("file", s_ptr->bmp_name);
        sgw.get("file_pos", s_ptr->bmp_pos);
        sgw.get("file_size", s_ptr->bmp_size);
        if(s_ptr->offset.x != 0.0 || s_ptr->offset.y != 0.0) {
            sgw.get("offset", s_ptr->offset);
        }
        if(s_ptr->scale.x != 1.0 || s_ptr->scale.y != 1.0) {
            sgw.get("scale", s_ptr->scale);
        }
        if(s_ptr->angle != 0.0) {
            sgw.get("angle", s_ptr->angle);
        }
        if(s_ptr->tint != COLOR_WHITE) {
            sgw.get("tint", s_ptr->tint);
        }
        
        if(save_top_data) {
            sgw.get("top_visible", s_ptr->top_visible);
            sgw.get("top_pos", s_ptr->top_pos);
            sgw.get("top_size", s_ptr->top_size);
            sgw.get("top_angle", s_ptr->top_angle);
        }
        
        if(!s_ptr->hitboxes.empty()) {

            //Hitboxes.
            DataNode* hitboxes_node = sprite_node->addNew("hitboxes");
            for(size_t h = 0; h < s_ptr->hitboxes.size(); h++) {

                //Hitbox.
                Hitbox* h_ptr = &s_ptr->hitboxes[h];
                DataNode* hitbox_node = hitboxes_node->addNew(h_ptr->body_part_name);
                GetterWriter hgw(hitbox_node);
                
                hgw.get("coords", p2s(h_ptr->pos, &h_ptr->z));
                hgw.get("height", h_ptr->height);
                hgw.get("radius", h_ptr->radius);
                hgw.get("type", h_ptr->type);
                hgw.get("value", h_ptr->value);
                if(
                    h_ptr->type == HITBOX_TYPE_NORMAL &&
                    h_ptr->can_pikmin_latch
                ) {
                    hgw.get("can_pikmin_latch", h_ptr->can_pikmin_latch);
                }
                if(!h_ptr->hazards_str.empty()) {
                    hgw.get("hazards", h_ptr->hazards_str);
                }
                if(
                    h_ptr->type == HITBOX_TYPE_ATTACK &&
                    h_ptr->knockback_outward
                ) {
                    hgw.get("knockback_outward", h_ptr->knockback_outward);
                }
                if(
                    h_ptr->type == HITBOX_TYPE_ATTACK &&
                    h_ptr->knockback_angle != 0
                ) {
                    hgw.get("knockback_angle", h_ptr->knockback_angle);
                }
                if(
                    h_ptr->type == HITBOX_TYPE_ATTACK &&
                    h_ptr->knockback != 0
                ) {
                    hgw.get("knockback", h_ptr->knockback);
                }
                if(
                    h_ptr->type == HITBOX_TYPE_ATTACK &&
                    h_ptr->wither_chance > 0
                ) {
                    hgw.get("wither_chance", h_ptr->wither_chance);
                }
            }
        }
    }
    
    //Body parts.
    DataNode* body_parts_node = node->addNew("body_parts");
    for(size_t b = 0; b < body_parts.size(); b++) {

        //Body part.
        BodyPart* b_ptr = body_parts[b];
        body_parts_node->addNew(b_ptr->name);
        
    }
}


/**
 * @brief Sorts all animations and sprites alphabetically,
 * making them more organized.
 */
void AnimationDatabase::sort_alphabetically() {
    sort(
        animations.begin(), animations.end(),
    [] (const Animation * a1, const Animation * a2) {
        return a1->name < a2->name;
    }
    );
    sort(
        sprites.begin(), sprites.end(),
    [] (const Sprite * s1, const Sprite * s2) {
        return s1->name < s2->name;
    }
    );
    
    for(size_t a = 0; a < animations.size(); a++) {
        Animation* a_ptr = animations[a];
        for(size_t f = 0; f < a_ptr->frames.size(); f++) {
            Frame* f_ptr = &(a_ptr->frames[f]);
            
            f_ptr->sprite_idx = find_sprite(f_ptr->sprite_name);
        }
    }
}


/**
 * @brief Constructs a new animation instance object.
 *
 * @param anim_db The animation database. Used when changing animations.
 */
AnimationInstance::AnimationInstance(AnimationDatabase* anim_db) :
    cur_anim(nullptr),
    anim_db(anim_db) {
    
}


/**
 * @brief Constructs a new animation instance object.
 *
 * @param ai2 The other animation instance.
 */
AnimationInstance::AnimationInstance(const AnimationInstance &ai2) :
    cur_anim(ai2.cur_anim),
    anim_db(ai2.anim_db) {
    
    to_start();
}


/**
 * @brief Copies data from another animation instance.
 *
 * @param ai2 The other animation instance.
 * @return The current object.
 */
AnimationInstance &AnimationInstance::operator=(
    const AnimationInstance &ai2
) {
    if(this != &ai2) {
        cur_anim = ai2.cur_anim;
        anim_db = ai2.anim_db;
    }
    
    to_start();
    
    return *this;
}


/**
 * @brief Returns the sprite of the current frame of animation.
 *
 * @param out_cur_sprite_ptr If not nullptr, the current sprite is
 * returned here.
 * @param out_next_sprite_ptr If not nullptr, the next sprite in the animation
 * is returned here.
 * @param out_interpolation_factor If not nullptr, the interpolation factor
 * (0 to 1) between the current sprite and the next one is returned here.
 */
void AnimationInstance::get_sprite_data(
    Sprite** out_cur_sprite_ptr, Sprite** out_next_sprite_ptr,
    float* out_interpolation_factor
) const {
    if(!valid_frame()) {
        if(out_cur_sprite_ptr) *out_cur_sprite_ptr = nullptr;
        if(out_next_sprite_ptr) *out_next_sprite_ptr = nullptr;
        if(out_interpolation_factor) *out_interpolation_factor = 0.0f;
        return;
    }
    
    Frame* cur_frame_ptr = &cur_anim->frames[cur_frame_idx];
    //First, the basics -- the current sprite.
    if(out_cur_sprite_ptr) {
        *out_cur_sprite_ptr = cur_frame_ptr->sprite_ptr;
    }
    
    //Now only bother with interpolation data if we actually need it.
    if(!out_next_sprite_ptr && !out_interpolation_factor) return;
    
    if(!cur_frame_ptr->interpolate) {
        //This frame doesn't even interpolate.
        if(out_next_sprite_ptr) *out_next_sprite_ptr = cur_frame_ptr->sprite_ptr;
        if(out_interpolation_factor) *out_interpolation_factor = 0.0f;
        return;
    }
    
    //Get the next sprite.
    size_t next_frame_idx = get_next_frame_idx();
    Frame* next_frame_ptr = &cur_anim->frames[next_frame_idx];
    
    if(out_next_sprite_ptr) *out_next_sprite_ptr = next_frame_ptr->sprite_ptr;
    
    //Get the interpolation factor.
    if(out_interpolation_factor) {
        if(cur_frame_ptr->duration == 0.0f) {
            *out_interpolation_factor = 0.0f;
        } else {
            *out_interpolation_factor =
                cur_frame_time /
                cur_frame_ptr->duration;
        }
    }
}


/**
 * @brief Returns the index of the next frame of animation, the one after
 * the current one.
 *
 * @param out_reached_end If not nullptr, true is returned here if we've reached
 * the end and the next frame loops back to the beginning.
 * @return The index, or INVALID on error.
 */
size_t AnimationInstance::get_next_frame_idx(bool* out_reached_end) const {
    if(out_reached_end) *out_reached_end = false;
    if(!cur_anim) return INVALID;
    
    if(cur_frame_idx < cur_anim->frames.size() - 1) {
        return cur_frame_idx + 1;
    } else {
        if(out_reached_end) *out_reached_end = true;
        if(cur_anim->loop_frame < cur_anim->frames.size()) {
            return cur_anim->loop_frame;
        } else {
            return 0;
        }
    }
}


/**
 * @brief Initializes the instance by setting its database to the given one,
 * its animation to the first one in the database, and setting the time
 * to the beginning.
 *
 * @param db Pointer to the animation database.
 */
void AnimationInstance::init_to_first_anim(AnimationDatabase* db) {
    anim_db = db;
    if(db && !anim_db->animations.empty()) {
        cur_anim = anim_db->animations[0];
    }
    to_start();
}


/**
 * @brief Skips the current animation instance ahead in time for a
 * random amount of time.
 *
 * The time is anywhere between 0 and the total duration of the
 * animation. Frame signals will be ignored.
 */
void AnimationInstance::skip_ahead_randomly() {
    if(!cur_anim) return;
    //First, find how long the animation lasts for.
    
    float total_duration = 0;
    for(size_t f = 0; f < cur_anim->frames.size(); f++) {
        total_duration += cur_anim->frames[f].duration;
    }
    
    tick(game.rng.f(0, total_duration));
}


/**
 * @brief Clears everything.
 */
void AnimationInstance::clear() {
    cur_anim = nullptr;
    anim_db = nullptr;
    cur_frame_time = 0;
    cur_frame_idx = INVALID;
}


/**
 * @brief Ticks the animation time by one frame of logic.
 *
 * @param delta_t How long the frame's tick is, in seconds.
 * @param signals Any frame that sends a signal adds it here.
 * @param sounds Any frame that should play a sound adds it here.
 * @return Whether or not the animation ended its final frame.
 */
bool AnimationInstance::tick(
    float delta_t, vector<size_t>* signals,
    vector<size_t>* sounds
) {
    if(!cur_anim) return false;
    size_t n_frames = cur_anim->frames.size();
    if(n_frames == 0) return false;
    Frame* cur_frame = &cur_anim->frames[cur_frame_idx];
    if(cur_frame->duration == 0) {
        return true;
    }
    
    cur_frame_time += delta_t;
    
    bool reached_end = false;
    
    //This is a while instead of an if because if the framerate is too low
    //and the next frame's duration is too short, it could be that a tick
    //goes over an entire frame, and lands 2 or more frames ahead.
    while(cur_frame_time > cur_frame->duration && cur_frame->duration != 0) {
        cur_frame_time = cur_frame_time - cur_frame->duration;
        bool reached_end_now = false;
        cur_frame_idx = get_next_frame_idx(&reached_end_now);
        reached_end |= reached_end_now;
        cur_frame = &cur_anim->frames[cur_frame_idx];
        if(cur_frame->signal != INVALID && signals) {
            signals->push_back(cur_frame->signal);
        }
        if(cur_frame->sound_idx != INVALID && sounds) {
            sounds->push_back(cur_frame->sound_idx);
        }
    }
    
    return reached_end;
}


/**
 * @brief Sets the animation state to the beginning.
 * It's called automatically when the animation is first set.
 */
void AnimationInstance::to_start() {
    cur_frame_time = 0;
    cur_frame_idx = 0;
}


/**
 * @brief Returns whether or not the animation instance is in a state where
 * it can show a valid frame.
 *
 * @return Whether it's in a valid state.
 */
bool AnimationInstance::valid_frame() const {
    if(!cur_anim) return false;
    if(cur_frame_idx >= cur_anim->frames.size()) return false;
    return true;
}


/**
 * @brief Constructs a new frame object.
 *
 * @param sn Name of the sprite.
 * @param si Index of the sprite in the animation database.
 * @param sp Pointer to the sprite.
 * @param d Duration.
 * @param in Whether to interpolate between this frame's transformation data
 * and the next's.
 * @param snd Sound name.
 * @param s Signal.
 */
Frame::Frame(
    const string &sn, size_t si, Sprite* sp, float d,
    bool in, const string &snd, size_t s
) :
    sprite_name(sn),
    sprite_idx(si),
    sprite_ptr(sp),
    duration(d),
    interpolate(in),
    sound(snd),
    signal(s) {
    
}


/**
 * @brief Constructs a new sprite object.
 *
 * @param name Internal name; should be unique.
 * @param b Bitmap.
 * @param h List of hitboxes.
 */
Sprite::Sprite(
    const string &name, ALLEGRO_BITMAP* const b, const vector<Hitbox> &h
) :
    name(name),
    bitmap(b),
    hitboxes(h) {
    
}


/**
 * @brief Constructs a new sprite object.
 *
 * @param name Internal name, should be unique.
 * @param b Parent bitmap.
 * @param b_pos X and Y of the top-left corner of the sprite, in the
 * parent's bitmap.
 * @param b_size Width and height of the sprite, in the parent's bitmap.
 * @param h List of hitboxes.
 */
Sprite::Sprite(
    const string &name, ALLEGRO_BITMAP* const b, const Point &b_pos,
    const Point &b_size, const vector<Hitbox> &h
) :
    name(name),
    parent_bmp(b),
    bmp_pos(b_pos),
    bmp_size(b_size),
    bitmap(
        b ?
        al_create_sub_bitmap(b, b_pos.x, b_pos.y, b_size.x, b_size.y) :
        nullptr
    ),
    hitboxes(h) {
    
}


/**
 * @brief Constructs a new sprite object.
 *
 * @param s2 The other sprite.
 */
Sprite::Sprite(const Sprite &s2) :
    name(s2.name),
    parent_bmp(nullptr),
    bmp_name(s2.bmp_name),
    bmp_pos(s2.bmp_pos),
    bmp_size(s2.bmp_size),
    offset(s2.offset),
    scale(s2.scale),
    angle(s2.angle),
    tint(s2.tint),
    top_pos(s2.top_pos),
    top_size(s2.top_size),
    top_angle(s2.top_angle),
    top_visible(s2.top_visible),
    bitmap(nullptr),
    hitboxes(s2.hitboxes) {
    
    set_bitmap(bmp_name, bmp_pos, bmp_size);
}


/**
 * @brief Destroys the sprite object.
 */
Sprite::~Sprite() {
    set_bitmap("", Point(), Point());
}


/**
 * @brief Creates the hitboxes, based on the body parts.
 *
 * @param adb The animation database the sprites and body parts belong to.
 * @param height The hitboxes's starting height.
 * @param radius The hitboxes's starting radius.
 */
void Sprite::create_hitboxes(
    AnimationDatabase* const adb, float height, float radius
) {
    hitboxes.clear();
    for(size_t b = 0; b < adb->body_parts.size(); b++) {
        hitboxes.push_back(
            Hitbox(
                adb->body_parts[b]->name,
                b,
                adb->body_parts[b],
                Point(), 0, height, radius
            )
        );
    }
}


/**
 * @brief Copies data from another sprite.
 *
 * @param s2 The other sprite.
 * @return The current object.
 */
Sprite &Sprite::operator=(const Sprite &s2) {
    if(this != &s2) {
        name = s2.name;
        parent_bmp = nullptr;
        bmp_pos = s2.bmp_pos;
        bmp_size = s2.bmp_size;
        offset = s2.offset;
        scale = s2.scale;
        angle = s2.angle;
        tint = s2.tint;
        top_pos = s2.top_pos;
        top_size = s2.top_size;
        top_angle = s2.top_angle;
        top_visible = s2.top_visible;
        bitmap = nullptr;
        hitboxes = s2.hitboxes;
        set_bitmap(s2.bmp_name, bmp_pos, bmp_size);
    }
    
    return *this;
}


/**
 * @brief Sets the bitmap and parent bitmap, according to the given information.
 * This automatically manages bitmap un/loading and such.
 * If the file name string is empty, sets to a nullptr bitmap
 * (and still unloads the old bitmap).
 *
 * @param new_bmp_name Internal name of the bitmap.
 * @param new_bmp_pos Top-left coordinates of the sub-bitmap inside the bitmap.
 * @param new_bmp_size Dimensions of the sub-bitmap.
 * @param node If not nullptr, this will be used to report an error with,
 * in case something happens.
 */
void Sprite::set_bitmap(
    const string &new_bmp_name,
    const Point &new_bmp_pos, const Point &new_bmp_size,
    DataNode* node
) {
    if(bitmap) {
        al_destroy_bitmap(bitmap);
        bitmap = nullptr;
    }
    if(new_bmp_name != bmp_name && parent_bmp) {
        game.content.bitmaps.list.free(bmp_name);
        parent_bmp = nullptr;
    }
    
    if(new_bmp_name.empty()) {
        bmp_name.clear();
        bmp_size = Point();
        bmp_pos = Point();
        return;
    }
    
    if(new_bmp_name != bmp_name || !parent_bmp) {
        parent_bmp = game.content.bitmaps.list.get(new_bmp_name, node, node != nullptr);
    }
    
    Point parent_size = get_bitmap_dimensions(parent_bmp);
    
    bmp_name = new_bmp_name;
    bmp_pos = new_bmp_pos;
    bmp_size = new_bmp_size;
    bmp_pos.x = clamp(new_bmp_pos.x, 0, parent_size.x - 1);
    bmp_pos.y = clamp(new_bmp_pos.y, 0, parent_size.y - 1);
    bmp_size.x = clamp(new_bmp_size.x, 0, parent_size.x - bmp_pos.x);
    bmp_size.y = clamp(new_bmp_size.y, 0, parent_size.y - bmp_pos.y);
    
    if(parent_bmp) {
        bitmap =
            al_create_sub_bitmap(
                parent_bmp, bmp_pos.x, bmp_pos.y,
                bmp_size.x, bmp_size.y
            );
    }
}


/**
 * @brief Returns the final transformation data for a "basic" sprite effect.
 * i.e. the translation, angle, scale, and tint. This makes use of
 * interpolation between two frames if applicable.
 *
 * @param base_pos Base position of the translation.
 * @param base_angle Base angle of the rotation.
 * @param base_angle_cos_cache If you have a cached value for the base angle's
 * cosine, write it here. Otherwise use LARGE_FLOAT.
 * @param base_angle_sin_cache If you have a cached value for the base angle's
 * sine, write it here. Otherwise use LARGE_FLOAT.
 * @param cur_s_ptr The current sprite.
 * @param next_s_ptr The next sprite, if any.
 * @param interpolation_factor Amount to interpolate the two sprites by, if any.
 * Ranges from 0 to 1.
 * @param out_eff_trans If not nullptr, the final translation is
 * returned here.
 * @param out_eff_angle If not nullptr, the final rotation angle is
 * returned here.
 * @param out_eff_scale If not nullptr, the final scale is
 * returned here.
 * @param out_eff_tint If not nullptr, the final tint color is
 * returned here.
 */
void get_sprite_basic_effects(
    const Point &base_pos, float base_angle,
    float base_angle_cos_cache, float base_angle_sin_cache,
    Sprite* cur_s_ptr, Sprite* next_s_ptr, float interpolation_factor,
    Point* out_eff_trans, float* out_eff_angle,
    Point* out_eff_scale, ALLEGRO_COLOR* out_eff_tint
) {
    if(base_angle_cos_cache == LARGE_FLOAT) {
        base_angle_cos_cache = cos(base_angle);
    }
    if(base_angle_sin_cache == LARGE_FLOAT) {
        base_angle_sin_cache = sin(base_angle);
    }
    
    if(out_eff_trans) {
        out_eff_trans->x =
            base_pos.x +
            base_angle_cos_cache * cur_s_ptr->offset.x -
            base_angle_sin_cache * cur_s_ptr->offset.y;
        out_eff_trans->y =
            base_pos.y +
            base_angle_sin_cache * cur_s_ptr->offset.x +
            base_angle_cos_cache * cur_s_ptr->offset.y;
    }
    if(out_eff_angle) {
        *out_eff_angle = base_angle + cur_s_ptr->angle;
    }
    if(out_eff_scale) {
        *out_eff_scale = cur_s_ptr->scale;
    }
    if(out_eff_tint) {
        *out_eff_tint = cur_s_ptr->tint;
    }
    
    if(next_s_ptr && interpolation_factor > 0.0f) {
        Point next_trans(
            base_pos.x +
            base_angle_cos_cache * next_s_ptr->offset.x -
            base_angle_sin_cache * next_s_ptr->offset.y,
            base_pos.y +
            base_angle_sin_cache * next_s_ptr->offset.x +
            base_angle_cos_cache * next_s_ptr->offset.y
        );
        float next_angle = base_angle + next_s_ptr->angle;
        Point next_scale = next_s_ptr->scale;
        ALLEGRO_COLOR next_tint = next_s_ptr->tint;
        
        if(out_eff_trans) {
            *out_eff_trans =
                interpolate_point(
                    interpolation_factor, 0.0f, 1.0f,
                    *out_eff_trans, next_trans
                );
        }
        if(out_eff_angle) {
            *out_eff_angle =
                interpolate_angle(
                    interpolation_factor, 0.0f, 1.0f,
                    *out_eff_angle, next_angle
                );
        }
        if(out_eff_scale) {
            *out_eff_scale =
                interpolate_point(
                    interpolation_factor, 0.0f, 1.0f,
                    *out_eff_scale, next_scale
                );
        }
        if(out_eff_tint) {
            *out_eff_tint =
                interpolate_color(
                    interpolation_factor, 0.0f, 1.0f,
                    *out_eff_tint, next_tint
                );
        }
    }
}


/**
 * @brief Returns the final transformation data for a Pikmin top's "basic"
 * sprite effect. i.e. the translation, angle, scale, and tint.
 * This makes use of interpolation between two frames if applicable.
 *
 * @param cur_s_ptr The current sprite.
 * @param next_s_ptr The next sprite, if any.
 * @param interpolation_factor Amount to interpolate the two sprites by, if any.
 * Ranges from 0 to 1.
 * @param out_eff_trans If not nullptr, the top's final translation is
 * returned here.
 * @param out_eff_angle If not nullptr, the top's final rotation angle is
 * returned here.
 * @param out_eff_size If not nullptr, the top's final size is
 * returned here.
 */
void get_sprite_basic_top_effects(
    Sprite* cur_s_ptr, Sprite* next_s_ptr, float interpolation_factor,
    Point* out_eff_trans, float* out_eff_angle,
    Point* out_eff_size
) {
    if(out_eff_trans) {
        *out_eff_trans = cur_s_ptr->top_pos;
    }
    if(out_eff_angle) {
        *out_eff_angle = cur_s_ptr->top_angle;
    }
    if(out_eff_size) {
        *out_eff_size = cur_s_ptr->top_size;
    }
    
    if(next_s_ptr && interpolation_factor > 0.0f) {
        Point next_trans = next_s_ptr->top_pos;
        float next_angle = next_s_ptr->top_angle;
        Point next_size = next_s_ptr->top_size;
        
        if(out_eff_trans) {
            *out_eff_trans =
                interpolate_point(
                    interpolation_factor, 0.0f, 1.0f,
                    *out_eff_trans, next_trans
                );
        }
        if(out_eff_angle) {
            *out_eff_angle =
                interpolate_angle(
                    interpolation_factor, 0.0f, 1.0f,
                    *out_eff_angle, next_angle
                );
        }
        if(out_eff_size) {
            *out_eff_size =
                interpolate_point(
                    interpolation_factor, 0.0f, 1.0f,
                    *out_eff_size, next_size
                );
        }
    }
}
