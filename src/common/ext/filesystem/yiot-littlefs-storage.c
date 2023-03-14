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

#include <yiot-littlefs-storage.h>

#include "lfs.h"
#include "lfs_util.h"
#include <yiot-littlefs.h>

// ----------------------------------------------------------------------------
static uint32_t _max_file_sz(vs_app_storage_type_t type) { return 10 * 1024; }

// ----------------------------------------------------------------------------
static uint32_t _flash_sz(vs_app_storage_type_t type) {
    switch (type) {
    case VS_APP_STORAGE_SLOTS: {
        return 32 * 1024;
    }

    case VS_APP_STORAGE_TRUST_LIST: {
        return 16 * 1024;
    }

    case VS_APP_STORAGE_SECBOX: {
        return 16 * 1024;
    }

    default: {
        return 0x00;
    }
    }
}

// ----------------------------------------------------------------------------
static uint64_t _base_addr(vs_app_storage_type_t type) {
    switch (type) {
    case VS_APP_STORAGE_SLOTS: {
        return 0;
    }

    case VS_APP_STORAGE_TRUST_LIST: {
        return _flash_sz(VS_APP_STORAGE_SLOTS);
    }

    case VS_APP_STORAGE_SECBOX: {
        return _flash_sz(VS_APP_STORAGE_SLOTS) + _flash_sz(VS_APP_STORAGE_TRUST_LIST);
    }

    default: {
        return 0;
    }
    }
}

// ----------------------------------------------------------------------------
vs_status_e vs_app_storage_init_impl(vs_storage_op_ctx_t *storage_impl,
                         vs_app_storage_type_t type) {
    CHECK_NOT_ZERO_RET(storage_impl, VS_CODE_ERR_INCORRECT_ARGUMENT);

    memset(storage_impl, 0, sizeof(*storage_impl));

    // Prepare TL storage
    storage_impl->impl_func = vs_uboot_storage_impl_func();
    storage_impl->impl_data = vs_uboot_storage_impl_data_init(type);
    storage_impl->file_sz_limit = _max_file_sz(type);

    return VS_CODE_OK;
}

// ----------------------------------------------------------------------------
static void _data_to_hex(const uint8_t *_data, uint32_t _len,
             uint8_t *_out_data, uint32_t *_in_out_len) {
    const uint8_t hex_str[] = "0123456789abcdef";

    VS_IOT_ASSERT(_in_out_len);
    VS_IOT_ASSERT(_data);
    VS_IOT_ASSERT(_out_data);
    VS_IOT_ASSERT(*_in_out_len >= _len * 2 + 1);

    *_in_out_len = _len * 2 + 1;
    _out_data[*_in_out_len - 1] = 0;
    size_t i;

    for (i = 0; i < _len; i++) {
        _out_data[i * 2 + 0] = hex_str[(_data[i] >> 4) & 0x0F];
        _out_data[i * 2 + 1] = hex_str[(_data[i]) & 0x0F];
    }
}

// ----------------------------------------------------------------------------
vs_storage_impl_data_ctx_t
vs_uboot_storage_impl_data_init(vs_app_storage_type_t type) {
    vs_storage_impl_data_ctx_t ctx = NULL;
    uint64_t base_addr = _base_addr(type);
    uint32_t flash_size = _flash_sz(type);

    CHECK_NOT_ZERO_RET(flash_size, NULL);

    ctx = (vs_storage_impl_data_ctx_t)vs_lfs_storage_init(
            base_addr, flash_size);
    CHECK_NOT_ZERO_RET(ctx, NULL);

    return ctx;
}

// ----------------------------------------------------------------------------
static vs_status_e
vs_uboot_storage_deinit_hal(vs_storage_impl_data_ctx_t storage_ctx) {
    CHECK_NOT_ZERO_RET(storage_ctx, VS_CODE_ERR_INCORRECT_PARAMETER);
    vs_lfs_delete_ctx((lfs_t *)storage_ctx);

    return VS_CODE_OK;
}

// ----------------------------------------------------------------------------
static vs_storage_file_t
vs_uboot_storage_open_hal(const vs_storage_impl_data_ctx_t storage_ctx,
                          const vs_storage_element_id_t id) {
    lfs_file_t *file = NULL;

    CHECK_NOT_ZERO(id);
    CHECK_NOT_ZERO(storage_ctx);
    lfs_t *lfs = (lfs_t *)storage_ctx;
    CHECK_NOT_ZERO(lfs->cfg);

    file = VS_IOT_MALLOC(sizeof(lfs_file_t));
    CHECK_NOT_ZERO(file);

    char filename[LFS_NAME_MAX];
    uint32_t len = LFS_NAME_MAX;
    _data_to_hex(id, sizeof(vs_storage_element_id_t), (uint8_t *)filename, &len);

    CHECK_RET(0 == lfs_file_open(lfs, file, filename, LFS_O_RDWR | LFS_O_CREAT),
              NULL, "Can't open file");

terminate:

    return (vs_storage_file_t)file;
}

// ----------------------------------------------------------------------------
vs_status_e static vs_uboot_storage_sync_hal(
        const vs_storage_impl_data_ctx_t storage_ctx,
        const vs_storage_file_t file) {

    return VS_CODE_OK;
}

// ----------------------------------------------------------------------------
static vs_status_e
vs_uboot_storage_close_hal(const vs_storage_impl_data_ctx_t storage_ctx,
                           vs_storage_file_t file) {
    vs_status_e res = VS_CODE_ERR_FILE;

    CHECK_NOT_ZERO(storage_ctx);
    CHECK_NOT_ZERO(file);
    lfs_t *lfs = (lfs_t *)storage_ctx;

    res = VS_CODE_OK;

    if (0 > lfs_file_close(lfs, (lfs_file_t *)file)) {
        VS_LOG_ERROR("Can't close file");
        res = VS_CODE_ERR_FILE;
    }

    VS_IOT_FREE(file);

terminate:

    return res;
}

// ----------------------------------------------------------------------------
static vs_status_e
vs_uboot_storage_save_hal(const vs_storage_impl_data_ctx_t storage_ctx,
                          const vs_storage_file_t file, size_t offset,
                          const uint8_t *data, size_t data_sz) {
    vs_status_e res = VS_CODE_ERR_FILE;

    CHECK_NOT_ZERO(data);
    CHECK_NOT_ZERO(storage_ctx);
    CHECK_NOT_ZERO(file);
    lfs_t *lfs = (lfs_t *)storage_ctx;
    lfs_file_t *f = (lfs_file_t *)file;

    CHECK(0 <= lfs_file_seek(lfs, f, offset, LFS_SEEK_SET), "Error change file position");

    CHECK(0 <= lfs_file_write(lfs, f, data, data_sz), "Error write file");

    res = VS_CODE_OK;

terminate:
    return res;
}

// ----------------------------------------------------------------------------
static vs_status_e
vs_uboot_storage_load_hal(const vs_storage_impl_data_ctx_t storage_ctx,
                          const vs_storage_file_t file, size_t offset,
                          uint8_t *out_data, size_t data_sz) {
    vs_status_e res = VS_CODE_ERR_FILE;

    CHECK_NOT_ZERO(out_data);
    CHECK_NOT_ZERO(storage_ctx);
    CHECK_NOT_ZERO(file);
    lfs_t *lfs = (lfs_t *)storage_ctx;
    lfs_file_t *f = (lfs_file_t *)file;

    CHECK(0 <= lfs_file_seek(lfs, f, offset, LFS_SEEK_SET),
          "Error change file position");
    CHECK(0 <= lfs_file_read(lfs, f, out_data, data_sz), "Error write file");

    res = VS_CODE_OK;

terminate:

    return res;
}

/*******************************************************************************/
static ssize_t
vs_uboot_storage_file_size_hal(const vs_storage_impl_data_ctx_t storage_ctx,
                               const vs_storage_element_id_t id) {
    ssize_t sz = 0;

    CHECK_NOT_ZERO(id);
    CHECK_NOT_ZERO(storage_ctx);
    lfs_t *lfs = (lfs_t *)storage_ctx;
    char filename[LFS_NAME_MAX];
    uint32_t len = LFS_NAME_MAX;
    _data_to_hex(id, sizeof(vs_storage_element_id_t), (uint8_t *)filename, &len);

    lfs_file_t f;
    CHECK(0 == lfs_file_open(lfs, &f, filename, LFS_O_RDONLY), "File not found");

    sz = lfs_file_size(lfs, &f);
    lfs_file_close(lfs, &f);

terminate:

    return sz;
}

// ----------------------------------------------------------------------------
static vs_status_e
vs_uboot_storage_del_hal(const vs_storage_impl_data_ctx_t storage_ctx,
                         const vs_storage_element_id_t id) {
    vs_status_e res = VS_CODE_ERR_FILE_DELETE;

    CHECK_NOT_ZERO(id);
    CHECK_NOT_ZERO(storage_ctx);
    lfs_t *lfs = (lfs_t *)storage_ctx;

    char filename[LFS_NAME_MAX];
    uint32_t len = LFS_NAME_MAX;
    _data_to_hex(id, sizeof(vs_storage_element_id_t), (uint8_t *)filename, &len);

    lfs_remove(lfs, filename);
    res = VS_CODE_OK;

terminate:
    return res;
}

// ----------------------------------------------------------------------------
vs_storage_impl_func_t vs_uboot_storage_impl_func(void) {
    vs_storage_impl_func_t impl;

    memset(&impl, 0, sizeof(impl));

    impl.size = vs_uboot_storage_file_size_hal;
    impl.deinit = vs_uboot_storage_deinit_hal;
    impl.open = vs_uboot_storage_open_hal;
    impl.sync = vs_uboot_storage_sync_hal;
    impl.close = vs_uboot_storage_close_hal;
    impl.save = vs_uboot_storage_save_hal;
    impl.load = vs_uboot_storage_load_hal;
    impl.del = vs_uboot_storage_del_hal;

    return impl;
}

// ----------------------------------------------------------------------------
