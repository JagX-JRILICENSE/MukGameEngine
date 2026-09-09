add_executable(MukEditor WIN32 standalone_app.cpp)

if(WIN32)
    target_link_libraries(MukEditor PRIVATE d3d12 dxgi d3dcompiler user32 gdi32)
    set_target_properties(MukEditor PROPERTIES
        OUTPUT_NAME "MukEditor"
        WIN32_EXECUTABLE TRUE
    )
    # Static runtime + optimizations → larger, self-contained Release exe
    target_compile_options(MukEditor PRIVATE /W3 /EHsc /O2 /MT)
    target_link_options(MukEditor PRIVATE /SUBSYSTEM:WINDOWS)
    if(EXISTS "${CMAKE_SOURCE_DIR}/Resources/app.rc")
        target_sources(MukEditor PRIVATE ${CMAKE_SOURCE_DIR}/Resources/app.rc)
    endif()
endif()
