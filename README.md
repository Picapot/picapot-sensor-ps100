# Picapot PS100 Sensor

This repository contains the latest firmware for the Picapot PS100 soil moisture and temperature sensor.

Full documentation for the sensor is available at https://www.picapot.com

## Compile from Source

A precompiled `.hex` file is available for direct upload to the microcontroller. If you don't need to change anything of the program, you can skip to Calibration and Upload.

### Add the MiniCore board to Arduino IDE

The code has been developed using the Arduino IDE and the MiniCore board package. To install the board, first add the following line to **Settings → Additional Boards Manager URLs**:

```text
https://mcudude.github.io/MiniCore/package_MCUdude_MiniCore_index.json
```
Then open **Tools → Board → Boards Manager**, search for MiniCore, and install the package.

### MiniCore board settings

- Microcontroller: `ATmega88`
- Variant: `88P / 88PA`
- Clock: `Internal 8 MHz`
- B.O.D.: `Disabled`
- Bootloader: `No bootloader`
- EEPROM: `Erase EEPROM`
- LTO: `Enabled (Os)`




## Calibration and Upload
To run the scripts of this repository make sure that [AVRDUDE](https://github.com/avrdudes/avrdude/releases) is available in the system `PATH`.

`CALIBRATE+UPLOAD_MKII.bat` performs the calibration and upload the firmware to the microcontroller. To use it:

1. Connect an AVR ISP MKII programmer to the PS100 ISP header and power the sensor correctly.
2. Make sure that `picapot-sensor-ps100-calib.ino.hex` and `picapot-sensor-ps100.ino.hex` are in the same directory as the batch file.
3. Connect a 10 kΩ resistor between the two probes.
4. Run `CALIBRATE+UPLOAD_MKII.bat`.
5. Wait for the final result. A green framed message confirms that calibration and upload completed successfully. A red framed message indicates an error that must be corrected before trying again.

During the procedure, the script uploads the calibration firmware `picapot-sensor-ps100-calib.ino.hex`, waits for it to calculate the calibration value, then uploads the sensor firmware `picapot-sensor-ps100.ino.hex` to the microcontroller. Finally, the script reads the EEPROM again and verifies that the calibration value was preserved. 



### Other Batch Scripts

`EEPROM_SHOW.bat` reads the complete ATmega88P EEPROM into `output1.txt` and prints its first line in the console. It is intended for inspecting the stored calibration and diagnostic bytes without compiling or uploading firmware. 


## Repository Contents

- **picapot-sensor-ps100.ino** — Main PS100 sensor firmware source
- **picapot-sensor-ps100.ino.hex** — Precompiled main sensor firmware
- **picapot-sensor-ps100-calib.ino.hex** — Precompiled calibration firmware
- **src/OneWireNoRes/** — Bundled no-external-resistor OneWire implementation
- **COMPILE.bat** — Arduino CLI compilation script
- **CALIBRATE+UPLOAD_MKII.bat** — Complete calibration and programming script
- **EEPROM_SHOW.bat** — EEPROM inspection script

## Dependencies

- **OneWireNoResistor**  
  Based on the OneWire library maintained by Paul Stoffregen and modified by Josh Levine to operate without an external DS18B20 pull-up resistor. http://wp.josh.com/2014/06/21/no-external-pull-up-needed-for-ds18b20-temp-sensor
