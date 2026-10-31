# MCP23017 A7/B7 input restriction

The current manufacturer data sheet DS20001952D explicitly marks GPA7 and GPB7 as output-only. The older revision C previously cited by this reference firmware did not state the restriction. Microchip's support article explains that an input level changing while the bit is transmitted can corrupt SDA and disrupt some hosts.

Primary sources: [current manufacturer data sheet](https://ww1.microchip.com/downloads/aemDocuments/documents/APID/ProductDocuments/DataSheets/MCP23017-Data-Sheet-DS20001952.pdf), [manufacturer support article](https://support.microchip.com/s/article/GPA7---GPB7-Cannot-Be-Used-as-Inputs-In-MCP23017).

The corrected architecture keeps all fourteen usable expander inputs for buttons/HOLD/NC-stop contacts/meter enable. A7/B7 are unused outputs held low. LCD_VIO_PRESENT and BB_FAULT are routed to two separate ESP32-S3 inputs through the revised HAT harness. Logical protocol snapshot bits7/15 retain their previous meaning and are composed from those direct inputs; they must never be confused with physical MCP input pins.

Assigned direct inputs are ESP GPIO6=LCD_VIO_PRESENT through HAT J4 pin3 and GPIO7=BB_FAULT_N through HAT J4 pin4. Both use external100k pulldowns in the revised HAT variant and Arduino INPUT without internal pulls. Panel J12 pins1/2 carry present/fault, with common ground in the existing harness. The Arduino adapter checks IODIRA/B before every GPIO read and rejects a missed reset that restored them to0xFF.

Native mapping, firmware and the final validation record must agree before these reference files are used. A pre-correction successful compilation is not evidence that the old circuit was correct. All physical validation remains pending.
