SUMMARY = "local database app for gateway smathome"
DESCRIPTION = "store local data"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "git://github.com/bluebirdvn/smart-home-gateway.git;protocol=https;branch=main"
SRCREV = "${AUTOREV}"
DEPENDS = "glib-2.0 dbus sqlite3"
DEPENDS += "cjson"
S = "${WORKDIR}/git/apps"
inherit cmake pkgconfig

EXTRA_OECMAKE += "-DBUILD_DATABASE=ON"
OECMAKE_TARGET_COMPILE = "local-database"

do_install() {
    install -d ${D}${bindir}
    install -d ${D}${sysconfdir}/gateway
    install -m 0755 ${B}/local-database/local-database ${D}${bindir}/local-database
    install -m 0644 ${S}/local-database/configs/local_db_mesh_ble.sql ${D}${sysconfdir}/gateway/
    install -d ${D}${localstatedir}/lib/gateway
}

FILES:${PN} += " \
    ${bindir}/local-database \
    ${sysconfdir}/gateway/local_db_mesh_ble.sql \
    ${localstatedir}/lib/gateway \
"