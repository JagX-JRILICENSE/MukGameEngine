# Muk Editor - Visual tools for the engine
# Currently a placeholder; will become a full ImGui / custom editor

add_executable(MukEditor main.cpp)
target_link_libraries(MukEditor PRIVATE MukEngine)

if(WIN32)
    set_target_properties(MukEditor PROPERTIES WIN32_EXECUTABLE FALSE)
endif()
