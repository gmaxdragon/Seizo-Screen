# Assembly options — quote planning only

The current revision is unchanged electrically. No factory assembly order or delivered quote has been placed. The existing parts estimate is $99.768 before fabrication, all assembly charges, delivery/tax, unpriced harness/mounting and the independent stop/retention hardware.

The mixed through-hole/SMT turnkey quotation package is now in [assembly](assembly/). It includes per-board panel and full matching-HAT BOMs, placements, paste layers, front/rear drawings and special-process instructions. The full matching-HAT BOM contains 48 fitted references; the earlier $1.53 added-parts list was only a delta. Factory acceptance, sourcing, quantity and actual delivered cost remain pending. No genuine cart screenshots were obtainable because browser automation failed to initialize.

## Most useful cost reduction

The current 26-piece 100nF capacitor group costs $12.688. A researched surface-mount alternative is KEMET C1206C104K5RACTU, 100nF, 50V, X7R, 10%, in stock KiCad footprint Capacitor_SMD:C_1206_3216Metric_Pad1.33x1.80mm_HandSolder. The distributor's retrieved price-break snapshot is $0.085 each at ten pieces, or $2.210 for 26: a $10.478 component saving before assembly/sourcing fees. This is not a factory price or checkout quote. All 26 placements and their routing would require a new review, including moving the present rear C25 to the front if practical. No replacement has been made in this board or BOM.

[Distributor source](https://www.digikey.com/en/products/detail/kemet/C1206C104K5RACTU/411248), [manufacturer specification](https://search.kemet.com/component-documentation/download/specsheet/C1206C104K5RACTU).

## Assembly process to quote

For a factory-oriented revision, convert ordinary resistors/capacitors to commonly stocked surface-mount parts and aim for placement on one side. Retain the exact meter-divider tolerance, protection functions and buffer electrical specifications. Ask the assembler whether one-off manual placement of C25 and installation of the Adafruit4683 module are supported before assuming a cheaper assembly tier. The module is not just the four-pin header represented by its carrier footprint. Display mounting, cable installation and the actual LCD logic-rail tap remain separate from populated-board assembly unless expressly included in the quote.

JLCPCB publishes a minimum of two assembled boards. Its Economic service is single-sided component placement; Standard supports double-sided placement. The current panel has one rear component. Two populated boards can duplicate much of the parts bill, so a small per-board parts saving alone does not establish a complete sub-$100 build. See [current capabilities](https://jlcpcb.com/capabilities/pcb-assembly-capabilities).

PCBWay supports mixed through-hole/surface-mount and double-sided assembly. Its capability page lists five pieces while its public examples show smaller orders; obtain explicit confirmation of the accepted quantity and total delivered price rather than assuming a one-board order. See [capabilities](https://www.pcbway.com/assembly-capabilities.html).

Reusing the owned USB hub could avoid a $23.05 new hub/supply purchase only if its actual power and simultaneous-load checks pass. That condition is unverified; do not remove a needed powered hub from the whole-system budget just to produce a lower total.

Decision: prepare an assembler-specific parts list, positions, drawings and special assembly instructions, obtain a full delivered quotation, then decide whether a surface-mount revision or reduced optional features is necessary. No full-assembly-under-$100 claim follows from the current estimate.
