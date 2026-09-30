SUMMARY = "mqtt app"
DESCRIPTION = "mqtt handling Mesh and Server message"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

DEPENDS = "glib-2.0 dbus paho-mqtt-c"
inherit cmake pkgconfig
SRC_URI = "git://github.com/ten-cua-ban/smart-home-gateway.git;protocol=https;branch=main"
SRCREV = "${AUTOREV}"
S = "${WORKDIR}/git/apps"

OECMAKE_TARGET_COMPILE = "mqtt-connect"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${B}/mqtt-connect/mqtt-connect ${D}${bindir}/mqtt-connect
}

FILES:${PN} += " \
    ${bindir}/mqtt-connect \
"