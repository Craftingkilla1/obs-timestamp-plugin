cmake_minimum_required(VERSION 3.16...3.26)

project(obs-timestamp-plugin VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OBS Studio
if(NOT DEFINED libobs_DIR)
    find_package(libobs QUIET)
endif()

# If libobs not found, try finding obs-frontend-api
if(NOT TARGET OBS::libobs)
    find_package(obs-frontend-api QUIET)
endif()

# Fallback: try to find by path
if(NOT TARGET OBS::libobs)
    if(WIN32)
        set(LIBOBS_LIB_DIR "${CMAKE_PREFIX_PATH}/bin/64bit" "${CMAKE_PREFIX_PATH}/bin")
        set(LIBOBS_INCLUDE_DIR "${CMAKE_PREFIX_PATH}/include")
    else()
        set(LIBOBS_LIB_DIR "${CMAKE_PREFIX_PATH}/lib")
        set(LIBOBS_INCLUDE_DIR "${CMAKE_PREFIX_PATH}/include")
    endif()

    find_library(OBS_LIB NAMES obs libobs PATHS ${LIBOBS_LIB_DIR})
    find_library(OBS_FRONTEND_LIB NAMES obs-frontend-api libobs-frontend-api PATHS ${LIBOBS_LIB_DIR})

    if(OBS_LIB)
        add_library(OBS::libobs UNKNOWN IMPORTED)
        set_target_properties(OBS::libobs PROPERTIES
            IMPORTED_LOCATION "${OBS_LIB}"
            INTERFACE_INCLUDE_DIRECTORIES "${LIBOBS_INCLUDE_DIR}"
        )
    endif()
endif()

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

# Include directories
target_include_directories(obs-timestamp-plugin PRIVATE
    ${LIBOBS_INCLUDE_DIR}
    ${CMAKE_PREFIX_PATH}/include
)

# Link against OBS libraries
if(TARGET OBS::libobs)
    target_link_libraries(obs-timestamp-plugin PRIVATE OBS::libobs)
elseif(OBS_LIB)
    target_link_libraries(obs-timestamp-plugin PRIVATE ${OBS_LIB})
    if(OBS_FRONTEND_LIB)
        target_link_libraries(obs-timestamp-plugin PRIVATE ${OBS_FRONTEND_LIB})
    endif()
else()
    message(FATAL_ERROR "Could not find OBS libraries. Please set CMAKE_PREFIX_PATH to your OBS installation.")
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
