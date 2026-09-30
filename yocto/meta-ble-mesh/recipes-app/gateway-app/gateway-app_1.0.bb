SUMMARY = "gateway-app daemon for gateway smarthome"
LICENSE = "MIT"

LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "git://github.com/ten-cua-ban/smart-home-gateway.git;protocol=https;branch=main"
SRCREV = "${AUTOREV}"
DEPENDS = "glib-2.0 dbus"

S = "${WORKDIR}/git/apps"
inherit cmake pkgconfig

OECMAKE_TARGET_COMPILE = "gateway-daemon"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${B}/gateway-daemon/gateway-daemon ${D}${bindir}/gateway-daemon
}

FILES:${PN} += "${bindir}/gateway-daemon"