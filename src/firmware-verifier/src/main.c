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

#include <virgil/iot/base64/base64.h>
#include <virgil/iot/logger/logger.h>
#include <virgil/iot/macros/macros.h>
#include <virgil/iot/firmware/firmware.h>
#include <virgil/iot/secmodule/secmodule.h>
#include <virgil/iot/secmodule/secmodule-helpers.h>
#include <virgil/iot/vs-soft-secmodule/vs-soft-secmodule.h>

// Device parameters
static vs_secmodule_impl_t *_secmodule_impl = NULL;

#define PUBKEY_SZ_MAX (1024)
#define HASH_SZ_MAX (1024)
#define SIGN_SZ_MAX (2 * 1024)
#define PARAM_SZ_MAX (4 * 1024)
#define CHUNK_SZ (10 * 1024)

typedef enum {
    YIOT_VERIFY_UNKNOWN,
    YIOT_VERIFY_DATA,
    YIOT_VERIFY_FILE,
} yiot_command_t;

// ----------------------------------------------------------------------------
static ssize_t
_get_file_size(const char *firmware_file) {
    FILE *fp = NULL;
    ssize_t res = -1;

    CHECK_NOT_ZERO_RET(firmware_file, VS_CODE_ERR_FILE_READ);

    fp = fopen(firmware_file, "rb");

    if (fp) {

        CHECK(0 == fseek(fp, 0, SEEK_END), "fseek error errno = %d (%s)", errno, strerror(errno));

        res = ftell(fp);

        if (res <= 0) {
            VS_LOG_ERROR("Unable to prepare file %s to read. errno = %d (%s)", firmware_file, errno, strerror(errno));
            res = -1;
            goto terminate;
        }
    } else {
        VS_LOG_WARNING("Unable to open file %s. errno = %d (%s)", firmware_file, errno, strerror(errno));
    }

terminate:
    if (fp) {
        fclose(fp);
    }
    return res;
}

// ----------------------------------------------------------------------------
static vs_status_e
_load_file_chunk(const char *firmware_file, uint32_t offset, uint8_t *data, size_t buf_sz, size_t *read_sz) {
    FILE *fp = NULL;
    int64_t max_avail_sz;
    vs_status_e res = VS_CODE_ERR_FILE_READ;

    CHECK_NOT_ZERO_RET(firmware_file, VS_CODE_ERR_FILE_READ);

    fp = fopen(firmware_file, "rb");

    if (fp) {
        CHECK(0 == fseek(fp, offset, SEEK_END), "fseek error errno = %d (%s)", errno, strerror(errno));

        max_avail_sz = ftell(fp) - offset;

        if (max_avail_sz < 0) {
            VS_LOG_ERROR("File %s is smaller than offset %u", buf_sz, firmware_file, offset);
            *read_sz = 0;
            goto terminate;
        }

        CHECK(0 == fseek(fp, offset, SEEK_SET), "fseek error errno = %d (%s)", errno, strerror(errno));

        *read_sz = max_avail_sz < buf_sz ? max_avail_sz : buf_sz;

        VS_LOG_DEBUG("Read file '%s', %d bytes", firmware_file, (int)*read_sz);

        if (1 == fread((void *)data, *read_sz, 1, fp)) {
            res = VS_CODE_OK;
        } else {
            VS_LOG_ERROR("Unable to read %d bytes from %s", *read_sz, firmware_file);
            *read_sz = 0;
        }

    } else {
        VS_LOG_ERROR("Unable to open file %s. errno = %d (%s)", firmware_file, errno, strerror(errno));
    }

terminate:

    if (fp) {
        fclose(fp);
    }

    return res;
}

// ----------------------------------------------------------------------------
static vs_status_e
_file_hash(vs_secmodule_impl_t *secmodule, uint8_t *name, uint8_t *hash, size_t hash_buf_sz, uint16_t *hash_sz) {
    ssize_t file_sz;
    vs_secmodule_sw_sha256_ctx hash_ctx;
    vs_status_e ret_code;
    size_t read_sz;
    uint8_t buf[CHUNK_SZ];
    uint32_t offset = 0;

    // Check input parameters
    CHECK_NOT_ZERO_RET(secmodule, VS_CODE_ERR_CRYPTO);
    CHECK_NOT_ZERO_RET(secmodule->hash_init, VS_CODE_ERR_CRYPTO);
    CHECK_NOT_ZERO_RET(secmodule->hash_update, VS_CODE_ERR_CRYPTO);
    CHECK_NOT_ZERO_RET(secmodule->hash_finish, VS_CODE_ERR_CRYPTO);
    CHECK_NOT_ZERO_RET(name, VS_CODE_ERR_CRYPTO);
    CHECK_NOT_ZERO_RET(hash, VS_CODE_ERR_CRYPTO);
    CHECK_NOT_ZERO_RET(hash_sz, VS_CODE_ERR_CRYPTO);

    // Get file size
    file_sz = _get_file_size(name);
    CHECK_RET(file_sz > 0, VS_CODE_ERR_FILE, "Empty firmware file");

    // Check hash buffer size
    *hash_sz = vs_secmodule_get_hash_len(VS_HASH_SHA_256);
    CHECK_RET(hash_buf_sz >= *hash_sz, VS_CODE_ERR_TOO_SMALL_BUFFER, "Hash buffer is not enough");

    // Initialize hash context
    secmodule->hash_init(&hash_ctx);

    // Calculate file hash
    while (offset < file_sz) {
        uint32_t fw_rest = file_sz - offset;
        uint32_t required_chunk_size = fw_rest > CHUNK_SZ ? CHUNK_SZ : fw_rest;

        STATUS_CHECK_RET(_load_file_chunk(name, offset, buf, required_chunk_size, &read_sz),
                         "Error read firmware chunk");

        _secmodule_impl->hash_update(&hash_ctx, buf, required_chunk_size);
        offset += required_chunk_size;
    }

    // Finalize hash
    _secmodule_impl->hash_finish(&hash_ctx, hash);

    return VS_CODE_OK;
}

// ----------------------------------------------------------------------------
static vs_status_e
_data_hash(vs_secmodule_impl_t *secmodule,
           uint8_t *data,
           uint16_t data_sz,
           uint8_t *hash,
           size_t hash_buf_sz,
           uint16_t *hash_sz) {
    // Check input parameters
    CHECK_NOT_ZERO_RET(secmodule, VS_CODE_ERR_CRYPTO);
    CHECK_NOT_ZERO_RET(secmodule->hash, VS_CODE_ERR_CRYPTO);
    CHECK_NOT_ZERO_RET(data, VS_CODE_ERR_CRYPTO);
    CHECK_NOT_ZERO_RET(data_sz, VS_CODE_ERR_CRYPTO);
    CHECK_NOT_ZERO_RET(hash, VS_CODE_ERR_CRYPTO);
    CHECK_NOT_ZERO_RET(hash_sz, VS_CODE_ERR_CRYPTO);

    // Hash data
    return secmodule->hash(VS_HASH_SHA_256, data, data_sz, hash, hash_buf_sz, hash_sz);
}

// ----------------------------------------------------------------------------
static vs_status_e
_verify_hash(vs_secmodule_impl_t *secmodule,
             const uint8_t *hash,
             uint16_t hash_sz,
             const uint8_t *sign,
             uint16_t sign_sz) {
    vs_status_e ret_code;
    uint16_t slot;
    uint8_t fw_key[PUBKEY_SZ_MAX];
    uint16_t fw_key_sz;
    vs_pubkey_dated_t *ref_key = (vs_pubkey_dated_t *)fw_key;
    uint8_t *pubkey;
    uint16_t pubkey_sz;
    int i;
    uint8_t raw_sign[VS_SIGNATURE_MAX_LEN];
    uint16_t raw_sign_sz;

    // Check input parameters
    CHECK_NOT_ZERO_RET(secmodule, VS_CODE_ERR_CRYPTO);
    CHECK_NOT_ZERO_RET(secmodule->slot_load, VS_CODE_ERR_CRYPTO);
    CHECK_NOT_ZERO_RET(secmodule->ecdsa_verify, VS_CODE_ERR_CRYPTO);
    CHECK_NOT_ZERO_RET(hash, VS_CODE_ERR_CRYPTO);
    CHECK_NOT_ZERO_RET(hash_sz, VS_CODE_ERR_CRYPTO);

    // Prepare Raw signature
    CHECK_RET(VS_CODE_OK == vs_secmodule_virgil_secp256_signature_to_tiny(
                                    (uint8_t *)sign, sign_sz, raw_sign, sizeof(raw_sign)),
              VS_CODE_ERR_CRYPTO,
              "Wrong signature format");
    raw_sign_sz = vs_secmodule_get_signature_len(VS_KEYPAIR_EC_SECP256R1);


    // Try to verify using all Firmware keys
    for (i = 0; i < 2; i++) {
        // Get slot ID
        vs_provision_element_id_e id = (0 == i) ? VS_PROVISION_PBF1 : VS_PROVISION_PBF2;

        // Load Firmware Key
        STATUS_CHECK_RET(vs_provision_get_slot_num(id, &slot), "Cannot get Firmware Key slot");
        STATUS_CHECK_RET(secmodule->slot_load(slot, fw_key, PUBKEY_SZ_MAX, &fw_key_sz), "Cannot load Firmware Key 1");

        // Get Raw key size
        pubkey_sz = vs_secmodule_get_pubkey_len(ref_key->pubkey.ec_type);
        if (pubkey_sz < 0) {
            return VS_CODE_ERR_INCORRECT_PARAMETER;
        }

        // Get Raw key
        pubkey = &ref_key->pubkey.meta_and_pubkey[ref_key->pubkey.meta_data_sz];

        // Verification attempt
        ret_code = _secmodule_impl->ecdsa_verify(
                ref_key->pubkey.ec_type, pubkey, pubkey_sz, VS_HASH_SHA_256, hash, raw_sign, raw_sign_sz);

        // Check verification result
        if (VS_CODE_OK == ret_code) {
            return VS_CODE_OK;
        }
    }

    return VS_CODE_ERR_VERIFY;
}

// ----------------------------------------------------------------------------
static yiot_command_t
_get_params(int argc,
            char *argv[],
            uint8_t *sign_buf,
            uint16_t sign_buf_sz,
            uint16_t *sign_sz,
            uint8_t *param_buf,
            uint16_t buf_sz,
            uint16_t *param_sz) {
    yiot_command_t res = YIOT_VERIFY_UNKNOWN;
    const char *param = NULL;
    int sz;

    // Clean result
    VS_IOT_MEMSET(param_buf, 0, buf_sz);
    *param_sz = 0;

    // Get 'Signature' parameter
    param = vs_app_get_commandline_arg(argc, argv, "-s", "--signature");
    if (param == NULL) {
        VS_LOG_ERROR("There is no signature parameter : -s|--signature <base64>");
        return false;
    }

    // Decode base64 signature param
    sz = sign_buf_sz;
    CHECK_RET(base64decode(param, (int)VS_IOT_STRLEN(param), sign_buf, &sz),
              YIOT_VERIFY_UNKNOWN,
              "Cannot decode base64 signature");
    *sign_sz = sz;

    // Check for 'File' command
    if (YIOT_VERIFY_UNKNOWN == res) {
        param = vs_app_get_commandline_arg(argc, argv, "-f", "--file");
        if (param != NULL) {
            res = YIOT_VERIFY_FILE;
        }
    }

    // Check for 'Data' command
    if (YIOT_VERIFY_UNKNOWN == res) {
        param = vs_app_get_commandline_arg(argc, argv, "-d", "--data");
        if (param != NULL) {
            res = YIOT_VERIFY_DATA;
        }
    }

    // Copy data
    if (res != YIOT_VERIFY_UNKNOWN) {
        VS_IOT_STRCPY(param_buf, param);
        *param_sz = VS_IOT_STRLEN(param);
    }

    return res;
}

// ----------------------------------------------------------------------------
int
main(int argc, char *argv[]) {
    bool verified = false;
    // Signature buffer
    uint8_t sign[SIGN_SZ_MAX];
    uint16_t sign_sz = 0;

    // Parameter buffer
    uint8_t param[PARAM_SZ_MAX];
    uint16_t param_sz = 0;

    // Hash buffer
    uint8_t hash[HASH_SZ_MAX];
    uint16_t hash_sz = 0;

    vs_provision_events_t provision_events = {NULL};
    const char *mtd_device = NULL;
    yiot_command_t command;

    // Implementation variables
    vs_storage_op_ctx_t tl_storage_impl;
    vs_storage_op_ctx_t slots_storage_impl;

    // Initialize Logger module
    if (vs_app_is_debug(argc, argv)) {
        vs_logger_init(VS_LOGLEV_DEBUG);
    } else {
        vs_logger_init(VS_LOGLEV_CRITICAL);
    }

    // Get command
    command = _get_params(argc, argv, sign, SIGN_SZ_MAX, &sign_sz, param, PARAM_SZ_MAX, &param_sz);
    CHECK_RET(command != YIOT_VERIFY_UNKNOWN, -3, "Cannot get command");

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

    //
    // -------------- Get data hash -------------
    //
    if (command == YIOT_VERIFY_FILE) {
        STATUS_CHECK(_file_hash(_secmodule_impl, param, hash, HASH_SZ_MAX, &hash_sz), "Cannot get file hash");
    } else if (command == YIOT_VERIFY_DATA) {
        STATUS_CHECK(_data_hash(_secmodule_impl, param, param_sz, hash, HASH_SZ_MAX, &hash_sz), "Cannot get data hash");
    } else {
        BOOL_CHECK(false, "Unknown command");
    }

    //
    // ---------- Check firmware image ----------
    //
    STATUS_CHECK(_verify_hash(_secmodule_impl, hash, hash_sz, sign, sign_sz), "Verifying firmware data fail");

    verified = true;

terminate:

    if (verified) {
        VS_LOG_INFO("Signature is correct");
    } else {
        VS_LOG_FATAL("Signature is WRONG");
    }

    // Deinit provision
    vs_provision_deinit();

    // Deinit Soft Security Module
    vs_soft_secmodule_deinit();

    return verified ? 0 : 1;
}

// ----------------------------------------------------------------------------
