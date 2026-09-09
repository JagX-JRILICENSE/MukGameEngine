# Real Windows app — DX12 window with triangle (not a MessageBox stub)
add_executable(MukEditor WIN32 standalone_app.cpp)

if(WIN32)
    target_link_libraries(MukEditor PRIVATE d3d12 dxgi d3dcompiler user32 gdi32)
    set_target_properties(MukEditor PROPERTIES
        OUTPUT_NAME "MukEditor"
        WIN32_EXECUTABLE TRUE
    )
    if(EXISTS "${CMAKE_SOURCE_DIR}/Resources/app.rc")
        target_sources(MukEditor PRIVATE ${CMAKE_SOURCE_DIR}/Resources/app.rc)
    endif()
endif()
