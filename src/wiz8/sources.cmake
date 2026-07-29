set(WIZ8_ORIGINAL_UNITS
    "src/wiz8/local_code/PC Item.cpp"
    src/wiz8/local_code/FormationAndFacing.cpp
    src/wiz8/local_code/Controls.cpp
    src/wiz8/local_code/Factions.cpp
    src/wiz8/local_code/LoadSaveGame.cpp
    src/wiz8/local_code/Magic.cpp
    "src/wiz8/local_code/Health Stamina Mana.cpp"
    src/wiz8/local_code/ConditionsAndEnchantments.cpp
    src/wiz8/local_code/Sight.cpp
    "src/wiz8/local_code/Combat Party Movement.cpp"
    "src/wiz8/local_code/Combat Range.cpp"
    src/wiz8/local_code/GameplayCode.cpp
    src/wiz8/local_code/Combat.cpp
    "src/wiz8/local_code/Combat Sound.cpp"
    src/wiz8/local_code/MonsterAI.cpp
    "src/wiz8/local_code/NPC Manager.cpp"
    "src/wiz8/local_code/Combat Attack.cpp"
    "src/wiz8/local_code/Magic Effects.cpp"
    src/wiz8/local_code/VideoObjectManager.cpp
    "src/wiz8/local_screens/IntroScreen.cpp"
    "src/wiz8/local_screens/MainMenuScreen.cpp"
    src/wiz8/local_screens/Screens.cpp
    src/wiz8/local_screens/RCSCommon.cpp
    src/wiz8/local_screens/MGSTextBox.cpp
    src/wiz8/local_screens/MainGameScreen.cpp
    src/wiz8/location_variables.cpp
    src/wiz8/local_code/MonsterManager.cpp
    src/wiz8/local_code/MonsterGroup.cpp
    src/wiz8/local_code/UtilityFunctions.cpp
    src/wiz8/local_code/Strings.cpp
    src/wiz8/local_code/Configuration.cpp
    src/wiz8/dialog_code/Dialog005A80A0.cpp
    src/wiz8/dialog_code/DialogBase005D25B0.cpp
    src/wiz8/dialog_code/Object005EF894.cpp
    src/wiz8/dialog_code/StatInfoDialogs.cpp
    src/wiz8/dialog_code/ProfRaceInfoDialog.cpp
    src/wiz8/engine_code/Object0043A910.cpp
    src/wiz8/engine_code/Object005EBCFC.cpp
    src/wiz8/engine_code/BitArray.cpp
    src/wiz8/engine_code/GameData.cpp
    src/wiz8/engine_code/Octree.cpp
    src/wiz8/engine_code/3dapi.cpp
    src/wiz8/engine_code/stMeshModel.cpp
    src/wiz8/engine_code/stModelInstance.cpp
    src/wiz8/engine_code/stParticle.cpp
    src/wiz8/engine_code/Trigger.cpp
    src/wiz8/engine_code/Environment.cpp
    src/wiz8/engine_code/Missile.cpp
    src/wiz8/engine_code/Spells.cpp
    src/wiz8/engine_code/GDProp.cpp
    src/wiz8/engine_code/Prop005EC1E0.cpp
    src/wiz8/engine_code/GrObject.cpp
    src/wiz8/local_code/RegionManager.cpp
    src/wiz8/local_code/chunk.cpp
    src/wiz8/engine_code/GrCycle.cpp
    src/wiz8/engine_code/stGroundShadow.cpp
    src/wiz8/local_code/GameplayDatabase.cpp
    src/wiz8/dialog_code/DialogInterface.cpp
    src/wiz8/3d_code/IList.cpp
    src/wiz8/local_code/ItemManager.cpp
    src/wiz8/engine_code/game_timer.cpp
    src/wiz8/engine_code/materials.cpp
    src/wiz8/engine_code/Monster.cpp
    src/wiz8/local_code/DisplayList.cpp
    src/wiz8/local_code/SurfaceFill.cpp
    src/wiz8/local_code/Targeting.cpp
    src/wiz8/local_code/Viewport.cpp
    src/wiz8/3d_code/PList.cpp
    src/wiz8/engine_code/3d.cpp
    src/wiz8/engine_code/Bink.cpp
)
# Address-qualified sources are distinct VC6 template emissions. Their
# original translation-unit ownership is not established, so grouping
# them here does not merge or rename any emission.
set(WIZ8_TEMPLATE_EMISSIONS
    src/wiz8/vector_005ebfb4.cpp
    src/wiz8/vector_005ec16c.cpp
    src/wiz8/vector_005ec2b8.cpp
    src/wiz8/vector_005ec324.cpp
    src/wiz8/vector_005ec514.cpp
    src/wiz8/vector_005eca98.cpp
    src/wiz8/vector_005ecaa0.cpp
    src/wiz8/vector_005ecad0.cpp
    src/wiz8/vector_005ece60.cpp
    src/wiz8/vector_005ecee4.cpp
    src/wiz8/vector_005ed018.cpp
    src/wiz8/vector_005ed2c8.cpp
    src/wiz8/vector_005ed7d4.cpp
    src/wiz8/vector_005ef08c.cpp
    src/wiz8/vector_005ec018.cpp
    src/wiz8/vector_005ed1b8.cpp
    src/wiz8/vector_005ec164.cpp
    src/wiz8/vector_005eca5c.cpp
    src/wiz8/vector_005ec294.cpp
    src/wiz8/vector_005ed840.cpp
    src/wiz8/vector_005eea28.cpp
    src/wiz8/vector_005ecf00.cpp
    src/wiz8/vector_005ec27c.cpp
)
set(WIZ8_UNATTRIBUTED_UNITS
    src/wiz8/bringup_gates.cpp
    src/wiz8/renderer_window.cpp
    src/wiz8/startup_render_state.cpp
    src/wiz8/startup_world.cpp
    src/wiz8/startup_subsystems.cpp
    src/wiz8/startup_color.cpp
    src/wiz8/startup_ui_state.cpp
    src/wiz8/startup_cursor.cpp
    src/wiz8/slf_archives.cpp
    src/wiz8/sgp_runtime_adapters.cpp
    src/wiz8/surface2d.cpp
    src/wiz8/video2.cpp
    src/wiz8/window_proc.cpp
    src/wiz8/character_skills.cpp
    src/wiz8/gameplay_boundaries.cpp
    src/wiz8/grcycle.cpp
    src/wiz8/fact_state.cpp
    src/wiz8/dirty_tiles.cpp
    src/wiz8/render_options.cpp
    src/wiz8/unattributed_helpers.cpp
    src/wiz8/frame_tick.cpp
    src/wiz8/game_init.cpp
    src/wiz8/gameplay_teardown.cpp
    src/wiz8/item_mesh.cpp
    src/wiz8/item_runtime_catalog.cpp
    src/wiz8/item_spawning.cpp
    src/wiz8/message_box.cpp
    src/wiz8/monster_info_dialog.cpp
    src/wiz8/monster_generators.cpp
    src/wiz8/monster_lookup.cpp
    src/wiz8/music_playlist.cpp
    src/wiz8/npc_item_lists.cpp
    src/wiz8/spell_backfire.cpp
    src/wiz8/state_getters.cpp
    src/wiz8/vector_conversions.cpp
    src/wiz8/virtual_file_stream.cpp
    src/wiz8/vc6_runtime.cpp
)

# These explicit categories preserve matching-sensitive object order. The
# glob is validation-only: it prevents a new recovered C++ source from
# silently escaping ownership without using the filesystem to order or
# populate the build.
set(WIZ8_RECOVERED_SOURCE_CATEGORIES
    WIZ8_ORIGINAL_UNITS
    WIZ8_TEMPLATE_EMISSIONS
    WIZ8_UNATTRIBUTED_UNITS
)
set(WIZ8_CLASSIFIED_SOURCES)
foreach(category IN LISTS WIZ8_RECOVERED_SOURCE_CATEGORIES)
    foreach(source IN LISTS ${category})
        if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
            message(FATAL_ERROR "${category} names missing source: ${source}")
        endif()
        if(source IN_LIST WIZ8_CLASSIFIED_SOURCES)
            message(FATAL_ERROR "Wiz8 source is classified more than once: ${source}")
        endif()
        list(APPEND WIZ8_CLASSIFIED_SOURCES "${source}")
    endforeach()
endforeach()
if(CMAKE_SCRIPT_MODE_FILE)
    file(GLOB_RECURSE WIZ8_CPP_SOURCES
        RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/wiz8/*.cpp"
    )
else()
    file(GLOB_RECURSE WIZ8_CPP_SOURCES CONFIGURE_DEPENDS
        RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/wiz8/*.cpp"
    )
endif()
set(WIZ8_UNCLASSIFIED_SOURCES ${WIZ8_CPP_SOURCES})
list(REMOVE_ITEM WIZ8_UNCLASSIFIED_SOURCES ${WIZ8_CLASSIFIED_SOURCES})
if(WIZ8_UNCLASSIFIED_SOURCES)
    list(JOIN WIZ8_UNCLASSIFIED_SOURCES ", " missing_sources)
    message(FATAL_ERROR "Unclassified Wiz8 C++ sources: ${missing_sources}")
endif()
