SUMMARY = "install config file into target path"
LICENSE = "MIT"

LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "git://github.com/bluebirdvn/smart-home-gateway.git;protocol=https;branch=main"
SRCREV = "${AUTOREV}"

S = "${WORKDIR}/git/apps/configs"


do_install() {
    install -d ${D}${sysconfdir}/gateway
    install -m 0644 ${S}/config.json ${D}${sysconfdir}/gateway/
    install -m 0644 ${S}/db.xml ${D}${sysconfdir}/gateway/
    install -m 0644 ${S}/mesh.xml ${D}${sysconfdir}/gateway/
    install -m 0644 ${S}/mqtt.xml ${D}${sysconfdir}/gateway/
    install -m 0644 ${S}/ui.xml ${D}${sysconfdir}/gateway/
    install -m 0644 ${S}/mqtt_config.json ${D}${sysconfdir}/gateway/

    install -d ${D}${sysconfdir}/dbus-1/system.d
    install -m 0644 ${S}/com.gateway.mesh.conf ${D}${sysconfdir}/dbus-1/system.d/
}

FILES:${PN} += " \
    ${sysconfdir}/gateway/* \
    ${sysconfdir}/dbus-1/system.d/* \
"
