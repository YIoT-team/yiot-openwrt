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
int
iot_flash_init(struct lfs_config *lfs_cfg, lfs_size_t sz) {
    if (!_mtd_device) {
        return -1;
    }

    mtd_info_t mtd_info;

    CHECK_NOT_ZERO_RET(lfs_cfg, -1);

    if (-1 == _fd) {
        _fd = open(_mtd_device, O_RDWR);
    }
    ioctl(_fd, MEMGETINFO, &mtd_info);

    VS_LOG_DEBUG("MTD type: %u", mtd_info.type);
    VS_LOG_DEBUG("MTD total size : %u bytes", mtd_info.size);
    VS_LOG_DEBUG("MTD erase size : %u bytes", mtd_info.erasesize);

    if (_fd > 0) {
        lfs_cfg->block_size = mtd_info.erasesize;
        lfs_size_t block_count;
        block_count = sz / lfs_cfg->block_size;
        if (!block_count) {
            block_count = 1;
        }
        lfs_cfg->read_size = 128;
        lfs_cfg->prog_size = mtd_info.writesize;
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
iot_flash_read(int offset, void *buf, size_t count) {
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
int
iot_flash_write(int offset, void *buf, size_t count) {
    int res = -1;
    CHECK_NOT_ZERO(buf);
    CHECK(offset >= 0, "MTD offset is less than zero");

    CHECK(offset == lseek(_fd, offset, SEEK_SET), "MTD seek error");
    CHECK(count == write(_fd, buf, count), "MTD write error");
    fsync(_fd);

    VS_LOG_DEBUG("write offset: %d  count: %d", offset, (int)count);
    VS_LOG_HEX(VS_LOGLEV_DEBUG, "write : ", buf, count);

    res = count;
terminate:
    return res;
}

// ----------------------------------------------------------------------------
int
iot_flash_erase(int offset, size_t count) {
    int res;
    erase_info_t ei;

    ei.start = offset;
    ei.length = count;

    ioctl(_fd, MEMUNLOCK, &ei);
    res = ioctl(_fd, MEMERASE, &ei);

    return (res == -1) ? -1 : 0;
}

// ----------------------------------------------------------------------------
