SUMMARY = "OpenTools touch G-code sender for grblHAL"
DESCRIPTION = "A Qt6/QML kiosk application, sized for an 800x480 touchscreen, that \
streams .nc G-code files to a grblHAL controller over USB serial. Launched at boot \
as a full-screen eglfs kiosk."
HOMEPAGE = "https://foss.dk"
# In-house application. Set to CLOSED so the recipe builds without a license
# checksum; switch to "MIT" + LIC_FILES_CHKSUM if/when you publish the app.
LICENSE = "CLOSED"

# Build the application straight from the in-repo source tree (Software/app/gsender)
# and pull the launch/config assets from this recipe's files/ directory.
FILESEXTRAPATHS:prepend := "${THISDIR}/../../../../app:"
SRC_URI = "\
    file://gsender \
    file://gsender.service \
    file://data.mount \
    file://usb-mount@.service \
    file://usb-mount.sh \
    file://99-opentools-usb-automount.rules \
"

S = "${WORKDIR}/gsender"

# opencv provides camera capture + hole detection for double-sided alignment.
DEPENDS = "qtbase qtdeclarative qtserialport opencv"
RDEPENDS:${PN} = "qtbase qtdeclarative qtdeclarative-qmlplugins qtserialport opencv"

inherit qt6-cmake systemd useradd

# --- Dedicated unprivileged kiosk user ---------------------------------------
# video   : DRM/KMS card node access for eglfs
# input   : evdev/libinput touch access
# dialout : /dev/ttyACM* serial access to grblHAL
# tty     : access the VT eglfs runs on
USERADD_PACKAGES = "${PN}"
GROUPADD_PARAM:${PN} = "-r kiosk"
USERADD_PARAM:${PN} = "-r -m -d /home/kiosk -s /sbin/nologin \
                       -g kiosk -G video,input,dialout,tty kiosk"

# --- systemd units -----------------------------------------------------------
SYSTEMD_SERVICE:${PN} = "gsender.service data.mount"
SYSTEMD_AUTO_ENABLE = "enable"

do_install:append() {
    # Kiosk launcher + writable /data mount + USB stick automount.
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/gsender.service      ${D}${systemd_system_unitdir}/
    install -m 0644 ${WORKDIR}/data.mount           ${D}${systemd_system_unitdir}/
    install -m 0644 ${WORKDIR}/usb-mount@.service   ${D}${systemd_system_unitdir}/

    install -d ${D}${bindir}
    install -m 0755 ${WORKDIR}/usb-mount.sh          ${D}${bindir}/usb-mount

    install -d ${D}${nonarch_base_libdir}/udev/rules.d
    install -m 0644 ${WORKDIR}/99-opentools-usb-automount.rules \
        ${D}${nonarch_base_libdir}/udev/rules.d/

    # Mount point for the writable data partition (must exist on the ro rootfs).
    install -d ${D}/data

    # Nothing else should grab DRM master on tty1 — mask the console getty.
    install -d ${D}${sysconfdir}/systemd/system
    ln -sf /dev/null ${D}${sysconfdir}/systemd/system/getty@tty1.service
}

FILES:${PN} += "\
    ${systemd_system_unitdir} \
    ${nonarch_base_libdir}/udev/rules.d \
    /data \
    ${sysconfdir}/systemd/system/getty@tty1.service \
"

# /data is a runtime mount point; don't complain that it's empty.
INSANE_SKIP:${PN} += "empty-dirs"
