#  ────────────────────────────────────────────────────────────
#                     ╔╗  ╔╗ ╔══╗      ╔════╗
#                     ║╚╗╔╝║ ╚╣╠╝      ║╔╗╔╗║
#                     ╚╗╚╝╔╝  ║║  ╔══╗ ╚╝║║╚╝
#                      ╚╗╔╝   ║║  ║╔╗║   ║║
#                       ║║   ╔╣╠╗ ║╚╝║   ║║
#                       ╚╝   ╚══╝ ╚══╝   ╚╝
#    ╔╗╔═╗                    ╔╗                     ╔╗
#    ║║║╔╝                   ╔╝╚╗                    ║║
#    ║╚╝╝  ╔══╗ ╔══╗ ╔══╗  ╔╗╚╗╔╝  ╔══╗ ╔╗ ╔╗╔╗ ╔══╗ ║║  ╔══╗
#    ║╔╗║  ║║═╣ ║║═╣ ║╔╗║  ╠╣ ║║   ║ ═╣ ╠╣ ║╚╝║ ║╔╗║ ║║  ║║═╣
#    ║║║╚╗ ║║═╣ ║║═╣ ║╚╝║  ║║ ║╚╗  ╠═ ║ ║║ ║║║║ ║╚╝║ ║╚╗ ║║═╣
#    ╚╝╚═╝ ╚══╝ ╚══╝ ║╔═╝  ╚╝ ╚═╝  ╚══╝ ╚╝ ╚╩╩╝ ║╔═╝ ╚═╝ ╚══╝
#                    ║║                         ║║
#                    ╚╝                         ╚╝
#
#    Lead Maintainer: Roman Kutashenko <kutashenko@gmail.com>
#  ────────────────────────────────────────────────────────────

cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

project(yiot VERSION 0.1.0 LANGUAGES C CXX)

# ---------------------------------------------------------------------------
#   C++
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
#	Add dependencies
# ---------------------------------------------------------------------------

#   Ext projects path
set(EXT_PATH "${CMAKE_CURRENT_LIST_DIR}/../../../ext")

# ---------------------------------------------------------------------------
#	Provisioning application
# ---------------------------------------------------------------------------
add_executable(yiot)

# ---------------------------------------------------------------------------
#	Header/Source files
# ---------------------------------------------------------------------------
target_sources(yiot
        PRIVATE
        # Headers
        ${CMAKE_CURRENT_LIST_DIR}/include/init.h
        ${CMAKE_CURRENT_LIST_DIR}/include/commands/device-info.h
        ${CMAKE_CURRENT_LIST_DIR}/include/commands/pc.h
        ${CMAKE_CURRENT_LIST_DIR}/include/commands/reset.h

        # Sources
        ${CMAKE_CURRENT_LIST_DIR}/src/main.c
        ${CMAKE_CURRENT_LIST_DIR}/src/init.c
        ${CMAKE_CURRENT_LIST_DIR}/src/commands/device-info.c
        ${CMAKE_CURRENT_LIST_DIR}/src/commands/pc.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/commands/reset.cpp
        )


# ---------------------------------------------------------------------------
#	Definitions
# ---------------------------------------------------------------------------
target_compile_definitions(yiot
        PRIVATE
        )


# ---------------------------------------------------------------------------
#   Include directories
# ---------------------------------------------------------------------------
target_include_directories(yiot
        PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${COMMON_NIX_DIR}/include
        )


# ---------------------------------------------------------------------------
#	Compile options
# ---------------------------------------------------------------------------
target_compile_options(yiot
        PRIVATE
        -Wno-multichar
        )

# ---------------------------------------------------------------------------
#	Link libraries
# ---------------------------------------------------------------------------
target_link_libraries(yiot
        PRIVATE

#        #   IoTKit
#        nix-common
#        vs-module-snap-thing
#        vs-module-snap-initializer
#        vs-json
#        vs-base64
        yiot-openwrt

        #   IoTKit extensions
        yiot-snap-thing-pc
        yiot-qos1-helper

        #   JSON
        nlohmann_json::nlohmann_json

        #   Helpers
        enable_sanitizers
        #       enable_pedantic_mode
        )


# ---------------------------------------------------------------------------
#   Format code
# ---------------------------------------------------------------------------
if (COMMAND add_clangformat)
    add_clangformat(yiot)
endif ()


# ---------------------------------------------------------------------------
#   Install
# ---------------------------------------------------------------------------
install(TARGETS yiot
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
