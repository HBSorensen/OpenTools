# Machine tuning for the OpenTools appliance on a Raspberry Pi 3.
#
# Included from the kas local_conf_header so it applies on top of the stock
# `raspberrypi3` machine without forking a whole machine definition.

# --- Display: official Raspberry Pi 7" DSI panel (800x480) via full KMS -------
# vc4-kms-v3d       : full KMS driver -> Mesa vc4 GLESv2 (what Qt eglfs uses)
# vc4-kms-dsi-7inch : the official 7" panel + edt-ft5406 capacitive touch
#
# NOTE: if the panel comes up black with the backlight off (a known KMS-DSI
# quirk in some builds), switch BOTH lines to the fake-KMS variants:
#     dtoverlay=vc4-fkms-v3d
#     dtoverlay=vc4-fkms-dsi-7inch
# Never mix kms and fkms. See Software/docs/BUILD.md.
RPI_EXTRA_CONFIG:append = '\n\
# --- OpenTools display ---\n\
dtoverlay=vc4-kms-v3d\n\
dtoverlay=vc4-kms-dsi-7inch\n\
disable_overscan=1\n\
# Quiet, fast boot for an appliance\n\
disable_splash=1\n\
boot_delay=0\n\
'

# We talk to grblHAL over USB (CDC-ACM), not the GPIO UART. Leaving the UART
# off keeps the PL011 free and shaves a little boot time.
ENABLE_UART = "0"

# Quiet kernel console for a clean appliance boot.
CMDLINE:append = " quiet logo.nologo vt.global_cursor_default=0"

# Custom partitioning: boot + read-only rootfs + writable /data.
WKS_FILE = "opentools-rpi.wks"
