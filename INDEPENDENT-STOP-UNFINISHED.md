# Seizo independent stop and gravity retention: design decision

Engineering review dated 31 October 2026. This is a technical design note, not a journal entry, assembly approval, certification, or claim of a working emergency stop. No arm motion, power-cut experiment or live electrical test was performed.

## Decision

The screen PCB cannot provide the requested guaranteed stop by itself. Keep the screen, Pi and HAT as the operator interface. Put the real wired stop, reset, motor-energy switching and gravity retention in a separate low-voltage safety subsystem mounted at the stationary arm base. Its stop action must still work with the Pi frozen, the USB hub disconnected and the panel unplugged.

A feasible architecture can be specified now, but selecting and qualifying the brakes and completing the power contacts is blocked by the actual arm assembly/load data and the servo-supply label. The existing unpowered dropping and loose output transmission mean that simply adding a relay, an ENABLE wire, a counterweight or a software delay does not close the problem.

## Facts used, and facts deliberately not substituted

| Item | Established fact | Consequence |
|---|---|---|
| Actual arm | Modified MK2 Plus, about 4 ft full extension; three NEMA17 stepper axes; large PETG parallel gripper | NEMA17 specifies a mounting size, not available holding torque, output ratio or a brake rating |
| Axes | X base rotation, Y reach, Z height; MG90S wrist and MG90S gripper | Separate gravity/output-load paths must be retained |
| Gravity | User reports every non-base part sags or drops with power removed | Cutting motor supplies alone creates a falling-load hazard |
| Transmission | Repo reports inconsistent Y gear mesh and a loose Z driven gear/coupling | A brake on a motor shaft upstream of that failure cannot hold the disconnected output |
| Present electronics | Uno, CNC Shield V3, A4988; Nano currently handles wrist/gripper; future HAT has PCA9685 servo outputs | Current and future servo feeds need the same independent cut-off function |
| Stepper supply | Latest direct user statement: 12 V, 4 A | This supersedes the stale repo's 12 V, 5 A entry |
| Servo supply | Separate supply; repo describes regulated 5 V, but exact current/label is unverified | Final fuse, wire, connector and DC contact ratings cannot be released |
| Intended payload | User now targets half a pound or less, approximately 227 g | A design target, not measured safe capacity |
| Modified gripper | User estimates added servo/camera modifications at no more than about 30 g; not measured | Use the actual complete moving assembly mass, not that estimate alone |
| Arm CAD | A targeted local search found and visually verified an original assembled MK2-style mesh: Downloads/Mk2 Plus Addons (6).stl | Useful original geometry is available; actual print scale, installed revision and custom gripper/camera still need reconciliation |
| Existing USB hub | Seven ports, USB-powered from Pi, no separate power jack | It neither provides an independent motor stop nor supplies this safety subsystem |
| Archived SCARA plan | Explicitly archived/not the active build | Its link lengths, reducers, Z travel, base and 100 g payload are not current dimensions |

Evidence: [Hardware.md](https://github.com/gmaxdragon/seizo/blob/main/Hardware.md), [CONTROL_MAP.md](https://github.com/gmaxdragon/seizo/blob/main/CONTROL_MAP.md), [Y/Z conversion plan](https://github.com/gmaxdragon/seizo/blob/main/docs/YZ_BELT_CONVERSION_PLAN_V1.md), [archived SCARA document](https://github.com/gmaxdragon/seizo/blob/main/docs/SEIZO_V2_SCARA_SPEC_V1.md). Read-only source snapshots are saved alongside this note. Latest direct user statements take priority where noted.

## What the HAT can and cannot do

The reviewed native HAT schematic is `hardware/main/test1.kicad_sch`, GitHub blob `168288f60eef398da83e531a8d681f2465e1708d`.

- Its `Net-(U2-EN)` joins ESP32 U2 pin 3, its reset switch, R3 and C2. This is ESP32 enable/reset, **not an A4988 motor-disable line**.
- No HAT A4988 STEP/DIR/ENABLE interface or independent stepper power cut-off is present.
- HAT J9 pin 1 feeds `/SERVO_IN+` through its fuse to `/SERVO_V+`, then the servo power pins. There is no independently monitored safety power switch in that path.
- The panel's HOLD and J3/J4 contacts go through the MCP23017 and firmware. They are operator/status inputs, not an independent safety path.
- A4988 ENABLE high disables its output FETs and removes holding torque. It is not a gravity brake or a safety-rated STO interface. [Allegro A4988 datasheet](https://www.pololu.com/file/0J450/A4988.pdf)

Keep a separate normal HOLD command: decelerate to a stop while maintaining required holding torque. Do not implement HOLD by closing the servo adapter or sending Nano `OFF ALL`, which can detach the servo outputs. Neither HOLD nor Linux's serial state replaces the wired stop.

## Independent low-voltage circuit to develop

The existing enclosed supplies remain closed. All new switching is on their low-voltage outputs; there is no user-built AC switch wiring.

```text
Separately mounted latching red mushroom, two direct-opening NC channels
      channel A -------------------- safety relay input A
      channel B -------------------- safety relay input B

Independent enclosed 24 V control supply -> monitored safety relay
Physical RESET + K1/K2 linked NC feedback -> manual-reset/EDM circuit

Safety permission -> separate manual ARM/START interlock -> K1 and K2 coils
Safety/brake controller -> independently monitored brake-release circuits

12 V PSU + -> branch fuse -> K1 NO power pole -> K2 NO power pole -> CNC VMOT +
Servo PSU + -> branch fuse -> K1 NO power pole -> K2 NO power pole -> servo-feed +
PSU returns ------------------------ correct existing motor/logic reference

Safety/brake status -> correctly rated isolated interface -> Pi/panel indication
```

K1 and K2 are separate devices, each with power poles rated for the actual DC load and linked/mirror NC feedback. One welded power contact must not leave either motor rail powered through the other device, and its feedback must prevent reset. Keep the two channels separately identifiable, strain-relieved and protected against cross-shorts; the commercial module's exact wiring/manual governs fault detection.

Use a commercial monitored manual-reset safety relay rather than inventing its diagnostic logic on this new screen PCB. An examined example is **Omron G9SE-201**: its instructions include dual-channel E-stop input, monitored reset/feedback and external forcibly guided devices. Its X1 output is a **24 V PNP transistor**, not a dry contact and must not be plugged into J3/J4 or Pi GPIO. See [manufacturer instructions](https://assets.omron.eu/downloads/latest/manual/en/g9se_instruction_instruction_manual_en.pdf). This is a documented architecture example; its category claims do not transfer automatically to this arm.

Omron **G7SA-3A1B DC24** illustrates a force-guided external relay with two required power poles plus feedback. It is **not approved here as the final motor switch**: its published 6 A at 30 V DC is a resistive rating, whereas its 24 V DC inductive rating is 1 A. A 4 A supply label does not qualify capacitive inrush, electronic motor load, regeneration or DC interruption. Final contacts need appropriate manufacturer-rated load capability. [G7SA ratings](https://www.ia.omron.com/products/family/386/specification.html)

The stop box belongs on the fixed base or bench edge within reach without entering the arm's sweep. Its enclosure needs covered terminals, strain relief and low-voltage rated connectors. It does not mount under the touchscreen, on the moving arm or on a solderless breadboard. The red actuator must latch and use direct-opening NC contacts; the yellow background and physical labels distinguish it from customizable buttons. [Omron switch specifications](https://www.ia.omron.com/products/family/1111/specification.html)

The reset and restart circuits remain distinct. Releasing the mushroom, reconnecting USB, restoring power or clearing an application error must never resume a queued move. RESET acknowledges a cleared fault and healthy feedback; a separate deliberate ARM/START is needed. Brakes stay applied until holding torque and restart conditions have been established. The current open-loop control cannot prove a slipped axis position, so E-stop, power loss and transmission slip invalidate the motion reference.

Servo PWM must not back-power a supposedly disconnected servo supply. Its signal interface needs an engineered power-off-safe isolation/buffering arrangement with output-side energy derived only from the switched servo rail; the exact implementation is not present in the current HAT. Similarly, CNC VMOT switching needs verified local capacitance and transient/regen management. Do not interrupt individual stepper phase wires or add a blanket flyback diode across an entire bidirectional drive. Suppression and fuse values must follow the actual drive, supply and interruption envelope, not the screen's 500 mA PTC.

## Retention design that actually addresses this arm

**Preferred direction:** mechanically secure each gravity-loaded output independently of the presently unreliable pinion/coupling, with spring-applied, power-released load-side brakes or rated load-arrest devices. Y/Z linkage geometry may permit one device to retain a coupled linkage, but the CAD must prove which degrees of freedom it restrains. Wrist retention and dropped-object containment remain separate requirements; holding Y/Z does not hold an MG90S-driven wrist or gripper shut.

A normal holding brake is not automatically rated to catch a moving arm. An immediate two-supply power cut is viable only after the retaining system is qualified for its worst-case engagement delay, droop, speed, energy and emergency stopping duty. Otherwise a controlled stop followed by brake engagement and torque removal needs a suitably monitored sequence and actual motion feedback; adding a fixed delay to Pi software cannot prove standstill. Manufacturer guidance distinguishes standstill holding and brake control from dynamic emergency arrest. [Pilz brake control](https://www.pilz.com/en-IE/support/lexicon/articles/072379), [Rockwell vertical-load application guide](https://literature.rockwellautomation.com/idc/groups/literature/documents/at/motion-at003_-en-p.pdf)

For each gravity-loaded output, derive the worst-case gravity moment from actual moving masses and centers of mass:

`Tg,j(q) = sum(mi * g * perpendicular lever arm from joint j)`

Then select brakes and mounts using the manufacturer's holding/dynamic ratings, required margin, wear, temperature and stop frequency. Include arm self-weight, camera, wiring, complete gripper and the target 227 g object. As an illustration only, a 227 g object on a 1.219 m horizontal lever produces about 2.71 N m before arm weight. The actual joint lever and torque are not established by “4 ft extension.”

A counterbalance can reduce required torque, but needs a verified torque-versus-position curve across the permitted poses and payload range. It does not inherently lock the arm or keep a loose output attached. A parking cradle or rigid props can support the already secured stationary arm during electronics work; they are not an emergency stop for a moving arm. No untested printed pawl, motor detent torque, microservo gearing or friction clamp is credited as retention.

## What can be finished now, and the minimum inputs left

Can close now: the division between normal HOLD, independent safety stopping and status; no-AC-wiring topology; dual-channel/reset/feedback policy; both-rail cut-off requirement; separate enclosure and mechanical location; screen firmware prohibition on automatic resume. The PCB's dry-contact/status provisions can remain useful without carrying motor power.

Cannot honestly finalize: brake torque/size/mount, allowable stopping droop and energy, servo contact/current/fuse size, power-off-safe PWM path, and verified restart/brake sequence. None is answered by repeatedly running KiCad DRC.

The targeted search subsequently found **C:/Users/mail2/Downloads/Mk2 Plus Addons (6).stl**, 4,670,584 bytes and 93,410 triangles. Its rendered shape is an assembled MK2-style arm with base, linkage and wrist. Original pose bounds are approximately 172.67 by 328.78 by 250.43 STL units. Repeated planar mesh-circle fits identify likely main linkage centers at Y/Z approximately (-82.238,77.000), (-17.029,195.206), and (59.239,69.540), yielding approximately **135 and 147 STL units** between these candidate pivots. STL encodes no units, assembly mates, component materials or mass. These are original mesh geometry measurements, not confirmed installed link dimensions. Do not treat this as the user's verified four-foot working envelope or use it to select a brake yet. Identification render and extraction records are saved in this folder. Addons (7).stl contains two separate small link-like parts, not the complete arm.

Only two essential information bundles should be requested next:

1. **Confirm whether the found assembled mesh matches the installed arm and its print scale**, plus the current gripper/camera modification geometry or already-secure, powered-off photographs that show its load-side mounting. No need to ask the user to find the original assembly again. Use this confirmation to reconcile the current geometry, travel and mounting space. Ask for actual mass information only where CAD/material data cannot establish it; no new drop or live motion experiment.
2. **Servo power-supply OUTPUT label photo/text** and which physical cable currently feeds the two servos. The 227 g payload target is already answered and should not be asked again.

The original CAD may omit real assembly play, materials, screws and custom changes; if their masses or load attachment remain unknown, that limitation must be resolved before brake qualification. Geometry alone does not certify a printed arm.

## Cost and release consequence

Reviewed panel/display parts are about **$73.64**, before PCB, harness, mounting, shipping, tax and this stop/retention system. The examined G9SE-201 listing is **$254.15 at quantity one**, with stock shown, before any brakes, external contacts or control supply. [Mouser listing](https://www.mouser.com/en/ProductDetail/Omron-Automation-and-Safety/G9SE-201?qs=dgwYPV2%2FDPgeCBxlOLhTQQ%3D%3D), checked 31 October 2026. This example is not a claim that every alternative costs that much; it means there is no researched complete $100 safety solution to promise.

Do not enlarge the screen PCB with an unqualified “safety relay” to conceal that budget gap. An off-arm electronics prototype is a distinct useful phase; a complete moving-arm product remains on hold until independent retention, stop circuitry, physical integration, firmware and supervised assembled validation are complete. No full production-readiness claim is supported yet.

