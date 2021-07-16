#  ────────────────────────────────────────────────────────────
#                     ╔╗  ╔╗ ╔══╗      ╔════╗
#                     ║╚╗╔╝║ ╚╣╠╝      ║╔╗╔╗║
#                     ╚╗╚╝╔╝  ║║  ╔══╗ ╚╝║║╚╝
#                      ╚╗╔╝   ║║  ║╔╗║   ║║
#                       ║║   ╔╣╠╗ ║╚╝║   ║║
#                       ╚╝   ╚══╝ ╚══╝   ╚╝
#    ╔╗╔═╗                    ╔╗                     ╔╗
#    ║║║╔╝                   ╔╝╚╗                    ║║
#    ║╚╝╝  ╔══╗ ╔══╗ ╔══╗  ╔╗╚╗╔╝  ╔══╗ ╔╗ ╔╗╔╗ ╔══╗ ║║  ╔══╗
#    ║╔╗║  ║║═╣ ║║═╣ ║╔╗║  ╠╣ ║║   ║ ═╣ ╠╣ ║╚╝║ ║╔╗║ ║║  ║║═╣
#    ║║║╚╗ ║║═╣ ║║═╣ ║╚╝║  ║║ ║╚╗  ╠═ ║ ║║ ║║║║ ║╚╝║ ║╚╗ ║║═╣
#    ╚╝╚═╝ ╚══╝ ╚══╝ ║╔═╝  ╚╝ ╚═╝  ╚══╝ ╚╝ ╚╩╩╝ ║╔═╝ ╚═╝ ╚══╝
#                    ║║                         ║║
#                    ╚╝                         ╚╝
#
#    Lead Maintainer: Roman Kutashenko <kutashenko@gmail.com>
#  ────────────────────────────────────────────────────────────

include $(TOPDIR)/rules.mk

PKG_NAME:=yiot-core
PKG_RELEASE:=1

USE_SOURCE_DIR:=$(shell pwd)/src/iotkit/sdk
PKG_LICENSE:=BSD-2
PKG_LICENSE_FILES:=

PKG_MAINTAINER:=Roman Kutashenko <kutashenko@gmail.com>

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/libyiot
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=YIoT Core library for user space
endef

# define Package/libyiot-uboot
#   SECTION:=libs
#   CATEGORY:=Libraries
#   TITLE:=YIoT Core library for U-BOOT
# endef

TARGET_CFLAGS += -I$(STAGING_DIR)/usr/include -fpie -ffunction-sections -fdata-sections
TARGET_LDFLAGS += -L$(STAGING_DIR)/usr/lib -L$(STAGING_DIR)/usr/lib/virgil/mbedtls -pie -Wl,--gc-sections

CMAKE_OPTIONS = \
	-DVIRGIL_IOT_CONFIG_DIRECTORY=$(shell pwd)/src/iotkit/sdk/config/pc \
	-DENABLE_TESTING=OFF \
	-DENABLE_HEAVY_TESTS=OFF \
	-DVIRGIL_IOT_CLOUD=OFF \
	-DVIRGIL_IOT_HIGH_LEVEL=OFF \
	-DVIRGIL_IOT_THREADSAFE=ON \
	-DYIOT_OPENWRT=ON \

define Build/InstallDev
	$(CP) -f $(shell pwd)/src/iotkit/sdk/ipkg-install/* $(STAGING_DIR)/
	mkdir -p  $(STAGING_DIR)/usr/lib/virgil/mbedtls/
	$(CP) -f $(shell pwd)/src/iotkit/sdk/depends/installed/lib/*.a $(STAGING_DIR)/usr/lib/virgil/mbedtls/
endef

$(eval $(call BuildPackage,libyiot))
# $(eval $(call BuildPackage,libyiot-uboot))
