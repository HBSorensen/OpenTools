# OpenTools Software

Embedded Linux appliance for the OpenTools CNC front-end: a **Raspberry Pi 3** that boots
directly into a **touch G-code sender** and streams `.nc` files to a **grblHAL**
controller.

## What's here

```
Software/
├── yocto/                     Yocto (scarthgap) image definition
│   ├── kas/opentools-rpi3.yml   one-command reproducible build (layers pinned)
│   └── meta-opentools/          custom layer: slim distro, image, app recipe, boot config
├── app/gsender/               the Qt6/QML touch application (built by the recipe)
│   ├── src/                     C++ backend: serial streamer, probing, height map
│   │   ├── pcb/                  Gerber/Excellon parsers, Clipper2 CAM, PcbCam
│   │   └── vision/              OpenCV camera fiducial alignment
│   ├── third_party/clipper2/    vendored polygon offsetting library (Boost licence)
│   └── qml/                     800×480 touch UI (Connect/Files/Jog/Probe/PCB/Align/Run)
└── docs/BUILD.md              build, flash, first-boot, troubleshooting
```

## Design at a glance

| Concern            | Choice                                                              |
|--------------------|--------------------------------------------------------------------|
| Slimness           | `core-image-minimal` base, **read-only rootfs**, no package manager |
| Graphics           | Qt6 on **eglfs/KMS** — no X11, no Wayland, no compositor            |
| Display            | Official 7" DSI panel, **800×480**, capacitive touch                |
| Controller link    | **USB serial** (`/dev/ttyACM0`), 115200 8-N-1, GRBL 1.1 protocol    |
| Boot behaviour     | systemd launches `gsender` full-screen as an unprivileged kiosk     |
| User data          | writable `/data` partition + USB stick automount under `/run/media` |
| Yocto / machine    | scarthgap (5.0 LTS) / `raspberrypi3` (32-bit)                       |

## The app (gsender)

- **`GrblController`** (C++/QtSerialPort): implements the standard GRBL **character-
  counting streaming** protocol, ~5 Hz status polling, real-time control (feed
  hold/resume, soft reset, feed override), jog/home/unlock/zero commands, and the full
  **probing** workflow (see below).
- **`HeightMap`**: grid of probed surface heights with bilinear interpolation, plus the
  **auto-level G-code transform** (subdivides G1 moves and offsets Z to follow the surface).
- **`NcFileModel`**: lists `.nc/.gcode/.ngc/.tap` programs from `/data` and USB sticks.
- **UI**: seven touch tabs — **Connect**, **Files**, **Jog**, **Probe**, **PCB**, **Align**,
  **Run** — with a persistent position/state readout. Keyboard-free (preset ComboBoxes +
  touch file/folder dialogs, no on-screen keyboard needed).

### Automatic probing & auto-correct (Probe tab)

- **Z touch-plate probe** — `G38.2` down to a plate, then sets work Z (accounting for
  plate thickness) and retracts.
- **Grid height map** — probes a grid over a chosen area with probe points a set
  **distance apart (default 5 mm)**; the grid dimensions are derived from the area. Origin
  is taken from the current position with one tap; probing follows a snake path and parses
  each `[PRB:…]` report into the map.
- **Auto-level correction** — with a map present, toggling **Auto-level** rewrites the
  loaded program so cutting moves follow the measured surface. Each probe cell is
  subdivided into **`interpolations` segments (default 5)** — i.e. 5 mm spacing / 5 = 1 mm
  Z-correction steps. Correction is **normalized to the grid origin** (Z zero is the origin
  point). Relative (G91) / inch (G20) programs are detected and streamed unmodified.
- **Heatmap view** — the probed grid is shown as a colored heatmap with the surface span.

### PCB isolation milling + double-sided alignment (PCB / Align tabs)

On-device CAM that turns **KiCad Gerbers** into isolation-milling G-code and registers a
double-sided board with a camera — no PC needed.

- **Gerber → G-code** (`src/pcb/`): a focused RS-274X parser builds copper polygons;
  **Clipper2** unions them and offsets them into **≥2 V-bit isolation passes** (FlatCAM
  algorithm: first pass at `width/2`, then step `width·(1−overlap)`). V-bit effective width
  is taken from **width + depth + tip angle**. Excellon `.drl` → drilling G-code.
- **Double-sided**: `*-F_Cu` / `*-B_Cu` auto-detected by KiCad naming; the bottom is
  **mirrored** about the board-centre X for milling from the top after a flip.
- **Camera alignment** (`src/vision/`, OpenCV): the **spindle-mounted USB camera** and the
  **3 alignment holes** (separate `.drl`) register the flipped board. The app **auto-jogs to
  each expected (mirrored) hole, detects it with HoughCircles, iteratively centers, and
  records the true machine coordinate**; a **rigid transform** (rotation+translation, solved
  over the 3 correspondences) is applied to the bottom program for precise drilling/milling.
- **Footprint note**: this adds **Clipper2** (vendored, small) and **OpenCV** (~tens of MB) —
  a deliberate trade against "slim", chosen so the whole workflow runs on the Pi.
- **Fisheye lens compensation**: a chessboard calibration (`cv::fisheye`) — place the board,
  tap **Capture board** a few times, then **Calibrate**. Parameters (K, D) are saved to
  `/data/camera_calib.yml` and reloaded on boot; hole detection and the preview then run on
  **undistorted** frames, so lens distortion doesn't skew the alignment.
- **Calibration**: the camera also needs an **mm/pixel** scale and X/Y flip signs (Align-tab
  ComboBoxes); tune these on the device against a known hole.

## Build

Yocto needs a **Linux** host (use WSL2 on Windows). From the repo root:

```bash
kas-container build Software/yocto/kas/opentools-rpi3.yml
```

Full instructions, flashing, and troubleshooting: **[docs/BUILD.md](docs/BUILD.md)**.
