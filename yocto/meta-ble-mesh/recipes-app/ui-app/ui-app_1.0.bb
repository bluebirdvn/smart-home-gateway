SUMMARY = "Qt Quick UI App for Smart Agriculture Gateway"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

DEPENDS = "qtbase qtdeclarative qtdeclarative-native qtcharts dbus"
inherit qt6-cmake pkgconfig

SRC_URI = "git://github.com/ten-cua-ban/smart-home-gateway.git;protocol=https;branch=main"
SRCREV = "${AUTOREV}"
S = "${WORKDIR}/git/apps"
OECMAKE_TARGET_COMPILE = "smart_gateway_ui"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${B}/smart_gateway_ui/smart_gateway_ui ${D}${bindir}/smart_gateway_ui
}

FILES:${PN} += "${bindir}/smart_gateway_ui"