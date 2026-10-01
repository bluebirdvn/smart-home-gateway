SUMMARY = "systemd service for start blemesh system"
DESCRIPTION = "start ui-app and daemon-app"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://COPYING;md5=d41d8cd98f00b204e9800998ecf8427e"
SRC_URI = "file://COPYING \
           file://daemon-uart.service \
           file://ui-qt.service \
           file://mqtt-connect.service \
           file://local-database.service \
"

S = "${WORKDIR}"
inherit systemd
SYSTEMD_AUTO_ENABLE = "enable"
SYSTEMD_SERVICE:${PN} = "daemon-uart.service ui-qt.service mqtt-connect.service local-database.service"


do_install() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${S}/daemon-uart.service ${D}${systemd_system_unitdir}/
    install -m 0644 ${S}/ui-qt.service ${D}${systemd_system_unitdir}/
    install -m 0644 ${S}/mqtt-connect.service ${D}${systemd_system_unitdir}/
    install -m 0644 ${S}/local-database.service ${D}${systemd_system_unitdir}/
}

RDEPENDS:${PN} += " \
    gateway-app \
    local-database-app \
    mqtt-connect \
    ui-app \
    apps-config \
"

FILES:${PN} += " \
    ${systemd_system_unitdir}/daemon-uart.service \
    ${systemd_system_unitdir}/ui-qt.service \
    ${systemd_system_unitdir}/mqtt-connect.service \
    ${systemd_system_unitdir}/local-database.service \
"