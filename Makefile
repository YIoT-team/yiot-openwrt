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

PKG_NAME:=yiot-openwrt
PKG_RELEASE:=1

YIOT_CMAKE_NINJA:=1

USE_SOURCE_DIR:=$(shell pwd)/src
PKG_LICENSE:=BSD-2
PKG_LICENSE_FILES:=

PKG_MAINTAINER:=Roman Kutashenko <kutashenko@iot.dev>

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/libconverters
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=YIoT format converters
  MAINTAINER:=Roman Kutashenko <kutashenko@yiot.dev>
  DEPENDS:=+libstdcpp
endef

define Package/libyiot-openwrt
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=YIoT core libraries
  MAINTAINER:=Roman Kutashenko <kutashenko@yiot.dev>
  DEPENDS:=+libconverters
endef

define Package/yiot-firmware-verifier
  SECTION:=yiot
  CATEGORY:=YIoT Applications
  TITLE:=YIoT Firmware Verifier
  MAINTAINER:=Roman Kutashenko <kutashenko@yiot.dev>
  DEPENDS:=+libyiot-openwrt
endef

define Package/yiot-license-processor
  SECTION:=yiot
  CATEGORY:=YIoT Applications
  TITLE:=YIoT License Processor
  MAINTAINER:=Roman Kutashenko <kutashenko@yiot.dev>
  DEPENDS:=+libyiot-openwrt
endef

define Package/yiot
  SECTION:=yiot
  CATEGORY:=YIoT Applications
  TITLE:=YIoT Security provisioner
  MAINTAINER:=Roman Kutashenko <kutashenko@yiot.dev>
  DEPENDS:=+libyiot-openwrt
endef

TARGET_CFLAGS += -I$(STAGING_DIR)/usr/include -fpie -ffunction-sections -fdata-sections
TARGET_LDFLAGS += -L$(STAGING_DIR)/usr/lib -pie -Wl,--gc-sections

CMAKE_OPTIONS = \
	-DYIOT_OPENWRT=ON

ifeq ($(YIOT_CMAKE_NINJA),1)
  CMAKE_OPTIONS += -G Ninja
else
  CMAKE_OPTIONS += -G "Unix Makefiles"
endif

define Package/libconverters/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/common/yiot-core/yiot/common/iotkit/modules/crypto/converters/libconverters.so $(1)/usr/lib/libconverters.so
endef

define Package/libyiot-openwrt/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/common/libyiot-openwrt.so $(1)/usr/lib/libyiot-openwrt.so
endef

define Package/yiot/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./src/security-provisioner/files/yiot.init $(1)/etc/init.d/gps
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/security-provisioner/yiot $(1)/usr/bin/yiot
endef

define Package/yiot-firmware-verifier/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/firmware-verifier/yiot-firmware-verifier $(1)/usr/bin/yiot-firmware-verifier
endef

define Package/yiot-license-processor/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/license-processor/yiot-license-processor $(1)/usr/bin/yiot-license-processor
endef

$(eval $(call BuildPackage,libconverters))
$(eval $(call BuildPackage,libyiot-openwrt))
$(eval $(call BuildPackage,yiot-firmware-verifier))
$(eval $(call BuildPackage,yiot-license-processor))
$(eval $(call BuildPackage,yiot))
