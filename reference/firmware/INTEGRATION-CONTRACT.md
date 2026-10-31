# Reference firmware integration contract

This is a reference implementation, not the participant's completed firmware or evidence of hardware testing. No serial device was opened, firmware flashed or arm command sent during this work.

## Integration API

The core uses namespace `seizo`. Construct `ArduinoPanelIo(Wire, SPI, 21, allChipSelectPins, count, 6, 7, lockCallback, unlockCallback, context)` and `PanelCore(io, lineWriter, context)`. GPIO6 reads LCD_VIO_PRESENT through revised HAT J4 pin3; GPIO7 reads BB_FAULT_N through J4 pin4. Begin I2C with the final HAT pins, and SPI with SCLK10, MISO8 and MOSI9. Call `core.begin(millis())` once and `core.tick(millis())` continuously.

`lineWriter(const char*, void*)` must append one newline. Device frames fit 512 bytes. Feed only complete, NUL-terminated ASCII host lines to `handleHostLine`. Discard an entire overlong line before resuming; never execute its suffix. Maximum host line length is 79 bytes.

The SPI lock callbacks must use the same mutex as the LCD and touch adapters. Lock returns false on failure; unlock releases it. Include every LCD, touch and ADC chip-select pin in `allChipSelectPins`. ADC reads deassert all these pins and use 100 kHz MODE0 transactions. Display and touch code must independently respect `LCD_VIO_PRESENT`.

The core exclusively owns MCP23017 address 0x20: BANK0 and sequential reads. **Current DS20001952D restricts A7/B7 to outputs.** Both unused physical pins are configured as low outputs; IODIRA/B and GPPUA/B are0x7F, with OLATA/B zeroed before direction changes. All fourteen remaining pins are inputs. Configuration is read back, and directions are checked before every GPIO read so a missed short expander reset is detected. Other code must not overwrite its direction, polarity or addressing registers.

| Port | Function |
|---|---|
| A0–A6 | Buttons 1–7, active low |
| Physical A7/B7 | Unused low outputs; never inputs |
| B0 | HOLD button, active low |
| B1/B2 | STOP1/STOP2 dry NC contacts: low/closed healthy, high/open asserts |
| B3–B5 | Buttons 8–10, active low |
| B6 | Meter enable, active low |
| Direct ESP GPIO6, logical bit7 | LCD_VIO_PRESENT, active high |
| Direct ESP GPIO7, logical bit15 | Breadboard fault, active low |

The HAT variant supplies an external100k ground pulldown on each direct input; plain Arduino INPUT is used. A disconnected VIO/fault sense wire therefore inhibits the panel. Existing panel /FAULT still has its10k3.3V pullup; do not add uncertain internal pulldowns against it. The two direct signals use panel J12 pins1/2 and share the existing harness ground. Logical snapshot bits7/15 are composed from these direct inputs and do not represent physical expander pin inputs.

## Input and connection behavior

Inputs debounce for 20 ms. VIO loss, breadboard fault and stop assertions inhibit immediately from the raw input; releases remain debounced. A key held at startup, reconnect or fault must release before producing a fresh press. Stop release, reconnect and I2C recovery never clear `needs_ack` automatically.

`acknowledge(now)` is for an explicit operator action only. It requires a live host, healthy inputs and no held keys. This panel inhibition is not a safety-rated arm stop. No actuator command mapping is implemented.

## Serial protocol

Host commands are ASCII:

```
PING 12345
STATUS
CFG 1000000 0
ACK 12345
```

PING nonce 1–2147483647 identifies a session. Repeat it within 1500 ms. CFG accepts gain 500000–1500000 parts per million and zero offset −1000–1000 input millivolts. Settings exist only in RAM. Until an explicit CFG is accepted, enabled meter state is `unconfigured` and voltage remains null. ACK must match the current nonce and is never automatically sent by the host bridge.

JSON responses are `hello`, `pong`, `state`, `event`, `error` and `link`. Every invalid meter state sends `mv:null`. `accuracy_verified` remains false regardless of gain and zero settings. Display `snapshot().millivolts` only when `snapshot().meter==MeterState::Valid`; the stored zero in an invalid snapshot is not a reading.

## Meter behavior

MCP3202 CH0 measures the divider output; CH1 measures the nominal 2.5 V reference. The core waits 50 ms after meter enable, discards the first conversion after each channel switch and averages eight samples per channel. Reference counts outside 2800–3400 are invalid. This catches lost or abnormal references and does not establish calibrated accuracy.

Conversion is `CH0/CH1 ×2500mV ×10.98`, followed by user gain and zero. Results outside 0–24000 mV are invalid. No current, resistance, isolated-probe or unknown-circuit measurement is implemented. Software cannot detect every plausible but incorrect ADC value.

## Pi bridge

`host/seizo_panel_bridge.py` needs pyserial only when run against an explicit `--port`. Importing it and running tests never opens ports. It does not discover motor controllers, import arm adapters, drive motors or offer an HTTP stop endpoint. It publishes JSON to stdout, validates session nonces and sequences, discards oversized frames and clears cached state on faults. Reconnection uses a new nonce and never auto-ACKs.

Opening a serial port can reset an ESP board, depending on its hardware. Run it only against the actual panel port during later authorized hardware work.

Primary references: [current MCP23017 revision-D datasheet](https://ww1.microchip.com/downloads/aemDocuments/documents/APID/ProductDocuments/DataSheets/MCP23017-Data-Sheet-DS20001952.pdf), [manufacturer A7/B7 restriction article](https://support.microchip.com/s/article/GPA7---GPB7-Cannot-Be-Used-as-Inputs-In-MCP23017), [MCP3202 datasheet](https://ww1.microchip.com/downloads/en/DeviceDoc/21034F.pdf).

## Display, touch, host selection and board target

The actual target is `esp32-s3-devkitc-1` with 8 MB flash. For the N8R8 module the extra OPI PSRAM is deliberately not initialized or used; the 153,600-byte framebuffer is in internal RAM. Native USB CDC is enabled at boot. Nothing in this firmware supplies `BOARD_3V3` from USB: the documented HAT AP2112 removal/DNP variant and external 3.3 V converter are still required.

| Build environment | Display controller | Protocol owner |
|---|---|---|
| `seizo_s3_st7789` | ST7789, maker's current default | Native USB CDC |
| `seizo_s3_hx8347d` | Older HX8347D module | Native USB CDC |
| `seizo_s3_st7789_uart` | ST7789 | Hardware Serial1, RX44/TX43, 115200 baud |

Select the controller from the actual module's markings/batch information; no controller detection is claimed. Exactly one protocol owner is compiled. The UART build does not read USB commands or send panel telemetry to USB. Pi UART TX must reach HAT RX44, and HAT TX43 reaches Pi UART RX, with common ground and 3.3 V logic. The Pi port's operating-system configuration and physical link have not been tested. Bluetooth is not implemented.

SPI pins are SCLK10/MISO8/MOSI9; LCD CS11/DC12/RST13/BL14; touch CS15/IRQ18; ADC CS21. I2C SDA17/SCL16 runs at 100 kHz. Every peripheral uses the same mutex and all CS lines are deasserted before a transaction. Display SPI is 4 MHz, touch 1 MHz and ADC 100 kHz, all MODE0. LCD flushes use two physical rows per transaction rather than a blocking full-frame transfer. Reset and vendor settling delays are cooperative, so input polling and LCD power checks continue through startup.

Waveshare register sequences are extracted from its licensed `LCD_Driver.c` and preserved in `vendor_init.h`. Native portrait 240×320 addressing is software rotated into landscape 320×240; no unverified MADCTL rotation value is invented. The UI displays 10 physical button states, host/input/breadboard status and HOLD/STOP requests. It explicitly labels these as requests rather than a physical safety stop. There is no motor adapter or actuator command mapping.

XPT2046 touch uses five trimmed samples. A five-point affine calibration must pass geometric checks before touch actions are used, then is saved with a checksum in NVS. ACK requires a deliberate 800 ms touch hold inside its area; leaving that area clears the hold timer. The meter's two-tap default CFG confirmation establishes nominal gain/zero only; it does not calibrate voltage accuracy. An invalid or unconfigured meter displays `--`, never a fabricated zero.

If I2C or actual LCD VIO is lost, display/touch transactions stop, the display control pins return to inputs, panel actions are inhibited and a fresh explicit ACK is required after recovery. This depends on the actual-VIO tap and hardware isolation circuit being correctly built; firmware cannot substitute for it.

Primary display references: [Waveshare product documentation](https://docs.waveshare.net/2.8inch_Resistive_Touch_LCD/), [maker resources](https://docs.waveshare.net/2.8inch_Resistive_Touch_LCD/Resources-And-Documents/), [maker code archive](https://www.waveshare.net/w/upload/6/6f/2.8inch_Resistive_Touch_LCD-Code.7z), [XPT2046 datasheet](https://www.waveshare.net/w/upload/b/b0/XPT2046.pdf). Exact source hashes and register operations are in `vendor-register-provenance.json`; the maker license is preserved in `THIRD-PARTY-NOTICES.txt`.

## Validation boundary

All three actual ESP32-S3 environments compile and link without flashing. Twenty Python protocol/session tests pass offline. The portable core, touch calibration/framebuffer and Arduino register/direct-GPIO adapter mocks compile with C++11, `-Wall -Wextra -Werror`, and execute successfully with native Windows GCC. The read-only native XML audit covers 160 pin/value/removal checks in the corrected design, including the new power/isolation parts and direct-sense header. That audit caught and corrected the reference firmware's original STOP1/STOP2 polarity mismatch with the actual NC-contact circuit. The later manufacturer A7/B7 restriction review required a separate circuit/adapter correction; passing the earlier compilation alone did not establish electrical correctness.

`validation.json` records final build sizes, hashes and test scope. Long Windows paths prevented the first compiler launch; local junctions `C:/Users/mail2/Documents/Codex/p23` and `f23` point to the workspace PlatformIO core and this project. They contain no separate source copy. A reproducible build uses those aliases and the pinned packages in `platformio.ini`. The native test compiler is a public w64devkit release extracted into `work/w64devkit`, with a similar `c23` alias; it is not a project runtime dependency.

No physical behavior, calibration, disconnect waveform, display response or actual Pi/HAT communication has been verified.
