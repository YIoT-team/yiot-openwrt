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

project(littlefs VERSION 0.1.0 LANGUAGES C)

#
#   Library options
#

#   general options
option(LFS_NO_MALLOC "Disable dynamic allocation" OFF)
option(LFS_NO_ASSERT "Disable assertion" OFF)

# log message options
option(LFS_NO_DEBUG "Disable debug messages" ON)
option(LFS_NO_WARN "Disable warning messages" ON)
option(LFS_NO_ERROR "Disable error message" ON)
option(LFS_YES_TRACE "Enable trace messages" OFF)

#   littlefs library config
set(LFS_NAME_MAX 65 CACHE STRING "Maximum name size in bytes")
set(LFS_FILE_MAX 65535 CACHE STRING "Maximum size of a file in bytes")
set(LFS_ATTR_MAX 1022 CACHE STRING "Maximum size of custom attributes in bytes")

add_library(littlefs)

target_sources(littlefs
        PRIVATE
        # headers
        "${CMAKE_CURRENT_LIST_DIR}/littlefs/lfs.h"
        "${CMAKE_CURRENT_LIST_DIR}/littlefs/lfs_util.h"
        # sources
        "${CMAKE_CURRENT_LIST_DIR}/littlefs/lfs.c"
        "${CMAKE_CURRENT_LIST_DIR}/littlefs/lfs_util.c"
        )

set_target_properties(littlefs
        PROPERTIES
        C_STANDARD 99
        C_STANDARD_REQUIRED ON
        )

target_compile_definitions(littlefs
        PRIVATE
        $<$<BOOL:${LFS_NO_DEBUG}>:LFS_NO_DEBUG=1>
        $<$<BOOL:${LFS_NO_WARN}>:LFS_NO_WARN=1>
        $<$<BOOL:${LFS_NO_ERROR}>:LFS_NO_ERROR=1>
        $<$<BOOL:${LFS_YES_TRACE}>:LFS_YES_TRACE=1>
        $<$<BOOL:${LFS_NO_MALLOC}>:LFS_NO_MALLOC=1>
        $<$<BOOL:${LFS_NO_ASSERT}>:LFS_NO_ASSERT=1>
        LFS_NAME_MAX=${LFS_NAME_MAX}
        LFS_FILE_MAX=${LFS_FILE_MAX}
        LFS_ATTR_MAX=${LFS_ATTR_MAX}
        )

target_compile_options(littlefs
        PRIVATE
        -Wall
        -Wextra -Wshadow -Wundef
        -Wno-missing-field-initializers
        )

target_link_libraries(littlefs
        PRIVATE
        )

target_include_directories (littlefs
        PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}"
        "${CMAKE_CURRENT_LIST_DIR}/littlefs"
        )