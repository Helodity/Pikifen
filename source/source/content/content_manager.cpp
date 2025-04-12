/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Content manager class and related functions.
 */

#include "content_manager.h"

#include "../core/misc_functions.h"
#include "../core/game.h"
#include "../core/load.h"
#include "../util/allegro_utils.h"


/**
 * @brief Constructs a new content manager object.
 */
ContentManager::ContentManager() {
    for(size_t c = 0; c < N_CONTENT_TYPES; c++) {
        loadLevels[c] = CONTENT_LOAD_LEVEL_UNLOADED;
    }
}


/**
 * @brief Returns the relevant content type manager for a given content type.
 *
 * @param type The content type.
 * @return Pointer to the manager.
 */
ContentTypeManager* ContentManager::getMgrPtr(CONTENT_TYPE type) {
    switch(type) {
    case CONTENT_TYPE_AREA: {
        return &areas;
        break;
    } case CONTENT_TYPE_BITMAP: {
        return &bitmaps;
        break;
    } case CONTENT_TYPE_GLOBAL_ANIMATION: {
        return &globalAnimDbs;
        break;
    } case CONTENT_TYPE_GUI: {
        return &guiDefs;
        break;
    } case CONTENT_TYPE_HAZARD: {
        return &hazards;
        break;
    } case CONTENT_TYPE_LIQUID: {
        return &liquids;
        break;
    } case CONTENT_TYPE_MISC: {
        return &miscConfigs;
        break;
    } case CONTENT_TYPE_MOB_ANIMATION: {
        return &mobAnimDbs;
        break;
    } case CONTENT_TYPE_MOB_TYPE: {
        return &mobTypes;
        break;
    } case CONTENT_TYPE_PARTICLE_GEN: {
        return &particleGens;
        break;
    } case CONTENT_TYPE_SONG: {
        return &songs;
        break;
    } case CONTENT_TYPE_SONG_TRACK: {
        return &songTracks;
        break;
    } case CONTENT_TYPE_SOUND: {
        return &sounds;
        break;
    } case CONTENT_TYPE_SPIKE_DAMAGE_TYPE: {
        return &spikeDamageTypes;
        break;
    } case CONTENT_TYPE_SPRAY_TYPE: {
        return &sprayTypes;
        break;
    } case CONTENT_TYPE_STATUS_TYPE: {
        return &statusTypes;
        break;
    } case CONTENT_TYPE_WEATHER_CONDITION: {
        return &weatherConditions;
        break;
    } case N_CONTENT_TYPES: {
        break;
    }
    }
    return nullptr;
}


/**
 * @brief Loads all pieces of game content of some type.
 * This begins by generating a manifest of all content on disk, with packs
 * in mind, and then reads all the files in the manifest.
 *
 * @param types Types of game content to load.
 * @param level Level to load at.
 */
void ContentManager::loadAll(const vector<CONTENT_TYPE> &types, CONTENT_LOAD_LEVEL level) {
    //Fill in all manifests first. This is because some content may rely on
    //another's manifest.
    for(size_t t = 0; t < types.size(); t++) {
        ContentTypeManager* mgr_ptr = getMgrPtr(types[t]);
        engineAssert(
            loadLevels[types[t]] == CONTENT_LOAD_LEVEL_UNLOADED,
            "Tried to load all content of type " + mgr_ptr->getName() +
            " even though it's already loaded!"
        );
        mgr_ptr->fillManifests();
    }
    
    //Now load the content.
    for(size_t t = 0; t < types.size(); t++) {
        ContentTypeManager* mgr_ptr = getMgrPtr(types[t]);
        const string &perf_mon_name = mgr_ptr->getPerfMonMeasurementName();
        if(!perf_mon_name.empty() && game.perfMon) {
            game.perfMon->startMeasurement(perf_mon_name);
        }
        mgr_ptr->loadAll(level);
        if(!perf_mon_name.empty() && game.perfMon) {
            game.perfMon->finishMeasurement();
        }
        loadLevels[types[t]] = level;
    }
    
}


/**
 * @brief Creates a new pack and updates the list of packs.
 *
 * @param internal_name Internal name of the pack, i.e. the pack's folder name.
 * @param name Proper name of the pack.
 * @param description Description.
 * @param maker Maker(s).
 * @return Whether it succeeded.
 */
bool ContentManager::createPack(
    const string &internal_name, const string &name,
    const string &description, const string &maker
) {
    string pack_path = FOLDER_PATHS_FROM_ROOT::GAME_DATA + "/" + internal_name;
    
    //Create the folder first.
    bool could_make_folder = al_make_directory(pack_path.c_str());
    if(!could_make_folder) return false;
    
    //Create the data file.
    DataNode data;
    GetterWriter gw(&data);
    gw.get("name", name);
    gw.get("description", description);
    gw.get("maker", maker);
    gw.get("version", "1.0.0");
    gw.get("engine_version", getEngineVersionString());
    gw.get("tags", "");
    gw.get("dependencies", "");
    gw.get("conflicts", "");
    gw.get("notes", "");
    data.saveFile(pack_path + "/" + FILE_NAMES::PACK_DATA, true, true);
    
    //Update the list and manifest.
    packs.unloadAll();
    packs.clearManifests();
    packs.fillManifests();
    packs.loadAll();
    
    return true;
}


/**
 * @brief Loads an area as the "current area". This does not load it into
 * the vector of areas.
 *
 * @param requested_area_path Path to the area folder.
 * @param manif_ptr Set the manifest pointer to this. If nullptr, it'll be
 * set from the list of manifests.
 * @param type Type of area this is.
 * @param level Level to load at.
 * @param from_backup If true, load from a backup, if any.
 * @return Whether it succeeded.
 */
bool ContentManager::loadAreaAsCurrent(
    const string &requested_area_path, ContentManifest* manif_ptr,
    CONTENT_LOAD_LEVEL level, bool from_backup
) {
    engineAssert(
        game.curAreaData == nullptr,
        "Tried to load area \"" + requested_area_path + "\" as the current "
        "one even though there is already a loaded current area, \"" +
        (
            game.curAreaData->manifest ?
            game.curAreaData->manifest->path :
            "(unsaved)"
        ) + "\"!"
    );
    
    game.curAreaData = new Area();
    bool success =
        areas.loadArea(
            game.curAreaData, requested_area_path, manif_ptr,
            level, from_backup
        );
        
    if(!success) {
        unloadCurrentArea(level);
    }
    return success;
}


/**
 * @brief Reloads all packs.
 * This only loads their manifests and metadata, not their content!
 */
void ContentManager::reloadPacks() {
    packs.unloadAll();
    packs.clearManifests();
    
    packs.fillManifests();
    packs.loadAll();
}


/**
 * @brief Unloads the "current area".
 *
 * @param level Should match the level at which the content got loaded.
 */
void ContentManager::unloadCurrentArea(CONTENT_LOAD_LEVEL level) {
    if(!game.curAreaData) return;
    game.curAreaData->clear();
    delete game.curAreaData;
    game.curAreaData = nullptr;
}


/**
 * @brief Unloads some loaded content.
 *
 * @param types Types of content to unload.
 */
void ContentManager::unloadAll(const vector<CONTENT_TYPE> &types) {
    for(size_t t = 0; t < types.size(); t++) {
        ContentTypeManager* mgr_ptr = getMgrPtr(types[t]);
        
        engineAssert(
            loadLevels[types[t]] != CONTENT_LOAD_LEVEL_UNLOADED,
            "Tried to unload all content of type " + mgr_ptr->getName() +
            " even though it's already unloaded!"
        );
        
        mgr_ptr->unloadAll(loadLevels[types[t]]);
        mgr_ptr->clearManifests();
        
        loadLevels[types[t]] = CONTENT_LOAD_LEVEL_UNLOADED;
    }
}


/**
 * @brief Clears all loaded manifests.
 */
void PackManager::clearManifests() {
    manifestsSansBaseRaw.clear();
    manifestsWithBaseRaw.clear();
    manifestsSansBase.clear();
    manifestsWithBase.clear();
}


/**
 * @brief Fills in the manifests.
 */
void PackManager::fillManifests() {
    //Raw manifests.
    vector<string> raw_folders =
        folderToVector(FOLDER_PATHS_FROM_ROOT::GAME_DATA, true);
        
    for(size_t f = 0; f < raw_folders.size(); f++) {
        if(raw_folders[f] != FOLDER_NAMES::BASE_PACK) {
            manifestsSansBaseRaw.push_back(raw_folders[f]);
        }
    }
    
    manifestsWithBaseRaw.push_back(FOLDER_NAMES::BASE_PACK);
    manifestsWithBaseRaw.insert(
        manifestsWithBaseRaw.end(),
        manifestsSansBaseRaw.begin(),
        manifestsSansBaseRaw.end()
    );
    
    //Organized manifests.
    vector<string> organized_folders =
        filterVectorWithBanList(raw_folders, game.options.packs.disabled);
    organized_folders =
        sortVectorWithPreferenceList(
            organized_folders, game.options.packs.order
        );
        
    for(size_t f = 0; f < organized_folders.size(); f++) {
        if(organized_folders[f] != FOLDER_NAMES::BASE_PACK) {
            manifestsSansBase.push_back(organized_folders[f]);
        }
    }
    
    manifestsWithBase.push_back(FOLDER_NAMES::BASE_PACK);
    manifestsWithBase.insert(
        manifestsWithBase.end(),
        manifestsSansBase.begin(),
        manifestsSansBase.end()
    );
}


/**
 * @brief Loads all packs in the manifests, including the base pack.
 * This only loads their metadata, not their content!
 * This also loads all packs, not just the ones organized via the
 * player options.
 */
void PackManager::loadAll() {
    for(size_t p = 0; p < manifestsWithBaseRaw.size(); p++) {
        DataNode pack_file =
            loadDataFile(
                FOLDER_PATHS_FROM_ROOT::GAME_DATA + "/" +
                manifestsWithBaseRaw[p] + "/" +
                FILE_NAMES::PACK_DATA
            );
            
        Pack pack_data;
        pack_data.name = manifestsWithBaseRaw[p];
        ReaderSetter rs(&pack_file);
        rs.set("name", pack_data.name);
        rs.set("description", pack_data.description);
        rs.set("tags", pack_data.tags);
        rs.set("maker", pack_data.maker);
        rs.set("version", pack_data.version);
        rs.set("engine_version", pack_data.engineVersion);
        rs.set("dependencies", pack_data.dependencies);
        rs.set("conflicts", pack_data.conflicts);
        rs.set("notes", pack_data.notes);
        
        list[manifestsWithBaseRaw[p]] = pack_data;
    }
}


/**
 * @brief Unloads all loaded packs.
 * This only unloads their metadata, not their content!
 */
void PackManager::unloadAll() {
    list.clear();
}
