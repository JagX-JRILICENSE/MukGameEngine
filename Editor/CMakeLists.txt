include(FetchContent)
if(POLICY CMP0169)
  cmake_policy(SET CMP0169 OLD)
endif()

FetchContent_Declare(imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG docking
  GIT_SHALLOW TRUE
)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
  FetchContent_Populate(imgui)
endif()

add_executable(MukEditor WIN32
  real_editor.cpp
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_dx12.cpp
)

target_include_directories(MukEditor PRIVATE
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
)

target_compile_definitions(MukEditor PRIVATE MUK_USE_IMGUI)

if(WIN32)
  target_link_libraries(MukEditor PRIVATE
    d3d12 dxgi d3dcompiler user32 gdi32 winhttp shell32
  )
  target_compile_options(MukEditor PRIVATE /W3 /EHsc /O2 /MT)
  set_target_properties(MukEditor PROPERTIES
    OUTPUT_NAME "MukEditor"
    WIN32_EXECUTABLE TRUE
  )
  if(EXISTS "${CMAKE_SOURCE_DIR}/Resources/app.rc")
    target_sources(MukEditor PRIVATE ${CMAKE_SOURCE_DIR}/Resources/app.rc)
  endif()
endif()
