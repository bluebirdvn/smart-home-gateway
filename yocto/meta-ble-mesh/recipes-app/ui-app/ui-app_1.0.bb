SUMMARY = "Qt Quick UI App for Smart Agriculture Gateway"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "git://github.com/bluebirdvn/smart-home-gateway.git;protocol=https;branch=main"
SRCREV = "${AUTOREV}"
DEPENDS = "qtbase qtdeclarative qtdeclarative-native qtcharts dbus"

S = "${WORKDIR}/git/apps"
inherit qt6-cmake pkgconfig

EXTRA_OECMAKE += "-DBUILD_UI=ON"
OECMAKE_TARGET_COMPILE = "smart_gateway_ui"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${B}/smart_gateway_ui/smart_gateway_ui ${D}${bindir}/smart_gateway_ui
}

FILES:${PN} += "${bindir}/smart_gateway_ui"