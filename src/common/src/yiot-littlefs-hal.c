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
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <mtd/mtd-user.h>

#include <yiot-littlefs.h>
#include <yiot-littlefs-hal.h>
#include <virgil/iot/logger/logger.h>
#include <virgil/iot/status_code/status_code.h>

static int _fd = -1;
static char *_mtd_device = NULL;
#define IOT_FLASH_SZ (64 * 1024)
static uint8_t _cache[IOT_FLASH_SZ];
static bool _use_mtd = false;
static uint32_t _erase_sz = IOT_FLASH_SZ;

#define MTD_DEV_PREFIX "/dev/mtd"

// ----------------------------------------------------------------------------
int iot_flash_set_device(const char *device) {
    VS_IOT_FREE(_mtd_device);
    if (!device) {
        return -1;
    }
    int len = VS_IOT_STRLEN(device) + 1;
    _mtd_device = VS_IOT_MALLOC(len);
    VS_IOT_MEMSET(_mtd_device, 0, len);
    VS_IOT_STRCPY(_mtd_device, device);

    return 0;
}

// ----------------------------------------------------------------------------
static int
hw_iot_flash_read(int offset, void *buf, size_t count) {
    int res = -1;
    CHECK_NOT_ZERO(buf);
    CHECK(offset >= 0, "MTD offset is less than zero");

    CHECK(offset == lseek(_fd, offset, SEEK_SET), "MTD seek error");
    CHECK(count == read(_fd, buf, count), "MTD read error");

    res = count;
terminate:
    return res;
}

// ----------------------------------------------------------------------------
static int
hw_iot_flash_write(int offset, void *buf, size_t count) {
    int res = -1;
    CHECK_NOT_ZERO(buf);
    CHECK(offset >= 0, "MTD offset is less than zero");

    CHECK(offset == lseek(_fd, offset, SEEK_SET), "MTD seek error");
    CHECK(count == write(_fd, buf, count), "MTD write error");
    fsync(_fd);

    res = count;
terminate:
    return res;
}

// ----------------------------------------------------------------------------
static int
hw_iot_flash_erase(int offset, size_t count) {
    int res;
    erase_info_t ei;

    ei.start = offset;
    ei.length = count;

    ioctl(_fd, MEMUNLOCK, &ei);
    res = ioctl(_fd, MEMERASE, &ei);

    return (res == -1) ? -1 : 0;
}

// ----------------------------------------------------------------------------
static int
_init_file(struct lfs_config *lfs_cfg, lfs_size_t sz) {
    long file_sz;

    // Check required data
    CHECK_NOT_ZERO_RET(_mtd_device, -1);
    CHECK_NOT_ZERO_RET(lfs_cfg, -1);

    // Open and get descriptor
    if (-1 == _fd) {
        _fd = open(_mtd_device, O_RDWR | O_CREAT);
        if (-1 != _fd) {
            file_sz = lseek(_fd, 0L, SEEK_END);

            // Resize if required
            if (file_sz < IOT_FLASH_SZ) {
                if (hw_iot_flash_write(0, _cache, IOT_FLASH_SZ) < 0) {
                    VS_LOG_CRITICAL("Allocate size on MTD flash emulator");
                }
            }

            if (IOT_FLASH_SZ != hw_iot_flash_read(0, _cache, IOT_FLASH_SZ)) {
                VS_LOG_CRITICAL("Cannot read MTD flash");
            }
        }
    }

    VS_LOG_DEBUG("MTD type: FILE %s", _mtd_device);
    VS_LOG_DEBUG("MTD total size : %u bytes", IOT_FLASH_SZ);
    VS_LOG_DEBUG("MTD erase size : %u bytes", 1024);

    if (_fd > 0) {
        lfs_cfg->block_size = 1024;
        lfs_size_t block_count;
        block_count = sz / lfs_cfg->block_size;
        if (!block_count) {
            block_count = 1;
        }
        lfs_cfg->read_size = 128;
        lfs_cfg->prog_size = lfs_cfg->block_size;
        lfs_cfg->block_count = block_count;
        lfs_cfg->cache_size = lfs_cfg->block_size;
        lfs_cfg->lookahead_size = 128;
        lfs_cfg->block_cycles = 1000;

        return VS_CODE_OK;
    }

    return VS_CODE_ERR_FILE;
}

// ----------------------------------------------------------------------------
static int
_init_mtd(struct lfs_config *lfs_cfg, lfs_size_t sz) {
    mtd_info_t mtd_info;

    // Check required data
    CHECK_NOT_ZERO_RET(_mtd_device, -1);
    CHECK_NOT_ZERO_RET(lfs_cfg, -1);

    // Open and get descriptor
    if (-1 == _fd) {
        _fd = open(_mtd_device, O_RDWR);
        if (-1 != _fd) {
            if (IOT_FLASH_SZ != hw_iot_flash_read(0, _cache, IOT_FLASH_SZ)) {
                VS_LOG_CRITICAL("Cannot read MTD flash");
            }
        }
    }
    ioctl(_fd, MEMGETINFO, &mtd_info);

    VS_LOG_DEBUG("MTD type: %u", mtd_info.type);
    VS_LOG_DEBUG("MTD total size : %u bytes", mtd_info.size);
    VS_LOG_DEBUG("MTD erase size : %u bytes", mtd_info.erasesize);

    if (mtd_info.erasesize > _erase_sz) {
        _erase_sz = mtd_info.erasesize;
    }

    if (_fd > 0) {
        lfs_cfg->block_size = 128;
        lfs_size_t block_count;
        block_count = sz / lfs_cfg->block_size;
        if (!block_count) {
            block_count = 1;
        }
        lfs_cfg->read_size = 128;
        lfs_cfg->prog_size = 1;
        lfs_cfg->block_count = block_count;
        lfs_cfg->cache_size = lfs_cfg->block_size;
        lfs_cfg->lookahead_size = 128;
        lfs_cfg->block_cycles = 1000;

        return VS_CODE_OK;
    }

    return VS_CODE_ERR_FILE;
}

// ----------------------------------------------------------------------------
int
iot_flash_init(struct lfs_config *lfs_cfg, lfs_size_t sz) {
    // Check required data
    CHECK_NOT_ZERO_RET(_mtd_device, -1);
    CHECK_NOT_ZERO_RET(lfs_cfg, -1);

    // Check if MTD device or file
    _use_mtd = 0 == VS_IOT_MEMCMP(_mtd_device, MTD_DEV_PREFIX, VS_IOT_STRLEN(MTD_DEV_PREFIX));

    if (_use_mtd) {
        return _init_mtd(lfs_cfg, sz);
    }

    return _init_file(lfs_cfg, sz);
}

// ----------------------------------------------------------------------------
int
iot_flash_read(int offset, void *buf, size_t count) {
    VS_IOT_MEMCPY(buf, _cache + offset, count);
    return count;
}

// ----------------------------------------------------------------------------
int
iot_flash_write(int offset, void *buf, size_t count) {
    VS_IOT_MEMCPY(_cache + offset, buf, count);
    return count;
}

// ----------------------------------------------------------------------------
int
iot_flash_erase(int offset, size_t count) {
    VS_IOT_MEMSET(_cache + offset, 0xFF, count);
    return 0;
}

// ----------------------------------------------------------------------------
int
iot_flash_hw_sync(void) {
    uint8_t tmp[IOT_FLASH_SZ];
    VS_IOT_MEMSET(tmp, 0xab, IOT_FLASH_SZ);
    if (hw_iot_flash_read(0, tmp, IOT_FLASH_SZ) < 0) {
        VS_LOG_CRITICAL("Cannot read MTD flash");
    }

    if (0 != VS_IOT_MEMCMP(tmp, _cache, IOT_FLASH_SZ)) {
        if (_use_mtd) {
            hw_iot_flash_erase(0, _erase_sz);
        }
        if (hw_iot_flash_write(0, _cache, IOT_FLASH_SZ) < 0) {
            VS_LOG_CRITICAL("Cannot sync MTD flash");
        }
    }

    return 0;
}

// ----------------------------------------------------------------------------
