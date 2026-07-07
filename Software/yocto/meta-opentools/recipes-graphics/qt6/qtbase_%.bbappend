# Enable Qt's eglfs KMS backend so QML apps render directly on the GPU/DRM with
# no X11 or Wayland compositor — the key to a slim, no-display-server appliance.
#
#   eglfs : full-screen EGL platform plugin
#   kms   : eglfs integration over DRM/KMS (eglfs_kms)
#   gbm   : buffer allocation for KMS
#
# xcb/wayland are already off because the distro drops those DISTRO_FEATURES.

PACKAGECONFIG:append = " eglfs kms gbm"
