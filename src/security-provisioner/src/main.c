//  ────────────────────────────────────────────────────────────
//                     ╔╗  ╔╗ ╔══╗      ╔════╗
//                     ║╚╗╔╝║ ╚╣╠╝      ║╔╗╔╗║
//                     ╚╗╚╝╔╝  ║║  ╔══╗ ╚╝║║╚╝
//                      ╚╗╔╝   ║║  ║╔╗║   ║║
//                       ║║   ╔╣╠╗ ║╚╝║   ║║
//                       ╚╝   ╚══╝ ╚══╝   ╚╝
//    ╔╗╔═╗                    ╔╗                     ╔╗
//    ║║║╔╝                   ╔╝╚╗                    ║║
//    ║╚╝╝  ╔══╗ ╔══╗ ╔══╗  ╔╗╚╗╔╝  ╔══╗ ╔╗ ╔╗╔╗ ╔══╗ ║║  ╔══╗
//    ║╔╗║  ║║═╣ ║║═╣ ║╔╗║  ╠╣ ║║   ║ ═╣ ╠╣ ║╚╝║ ║╔╗║ ║║  ║║═╣
//    ║║║╚╗ ║║═╣ ║║═╣ ║╚╝║  ║║ ║╚╗  ╠═ ║ ║║ ║║║║ ║╚╝║ ║╚╗ ║║═╣
//    ╚╝╚═╝ ╚══╝ ╚══╝ ║╔═╝  ╚╝ ╚═╝  ╚══╝ ╚╝ ╚╩╩╝ ║╔═╝ ╚═╝ ╚══╝
//                    ║║                         ║║
//                    ╚╝                         ╚╝
//
//    Lead Maintainer: Roman Kutashenko <kutashenko@gmail.com>
//  ────────────────────────────────────────────────────────────

// IoTKit modules
#include <virgil/iot/logger/logger.h> // Logger
#include <virgil/iot/macros/macros.h> // Helper macroses
#include <virgil/iot/secbox/secbox.h> // SecBox - secure storage

// Communication protocol
#include <common/protocols/snap/user/user-server.h>     // Specific command for RPi
#include <virgil/iot/protocols/snap.h>                  // Common functionality of protocol
#include <virgil/iot/protocols/snap/cfg/cfg-server.h>   // Device configuration server
#include <virgil/iot/protocols/snap/info/info-server.h> // Device information server

// Queue for packets to be processed
#include "common/iotkit-impl/packets-queue.h"

// Implementation Network interfaces
#include "common/iotkit-impl/netif/netif-udp.h" // UDP networking

// Software implementation of Security API
#include <virgil/iot/vs-soft-secmodule/vs-soft-secmodule.h>

// Command processing modules
#include "commands/device-info.h"
#include "commands/pc.h"

// Platform-specific helpers
#include "common/helpers/app-helpers.h" // Different helpers
#include "yiot-littlefs-storage.h"      // Data Storage helpers
#include <yiot-littlefs-hal.h>

// High-level wrapper to simplify initialization/deinitialization
#include "init.h"


static void
_print_title(const char *manufacturer_string,
             const char *model_string,
             const char *serial_string);

//-----------------------------------------------------------------------------
int
main(int argc, char *argv[]) {
    int res = -1;
    const char *mtd_device = NULL;
    const char *netif = NULL;
    const char *manufacturer_string = NULL;
    const char *model_string = NULL;
    const char *serial_string = NULL;

    // Holder of Network interfaces list
    vs_netif_t *netifs_impl[2] = {0};

    // Configuration server callbacks
    vs_snap_cfg_server_service_t cfg_server_cb = {NULL, NULL, NULL, NULL};

    // RPi-specific callbacks
    vs_snap_user_server_service_t user_server_cb = {ks_snap_pc_get_info_cb, // Get RPi information
                                                    ks_snap_pc_command_cb}; // Process RPi command

    // Security API implementation
    vs_secmodule_impl_t *secmodule_impl = NULL;

    // Different storages context
    vs_storage_op_ctx_t tl_storage_impl;     // TrustList storage
    vs_storage_op_ctx_t slots_storage_impl;  // Emulation of HSM's data slots
    vs_storage_op_ctx_t secbox_storage_impl; // SecBox storage

    // Device parameters
    vs_device_manufacture_id_t manufacture_id;
    vs_device_type_t device_type;
    vs_device_serial_t serial;

    // Initialize Logger module
    vs_logger_init(VS_LOGLEV_CRITICAL);

    //
    //  Parameters
    //

    // Set MTD device to be used
    mtd_device = vs_app_get_commandline_arg(argc, argv, "-m", "--mtd");
    CHECK_RET(mtd_device, -1, "MTD device is not set. Example : --mtd /dev/mtd0");
    iot_flash_set_device(mtd_device);

    // Get network interface to be used
    netif = vs_app_get_commandline_arg(argc, argv, "-i", "--interface");

    // Get manufacturer
    manufacturer_string = vs_app_get_commandline_arg(argc, argv, "-f", "--manufacturer");
    if (!manufacturer_string) {
        manufacturer_string = "YIoT-dev";
    }

    // Get model
    model_string = vs_app_get_commandline_arg(argc, argv, "-d", "--model");
    if (!model_string) {
        model_string = "TEST";
    }

    // Get serial
    serial_string = vs_app_get_commandline_arg(argc, argv, "-s", "--serial");
    if (!serial_string) {
        serial_string = "0123";
    }

    vs_app_str_to_bytes(manufacture_id, manufacturer_string, VS_DEVICE_MANUFACTURE_ID_SIZE);
    vs_app_str_to_bytes(device_type, model_string, VS_DEVICE_TYPE_SIZE);
    vs_app_str_to_bytes(serial, serial_string, sizeof(vs_device_serial_t));

    //
    //  ~Parameters
    //

    // Print title
    _print_title(manufacturer_string,
                 model_string,
                 serial_string);

    //
    // ---------- Create implementations ----------
    //

    // Network interface
    vs_packets_queue_init(vs_snap_default_processor); // Initialize Queue for incoming packets
    netifs_impl[0] = vs_hal_netif_udp(netif);              // Initialize UDP-based transport

    // TrustList storage
    STATUS_CHECK(vs_app_storage_init_impl(&tl_storage_impl, VS_APP_STORAGE_TRUST_LIST),
                 "Cannot create TrustList storage");

    // Slots storage
    STATUS_CHECK(vs_app_storage_init_impl(&slots_storage_impl, VS_APP_STORAGE_SLOTS),
                 "Cannot create TrustList storage");

    // Secbox storage
    STATUS_CHECK(vs_app_storage_init_impl(&secbox_storage_impl, VS_APP_STORAGE_SECBOX), "Cannot create Secbox storage");

    // Soft Security Module
    secmodule_impl = vs_soft_secmodule_impl(&slots_storage_impl);

    // Secbox module
    STATUS_CHECK(vs_secbox_init(&secbox_storage_impl, secmodule_impl), "Unable to initialize Secbox module");

    //
    // ---------- Initialize IoTKit internals ----------
    //

    // Initialize IoTKit
    STATUS_CHECK(ks_iotkit_init(manufacture_id, // Set device information
                                device_type,
                                serial,
                                VS_SNAP_DEV_THING,
                                netifs_impl,   // Set Network interfaces
                                cfg_server_cb, // Set protocol callbacks
                                user_server_cb,
                                vs_packets_queue_add, // Setup packets processing using queue
                                secmodule_impl,       // Security API implementation
                                &tl_storage_impl),    // TrustList storage
                 "Cannot initialize IoTKit");

    //
    // ---------- Application work ----------
    //

    // Inform about no need of WiFi credentials
    vs_snap_info_set_need_cred(false);

    // Send broadcast notification about self start
    const vs_netif_t *n = vs_snap_default_netif();
    vs_snap_user_start_notification(n);
    vs_snap_info_start_notification(n);

    // Sleep until CTRL_C
    vs_app_sleep_until_stop();

    //
    // ---------- Terminate application ----------
    //

    res = 0;

terminate:

    VS_LOG_INFO("Terminating application ...");

    // De-initialize IoTKit internals
    ks_iotkit_deinit();

    // Deinit Soft Security Module
    vs_soft_secmodule_deinit();

    // De-initialize SNAP packets queue
    vs_packets_queue_deinit();

    return res;
}

//-----------------------------------------------------------------------------
static void
_print_title(const char *manufacturer_string,
             const char *model_string,
             const char *serial_string) {
    printf("\n\n");
    printf(" ──────────────────────────────────────────────────────────\n");
    printf("                   ╔╗  ╔╗ ╔══╗      ╔════╗                 \n");
    printf("                   ║╚╗╔╝║ ╚╣╠╝      ║╔╗╔╗║                 \n");
    printf("                   ╚╗╚╝╔╝  ║║  ╔══╗ ╚╝║║╚╝                 \n");
    printf("                    ╚╗╔╝   ║║  ║╔╗║   ║║                   \n");
    printf("                     ║║   ╔╣╠╗ ║╚╝║   ║║                   \n");
    printf("                     ╚╝   ╚══╝ ╚══╝   ╚╝                   \n");
    printf("  ╔╗╔═╗                    ╔╗                     ╔╗       \n");
    printf("  ║║║╔╝                   ╔╝╚╗                    ║║       \n");
    printf("  ║╚╝╝  ╔══╗ ╔══╗ ╔══╗  ╔╗╚╗╔╝  ╔══╗ ╔╗ ╔╗╔╗ ╔══╗ ║║  ╔══╗ \n");
    printf("  ║╔╗║  ║║═╣ ║║═╣ ║╔╗║  ╠╣ ║║   ║ ═╣ ╠╣ ║╚╝║ ║╔╗║ ║║  ║║═╣ \n");
    printf("  ║║║╚╗ ║║═╣ ║║═╣ ║╚╝║  ║║ ║╚╗  ╠═ ║ ║║ ║║║║ ║╚╝║ ║╚╗ ║║═╣ \n");
    printf("  ╚╝╚═╝ ╚══╝ ╚══╝ ║╔═╝  ╚╝ ╚═╝  ╚══╝ ╚╝ ╚╩╩╝ ║╔═╝ ╚═╝ ╚══╝ \n");
    printf("                  ║║                         ║║            \n");
    printf("                  ╚╝                         ╚╝            \n");
    printf("                                                           \n");
    printf("                     YIoT provision tool                   \n");
    printf(" ──────────────────────────────────────────────────────────\n");
    printf("  Manufacture ID = \"%s\"\n", manufacturer_string);
    printf("  Device type    = \"%s\"\n", model_string);
    printf("  Device serial  = \"%s\"\n", serial_string);
    printf(" ──────────────────────────────────────────────────────────\n\n");
}

//-----------------------------------------------------------------------------
