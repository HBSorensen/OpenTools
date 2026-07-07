# Building the OpenTools G-code Sender Image

This produces a slim, appliance-style Linux image for a **Raspberry Pi 3** that boots
straight into the **gsender** touch UI (Qt6 on eglfs/KMS) and streams `.nc` files to a
**grblHAL** controller over **USB serial**.

- **Yocto**: scarthgap (5.0 LTS)
- **Machine**: `raspberrypi3` (32-bit armv7)
- **Display**: official Raspberry Pi 7" DSI panel (800×480, capacitive touch)
- **Rootfs**: read-only; user data on a writable `/data` partition + USB automount

---

## 1. Prerequisites

Yocto builds require a **Linux** host. On Windows, use **WSL2** (Ubuntu) or a Linux VM/box.

Recommended path — the containerised **kas** tool (no host packages to install beyond
Docker + git):

```bash
# On the Linux host / WSL2:
sudo apt-get update
sudo apt-get install -y docker.io git
pip install --user kas        # or: pipx install kas
```

Make sure your user can run Docker (`sudo usermod -aG docker $USER`, then re-login).

Budget: **~60–90 GB** disk and **2–6 hours** for the first build (subsequent builds are
minutes thanks to sstate).

> WSL2 note: build inside the Linux filesystem (e.g. `~/opentools`), **not** under
> `/mnt/c`. The Windows drive lacks the permissions/case-sensitivity Yocto needs.

---

## 2. Build

Clone the repo on the Linux host and build with kas-container from the repo root:

```bash
git clone <this-repo> opentools && cd opentools
kas-container build Software/yocto/kas/opentools-rpi3.yml
```

If you installed kas directly (not the container):

```bash
kas build Software/yocto/kas/opentools-rpi3.yml
```

### Alternative: classic bitbake setup (kas-free)

`setup-yocto.sh` installs host dependencies, clones poky + all meta-layers at
scarthgap into a folder you choose, and configures a ready-to-build tree:

```bash
bash Software/yocto/scripts/setup-yocto.sh ~/opentools-yocto
cd ~/opentools-yocto
source layers/poky/oe-init-build-env build
bitbake opentools-image
```

(Run `bash Software/yocto/scripts/setup-yocto.sh --help` for options such as
`--skip-deps`, `--full-clone`, and `--release`.)

Output image (either path):

```
build/tmp/deploy/images/raspberrypi3/opentools-image-raspberrypi3.wic.bz2
build/tmp/deploy/images/raspberrypi3/opentools-image-raspberrypi3.wic.bmap
```

---

## 3. Flash

**Raspberry Pi Imager**: choose "Use custom", select the `.wic.bz2`, write to the SD card.

**bmaptool** (fastest, verifies as it writes):

```bash
bmaptool copy \
  build/tmp/deploy/images/raspberrypi3/opentools-image-raspberrypi3.wic.bz2 \
  /dev/sdX          # <-- your SD card device, double-check this!
```

**dd** (fallback):

```bash
bzcat opentools-image-raspberrypi3.wic.bz2 | sudo dd of=/dev/sdX bs=4M conv=fsync
```

---

## 4. First boot

1. Connect the official 7" DSI panel and insert the SD card.
2. Connect the grblHAL controller to a **USB** port of the Pi.
3. Power on. After a few seconds the **gsender** UI appears full-screen — no console,
   no desktop.
4. On the **Connect** tab, pick the port (usually `ttyACM0`) and tap **Connect**.
5. On the **Files** tab, pick a `.nc` file (from `/data` or a plugged-in USB stick).
6. *(Optional)* On the **Probe** tab, set up probing and auto-level (below).
7. On the **Run** tab, tap **Start**. Use **Hold/Resume/Stop** and the feed override
   as needed.

### Probing & auto-level (Probe tab)

Wire your probe input to the grblHAL probe pin (touch plate + clip is typical).

1. Jog the tool over the workpiece origin. Tap **Origin = current XY**.
2. Set **Plate (mm)** to your touch-plate thickness and tap **Probe Z** — the tool
   probes down, sets work Z, and retracts. (This also establishes the height-map
   reference: correction is zero at the origin.)
3. Choose the grid **Width/Height**, **Columns/Rows**, feed, depth and clearance, then
   tap **Probe grid**. Watch progress and the live heatmap.
4. Flip **Auto-level** on — the loaded program is rewritten to follow the surface. The
   Run tab shows "Auto-level ON". Start as usual.

All inputs are preset ComboBoxes, so no keyboard is required. Programs in relative (G91)
or inch (G20) mode are streamed unmodified (a note appears in the Connect console).

### PCB isolation milling & double-sided alignment (PCB / Align tabs)

Turn KiCad Gerbers into isolation G-code and register a two-sided board with the camera.

1. **PCB tab → Scan folder**: point it at your KiCad Gerber export folder. It auto-detects
   `*-F_Cu`, `*-B_Cu`, and the drill `.drl`. Use **Align .drl** to pick the 3-hole
   alignment drill file.
2. Set the V-bit (width / depth / tip angle), passes (≥2), overlap, feeds. Tap
   **Top isolation** → writes `pcb_top.nc` to `/data`. Optionally **Drill (top)**.
3. Load `pcb_top.nc` on the Files tab and run it; drill the 3 alignment holes.
4. **Flip the board** (about its vertical centre). Tap **Bottom isolation** →
   `pcb_bottom.nc` (mirrored).
5. **Align tab → Open camera**.
   - **Fisheye calibration (one-time)**: set the chessboard **Cols/Rows** (inner corners)
     and square size, place the chessboard under the camera, and tap **Capture board** from
     several angles/positions (≥6). Then tap **Calibrate** — it runs `cv::fisheye::calibrate`,
     shows the RMS error, saves to `/data/camera_calib.yml`, and enables **Undistort**.
     Subsequent boots reload it automatically; detection then runs on undistorted frames.
   - Set **mm/px** and the **X/Y flip** to match your camera mounting.
6. Tap **Start alignment**: the machine jogs to each expected hole, the preview shows
   detection, it centers and records the real position. On success it writes
   `pcb_bottom_aligned.nc` to `/data`.
7. Run `pcb_bottom_aligned.nc`.

> **Footprint**: OpenCV adds tens of MB to the image (accepted trade-off for on-device
> vision). The camera must be a UVC USB device (`/dev/video0`); the `kiosk` user is already
> in the `video` group. If `find_package(OpenCV)` fails at build, confirm `opencv` is in
> `IMAGE_INSTALL` and the meta-oe layer is present. If the image build errors on
> `kernel-module-uvcvideo` (UVC compiled built-in rather than as a module in your kernel
> config), just remove that line from `opentools-image.bb` — the camera still works.

> The Gerber parser targets common KiCad output (C/R/O/P apertures, regions, arcs). It does
> **not** handle aperture macros (`%AM`) or step-repeat; those flashes are skipped with a
> warning in the PCB log. Verify generated `.nc` before cutting.

Loading programs:
- **USB stick**: plug it in — partitions auto-mount under `/run/media/*` and show up on
  the Files tab (tap **Refresh** if needed).
- **On the SD card**: copy files into the `/data` partition (label `data`), readable on
  a PC from the third partition of the card.

---

## 5. Troubleshooting

**Black screen / backlight off on the 7" DSI panel.** This is a known full-KMS quirk on
some builds. Edit `Software/yocto/meta-opentools/conf/machine/include/opentools-rpi.inc`
and switch *both* overlay lines to the fake-KMS variants, then rebuild:

```
dtoverlay=vc4-fkms-v3d
dtoverlay=vc4-fkms-dsi-7inch
```

Never mix `kms` and `fkms`. (You can also edit `config.txt` directly on the SD card's
boot partition to test without a rebuild.)

**Touch not working.** The panel's `edt-ft5406` touch comes in via the DSI overlay above.
Confirm `/dev/input/event*` exists and that Qt uses libinput (the service sets the eglfs
env). `evtest` (add to a dev image) helps.

**App can't open the serial port.** grblHAL usually enumerates as `ttyACM0`. Check it
exists (`ls /dev/ttyACM*`). The `kiosk` user is in the `dialout` group already.

**A Qt/QML package name errors during build** (e.g. `qtdeclarative-qmlplugins`). Package
splitting can vary slightly between meta-qt6 revisions. Run
`bitbake -e qtdeclarative | grep ^PACKAGES=` to see the exact subpackage names and adjust
`IMAGE_INSTALL` in `opentools-image.bb` / `RDEPENDS` in `gsender_1.0.bb`.

**Get a shell for debugging.** The shipping image has no SSH/getty. For a dev build, add
to `local.conf` (or a dev kas fragment):

```
EXTRA_IMAGE_FEATURES += "debug-tweaks ssh-server-dropbear"
```

and comment out the `getty@tty1` mask in `gsender_1.0.bb` if you need a console.

---

## 6. Sanity checks before a long build

```bash
kas dump Software/yocto/kas/opentools-rpi3.yml            # resolved config
bitbake -e opentools-image | grep -E '^DISTRO_FEATURES='  # no x11/wayland
bitbake -e qtbase | grep -E '^PACKAGECONFIG='             # contains eglfs kms gbm
```
