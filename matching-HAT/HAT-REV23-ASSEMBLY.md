# Seizo Screen: matching HAT variant

This is a new PCB revision. **Do not install SN74LVC1G17 in the U3 position on the original AP2112 HAT PCB.** The package is the same size, but the pin wiring is different. Use this variant's matching schematic, PCB and manufacturing exports together. The original HAT files remain preserved.

## Power connections

The former AP2112 regulator is removed. U3 is now an SN74LVC1G17DBVR signal buffer, not a voltage regulator. The screen carrier's Adafruit 4683 3.3 V converter supplies HAT `BOARD_3V3` through a dedicated 22 AWG pair: **J10 pin 1 = GND; J10 pin 2 = BOARD_3V3**. Never connect another 3.3 V regulator output in parallel with this supply.

The Pi's own `+3V3` rail stays separate. It powers the receiver buffer for the Pi UART input. Grounds remain shared. Existing Pi 5 V power connections stay in place. C7, the old AP2112 input bypass capacitor, is DNP and absent from this variant's PCB; its obsolete Pi 5 V branch has been removed. U3 instead has its own new 100 nF bypass, C9, on `BOARD_3V3`.

## Communication and diagnostic wires

|Connection|HAT connection|Meaning|
|---|---|---|
|Pi physical pin 8|Existing HAT Pi header, U4 receiver path|Pi TX to ESP32-S3 GPIO44 RX|
|Pi physical pin 10|Existing HAT Pi header, U5 receiver path|ESP32-S3 GPIO43 TX to Pi RX|
|Panel J12 pin 1|HAT J4 pin 3|LCD logic-rail indication, ESP32-S3 GPIO6|
|Panel J12 pin 2|HAT J4 pin 4|Active-low prototype-power fault, ESP32-S3 GPIO7|
|USB-C VBUS|Existing HAT USB connector and new U3|Noninverted VBUS indication to ESP32-S3 GPIO39|

GPIO6/7 above are **ESP32-S3 GPIO numbers**, not Raspberry Pi GPIO numbers. They are now reserved for these two diagnostic inputs. R22/R23 provide 100 kΩ pulldowns, so a disconnected diagnostic wire reads absent/fault. Connect the two boards with all power off; this design does not certify arbitrary hot plugging.

The panel's MCP23017 bit 7 pins remain unused outputs low: `IODIRA = IODIRB = 0x7F`. Current Microchip documentation makes GPA7 and GPB7 output-only. The two diagnostic signals therefore use the ESP32 inputs above rather than those expander pins.

## Exact added parts

|References|Part|Stock KiCad footprint|
|---|---|---|
|U3, U4, U5|SN74LVC1G17DBVR|Package_TO_SOT_SMD:SOT-23-5|
|C9, C10, C11|KEMET C0805C104K5RACTU, 100 nF, 50 V, X7R|Capacitor_SMD:C_0805_2012Metric|
|R16, R17, R20, R21, R22, R23|Yageo RC0805FR-07100KL, 100 kΩ, 1%|Resistor_SMD:R_0805_2012Metric|
|R18, R19|Yageo RC0805FR-0733RL, 33 Ω, 1%|Resistor_SMD:R_0805_2012Metric|

R11 is retained as a 100 kΩ USB VBUS pulldown. R10, the old divider resistor, is removed. C7 is DNP/off-board. No changes to the original servo supply connections, driver signals or ESP32 module are intended.

U3/U4/U5 all use pin 1 NC, pin 2 A, pin 3 GND, pin 4 Y and pin 5 VCC. U3/U4 VCC is `BOARD_3V3`; U5 VCC is the actual Pi `+3V3` rail. R16/R17 pull UART inputs up to their source supply; R20/R21 pull receiver-side UART inputs up to the receiver supply. Outputs are noninverted.

## Release limits

Use the final manifest for the exact native DRC, connectivity, schematic parity and file hashes. CAD checks do not constitute powered hardware tests. After assembly, the board still requires inspection, current-limited power-up, rail and signal measurements, and communication testing before use on an arm. The arm's independent stopping and gravity-retention system is separate; these diagnostic and UART circuits do not implement it.

The TI buffer supports 1.65–5.5 V operation, inputs to 5.5 V and specified partial-power-down protection. Its specified Ioff is finite, not galvanic isolation, and behavior below the operating supply range is not a functional guarantee. The expected normal partial-power cases must still be checked on the assembled boards.

Sources: [TI SN74LVC1G17](https://www.ti.com/lit/ds/symlink/sn74lvc1g17.pdf), [Microchip MCP23017](https://ww1.microchip.com/downloads/aemDocuments/documents/APID/ProductDocuments/DataSheets/MCP23017-Data-Sheet-DS20001952.pdf), [KEMET capacitor](https://search.kemet.com/component-documentation/download/specsheet/C0805C104K5RACTU), [Yageo resistors](https://yageogroup.com/content/datasheet/asset/file/PYU-RC_GROUP_51_ROHS_L).
