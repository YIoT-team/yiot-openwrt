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

USE_SOURCE_DIR:=$(shell pwd)/src
PKG_LICENSE:=BSD-2
PKG_LICENSE_FILES:=

PKG_MAINTAINER:=Roman Kutashenko <kutashenko@gmail.com>

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/yiot-firmware-verifier
  SECTION:=yiot-openwrt
  CATEGORY:=YIoT Applications
  TITLE:=YIoT Firmware Verifier
  MAINTAINER:=Roman Kutashenko <kutashenko@yiot-dev.io>
endef

define Package/yiot
  SECTION:=yiot-openwrt
  CATEGORY:=YIoT Applications
  TITLE:=YIoT Security provisioner
  MAINTAINER:=Roman Kutashenko <kutashenko@yiot-dev.io>
endef

TARGET_CFLAGS += -I$(STAGING_DIR)/usr/include -fpie -ffunction-sections -fdata-sections
TARGET_LDFLAGS += -L$(STAGING_DIR)/usr/lib -pie -Wl,--gc-sections

CMAKE_OPTIONS = \
	-DYIOT_OPENWRT=ON

# define Build/InstallDev
# 	$(CP) -f $(shell pwd)/src/iotkit/sdk/ipkg-install/* $(STAGING_DIR)/
# 	mkdir -p  $(STAGING_DIR)/usr/lib/virgil/mbedtls/
# 	$(CP) -f $(shell pwd)/src/iotkit/sdk/depends/installed/lib/*.a $(STAGING_DIR)/usr/lib/virgil/mbedtls/
# endef

$(eval $(call BuildPackage,yiot-firmware-verifier))
$(eval $(call BuildPackage,yiot))
