# Explicit link order for recovered first-party sources. Renaming or moving a
# unit must not move its slot. src/wiz8/vector.cpp collects compiler-emitted
# growable-vector specializations whose original translation-unit ownership is
# not established; every other entry is recovered source whose placement
# provenance lives in recovery evidence, not in this list.
set(WIZ8_SOURCE_UNITS
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
    "src/wiz8/local_code/Combat Hostility.cpp"
    "src/wiz8/local_code/Magic Effects.cpp"
    src/wiz8/local_code/VideoObjectManager.cpp
    "src/wiz8/local_screens/IntroScreen.cpp"
    "src/wiz8/local_screens/MainMenuScreen.cpp"
    src/wiz8/local_screens/MGSKeyboard.cpp
    src/wiz8/local_screens/OptionsScreen.cpp
    src/wiz8/local_screens/PleaseWaitScreen.cpp
    src/wiz8/local_screens/Screens.cpp
    src/wiz8/local_screens/ReviewCharacterScreen.cpp
    src/wiz8/local_screens/MGSSpellIcons.cpp
    src/wiz8/local_screens/RCSCommon.cpp
    src/wiz8/local_screens/MGSTextBox.cpp
    src/wiz8/local_screens/MainGameScreen.cpp
    src/wiz8/local_screens/CharacterScreen.cpp
    src/wiz8/local_screens/CharacterPages.cpp
    src/wiz8/local_screens/CGSSpellsPage.cpp
    src/wiz8/local_screens/CGSStatsPage.cpp
    src/wiz8/local_screens/JournalScreen.cpp
    src/wiz8/local_screens/CreditsScreen.cpp
    src/wiz8/local_screens/MGSPortraits.cpp
    src/wiz8/local_screens/MGSUseItemSelect.cpp
    src/wiz8/local_screens/MGSPartyMovement.cpp
    src/wiz8/location_variables.cpp
    src/wiz8/local_code/MonsterManager.cpp
    src/wiz8/local_code/MonsterGroup.cpp
    src/wiz8/local_code/UtilityFunctions.cpp
    src/wiz8/local_code/Strings.cpp
    src/wiz8/local_code/Configuration.cpp
    src/wiz8/local_code/ButtonSound.cpp
    src/wiz8/dialog_code/NotificationDialog.cpp
    src/wiz8/dialog_code/ModalDialogBase.cpp
    src/wiz8/dialog_code/DialogBase.cpp
    src/wiz8/dialog_code/DialogFactoryDialogs.cpp
    src/wiz8/dialog_code/DialogTextEntry.cpp
    src/wiz8/dialog_code/StatInfoDialogs.cpp
    src/wiz8/dialog_code/ProfRaceInfoDialog.cpp
    src/wiz8/engine_code/Object0043A910.cpp
    src/wiz8/engine_code/IntervalGate.cpp
    src/wiz8/engine_code/BitArray.cpp
    src/wiz8/engine_code/GameData.cpp
    src/wiz8/engine_code/Octree.cpp
    src/wiz8/engine_code/OctPath.cpp
    src/wiz8/engine_code/OctPreTree.cpp
    src/wiz8/engine_code/Levels.cpp
    src/wiz8/engine_code/ReadLevel.cpp
    src/wiz8/engine_code/ReadMesh.cpp
    src/wiz8/engine_code/quad.cpp
    src/wiz8/engine_code/3dapi.cpp
    src/wiz8/engine_code/stMeshModel.cpp
    src/wiz8/engine_code/stModelInstance.cpp
    src/wiz8/engine_code/stTextureAnim.cpp
    src/wiz8/engine_code/stTextureFile.cpp
    src/wiz8/engine_code/stParticle.cpp
    src/wiz8/engine_code/OctMeshModel.cpp
    src/wiz8/engine_code/Trigger.cpp
    src/wiz8/engine_code/OctBuildTree.cpp
    src/wiz8/engine_code/Environment.cpp
    src/wiz8/engine_code/AmbientSound.cpp
    src/wiz8/engine_code/AnimObj.cpp
    src/wiz8/engine_code/AniMesh.cpp
    src/wiz8/engine_code/Item.cpp
    src/wiz8/engine_code/Missile.cpp
    src/wiz8/engine_code/PathAI.cpp
    src/wiz8/engine_code/Spells.cpp
    src/wiz8/engine_code/OctBuildPreTree.cpp
    src/wiz8/engine_code/SoundEvent.cpp
    src/wiz8/engine_code/AnimRep.cpp
    src/wiz8/engine_code/GDProp.cpp
    src/wiz8/engine_code/Prop.cpp
    src/wiz8/engine_code/Navigator.cpp
    src/wiz8/engine_code/GrObject.cpp
    src/wiz8/local_code/RegionManager.cpp
    src/wiz8/local_code/chunk.cpp
    src/wiz8/engine_code/GrCycle.cpp
    src/wiz8/engine_code/UpdateMesh.cpp
    src/wiz8/engine_code/stGroundShadow.cpp
    src/wiz8/local_code/GameplayDatabase.cpp
    src/wiz8/dialog_code/DialogInterface.cpp
    src/wiz8/3d_code/IList.cpp
    src/wiz8/local_code/ItemManager.cpp
    src/wiz8/engine_code/game_timer.cpp
    src/wiz8/engine_code/materials.cpp
    src/wiz8/engine_code/Monster.cpp
    src/wiz8/engine_code/stScript.cpp
    src/wiz8/local_code/Targeting.cpp
    src/wiz8/local_code/Viewport.cpp
    src/wiz8/3d_code/PList.cpp
    src/wiz8/engine_code/3d.cpp
    src/wiz8/engine_code/Bink.cpp
    src/wiz8/level_specific_code/MasterFunctionList.cpp
    src/wiz8/vector.cpp
    src/wiz8/imports/mss.cpp
    src/wiz8/bringup_gates.cpp
    src/wiz8/renderer_window.cpp
    src/wiz8/engine_code/Quality.cpp
    src/wiz8/startup_render_state.cpp
    src/wiz8/startup_world.cpp
    src/wiz8/startup_subsystems.cpp
    src/wiz8/startup_ui_state.cpp
    src/wiz8/startup_cursor.cpp
    src/wiz8/sound_man.cpp
    src/wiz8/surface2d.cpp
    src/wiz8/engine_code/Video2.cpp
    src/wiz8/character_skills.cpp
    src/wiz8/fact_state.cpp
    src/wiz8/dirty_tiles.cpp
    src/wiz8/render_options.cpp
    src/wiz8/engine_code/registry_classes.cpp
    src/wiz8/engine_code/MonsterLight.cpp
    src/wiz8/engine_code/GDCamera.cpp
    src/wiz8/engine_code/world_selection.cpp
    src/wiz8/engine_code/stCube.cpp
    src/wiz8/engine_code/Cursor3d.cpp
    src/wiz8/engine_code/stLight.cpp
    src/wiz8/engine_code/bounds.cpp
    src/wiz8/engine_code/OctRegionPolygon.cpp
    src/wiz8/version.cpp
    src/wiz8/local_code/party_encumbrance.cpp
    src/wiz8/local_code/Search.cpp
    src/wiz8/local_code/npc_interaction.cpp
    "src/wiz8/local_code/NPC Scripting.cpp"
    src/wiz8/local_code/character_events.cpp
    src/wiz8/local_code/formation_state.cpp
    src/wiz8/local_code/CharGeneration.cpp
    src/wiz8/local_screens/AutomapScreen.cpp
    src/wiz8/local_screens/screen12.cpp
    src/wiz8/local_screens/MGSSpellCasting.cpp
    src/wiz8/local_screens/PartySelectionScreen.cpp
    src/wiz8/local_screens/mipeEdit.cpp
    src/wiz8/local_code/Text_Input.cpp
    src/wiz8/local_code/Traps.cpp
    src/wiz8/local_code/Gameloop.cpp
    src/wiz8/game_init.cpp
    src/wiz8/gameplay_teardown.cpp
    src/wiz8/item_spawning.cpp
    src/wiz8/message_box.cpp
    src/wiz8/monster_info_dialog.cpp
    src/wiz8/dialog_code/DialogTextArea.cpp
    src/wiz8/dialog_code/DialogButton.cpp
    src/wiz8/dialog_code/DialogScrollBar.cpp
    src/wiz8/engine_code/MonGen.cpp
    src/wiz8/monster_lookup.cpp
    src/wiz8/music_playlist.cpp
    src/wiz8/npc_items.cpp
    src/wiz8/record_file_0055a480.cpp
    src/wiz8/spell_backfire.cpp
    src/wiz8/state_getters.cpp
    src/wiz8/surrender_math.cpp
    src/wiz8/virtual_file_stream.cpp
    src/wiz8/vc6_runtime.cpp
)

# Every recovered C++ source must appear exactly once. The glob is
# validation-only: it prevents a new recovered source from silently escaping
# ownership without using the filesystem to order or populate the build.
set(WIZ8_CLASSIFIED_SOURCES)
foreach(source IN LISTS WIZ8_SOURCE_UNITS)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
        message(FATAL_ERROR "WIZ8_SOURCE_UNITS names missing source: ${source}")
    endif()
    if(source IN_LIST WIZ8_CLASSIFIED_SOURCES)
        message(FATAL_ERROR "Wiz8 source is classified more than once: ${source}")
    endif()
    list(APPEND WIZ8_CLASSIFIED_SOURCES "${source}")
endforeach()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/wiz8/unattributed_helpers.cpp")
    message(FATAL_ERROR "Synthetic catch-all source is forbidden: unattributed_helpers.cpp")
endif()
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
