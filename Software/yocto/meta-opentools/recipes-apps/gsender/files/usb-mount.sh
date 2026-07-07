#!/bin/sh
# Minimal USB mass-storage automounter for the OpenTools appliance.
# Mounts partitions read-only under /run/media/<dev> (tmpfs, so the read-only
# rootfs is never touched). Invoked by usb-mount@.service from a udev rule.

ACTION="$1"    # add | remove
DEV="$2"       # e.g. sda1
MNT="/run/media/${DEV}"

case "$ACTION" in
  add)
    [ -b "/dev/${DEV}" ] || exit 0
    mkdir -p "$MNT"
    # Read-only, no exec/suid; try common removable-media filesystems.
    if ! mount -o ro,noatime,nosuid,nodev,noexec "/dev/${DEV}" "$MNT" 2>/dev/null; then
        rmdir "$MNT" 2>/dev/null
        exit 1
    fi
    ;;
  remove)
    if mountpoint -q "$MNT"; then
        umount -l "$MNT"
    fi
    rmdir "$MNT" 2>/dev/null
    ;;
  *)
    echo "usage: usb-mount {add|remove} <device>" >&2
    exit 1
    ;;
esac
