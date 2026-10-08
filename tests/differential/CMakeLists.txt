# Differential runner for exported SurRender behavior. It links the provider
# import library (whose names the SURRENDER build validates against the retail
# export table) and runs unchanged against retail and rebuilt SR.DLL through
# `uv run wiz8 sr-difftest`.
if(NOT WIZ8_ANALYSIS_BUILD)
    add_executable(SR_DIFFTEST sr_difftest.cpp sr_probes.cpp sr_surface.cpp sr_huffman.cpp sr_misc.cpp)
    target_link_libraries(SR_DIFFTEST PRIVATE wiz8_compile_settings SURRENDER msvcrt.lib user32.lib)
    target_compile_options(SR_DIFFTEST PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/GX>")
    target_link_options(SR_DIFFTEST PRIVATE
        /INCREMENTAL:NO /SUBSYSTEM:CONSOLE,4.0 /OSVERSION:4.0)
    set_target_properties(SR_DIFFTEST PROPERTIES OUTPUT_NAME sr_difftest)
endif()
