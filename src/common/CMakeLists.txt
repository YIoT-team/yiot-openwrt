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

include(GNUInstallDirs)

# ---------------------------------------------------------------------------
# YIoT Core
# ---------------------------------------------------------------------------
set (VIRGIL_IOT_CONFIG_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/include/common/config/pc")
set (VIRGIL_IOT_CLOUD OFF)
set (VIRGIL_IOT_HIGH_LEVEL OFF)
set (VIRGIL_IOT_THREADSAFE ON)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../core yiot-core)

# ---------------------------------------------------------------------------
# LittleFS
# ---------------------------------------------------------------------------
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/ext/filesystem)

project(yiot-openwrt VERSION 0.1.0 LANGUAGES C)

    #
    #	Firmware verifier target
    #
    add_library(yiot-openwrt SHARED)

    #
    #	Header/Source files
    #
    target_sources(yiot-openwrt
             PRIVATE

             #	Headers
             ${CMAKE_CURRENT_LIST_DIR}/include/common/iotkit-impl/packets-queue.h
             ${CMAKE_CURRENT_LIST_DIR}/include/common/iotkit-impl/netif/netif-udp.h
             ${CMAKE_CURRENT_LIST_DIR}/ext/filesystem/yiot-littlefs-storage.h
             ${CMAKE_CURRENT_LIST_DIR}/ext/filesystem/yiot-littlefs.h
             ${CMAKE_CURRENT_LIST_DIR}/include/common/helpers/file-io.h
             ${CMAKE_CURRENT_LIST_DIR}/include/common/helpers/event-group-bits.h
             ${CMAKE_CURRENT_LIST_DIR}/include/common/helpers/app-helpers.h
             ${CMAKE_CURRENT_LIST_DIR}/include/common/helpers/timer.h
             ${CMAKE_CURRENT_LIST_DIR}/include/common/helpers/settings.h
             ${CMAKE_CURRENT_LIST_DIR}/include/common/helpers/msg-queue.h
             ${CMAKE_CURRENT_LIST_DIR}/include/common/config/pc/endian-config.h
             ${CMAKE_CURRENT_LIST_DIR}/include/common/config/pc/global-hal.h
             ${CMAKE_CURRENT_LIST_DIR}/include/common/config/pc/logger-config.h
             ${CMAKE_CURRENT_LIST_DIR}/include/common/config/pc/stdlib-config.h
             ${CMAKE_CURRENT_LIST_DIR}/include/common/config/pc/trust_list-config.h
             ${CMAKE_CURRENT_LIST_DIR}/include/common/config/pc/update-config.h

             #	Sources
             ${CMAKE_CURRENT_LIST_DIR}/src/iotkit-impl/packets-queue.c
             ${CMAKE_CURRENT_LIST_DIR}/src/iotkit-impl/netif/netif-udp.cpp
             ${CMAKE_CURRENT_LIST_DIR}/ext/filesystem/yiot-littlefs-storage.c
             ${CMAKE_CURRENT_LIST_DIR}/ext/filesystem/yiot-littlefs.c
             ${CMAKE_CURRENT_LIST_DIR}/src/iotkit-impl/threadsafe/lock-nix-hal.c
             ${CMAKE_CURRENT_LIST_DIR}/src/iotkit-impl/firmware/firmware-impl.c
             ${CMAKE_CURRENT_LIST_DIR}/src/iotkit-impl/threadsafe/lock-nix-hal.c
             ${CMAKE_CURRENT_LIST_DIR}/src/iotkit-impl/helpers.c
             ${CMAKE_CURRENT_LIST_DIR}/src/helpers/file-io.c
             ${CMAKE_CURRENT_LIST_DIR}/src/helpers/event-group-bits.c
             ${CMAKE_CURRENT_LIST_DIR}/src/helpers/app-helpers.c
             ${CMAKE_CURRENT_LIST_DIR}/src/helpers/timer.cpp
             ${CMAKE_CURRENT_LIST_DIR}/src/helpers/settings.cpp
             ${CMAKE_CURRENT_LIST_DIR}/src/helpers/msg-queue.c
            )

    #
    #	Definitions
    #
    target_compile_definitions(yiot-openwrt
            PRIVATE
            )

    #
    #   Include directories
    #
    target_include_directories(yiot-openwrt
            PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}/include
            ${CMAKE_CURRENT_LIST_DIR}/include/config/pc
            )

    #
    #	Link libraries
    #
    find_package (Threads)
    target_link_libraries(yiot-openwrt
            PUBLIC

            #   IoTKit
            vs-module-snap-thing
            vs-module-snap-initializer
            vs-json
            vs-base64

            yiot-snap-thing-pc
            yiot-qos1-helper
            converters
            vs-module-secbox
            vs-module-provision
            vs-default-soft-secmodule
            vs-module-firmware
            vs-module-logger
            vs-module-secmodule
            vs-module-crypto-high-level
            trust_list
            update
            vs-threadsafe

            #   LittleFS
            littlefs

            # Threads
            ${CMAKE_THREAD_LIBS_INIT}
            )

    #
    #   Install library
    #
    install(TARGETS yiot-openwrt
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            )
