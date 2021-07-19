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

project(bootloader VERSION 0.1.0 LANGUAGES C)

    #
    #	Firmware verifier target
    #
    add_executable(kwn-firmware-verifier)

    #
    #	Header/Source files
    #
    target_sources(kwn-firmware-verifier
             PRIVATE

             #	Headers
             ${CMAKE_CURRENT_LIST_DIR}/include/sdk-impl/netif/packets-queue.h
             ${CMAKE_CURRENT_LIST_DIR}/include/sdk-impl/netif/netif-udp-broadcast.h
             ${CMAKE_CURRENT_LIST_DIR}/include/helpers/file-io.h
             ${CMAKE_CURRENT_LIST_DIR}/include/helpers/event-group-bits.h
             ${CMAKE_CURRENT_LIST_DIR}/include/helpers/msg-queue.h
             ${CMAKE_CURRENT_LIST_DIR}/include/helpers/app-helpers.h
             ${CMAKE_CURRENT_LIST_DIR}/include/helpers/app-storage.h
             ${CMAKE_CURRENT_LIST_DIR}/include/helpers/file-cache.h
             ${CMAKE_CURRENT_LIST_DIR}/include/config/pc/endian-config.h
             ${CMAKE_CURRENT_LIST_DIR}/include/config/pc/global-hal.h
             ${CMAKE_CURRENT_LIST_DIR}/include/config/pc/logger-config.h
             ${CMAKE_CURRENT_LIST_DIR}/include/config/pc/stdlib-config.h
             ${CMAKE_CURRENT_LIST_DIR}/include/config/pc/trust_list-config.h
             ${CMAKE_CURRENT_LIST_DIR}/include/config/pc/update-config.h

             #	Sources
             ${CMAKE_CURRENT_LIST_DIR}/src/main.c
             ${CMAKE_CURRENT_LIST_DIR}/src/sdk-impl/netif/packets-queue.c
             ${CMAKE_CURRENT_LIST_DIR}/src/sdk-impl/netif/netif-udp-broadcast.c
             ${CMAKE_CURRENT_LIST_DIR}/src/sdk-impl/storage/storage-nix-impl.c
             ${CMAKE_CURRENT_LIST_DIR}/src/sdk-impl/logger/logger-impl.c
             ${CMAKE_CURRENT_LIST_DIR}/src/sdk-impl/threadsafe/lock-nix-hal.c
             ${CMAKE_CURRENT_LIST_DIR}/src/helpers/file-io.c
             ${CMAKE_CURRENT_LIST_DIR}/src/helpers/file-cache.c
             ${CMAKE_CURRENT_LIST_DIR}/src/helpers/event-group-bits.c
             ${CMAKE_CURRENT_LIST_DIR}/src/helpers/msg-queue.c
             ${CMAKE_CURRENT_LIST_DIR}/src/helpers/app-helpers.c
             ${CMAKE_CURRENT_LIST_DIR}/src/helpers/app-storage.c
             ${CMAKE_CURRENT_LIST_DIR}/src/sdk-impl/firmware/firmware-impl.c
             ${CMAKE_CURRENT_LIST_DIR}/src/sdk-impl/threadsafe/lock-nix-hal.c
            )

    #
    #	Definitions
    #
    target_compile_definitions(kwn-firmware-verifier
            PRIVATE
            )

    #
    #   Include directories
    #
    target_include_directories(kwn-firmware-verifier
            PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}/include
            ${CMAKE_CURRENT_LIST_DIR}/include/config/pc
            )

    #
    #	Link libraries
    #
    target_link_libraries(kwn-firmware-verifier
            PRIVATE

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
            mbedtls
            mbedcrypto
            ed25519
            )

    #
    #   Install app
    #
    install(TARGETS kwn-firmware-verifier
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            )

