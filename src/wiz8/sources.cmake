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
    src/wiz8/local_screens/OptionsScreen.cpp
    src/wiz8/local_screens/PleaseWaitScreen.cpp
    src/wiz8/local_screens/Screens.cpp
    src/wiz8/local_screens/RCSCommon.cpp
    src/wiz8/local_screens/MGSTextBox.cpp
    src/wiz8/local_screens/MainGameScreen.cpp
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
    src/wiz8/dialog_code/Object005EF894.cpp
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
    src/wiz8/local_code/DisplayList.cpp
    src/wiz8/local_code/Targeting.cpp
    src/wiz8/local_code/Viewport.cpp
    src/wiz8/3d_code/PList.cpp
    src/wiz8/engine_code/3d.cpp
    src/wiz8/engine_code/Bink.cpp
    src/wiz8/level_specific_code/MasterFunctionList.cpp
)
# Compiler-emitted growable-vector specializations whose original
# translation-unit ownership is not established.
set(WIZ8_TEMPLATE_EMISSIONS
    src/wiz8/vector.cpp
)

# Original translation-unit names remain unproved for these units. A descriptive
# filename records a coherent subsystem, not a claim about the original name.
# Unknown fragments retain bounded address names under unattributed/.
# Keep this single list in link order: renaming a unit must not move its slot.
set(WIZ8_PROVISIONAL_AND_QUARANTINE_UNITS
    src/wiz8/bringup_gates.cpp
    src/wiz8/renderer_window.cpp
    src/wiz8/startup_render_state.cpp
    src/wiz8/startup_world.cpp
    src/wiz8/startup_subsystems.cpp
    src/wiz8/startup_ui_state.cpp
    src/wiz8/startup_cursor.cpp
    src/wiz8/slf_archives.cpp
    src/wiz8/sgp_runtime_adapters.cpp
    src/wiz8/surface2d.cpp
    src/wiz8/video2.cpp
    src/wiz8/window_proc.cpp
    src/wiz8/character_skills.cpp
    src/wiz8/fact_state.cpp
    src/wiz8/dirty_tiles.cpp
    src/wiz8/render_options.cpp
    src/wiz8/engine_code/registry_classes.cpp
    src/wiz8/engine_code/MonsterLight.cpp
    src/wiz8/engine_code/GDCamera.cpp
    src/wiz8/unattributed/00401001_0041ab3f.cpp
    src/wiz8/unattributed/0041f261_0042403f.cpp
    src/wiz8/unattributed/00424041_0042a36f.cpp
    src/wiz8/engine_code/world_selection.cpp
    src/wiz8/unattributed/0046c0f1_0046dc8f.cpp
    src/wiz8/unattributed/0047a791_0047b4ff.cpp
    src/wiz8/unattributed/0048e7b1_00490c5f.cpp
    src/wiz8/unattributed/00490c61_00497aef.cpp
    src/wiz8/engine_code/stLight.cpp
    src/wiz8/unattributed/004b6bd1_004b6f2f.cpp
    src/wiz8/engine_code/bounds.cpp
    src/wiz8/engine_code/OctRegionPolygon.cpp
    src/wiz8/version.cpp
    src/wiz8/local_code/party_encumbrance.cpp
    src/wiz8/unattributed/004f0c81_004f203f.cpp
    src/wiz8/local_code/missile_references.cpp
    src/wiz8/unattributed/00516f01_00517c5f.cpp
    src/wiz8/local_code/npc_interaction.cpp
    src/wiz8/unattributed/00526e91_0052a88f.cpp
    src/wiz8/local_code/character_events.cpp
    src/wiz8/local_code/formation_state.cpp
    src/wiz8/unattributed/005587c1_005592cf.cpp
    src/wiz8/unattributed/0055f081_0056af7f.cpp
    src/wiz8/local_screens/screen8.cpp
    src/wiz8/unattributed/00583bc1_0058abff.cpp
    src/wiz8/local_screens/screen12.cpp
    src/wiz8/unattributed/0059e0f1_0059f70f.cpp
    src/wiz8/unattributed/005a1151_005a19af.cpp
    src/wiz8/unattributed/005a8ed1_005b478f.cpp
    src/wiz8/local_screens/PartySelectionScreen.cpp
    src/wiz8/unattributed/005c4341_005c87af.cpp
    src/wiz8/local_code/text_input.cpp
    src/wiz8/unattributed/005e2cc1_005e37ff.cpp
    src/wiz8/frame_tick.cpp
    src/wiz8/game_init.cpp
    src/wiz8/gameplay_teardown.cpp
    src/wiz8/item_spawning.cpp
    src/wiz8/message_box.cpp
    src/wiz8/monster_info_dialog.cpp
    src/wiz8/monster_generators.cpp
    src/wiz8/monster_lookup.cpp
    src/wiz8/music_playlist.cpp
    src/wiz8/npc_items.cpp
    src/wiz8/npc_item_lists.cpp
    src/wiz8/record_file_0055a480.cpp
    src/wiz8/spell_backfire.cpp
    src/wiz8/state_getters.cpp
    src/wiz8/surrender_math.cpp
    src/wiz8/virtual_file_stream.cpp
    src/wiz8/vc6_runtime.cpp
)

# Keep logical ownership separate from original translation-unit certainty.
# Descriptive paths are provisional; only numeric files under unattributed/
# remain address-bounded quarantine units.
set(WIZ8_PROVISIONAL_UNITS)
set(WIZ8_ADDRESS_QUARANTINE_UNITS)
foreach(source IN LISTS WIZ8_PROVISIONAL_AND_QUARANTINE_UNITS)
    if(source MATCHES "^src/wiz8/unattributed/[0-9a-f]+_[0-9a-f]+[.]cpp$")
        list(APPEND WIZ8_ADDRESS_QUARANTINE_UNITS "${source}")
    else()
        list(APPEND WIZ8_PROVISIONAL_UNITS "${source}")
    endif()
endforeach()

# These explicit categories preserve matching-sensitive object order. The
# glob is validation-only: it prevents a new recovered C++ source from
# silently escaping ownership without using the filesystem to order or
# populate the build.
set(WIZ8_RECOVERED_SOURCE_CATEGORIES
    WIZ8_ORIGINAL_UNITS
    WIZ8_TEMPLATE_EMISSIONS
    WIZ8_PROVISIONAL_AND_QUARANTINE_UNITS
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
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/wiz8/unattributed_helpers.cpp")
    message(FATAL_ERROR "Synthetic catch-all source is forbidden: unattributed_helpers.cpp")
endif()
foreach(source IN LISTS WIZ8_ADDRESS_QUARANTINE_UNITS)
    if(source MATCHES "^src/wiz8/unattributed/([0-9a-f]+)_([0-9a-f]+)[.]cpp$")
        set(lower "${CMAKE_MATCH_1}")
        set(upper "${CMAKE_MATCH_2}")
        string(LENGTH "${lower}" lower_length)
        string(LENGTH "${upper}" upper_length)
        if(NOT lower_length EQUAL 8 OR NOT upper_length EQUAL 8)
            message(FATAL_ERROR "Quarantine bounds must be eight hex digits: ${source}")
        endif()
        file(READ "${CMAKE_CURRENT_SOURCE_DIR}/${source}" contents)
        string(REGEX MATCHALL "// FUNCTION: WIZ8 0x[0-9A-Fa-f]+" markers "${contents}")
        if(NOT markers)
            message(FATAL_ERROR "Address quarantine has no FUNCTION markers: ${source}")
        endif()
        foreach(marker IN LISTS markers)
            string(REGEX REPLACE ".*0x" "" address "${marker}")
            string(TOLOWER "${address}" address)
            if(address STRLESS lower OR upper STRLESS address)
                message(FATAL_ERROR
                    "${source} owns ${address}, outside its ${lower}-${upper} bounds")
            endif()
        endforeach()
    endif()
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
