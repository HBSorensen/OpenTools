SUMMARY = "OpenTools slim appliance image — boots straight into the gsender touch UI"
DESCRIPTION = "Minimal Raspberry Pi 3 image that starts a Qt6/eglfs G-code sender \
(gsender) on the 7\" DSI touchscreen and streams .nc files to a grblHAL controller \
over USB serial. Read-only rootfs, no display server, no package manager."
LICENSE = "MIT"

inherit core-image

# --- Slimness knobs ----------------------------------------------------------
# read-only-rootfs: rootfs mounts ro; all writable state lives on /data + tmpfs.
IMAGE_FEATURES += "read-only-rootfs"
# Strip the package manager from the shipping image (dev builds can re-add it).
IMAGE_FEATURES:remove = "package-management"

# Keep the base tiny; we add only what the appliance needs.
IMAGE_INSTALL = "\
    packagegroup-core-boot \
    ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'systemd-vconsole-setup', '', d)} \
    \
    gsender \
    \
    qtbase \
    qtdeclarative \
    qtdeclarative-qmlplugins \
    qtserialport \
    \
    mesa \
    mesa-megadriver \
    libinput \
    \
    fontconfig \
    ttf-dejavu-sans \
    \
    kernel-module-cdc-acm \
    kernel-module-ftdi-sio \
    kernel-module-usbserial \
    \
    e2fsprogs-mke2fs \
    dosfstools \
    \
    opencv \
    kernel-module-uvcvideo \
"

# No locale bloat, no dev tooling in the shipped image.
IMAGE_LINGUAS = ""

# Deploy a flashable SD image + a bmap for fast flashing.
IMAGE_FSTYPES = "wic.bz2 wic.bmap"

# Give the read-only rootfs a little slack for the QML plugin tree + fonts.
IMAGE_OVERHEAD_FACTOR = "1.2"
IMAGE_ROOTFS_EXTRA_SPACE = "24576"
