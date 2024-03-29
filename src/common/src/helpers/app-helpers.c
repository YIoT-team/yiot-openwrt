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

#if !defined(WIN32)

#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <pthread.h>
#include <virgil/iot/macros/macros.h>
#include <virgil/iot/status_code/status_code.h>
#include <virgil/iot/protocols/snap/snap-structs.h>

static pthread_mutex_t _sleep_lock;
static bool _need_restart = false;

/******************************************************************************/
bool
vs_app_is_debug(int argc, char *argv[]) {
    size_t pos;

    if (!argv) {
        return false;
    }

    for (pos = 0; pos < argc; ++pos) {
        if (!strcmp(argv[pos], "--debug"))
            return true;
    }

    return false;
}

/******************************************************************************/
char *
vs_app_get_commandline_arg(int argc, char *argv[], const char *shortname, const char *longname) {
    size_t pos;

    if (!(argv && shortname && *shortname && longname && *longname)) {
        return NULL;
    }

    for (pos = 0; pos < argc; ++pos) {
        if (!strcmp(argv[pos], shortname) && (pos + 1) < argc)
            return argv[pos + 1];
        if (!strcmp(argv[pos], longname) && (pos + 1) < argc)
            return argv[pos + 1];
    }

    return NULL;
}

/******************************************************************************/
static bool
_read_mac_address(const char *arg, vs_mac_addr_t *mac) {
    unsigned int values[6];
    int i;

    if (6 ==
        sscanf(arg, "%x:%x:%x:%x:%x:%x%*c", &values[0], &values[1], &values[2], &values[3], &values[4], &values[5])) {
        /* convert to uint8_t */
        for (i = 0; i < 6; ++i) {
            mac->bytes[i] = (uint8_t)values[i];
        }
        return true;
    }

    return false;
}

/******************************************************************************/
vs_status_e
vs_app_get_mac_from_commandline_params(int argc, char *argv[], vs_mac_addr_t *forced_mac_addr) {
    static const char *MAC_SHORT = "-m";
    static const char *MAC_FULL = "--mac";
    char *mac_str;

    if (!argv || !argc || !forced_mac_addr) {
        VS_LOG_ERROR("Wrong input parameters.");
        return VS_CODE_ERR_INCORRECT_ARGUMENT;
    }

    mac_str = vs_app_get_commandline_arg(argc, argv, MAC_SHORT, MAC_FULL);

    // Check input parameters
    if (!mac_str) {
        VS_LOG_ERROR("usage: %s/%s <forces MAC address>", MAC_SHORT, MAC_FULL);
        return VS_CODE_ERR_INCORRECT_ARGUMENT;
    }

    if (!_read_mac_address(mac_str, forced_mac_addr)) {
        VS_LOG_ERROR("Incorrect forced MAC address \"%s\" was specified", mac_str);
        return VS_CODE_ERR_INCORRECT_ARGUMENT;
    }

    return VS_CODE_OK;
}

/******************************************************************************/
vs_status_e
vs_app_get_image_path_from_commandline_params(int argc, char *argv[], char **path) {
    static const char *PATH_TO_IMAGE_SHORT = "-i";
    static const char *PATH_TO_IMAGE_FULL = "--image";
    char *path_to_str;

    if (!argv || !argc || !path) {
        VS_LOG_ERROR("Wrong input parameters.");
        return VS_CODE_ERR_INCORRECT_ARGUMENT;
    }

    path_to_str = vs_app_get_commandline_arg(argc, argv, PATH_TO_IMAGE_SHORT, PATH_TO_IMAGE_FULL);

    // Check input parameters
    if (!path_to_str) {
        VS_LOG_ERROR("usage: %s/%s <path to image which need to start>", PATH_TO_IMAGE_SHORT, PATH_TO_IMAGE_FULL);
        return VS_CODE_ERR_INCORRECT_ARGUMENT;
    }

    *path = path_to_str;


    return VS_CODE_OK;
}

/******************************************************************************/
void
vs_app_print_title(const char *devices_dir,
                   const char *app_file,
                   const char *manufacture_id_str,
                   const char *device_type_str) {
    VS_LOG_INFO("--------------------------------------------");
    VS_LOG_INFO("%s app at %s", devices_dir, app_file);
    VS_LOG_INFO("Manufacture ID = \"%s\" , Device type = \"%s\"", manufacture_id_str, device_type_str);
    VS_LOG_INFO("--------------------------------------------\n");
}

/******************************************************************************/
static void
_wait_signal_process(int sig, siginfo_t *si, void *context) {
    pthread_mutex_unlock(&_sleep_lock);
}

/******************************************************************************/
void
vs_app_sleep_until_stop(void) {
    struct sigaction sigaction_ctx;

    memset(&sigaction_ctx, 0, sizeof(sigaction_ctx));

    // Catch Signals to terminate application correctly
    sigaction_ctx.sa_flags = SA_SIGINFO;
    sigaction_ctx.sa_sigaction = _wait_signal_process;
    sigaction(SIGINT, &sigaction_ctx, NULL);
    sigaction(SIGTERM, &sigaction_ctx, NULL);

    if (0 != pthread_mutex_init(&_sleep_lock, NULL)) {
        VS_LOG_ERROR("Mutex init failed");
        return;
    }

    pthread_mutex_lock(&_sleep_lock);
    pthread_mutex_lock(&_sleep_lock);

    pthread_mutex_destroy(&_sleep_lock);
}

/******************************************************************************/
void
vs_app_restart(void) {
    _need_restart = true;
    pthread_mutex_unlock(&_sleep_lock);
}
/******************************************************************************/
bool
vs_app_data_to_hex(const uint8_t *_data, uint32_t _len, uint8_t *_out_data, uint32_t *_in_out_len) {
    const uint8_t hex_str[] = "0123456789abcdef";

    VS_IOT_ASSERT(_in_out_len);
    VS_IOT_ASSERT(_data);
    VS_IOT_ASSERT(_out_data);
    VS_IOT_ASSERT(*_in_out_len >= _len * 2 + 1);

    CHECK_NOT_ZERO_RET(_len, false);
    CHECK_NOT_ZERO_RET(_data, false);
    CHECK_NOT_ZERO_RET(_in_out_len, false);
    BOOL_CHECK_RET(*_in_out_len >= _len * 2 + 1, "Output buffer is too small");

    *_in_out_len = _len * 2 + 1;
    _out_data[*_in_out_len - 1] = 0;
    size_t i;

    for (i = 0; i < _len; i++) {
        _out_data[i * 2 + 0] = hex_str[(_data[i] >> 4) & 0x0F];
        _out_data[i * 2 + 1] = hex_str[(_data[i]) & 0x0F];
    }
    return true;
}

/******************************************************************************/
void
vs_app_str_to_bytes(uint8_t *dst, const char *src, size_t elem_buf_size) {
    size_t pos;
    size_t len;

    assert(src && *src);
    assert(elem_buf_size);

    memset(dst, 0, elem_buf_size);

    len = strlen(src);
    for (pos = 0; pos < len && pos < elem_buf_size; ++pos, ++src, ++dst) {
        *dst = *src;
    }
}

/******************************************************************************/
void
vs_app_get_serial(vs_device_serial_t serial, vs_mac_addr_t mac) {
    VS_IOT_MEMSET(serial, 0x03, VS_DEVICE_SERIAL_SIZE);
    VS_IOT_MEMCPY(serial, mac.bytes, ETH_ADDR_LEN);
}

/******************************************************************************/
bool
vs_app_is_need_restart(void) {
    return _need_restart;
}

/******************************************************************************/
#endif // !WIN32
