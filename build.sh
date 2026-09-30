#!/bin/bash
set -e

MY_META_LAYER_DIR="./yocto/"
YOCTO_BRANCH="scarthgap"
POKY_DIR="poky"
BUILD_DIR="build"


echo "check and clone poky"

if [ ! -d "${POKY_DIR}" ]; then
    git clone -b ${YOCTO_BRANCH} git://git.yoctoproject.org/poky.git ${POKY_DIR}
fi

cp -r ${MY_META_LAYER_DIR}/* ${POKY_DIR}/

cd ${POKY_DIR}
[ ! -d "meta-raspberrypi" ] && git clone -b ${YOCTO_BRANCH} git://git.yoctoproject.org/meta-raspberrypi
[ ! -d "meta-openembedded" ] && git clone -b ${YOCTO_BRANCH} git://git.openembedded.org/meta-openembedded
[ ! -d "meta-qt6" ] && git clone -b ${YOCTO_BRANCH} git://code.qt.io/yocto/meta-qt6.git


echo "initializing building env"

cd ..

source ${POKY_DIR}/oe-init-build-env ${BUILD_DIR}

bitbake-layers add-layer ../${POKY_DIR}/meta-raspberrypi
bitbake-layers add-layer ../${POKY_DIR}/meta-openembedded/meta-oe
bitbake-layers add-layer ../${POKY_DIR}/meta-openembedded/meta-python
bitbake-layers add-layer ../${POKY_DIR}/meta-openembedded/meta-networking
bitbake-layers add-layer ../${POKY_DIR}/meta-qt6
bitbake-layers add-layer ../${POKY_DIR}/meta-ble-mesh

cat << 'EOF' >> conf/local.conf

DL_DIR = "${TOPDIR}/../downloads"
SSTATE_DIR ?= "${TOPDIR}/../sstate-cache"
TMPDIR = "${TOPDIR}/tmp"

MACHINE = "raspberrypi0-2w-64"

ENABLE_UART = "1"
CONF_VERSION = "2"
IMAGE_INSTALL:append = " wpa-supplicant iw"

DISTRO_FEATURES:append = " systemd wifi usrmerge dbus"
VIRTUAL-RUNTIME_init_manager = "systemd"
DISTRO_FEATURES_BACKFILL_CONSIDERED = "sysvinit"
VIRTUAL-RUNTIME_initscripts = ""


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


SYSTEMD_DEFAULT_TARGET = "multi-user.target"

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
    gdb \
"

IMAGE_INSTALL:append = " \
    glib-2.0 \
    glib-2.0-utils \
"
IMAGE_INSTALL:append = " \
    sqlite3 \
    libsqlite3 \
    python3-sqlite3 \
"
IMAGE_INSTALL:append = " \
    qtbase \
    qtbase-plugins \
    qtdeclarative \
    qtdeclarative-qmlplugins \
    qtcharts \
    qttools \
"
IMAGE_INSTALL:append = " ttf-bitstream-vera"
IMAGE_INSTALL:append = " \
    paho-mqtt-c \
"
IMAGE_INSTALL:append = " wifi-config"
IMAGE_INSTALL:append = " sqlite3 python3-sqlite3"

IMAGE_INSTALL:append = " kernel-modules linux-firmware-rpidistro-bcm43430 linux-firmware-rpidistro-bcm43436 linux-firmware-rpidistro-bcm43436s"
MACHINE_EXTRA_RRECOMMENDS += "linux-firmware-rpidistro-bcm43430 linux-firmware-rpidistro-bcm43436s"

IMAGE_INSTALL:append = " screen-overlayer"

# shutdown serial (getty)
SERIAL_CONSOLES = ""
#shutdown log through terminal
CMDLINE_SERIAL = ""

IMAGE_INSTALL:append = " app-service"

MACHINE_ESSENTIAL_EXTRA_RRECOMMENDS += "kernel-module-fb-ili9341"

KERNEL_MODULE_AUTOLOAD += "fb_ili9341"

INHERIT += "externalsrc"

IMAGE_FSTYPES = "wic"

EOF

echo "configuration complete"

echo "start build image"

bitbake core-image-minimal

