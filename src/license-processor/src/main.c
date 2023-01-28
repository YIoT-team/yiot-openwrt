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

// Current License data
static uint8_t cur_license[YIOT_LICENSE_SZ_MAX];
static uint16_t cur_license_sz = 0;
static uint8_t cur_license_data[YIOT_LICENSE_SZ_MAX];
static uint16_t cur_license_data_sz = 0;
static uint64_t cur_timestamp = 0;
static vs_device_manufacture_id_t cur_manufacturer = {0};
static vs_device_type_t cur_model = {0};
static vs_device_serial_t cur_serial = {0};
static uint32_t cur_roles = 0;
static vs_mac_addr_t cur_mac;
static uint8_t cur_key_type = 0;
static uint8_t cur_ec_type = 0;
static char cur_pubkey[VS_LICENSE_KEY_MAX_SZ];
static uint16_t cur_pubkey_sz = 0;
static char cur_sign[VS_LICENSE_SIGN_MAX_SZ];
static uint16_t cur_sign_sz = 0;
static char cur_extra_data[VS_LICENSE_DATA_MAX_SZ];
static uint16_t cur_extra_data_sz = 0;

// Get command to work with licenses
static yiot_command_t
_get_command(int argc, char *argv[], uint8_t *param_buf, uint16_t buf_sz, uint16_t *param_sz);

// Process 'Get' command
static vs_status_e
_cmd_get(void);

// Process 'Save' command
static vs_status_e
_cmd_save(uint8_t *license, uint16_t license_sz);

// Process 'Verify' command
static vs_status_e
_cmd_verify(uint8_t *license, uint16_t license_sz);

// Helper to print current license data
static void
_current_license_print(const char *prefix);

// Helper to load current license
static vs_status_e
_load_license(void);

// ----------------------------------------------------------------------------
int
main(int argc, char *argv[]) {
    int res;
    uint8_t license[YIOT_LICENSE_SZ_MAX];
    uint16_t license_sz = 0;
    vs_provision_events_t provision_events = {NULL};
    const char *mtd_device = NULL;
    yiot_command_t command;

    // Implementation variables
    vs_storage_op_ctx_t tl_storage_impl;
    vs_storage_op_ctx_t slots_storage_impl;

    // Initialize Logger module
    vs_logger_init(VS_LOGLEV_ERROR);

    // Get command
    command = _get_command(argc, argv, license, YIOT_LICENSE_SZ_MAX, &license_sz);
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

    STATUS_CHECK(_load_license(), "Cannot load current license");

    switch (command) {
    case YIOT_LIC_CMD_GET: {
        res = (VS_CODE_OK == _cmd_get()) ? 0 : 1;
        break ;
    }
    case YIOT_LIC_CMD_SAVE: {
        res = (VS_CODE_OK == _cmd_save(license, license_sz)) ? 0 : 1;
        break ;
    }
    case YIOT_LIC_CMD_VERIFY: {
        res = (VS_CODE_OK == _cmd_verify(license, license_sz)) ? 0 : 1;
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
    int sz;

    // Clean result
    *param_sz = 0;

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
    sz = buf_sz;
    CHECK_RET(base64decode(license_base64,
                       (int)VS_IOT_STRLEN(license_base64),
                           param_buf,
                           &sz),
              YIOT_LIC_CMD_UNKNOWN,
          "Cannot decode base64 license");

    *param_sz = sz;

    return res;
}

// ----------------------------------------------------------------------------
static void
_current_license_print(const char *prefix) {
    char tmp[sizeof(vs_device_manufacture_id_t) + 1];
    // Print plain data
    VS_LOG_INFO("%s:", prefix);
    VS_LOG_INFO("Data            : %s", (const char *)cur_license_data);

    // Print timestamp
    VS_LOG_INFO("Timestamp       : %llu", (unsigned long long )cur_timestamp);

    // Print manufacturer
    VS_IOT_MEMSET(tmp, 0, sizeof(tmp));
    VS_IOT_MEMCPY(tmp, cur_manufacturer, sizeof(cur_manufacturer));
    VS_LOG_INFO("Manufacturer    : %s", tmp);

    // Print model
    VS_IOT_MEMSET(tmp, 0, sizeof(tmp));
    VS_IOT_MEMCPY(tmp, cur_model, sizeof(cur_model));
    VS_LOG_INFO("Model           : %s", tmp);

    // Print serial
    VS_LOG_HEX(VS_LOGLEV_INFO, "Serial          : ", cur_serial, sizeof(cur_serial));

    // Print MAC address
    VS_LOG_INFO("MAC address     : %02x:%02x:%02x:%02x:%02x:%02x",
                cur_mac.bytes[0], cur_mac.bytes[1], cur_mac.bytes[2],
                cur_mac.bytes[3], cur_mac.bytes[4], cur_mac.bytes[5]);

    // Print RAW public key
    VS_LOG_HEX(VS_LOGLEV_INFO, "RAW public key  : ", cur_pubkey, cur_pubkey_sz);

    // Print signature
    VS_LOG_HEX(VS_LOGLEV_INFO, "Device sign     : ", cur_sign, cur_sign_sz);

    // Print key type
    VS_LOG_INFO("Key type        : %02x", cur_key_type);

    // Print EC type
    VS_LOG_INFO("EC type         : %02x", cur_ec_type);

    // Print extra data
    VS_LOG_INFO("Extra data      : %s", cur_extra_data);
}

// ----------------------------------------------------------------------------
static vs_status_e
_load_license(void) {
    vs_status_e res = VS_CODE_ERR_NOT_FOUND;

    // Load license
    if (VS_CODE_OK != vs_license_get(cur_license, YIOT_LICENSE_SZ_MAX, &cur_license_sz)) {
        VS_LOG_ERROR("Cannot load a license");
        return res;
    }

    // Get plain data of a license
    if (VS_CODE_OK != vs_license_plain_data(cur_license, cur_license_sz,
                                            cur_license_data, YIOT_LICENSE_SZ_MAX, &cur_license_data_sz)) {
        VS_LOG_ERROR("Cannot get plain data of a license");
        return res;
    }

    // Parse plain data
    if (VS_CODE_OK != vs_license_plain_data_parse(cur_license_data, cur_license_data_sz,
                                                  &cur_timestamp,
                                                  cur_manufacturer,
                                                  cur_model,
                                                  cur_serial,
                                                  &cur_roles,
                                                  &cur_mac,
                                                  &cur_key_type,
                                                  &cur_ec_type,
                                                  cur_pubkey, VS_LICENSE_KEY_MAX_SZ, &cur_pubkey_sz,
                                                  cur_sign, VS_LICENSE_SIGN_MAX_SZ, &cur_sign_sz,
                                                  cur_extra_data, VS_LICENSE_DATA_MAX_SZ, &cur_extra_data_sz)) {
        VS_LOG_ERROR("Cannot parse plain data of a license");
        return res;
    }

    return VS_CODE_OK;
}

// ----------------------------------------------------------------------------
static vs_status_e
_cmd_get(void) {
    _current_license_print("Stored license");
    return VS_CODE_OK;
}

// ----------------------------------------------------------------------------
static vs_status_e
_cmd_save(uint8_t *license, uint16_t license_sz) {
    vs_status_e res = VS_CODE_ERR_VERIFY;

    // Check if license for current device
    if (VS_CODE_OK != vs_license_matches(license, license_sz,
                                         cur_pubkey, cur_pubkey_sz,
                                         cur_mac,
                                         cur_serial,
                                         cur_manufacturer,
                                         cur_model)) {
        VS_LOG_ERROR("License doesn't belong to current device");
        return res;
    }

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
    if (VS_CODE_OK != _load_license()) {
        VS_LOG_ERROR("Cannot load a license");
        return res;
    }

    // Print license data
    _current_license_print("Stored license");

    return VS_CODE_OK;
}

// ----------------------------------------------------------------------------
static vs_status_e
_cmd_verify(uint8_t *license, uint16_t license_sz) {
    vs_status_e res = VS_CODE_ERR_VERIFY;

    // Check if license for current device
    if (VS_CODE_OK != vs_license_matches(license, license_sz,
                                         cur_pubkey, cur_pubkey_sz,
                                         cur_mac,
                                         cur_serial,
                                         cur_manufacturer,
                                         cur_model)) {
        VS_LOG_ERROR("License doesn't belong to current device");
        return res;
    }

    // Verify a license
    if (VS_CODE_OK != vs_license_verify(license, license_sz)) {
        VS_LOG_ERROR("Cannot verify a license");
        return res;
    }

    // Get plain data of a license
    if (VS_CODE_OK != vs_license_plain_data(license, license_sz,
                                            cur_license_data, YIOT_LICENSE_SZ_MAX, &cur_license_data_sz)) {
        VS_LOG_ERROR("Cannot get plain data of a license");
        return res;
    }

    // Parse plain data
    if (VS_CODE_OK != vs_license_plain_data_parse(cur_license_data, cur_license_data_sz,
                                                  &cur_timestamp,
                                                  cur_manufacturer,
                                                  cur_model,
                                                  cur_serial,
                                                  &cur_roles,
                                                  &cur_mac,
                                                  &cur_key_type,
                                                  &cur_ec_type,
                                                  cur_pubkey, VS_LICENSE_KEY_MAX_SZ, &cur_pubkey_sz,
                                                  cur_sign, VS_LICENSE_SIGN_MAX_SZ, &cur_sign_sz,
                                                  cur_extra_data, VS_LICENSE_DATA_MAX_SZ, &cur_extra_data_sz)) {
        VS_LOG_ERROR("Cannot parse plain data of a license");
        return res;
    }

    // Print license data
    _current_license_print("Verified license");

    return VS_CODE_OK;
}

// ----------------------------------------------------------------------------
