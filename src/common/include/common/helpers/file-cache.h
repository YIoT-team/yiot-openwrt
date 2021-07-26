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

#ifndef VS_IOT_FILE_CACHE_H
#define VS_IOT_FILE_CACHE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/types.h>

int
vs_file_cache_enable(bool enable);

bool
vs_file_cache_is_enabled();

int
vs_file_cache_open(const char *file_name);

int
vs_file_cache_create(const char *file_name, const uint8_t *data, size_t data_sz);

ssize_t
vs_file_cache_get_len(const char *file_name);

int
vs_file_cache_read(const char *file_name, uint32_t offset, uint8_t *data, size_t buf_sz, size_t *read_sz);

int
vs_file_cache_write(const char *file_name, uint32_t offset, const uint8_t *data, size_t data_sz);

int
vs_file_cache_sync(const char *file_name);

void
vs_file_cache_close(const char *file_name);

void
vs_file_cache_clean(void);

#endif // VS_IOT_FILE_CACHE_H
