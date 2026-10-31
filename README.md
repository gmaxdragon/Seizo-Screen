# Seizo Screen

![Seizo Screen PCB in KiCad's 3D viewer](images/pcb-3d.png)

![Seizo Screen routed PCB layout](images/pcb-layout.png)

## Files

- [KiCad project](szscreen.kicad_pro), [schematic](szscreen.kicad_sch) and [PCB](szscreen.kicad_pcb)
- [Schematic PDF](schematic.pdf)
- [Parts list](BOM-panel.csv) and [connector pin map](CONNECTOR-PIN-MAP.csv)
- [Panel fabrication ZIP](szscreen-panel-Gerbers.zip)
- [Matching Pi HAT revision](matching-HAT/) and [assembly connections](ASSEMBLY-CONNECTIONS.txt)
- [Reference firmware](reference/firmware/) and [verification report](VERIFICATION.json)

## Contributions

The original schematic, PCB placement and full earlier routing were made by Vivaan, with his friend helping with ideas, parts, organization, routing and troubleshooting. ChatGPT/Codex helped with later schematic and PCB revisions, component and footprint selection, routing, checks and technical documentation. It generated the reference firmware from Vivaan's detailed behavior requirements. That code is reference only; Vivaan plans to write the actual implementation. The original earlier routing is preserved in [history/user-routed-rev19](history/user-routed-rev19/).
