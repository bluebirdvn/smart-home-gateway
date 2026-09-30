SUMMARY = "Custom Device Tree Overlay for screen"
DESCRIPTION = "Custom device tree overlay for ILI9341 and XPT2046"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://COPYING;md5=d41d8cd98f00b204e9800998ecf8427e"
SRC_URI = "file://screen_overlayer.dts \
           file://COPYING \
"
DEPENDS += "dtc-native virtual/kernel"
S = "${WORKDIR}"
DEPENDS += "dtc-native"
do_compile() {
    cpp -nostdinc -undef -x assembler-with-cpp \
        -I${STAGING_KERNEL_DIR}/include \
        -I${STAGING_KERNEL_DIR}/arch/arm/boot/dts \
        -I${STAGING_KERNEL_DIR}/arch/arm64/boot/dts \
        -o ${WORKDIR}/screen_overlayer.pre.dts ${WORKDIR}/screen_overlayer.dts

    dtc -I dts -O dtb -o screen_overlayer.dtbo ${WORKDIR}/screen_overlayer.pre.dts
}

do_install() {
    install -d ${D}/boot/overlays
    
    install -m 0644 ${WORKDIR}/screen_overlayer.dtbo ${D}/boot/overlays/screen-overlayer.dtbo
}

FILES:${PN} += "/boot/overlays/screen-overlayer.dtbo"