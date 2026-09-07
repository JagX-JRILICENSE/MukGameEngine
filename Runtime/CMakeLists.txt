add_executable(MukRuntime main.cpp)

target_link_libraries(MukRuntime PRIVATE MukEngine)

if(WIN32)
    set_target_properties(MukRuntime PROPERTIES
        WIN32_EXECUTABLE FALSE  # Console for now (logging)
    )
endif()
