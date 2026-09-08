add_executable(MukEditor main.cpp)
target_link_libraries(MukEditor PRIVATE MukEngine)

if(WIN32)
    # Product display name + version resource
    if(EXISTS "${CMAKE_SOURCE_DIR}/Resources/app.rc")
        target_sources(MukEditor PRIVATE ${CMAKE_SOURCE_DIR}/Resources/app.rc)
    endif()
    set_target_properties(MukEditor PROPERTIES
        WIN32_EXECUTABLE FALSE
        OUTPUT_NAME "MukEditor"
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
endif()
