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

#include <unistd.h>

#include <common/helpers/app-helpers.h>
#include <yiot-littlefs-storage.h>
#include <yiot-littlefs-hal.h>

#include <virgil/iot/logger/logger.h>
#include <virgil/iot/macros/macros.h>
#include <virgil/iot/provision/license.h>
#include <virgil/iot/base64/base64.h>
#include <virgil/iot/firmware/firmware.h>
#include <virgil/iot/secmodule/secmodule.h>
#include <virgil/iot/vs-soft-secmodule/vs-soft-secmodule.h>

// Device parameters
static vs_secmodule_impl_t *_secmodule_impl = NULL;

// Maximum size of a command
#define YIOT_COMMAND_SZ_MAX (64)

// Maximum size of a license
#define YIOT_LICENSE_SZ_MAX (10 * 1024)

typedef enum {
    YIOT_LIC_CMD_UNKNOWN,
    YIOT_LIC_CMD_GET,
    YIOT_LIC_CMD_SAVE,
    YIOT_LIC_CMD_VERIFY
} yiot_command_t;

// Get command to work with licenses
static yiot_command_t
_get_command(int argc, char *argv[], uint8_t *param_buf, uint16_t buf_sz, uint16_t *param_sz);

// Process 'Get' command
static vs_status_e
_cmd_get(void);

// Process 'Save' command
static vs_status_e
_cmd_save(const uint8_t *license, uint16_t license_sz);

// Process 'Verify' command
static vs_status_e
_cmd_verify(const uint8_t *license, uint16_t license_sz);

// ----------------------------------------------------------------------------
int
main(int argc, char *argv[]) {
    int res = -1;
    uint8_t license[YIOT_LICENSE_SZ_MAX];
    uint16_t license_sz = 0;
    vs_provision_events_t provision_events = {NULL};
    const char *mtd_device = NULL;
    yiot_command_t command;

    // Implementation variables
    vs_storage_op_ctx_t tl_storage_impl;
    vs_storage_op_ctx_t slots_storage_impl;

    // Initialize Logger module
    vs_logger_init(VS_LOGLEV_INFO);

    // Get command
    command = _get_command(argc, argv, license, YIOT_LICENSE_SZ_MAX, license_sz);
    CHECK_RET(command != YIOT_LIC_CMD_UNKNOWN, -3, "Cannot get command");

    // Set MTD device to be used
    mtd_device = vs_app_get_commandline_arg(argc, argv, "-m", "--mtd");
    CHECK_RET(mtd_device, -1, "MTD device is not set. Example : --mtd /dev/mtd0");
    CHECK_RET(0 == iot_flash_set_device(mtd_device), -2, "Cannot set MTD device name");

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

    // Initialize Licenses module
    STATUS_CHECK(vs_license_init(_secmodule_impl),
                 "Cannot initialize license module");

    //
    // ---------- Process commands ----------
    //
    switch (command) {
    case YIOT_LIC_CMD_GET: {
        res = VS_CODE_OK == _cmd_get();
        break ;
    }
    case YIOT_LIC_CMD_SAVE: {
        res = VS_CODE_OK == _cmd_save(license, license_sz);
        break ;
    }
    case YIOT_LIC_CMD_VERIFY: {
        res = VS_CODE_OK == _cmd_verify(license, license_sz);
        break ;
    }
    }

terminate:

    // Deinit provision
    vs_provision_deinit();

    // Deinit Soft Security Module
    vs_soft_secmodule_deinit();

    return res;
}

// ----------------------------------------------------------------------------
static yiot_command_t
_get_command(int argc, char *argv[], uint8_t *param_buf, uint16_t buf_sz, uint16_t *param_sz) {
    yiot_command_t res = YIOT_LIC_CMD_UNKNOWN;
    const char *license_base64 = NULL;

    // Check for 'Save' command
    license_base64 = vs_app_get_commandline_arg(argc, argv, "-s", "--save");
    if (license_base64 != NULL) {
        res = YIOT_LIC_CMD_SAVE;
    }

    // Check for 'Verify' command
    license_base64 = vs_app_get_commandline_arg(argc, argv, "-v", "--verify");
    if (license_base64 != NULL) {
        res = YIOT_LIC_CMD_VERIFY;
    }

    // 'Get' command if not other
    if (res == YIOT_LIC_CMD_UNKNOWN) {
        return YIOT_LIC_CMD_GET;
    }

    // Decode base64 license param
    *param_sz = buf_sz;
    CHECK_RET(base64decode(license_base64,
                       (int)VS_IOT_STRLEN(license_base64),
                           param_buf,
                           param_sz),
              YIOT_LIC_CMD_UNKNOWN,
          "Cannot decode base64 license");

    return res;
}

// ----------------------------------------------------------------------------
static vs_status_e
_cmd_get(void) {
    vs_status_e res = VS_CODE_ERR_NOT_FOUND;
    uint8_t license[YIOT_LICENSE_SZ_MAX];
    uint16_t license_sz = 0;
    uint8_t license_data[YIOT_LICENSE_SZ_MAX];
    uint16_t license_data_sz = 0;

    // Load license
    if (VS_CODE_OK != vs_license_get(license, YIOT_LICENSE_SZ_MAX, &license_sz)) {
        VS_LOG_ERROR("Cannot load a license");
        return res;
    }

    // Get plain data of a license
    if (VS_CODE_OK != vs_license_plain_data(license, license_sz,
                                            license_data, YIOT_LICENSE_SZ_MAX, &license_data_sz)) {
        VS_LOG_ERROR("Cannot get plain data of a license");
        return res;
    }

    // Print plain data
    VS_LOG_INFO("Stored License data: %s\n", (const char *)license_data);

    return VS_CODE_OK;
}

// ----------------------------------------------------------------------------
static vs_status_e
_cmd_save(const uint8_t *license, uint16_t license_sz) {
    vs_status_e res = VS_CODE_ERR_VERIFY;
    uint8_t license_loaded[YIOT_LICENSE_SZ_MAX];
    uint16_t license_loaded_sz = 0;
    uint8_t license_data[YIOT_LICENSE_SZ_MAX];
    uint16_t license_data_sz = 0;

    // Verify a license
    if (VS_CODE_OK != vs_license_verify(license, license_sz)) {
        VS_LOG_ERROR("Cannot verify a license");
        return res;
    }

    // Save a license
    if (VS_CODE_OK != vs_license_save(license, license_sz)) {
        VS_LOG_ERROR("Cannot save a license");
        return res;
    }

    // Load license
    if (VS_CODE_OK != vs_license_get(license_loaded, YIOT_LICENSE_SZ_MAX, &license_loaded_sz)) {
        VS_LOG_ERROR("Cannot load a license");
        return res;
    }

    // Get plain data of a license
    if (VS_CODE_OK != vs_license_plain_data(license_loaded, license_loaded_sz,
                                            license_data, YIOT_LICENSE_SZ_MAX, &license_data_sz)) {
        VS_LOG_ERROR("Cannot get plain data of a license");
        return res;
    }

    // Print plain data
    VS_LOG_INFO("Stored License data: %s\n", (const char *)license_data);

    return VS_CODE_OK;
}

// ----------------------------------------------------------------------------
static vs_status_e
_cmd_verify(const uint8_t *license, uint16_t license_sz) {
    vs_status_e res = VS_CODE_ERR_VERIFY;
    uint8_t license_data[YIOT_LICENSE_SZ_MAX];
    uint16_t license_data_sz = 0;

    // Verify a license
    if (VS_CODE_OK != vs_license_verify(license, license_sz)) {
        VS_LOG_ERROR("Cannot verify a license");
        return res;
    }

    // Get plain data of a license
    if (VS_CODE_OK != vs_license_plain_data(license, license_sz,
                                            license_data, YIOT_LICENSE_SZ_MAX, &license_data_sz)) {
        VS_LOG_ERROR("Cannot get plain data of a license");
        return res;
    }

    // Print plain data
    VS_LOG_INFO("Verified License data: %s\n", (const char *)license_data);

    return VS_CODE_OK;
}

// ----------------------------------------------------------------------------
