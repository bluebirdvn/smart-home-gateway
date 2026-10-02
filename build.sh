#!/bin/bash
set -e

MY_META_LAYER_DIR="./yocto/"
YOCTO_BRANCH="scarthgap"
POKY_DIR="poky"
BUILD_DIR="build"

echo "check and clone poky"

if [ ! -d "${POKY_DIR}" ]; then
    git clone -b ${YOCTO_BRANCH} https://git.yoctoproject.org/poky.git ${POKY_DIR}
fi

cp -r ${MY_META_LAYER_DIR}/* ${POKY_DIR}/

cd ${POKY_DIR}
[ ! -d "meta-raspberrypi" ] && git clone -b ${YOCTO_BRANCH} https://git.yoctoproject.org/meta-raspberrypi
[ ! -d "meta-openembedded" ] && git clone -b ${YOCTO_BRANCH} https://git.openembedded.org/meta-openembedded
[ ! -d "meta-qt6" ] && git clone -b 6.8 https://code.qt.io/yocto/meta-qt6.git

echo "initializing building env"

cd ..

source ${POKY_DIR}/oe-init-build-env ${BUILD_DIR}

bitbake-layers add-layer ../${POKY_DIR}/meta-raspberrypi || true
bitbake-layers add-layer ../${POKY_DIR}/meta-openembedded/meta-oe || true
bitbake-layers add-layer ../${POKY_DIR}/meta-openembedded/meta-python || true
bitbake-layers add-layer ../${POKY_DIR}/meta-openembedded/meta-networking || true
bitbake-layers add-layer ../${POKY_DIR}/meta-qt6 || true
bitbake-layers add-layer ../${POKY_DIR}/meta-ble-mesh || true

cat << 'EOF' > conf/local.conf
EXTRA_IMAGE_FEATURES ?= "debug-tweaks"
PATCHRESOLVE = "noop"
USER_CLASSES ?= "buildstats"

BB_DISKMON_DIRS ??= "\
    STOPTASKS,${TMPDIR},1G,100K \
    STOPTASKS,${DL_DIR},1G,100K \
    STOPTASKS,${SSTATE_DIR},1G,100K \
    STOPTASKS,/tmp,100M,100K \
    HALT,${TMPDIR},100M,1K \
    HALT,${DL_DIR},100M,1K \
    HALT,${SSTATE_DIR},100M,1K \
    HALT,/tmp,10M,1K"

DL_DIR = "${TOPDIR}/../downloads"
SSTATE_DIR ?= "${TOPDIR}/../sstate-cache"
TMPDIR = "${TOPDIR}/tmp"

MACHINE = "raspberrypi0-2w-64"

ENABLE_UART = "1"
CONF_VERSION = "2"
IMAGE_FSTYPES = "wic"

DISTRO_FEATURES:append = " systemd wifi usrmerge dbus"
VIRTUAL-RUNTIME_init_manager = "systemd"
DISTRO_FEATURES_BACKFILL_CONSIDERED = "sysvinit"
VIRTUAL-RUNTIME_initscripts = ""
SYSTEMD_DEFAULT_TARGET = "multi-user.target"

RPI_EXTRA_CONFIG = "\
dtoverlay=disable-bt\n\
dtparam=audio=off\n\
dtparam=spi=on\n\
force_turbo=0\n\
over_voltage=-2\n\
arm_freq=700\n\
gpu_mem=32\n\
framebuffer_width=320\n\
framebuffer_height=240\n\
dtoverlay=screen-overlayer\n\
"
IMAGE_BOOT_FILES:append = " screen-overlayer.dtbo;overlays/screen-overlayer.dtbo"

KERNEL_MODULE_AUTOLOAD:rpi += "brcmfmac"
MACHINE_FEATURES:append = " wifi"
ENABLE_BLUETOOTH = "0"

LICENSE_FLAGS_ACCEPTED += "synaptics-killswitch"

PREFERRED_PROVIDER_virtual/sh = "bash"
IMAGE_INSTALL:append = " \
    bash \
    bash-completion \
    openssh \
    dbus \
    dbus-tools \
    systemd-analyze \
    procps \
    util-linux \
    coreutils \
    iproute2 \
    iputils \
    htop \
    strace \
    gdbserver \
    wpa-supplicant \
    iw \
    wifi-config \
    glib-2.0 \
    glib-2.0-utils \
    sqlite3 \
    libsqlite3 \
    python3-sqlite3 \
    paho-mqtt-c \
    ttf-bitstream-vera \
    kernel-modules \
    linux-firmware-rpidistro-bcm43430 \
    linux-firmware-rpidistro-bcm43436 \
    linux-firmware-rpidistro-bcm43436s \
    screen-overlayer \
    app-service \
"

IMAGE_INSTALL:append = " \
    qtbase \
    qtbase-plugins \
    qtdeclarative \
    qtdeclarative-qmlplugins \
    qtcharts \
"

MACHINE_ESSENTIAL_EXTRA_RRECOMMENDS += "kernel-module-fb-ili9341"
KERNEL_MODULE_AUTOLOAD += "fb_ili9341"

SERIAL_CONSOLES = ""
CMDLINE_SERIAL = ""

INHERIT += "externalsrc"
EXTERNALSRC:pn-gateway-app        = "/workdir/apps"
EXTERNALSRC:pn-local-database-app = "/workdir/apps"
EXTERNALSRC:pn-mqtt-connect       = "/workdir/apps"
EXTERNALSRC:pn-ui-app             = "/workdir/apps"
EXTERNALSRC:pn-apps-config        = "/workdir/apps/configs"
EOF

bitbake -c cleansstate gateway-app local-database-app mqtt-connect ui-app

echo "configuration complete"
echo "start build image"

bitbake core-image-minimal