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

#include <errno.h>
#include <unistd.h>

#include <common/helpers/app-helpers.h>
#include <yiot-littlefs-storage.h>
#include <yiot-littlefs-hal.h>

#include <virgil/iot/logger/logger.h>
#include <virgil/iot/macros/macros.h>
#include <virgil/iot/firmware/firmware.h>
#include <virgil/iot/secmodule/secmodule.h>
#include <virgil/iot/secmodule/secmodule-helpers.h>
#include <virgil/iot/vs-soft-secmodule/vs-soft-secmodule.h>

#include <trust_list-config.h>
#include <update-config.h>

// Device parameters
static vs_device_manufacture_id_t _manufacture_id;
static vs_device_type_t _device_type;
static const vs_key_type_e sign_rules_list[VS_FW_SIGNATURES_QTY] = VS_FW_SIGNER_TYPE_LIST;
static char *_path_to_image = NULL;
static vs_secmodule_impl_t *_secmodule_impl = NULL;

// Maximum size of a license
#define YIOT_LICENSE_SZ_MAX (10 * 1024)

// ----------------------------------------------------------------------------
int
main(int argc, char *argv[]) {
    uint8_t license[YIOT_LICENSE_SZ_MAX];
    uint8_t license_data[YIOT_LICENSE_SZ_MAX];
    uint16_t license_sz = 0;
    uint16_t license_data_sz = 0;
    vs_provision_events_t provision_events = {NULL};

    // Implementation variables
    vs_storage_op_ctx_t tl_storage_impl;
    vs_storage_op_ctx_t slots_storage_impl;

    // Initialize Logger module
    vs_logger_init(VS_LOGLEV_INFO);

    const char *MANUFACTURE_ID = "YIoT-dev";
    const char *DEVICE_MODEL = "TEST";

    // Prepare local storage
    // Set MTD device
    if (argc > 1) {
        iot_flash_set_device(argv[1]);
    } else {
        iot_flash_set_device("/dev/mtd0");
    }

    vs_app_str_to_bytes(_manufacture_id, MANUFACTURE_ID, VS_DEVICE_MANUFACTURE_ID_SIZE);
    vs_app_str_to_bytes(_device_type, DEVICE_MODEL, VS_DEVICE_TYPE_SIZE);

    //
    // ---------- Create implementations ----------
    //

    // TrustList storage
    STATUS_CHECK(vs_app_storage_init_impl(&tl_storage_impl, VS_APP_STORAGE_TRUST_LIST),
                 "Cannot create TrustList storage");

    // Slots storage
    STATUS_CHECK(vs_app_storage_init_impl(&slots_storage_impl, VS_APP_STORAGE_SLOTS),
                 "Cannot create TrustList storage");

    // Soft Security Module
    _secmodule_impl = vs_soft_secmodule_impl(&slots_storage_impl);

    //
    // ---------- Initialize YIoT SDK modules ----------
    //

    // Provision module
    STATUS_CHECK(vs_provision_init(&tl_storage_impl, _secmodule_impl, provision_events),
                 "Cannot initialize Provision module");

    //
    // ---------- Get license ----------
    //
    STATUS_CHECK(vs_license_init(_secmodule_impl),
                 "Cannot initialize license module");
    STATUS_CHECK(vs_license_get(license, YIOT_LICENSE_SZ_MAX, &license_sz),
                 "Cannot load a license");
    STATUS_CHECK(vs_license_plain_data(license, license_sz,
                                       license_data, YIOT_LICENSE_SZ_MAX, &license_data_sz), "Cannot get plain data of a license");
    VS_LOG_INFO("License: %s\n", (const char *)license_data);

terminate:

    // Deinit provision
    vs_provision_deinit();

    // Deinit Soft Security Module
    vs_soft_secmodule_deinit();
}

// ----------------------------------------------------------------------------
