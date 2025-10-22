cmake_minimum_required(VERSION 3.16...3.26)

project(obs-timestamp-plugin VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Plugin source files
set(PLUGIN_SOURCES
    src/plugin-main.c
    src/timestamp-plugin.c
)

# Plugin headers
set(PLUGIN_HEADERS
    src/timestamp-plugin.h
)

# Create the plugin module
add_library(obs-timestamp-plugin MODULE ${PLUGIN_SOURCES} ${PLUGIN_HEADERS})

# Find OBS headers
if(WIN32)
    set(LIBOBS_INCLUDE_DIR "${CMAKE_PREFIX_PATH}/include" "${CMAKE_PREFIX_PATH}/include/obs")
    set(LIBOBS_LIB_DIR "${CMAKE_PREFIX_PATH}/bin/64bit")
elseif(APPLE)
    set(LIBOBS_INCLUDE_DIR "${CMAKE_PREFIX_PATH}/include" "/usr/local/include")
    set(LIBOBS_LIB_DIR "${CMAKE_PREFIX_PATH}/lib" "/usr/local/lib")
else()
    set(LIBOBS_INCLUDE_DIR "${CMAKE_PREFIX_PATH}/include" "/usr/include")
    set(LIBOBS_LIB_DIR "${CMAKE_PREFIX_PATH}/lib" "/usr/lib")
endif()

# Include directories
target_include_directories(obs-timestamp-plugin PRIVATE ${LIBOBS_INCLUDE_DIR})

# On Windows, link against the DLLs directly (they'll be delay-loaded)
if(WIN32)
    find_file(OBS_DLL NAMES obs.dll PATHS ${LIBOBS_LIB_DIR} REQUIRED)
    find_file(OBS_FRONTEND_DLL NAMES obs-frontend-api.dll PATHS ${LIBOBS_LIB_DIR})

    # For Windows, we need to tell the linker where to find the DLLs
    # But since it's a plugin loaded by OBS, the symbols will be resolved at runtime
    target_link_libraries(obs-timestamp-plugin PRIVATE ${OBS_DLL})
    if(OBS_FRONTEND_DLL)
        target_link_libraries(obs-timestamp-plugin PRIVATE ${OBS_FRONTEND_DLL})
    endif()
else()
    # On Linux/macOS, find the actual libraries
    find_library(OBS_LIB NAMES obs libobs PATHS ${LIBOBS_LIB_DIR} REQUIRED)
    find_library(OBS_FRONTEND_LIB NAMES obs-frontend-api libobs-frontend-api PATHS ${LIBOBS_LIB_DIR})

    target_link_libraries(obs-timestamp-plugin PRIVATE ${OBS_LIB})
    if(OBS_FRONTEND_LIB)
        target_link_libraries(obs-timestamp-plugin PRIVATE ${OBS_FRONTEND_LIB})
    endif()
endif()

# Set plugin properties
set_target_properties(obs-timestamp-plugin PROPERTIES
    FOLDER "plugins"
    VERSION ${PROJECT_VERSION}
    PREFIX ""
)

# Installation
if(WIN32)
    install(TARGETS obs-timestamp-plugin
        RUNTIME DESTINATION "obs-plugins/64bit"
        LIBRARY DESTINATION "obs-plugins/64bit"
    )
    install(DIRECTORY data/
        DESTINATION "data/obs-plugins/obs-timestamp-plugin"
    )
elseif(APPLE)
    install(TARGETS obs-timestamp-plugin
        LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/obs-plugins"
    )
    install(DIRECTORY data/
        DESTINATION "${CMAKE_INSTALL_PREFIX}/data/obs-plugins/obs-timestamp-plugin"
    )
else()
    install(TARGETS obs-timestamp-plugin
        LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/obs-plugins"
    )
    install(DIRECTORY data/
        DESTINATION "${CMAKE_INSTALL_PREFIX}/share/obs/obs-plugins/obs-timestamp-plugin"
    )
endif()
