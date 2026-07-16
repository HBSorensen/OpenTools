# TestBoard1 generator

The KiCad schematic (`*.kicad_sch`) and PCB (`TestBoard 1.kicad_pcb`) in the parent folder are
generated programmatically from a single shared net-list model, so the schematic and PCB are
consistent by construction (every global label on a pin becomes the matching pad net).

| File | Role |
|------|------|
| `klib.py`  | KiCad 10 s-expression parser/embedder (symbols, footprints, `extends` flattening) |
| `build.py` | `Builder`: net model, label-based schematic emit, PCB emit (stackup, zones, pad nets) |
| `design.py`| The actual design: SoM pin map, all sheets, part choices, and PCB placement |

## Regenerate

```sh
python "generator/design.py"
```

Requires KiCad 10 installed at `C:/Program Files/KiCad/10.0` (for the stock symbol/footprint
libraries) and the sibling `../Library` VAR-SOM library. Paths are set at the top of
`build.py` / `design.py`.

## Validate / render

```sh
CLI="C:/Program Files/KiCad/10.0/bin/kicad-cli.exe"
export KICAD10_3DMODEL_DIR="C:/Program Files/KiCad/10.0/share/kicad/3dmodels"
export VARSOM_LIB="C:/Development/OpenTools/Hardware/VAR-SOM based/Library"
"$CLI" sch erc "TestBoard 1.kicad_sch"          # 0 violations
"$CLI" pcb render --side top  -o r_top.png  "TestBoard 1.kicad_pcb"
"$CLI" pcb render --perspective -o r_persp.png "TestBoard 1.kicad_pcb"
```

## Notes / verify-before-fab

- Connectivity is by **global labels + power symbols** placed on pin endpoints (label-based nets),
  so cross-sheet nets join by name. ERC is clean (0 violations).
- The PCB is **placed and netlisted but not routed** — copper traces still need routing
  (ratsnest shown as unconnected in DRC). Inner layers are GND (In1) / +3V3 (In2) planes;
  zones are defined but must be filled in pcbnew (Edit → Fill All Zones / `B`).
- `lib_footprint_issues` in DRC come from the stock KiCad footprints, not this layout.
- Ethernet PHY (KSZ9031) strap resistors are left at defaults — confirm PHYAD / mode and add
  RGMII series/termination resistors per the datasheet before fabrication.
- SoM `VCC_SOM` is fed from +5V; `VDD_ENET*` from +3V3; module `USBx_VBUS` / `SOM_3V3_PER`
  outputs are left NC (carrier supplies host VBUS from +5V via poly-fuse + ferrite + ESD).
