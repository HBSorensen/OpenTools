#!/usr/bin/env bash
#
# setup-yocto.sh — one-shot setup of the OpenTools Yocto build tree.
#
# Installs host build dependencies, clones poky + all required meta-layers at the
# scarthgap (5.0 LTS) release into a folder you provide, and configures a build
# directory (bblayers.conf + local.conf) ready to `bitbake opentools-image`.
#
# This is the classic, kas-free path. (The kas equivalent is a single
# `kas-container build Software/yocto/kas/opentools-rpi3.yml` — see docs/BUILD.md.)
#
# Usage:
#   bash Software/yocto/scripts/setup-yocto.sh <target-dir> [options]
#
# Options:
#   --skip-deps     Do not attempt to install host packages (apt).
#   --full-clone    Full git history instead of shallow (--depth 1) clones.
#   --release NAME  Yocto release branch to use (default: scarthgap).
#   -h, --help      Show this help.
#
# Example:
#   bash Software/yocto/scripts/setup-yocto.sh ~/opentools-yocto
#   cd ~/opentools-yocto && source layers/poky/oe-init-build-env build
#   bitbake opentools-image
#
set -euo pipefail

# --- Locate the repo root (this script lives in Software/yocto/scripts) --------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
META_OPENTOOLS="$REPO_ROOT/Software/yocto/meta-opentools"

# --- Defaults / arg parsing ----------------------------------------------------
RELEASE="scarthgap"
SKIP_DEPS=0
CLONE_DEPTH="--depth 1 --single-branch"
TARGET=""

log()  { printf '\033[1;34m==>\033[0m %s\n' "$*"; }
warn() { printf '\033[1;33m[!]\033[0m %s\n' "$*" >&2; }
die()  { printf '\033[1;31m[x]\033[0m %s\n' "$*" >&2; exit 1; }

usage() { sed -n '2,30p' "$0" | sed 's/^# \{0,1\}//'; exit 0; }

while [ $# -gt 0 ]; do
    case "$1" in
        --skip-deps)   SKIP_DEPS=1 ;;
        --full-clone)  CLONE_DEPTH="" ;;
        --release)     RELEASE="${2:?--release needs a value}"; shift ;;
        -h|--help)     usage ;;
        -*)            die "Unknown option: $1" ;;
        *)             [ -z "$TARGET" ] && TARGET="$1" || die "Unexpected arg: $1" ;;
    esac
    shift
done

[ -n "$TARGET" ] || die "No target directory given. See --help."

# meta-qt6 tracks Yocto release names as branches (scarthgap, styhead, ...).
declare -A REPOS=(
    [poky]="https://git.yoctoproject.org/poky"
    [meta-openembedded]="https://git.openembedded.org/meta-openembedded"
    [meta-raspberrypi]="https://git.yoctoproject.org/meta-raspberrypi"
    [meta-qt6]="https://code.qt.io/yocto/meta-qt6.git"
)

# --- Sanity checks -------------------------------------------------------------
[ -d "$META_OPENTOOLS" ] || die "Cannot find meta-opentools at $META_OPENTOOLS (run from the repo)."
case "$(uname -s)" in
    Linux) ;;
    *) die "Yocto builds require Linux. On Windows use WSL2 (Ubuntu) and run this there." ;;
esac
command -v git >/dev/null 2>&1 || die "git is required."

mkdir -p "$TARGET"
TARGET="$(cd "$TARGET" && pwd)"
LAYERS="$TARGET/layers"
mkdir -p "$LAYERS"

# --- 1. Host dependencies ------------------------------------------------------
install_host_deps() {
    if [ "$SKIP_DEPS" -eq 1 ]; then
        log "Skipping host dependency install (--skip-deps)."
        return
    fi
    if ! command -v apt-get >/dev/null 2>&1; then
        warn "Non-apt host detected. Install the Yocto host packages manually:"
        warn "  https://docs.yoctoproject.org/ref-manual/system-requirements.html"
        return
    fi
    log "Installing host build dependencies (sudo apt-get)…"
    local pkgs=(
        gawk wget git diffstat unzip texinfo gcc build-essential chrpath socat
        cpio python3 python3-pip python3-pexpect xz-utils debianutils iputils-ping
        python3-git python3-jinja2 python3-subunit zstd liblz4-tool file locales
        libacl1 bmap-tools
    )
    sudo apt-get update -y
    sudo apt-get install -y "${pkgs[@]}"
    # Yocto requires a UTF-8 locale.
    if ! locale -a 2>/dev/null | grep -qiE 'en_US\.utf-?8'; then
        log "Generating en_US.UTF-8 locale…"
        sudo locale-gen en_US.UTF-8 || warn "Could not generate locale; set LANG manually."
    fi
}

# --- 2. Clone / update the layers ----------------------------------------------
clone_repo() {
    local name="$1" url="$2" dir="$LAYERS/$name"
    if [ -d "$dir/.git" ]; then
        log "Updating $name ($RELEASE)…"
        git -C "$dir" fetch origin "$RELEASE" --tags
        git -C "$dir" checkout "$RELEASE"
        git -C "$dir" pull --ff-only origin "$RELEASE" || warn "$name: could not fast-forward (shallow?)."
    else
        log "Cloning $name ($RELEASE)…"
        # shellcheck disable=SC2086
        git clone $CLONE_DEPTH -b "$RELEASE" "$url" "$dir"
    fi
}

fetch_layers() {
    for name in poky meta-openembedded meta-raspberrypi meta-qt6; do
        clone_repo "$name" "${REPOS[$name]}"
    done
}

# --- 3. Configure the build directory ------------------------------------------
configure_build() {
    local build="$TARGET/build"
    log "Initialising build env at $build…"
    # oe-init-build-env creates conf/{local.conf,bblayers.conf} from templates and
    # cd's into the build dir. Run in a subshell so our CWD is unaffected.
    (
        set +u
        source "$LAYERS/poky/oe-init-build-env" "$build" >/dev/null
        set -u
    )

    log "Writing bblayers.conf…"
    cat > "$build/conf/bblayers.conf" <<EOF
# Generated by setup-yocto.sh — OpenTools ($RELEASE)
POKY_BBLAYERS_CONF_VERSION = "2"
BBPATH = "\${TOPDIR}"
BBFILES ?= ""
BBLAYERS ?= " \\
  $LAYERS/poky/meta \\
  $LAYERS/poky/meta-poky \\
  $LAYERS/meta-openembedded/meta-oe \\
  $LAYERS/meta-openembedded/meta-python \\
  $LAYERS/meta-openembedded/meta-networking \\
  $LAYERS/meta-openembedded/meta-multimedia \\
  $LAYERS/meta-raspberrypi \\
  $LAYERS/meta-qt6 \\
  $META_OPENTOOLS \\
  "
EOF

    log "Appending OpenTools settings to local.conf…"
    # Idempotent: only append our block once.
    if ! grep -q "OpenTools appliance settings" "$build/conf/local.conf"; then
        cat >> "$build/conf/local.conf" <<EOF

# --- OpenTools appliance settings ---
MACHINE = "raspberrypi3"
DISTRO = "opentools"
require conf/machine/include/opentools-rpi.inc
IMAGE_FSTYPES = "wic.bz2 wic.bmap"
PACKAGE_CLASSES = "package_ipk"
# Share downloads / sstate outside the build dir so rebuilds are fast.
DL_DIR ?= "$TARGET/downloads"
SSTATE_DIR ?= "$TARGET/sstate-cache"
EOF
    fi
}

# --- Run -----------------------------------------------------------------------
install_host_deps
fetch_layers
configure_build

cat <<EOF

$(log "Done.")
Yocto tree is ready in: $TARGET

Next steps:
  cd "$TARGET"
  source layers/poky/oe-init-build-env build
  bitbake opentools-image

Result: build/tmp/deploy/images/raspberrypi3/opentools-image-raspberrypi3.wic.bz2
Flashing and troubleshooting: Software/docs/BUILD.md
EOF
