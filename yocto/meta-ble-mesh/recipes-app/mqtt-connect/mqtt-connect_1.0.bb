SUMMARY = "mqtt app"
DESCRIPTION = "mqtt handling Mesh and Server message"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "git://github.com/bluebirdvn/smart-home-gateway.git;protocol=https;branch=main"
SRCREV = "${AUTOREV}"
DEPENDS = "glib-2.0 dbus paho-mqtt-c"
DEPENDS += "cjson"
S = "${WORKDIR}/git/apps"
inherit cmake pkgconfig

EXTRA_OECMAKE += "-DBUILD_MQTT=ON"
OECMAKE_TARGET_COMPILE = "mqtt-connect"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${B}/mqtt-connect/mqtt-connect ${D}${bindir}/mqtt-connect
}

FILES:${PN} += " \
    ${bindir}/mqtt-connect \
"