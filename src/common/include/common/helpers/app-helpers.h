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

#ifndef VS_IOT_APP_HELPERS_H
#define VS_IOT_APP_HELPERS_H

#include <virgil/iot/protocols/snap.h>
char *
vs_app_get_commandline_arg(int argc, char *argv[], const char *shortname, const char *longname);

vs_status_e
vs_app_get_mac_from_commandline_params(int argc, char *argv[], vs_mac_addr_t *forced_mac_addr);

vs_status_e
vs_app_get_image_path_from_commandline_params(int argc, char *argv[], char **path);

void
vs_app_print_title(const char *devices_dir,
                   const char *app_file,
                   const char *manufacture_id_str,
                   const char *device_type_str);

void
vs_app_sleep_until_stop(void);

void
vs_app_restart(void);

bool
vs_app_data_to_hex(const uint8_t *_data, uint32_t _len, uint8_t *_out_data, uint32_t *_in_out_len);

void
vs_app_str_to_bytes(uint8_t *dst, const char *src, size_t elem_buf_size);

void
vs_app_get_serial(vs_device_serial_t serial, vs_mac_addr_t mac);

bool
vs_app_is_need_restart(void);

#endif // VS_IOT_APP_HELPERS_H
