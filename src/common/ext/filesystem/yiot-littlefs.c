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

#include <stdio.h>
#include <stdbool.h>

#include "lfs.h"
#include "lfs_util.h"

#include <yiot-littlefs.h>
#include <yiot-littlefs-hal.h>
#include <stdlib-config.h>
#include <virgil/iot/logger/logger.h>
#include <virgil/iot/macros/macros.h>
#include <virgil/iot/storage_hal/storage_hal.h>

typedef struct {
    uint32_t base_addr;
} vs_lfs_storage_ctx_t;

// ----------------------------------------------------------------------------
// Read a region in a block. Negative error codes are propogated
// to the user.
static int
_lfs_read_block_hal(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size) {
    CHECK_NOT_ZERO_RET(c, -1);
    vs_lfs_storage_ctx_t *storage_ctx = (vs_lfs_storage_ctx_t *)(c->context);
    CHECK_RET(block < c->block_count, -1, "Error try to read outside device");
    CHECK_NOT_ZERO_RET(storage_ctx, -1);
    int addr = storage_ctx->base_addr + c->block_size * block + off;

    return (0 < iot_flash_read(addr, buffer, size)) ? 0 : -1;
}

// ----------------------------------------------------------------------------
// Program a region in a block. The block must have previously
// been erased. Negative error codes are propogated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
static int
_lfs_prog_block_hal(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size) {
    vs_lfs_storage_ctx_t *storage_ctx = (vs_lfs_storage_ctx_t *)(c->context);
    CHECK_RET(block < c->block_count, -1, "Error try to write outside device");
    CHECK_NOT_ZERO_RET(storage_ctx, -1);
    int addr = storage_ctx->base_addr + c->block_size * block + off;

    return (0 < iot_flash_write(addr, (void *)buffer, size)) ? 0 : -1;
}

// ----------------------------------------------------------------------------
// Erase a block. A block must be erased before being programmed.
// The state of an erased block is undefined. Negative error codes
// are propogated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
static int
_lfs_erase_block_hal(const struct lfs_config *c, lfs_block_t block) {
    vs_lfs_storage_ctx_t *storage_ctx = (vs_lfs_storage_ctx_t *)(c->context);
    CHECK_RET(block < c->block_count, -1, "Error try to write outside device");
    CHECK_NOT_ZERO_RET(storage_ctx, -1);
    return iot_flash_erase(storage_ctx->base_addr + block * c->block_size, c->block_size);
}

// ----------------------------------------------------------------------------
// Sync the state of the underlying block device. Negative error codes
// are propogated to the user.
static int
_lfs_sync_block_hal(const struct lfs_config *c) {
    return 0;
}

// ----------------------------------------------------------------------------
lfs_t *
vs_lfs_storage_init(uint32_t base_addr, lfs_size_t sz) {
    lfs_t *lfs = NULL;
    struct lfs_config *lfs_cfg = NULL;

    vs_lfs_storage_ctx_t *ctx = (vs_lfs_storage_ctx_t *)VS_IOT_MALLOC(sizeof(vs_lfs_storage_ctx_t));
    VS_IOT_ASSERT(ctx);

    if (NULL == ctx) {
        VS_LOG_ERROR("Error memory allocation");
        goto terminate_fail;
    }

    lfs_cfg = (struct lfs_config *)VS_IOT_MALLOC(sizeof(struct lfs_config));
    if (NULL == lfs_cfg) {
        VS_LOG_ERROR("Error memory allocation");
        goto terminate_fail;
    }
    VS_IOT_MEMSET(lfs_cfg, 0, sizeof(struct lfs_config));

    lfs = (lfs_t *)VS_IOT_MALLOC(sizeof(lfs_t));
    if (NULL == lfs) {
        VS_LOG_ERROR("Error memory allocation");
        goto terminate_fail;
    }
    VS_IOT_MEMSET(lfs, 0, sizeof(lfs_t));

    VS_LOG_DEBUG("init_storage. base_addr = 0x%x, size = %d", base_addr, sz);

    lfs_cfg->read = _lfs_read_block_hal;
    lfs_cfg->prog = _lfs_prog_block_hal;
    lfs_cfg->erase = _lfs_erase_block_hal;
    lfs_cfg->sync = _lfs_sync_block_hal;

    if (iot_flash_init(lfs_cfg, sz) < 0) {
        VS_LOG_ERROR("Error init flash");
        goto terminate_fail;
    }

    ctx->base_addr = base_addr;

    lfs_cfg->context = ctx;

    if (0 != lfs_mount(lfs, lfs_cfg)) {
        if (0 != lfs_format(lfs, lfs_cfg)) {
            VS_LOG_ERROR("Error format device");
            goto terminate_fail;
        }

        if (0 != lfs_mount(lfs, lfs_cfg)) {
            VS_LOG_ERROR("Error mount device");
            goto terminate_fail;
        }
    }

    return lfs;

terminate_fail:
    if (ctx) {
        VS_IOT_FREE(ctx);
    }

    if (lfs_cfg) {
        VS_IOT_FREE(lfs_cfg);
    }

    if (lfs) {
        VS_IOT_FREE(lfs);
    }
    return NULL;
}

// ----------------------------------------------------------------------------
int
vs_lfs_storage_erase(lfs_t *lfs) {
    return -1;
}

// ----------------------------------------------------------------------------
size_t
vs_lfs_storage_size(void) {
    return 64 * 1024;
}

// ----------------------------------------------------------------------------
void
vs_lfs_delete_ctx(lfs_t *lfs) {
    if (lfs && lfs->cfg) {
        struct lfs_config *lfs_cfg = (struct lfs_config *)lfs->cfg;

        if (NULL == lfs_cfg->context) {
            VS_LOG_ERROR("Error. ctx is corrupt");
            return;
        }

        vs_lfs_storage_ctx_t *storage_ctx = (vs_lfs_storage_ctx_t *)(lfs_cfg->context);
        lfs_unmount(lfs);
        VS_IOT_FREE(storage_ctx);
        VS_IOT_FREE(lfs_cfg);
        VS_IOT_FREE(lfs);
    }
}

// ----------------------------------------------------------------------------
size_t strcspn(const char *string, const char *reject) {
    size_t count = 0;
    while (strchr(reject, *string) == 0) {
        ++count, ++string;
    }

    return count;
}

// ----------------------------------------------------------------------------
