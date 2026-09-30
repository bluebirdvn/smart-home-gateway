SUMMARY = "Custom WiFi configuration and watchdog services for Raspberry Pi"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"

SRC_URI = " \
    file://wifi.conf \
    file://wifi-connect.service \
    file://wifi-connect.sh \
    file://wifi-init.service \
    file://wpa_supplicant@wlan0.service \
    file://wpa_supplicant-wlan0.conf.custom \
"

S = "${WORKDIR}"

inherit systemd

SYSTEMD_SERVICE:${PN} = " \
    wifi-init.service \
    wifi-connect.service \
    wpa_supplicant@wlan0.service \
"
SYSTEMD_AUTO_ENABLE = "enable"

do_install() {
    install -d ${D}${sysconfdir}/wpa_supplicant/
    install -m 0600 ${WORKDIR}/wpa_supplicant-wlan0.conf.custom ${D}${sysconfdir}/wpa_supplicant/wpa_supplicant-wlan0.conf
    install -d ${D}${sysconfdir}/modules-load.d/
    install -m 0644 ${WORKDIR}/wifi.conf ${D}${sysconfdir}/modules-load.d/
    install -d ${D}${sbindir}/
    install -m 0755 ${WORKDIR}/wifi-connect.sh ${D}${sbindir}/
    install -d ${D}${systemd_system_unitdir}/
    install -m 0644 ${WORKDIR}/wifi-connect.service ${D}${systemd_system_unitdir}/
    install -m 0644 ${WORKDIR}/wifi-init.service ${D}${systemd_system_unitdir}/
    install -m 0644 ${WORKDIR}/wpa_supplicant@wlan0.service ${D}${systemd_system_unitdir}/
}

FILES:${PN} = " \
    ${sysconfdir}/wpa_supplicant/* \
    ${sysconfdir}/modules-load.d/* \
    ${sbindir}/wifi-connect.sh \
    ${systemd_system_unitdir}/*.service \
"

RDEPENDS:${PN} = "wpa-supplicant dhcpcd bash"