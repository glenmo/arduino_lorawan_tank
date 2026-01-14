# Arduino LoRaWAN Tank Level

Firmware for an Arduino UNO R4 WiFi with a Duinotech Long Range LoRa Shield XC4392 (SX1276 @ 915 MHz) and a DFRobot RS485 shield reading a water depth pressure sensor over Modbus RTU.

## Quick start

- Identify the board/port:

  `arduino-cli board list`

- Compile:

  `arduino-cli compile --fqbn arduino:renesas_uno:unor4wifi lorawan_tank_level/`

- Upload:

  `arduino-cli upload --fqbn arduino:renesas_uno:unor4wifi -p /dev/cu.usbmodem9C139EEBDB442 lorawan_tank_level/`

## Configure

- Update OTAA credentials in `lorawan_tank_level/lorawan_tank_level.ino`.
- Confirm the Modbus register map for the pressure sensor and update `kModbusRegDepthMm`.
- Verify LoRa shield pin mapping (NSS/RST/DIO0-2) and update the LMIC pinmap if needed.

## Notes

- AU915 is configured via `lorawan_tank_level/lmic_project_config.h`.
- LED blinks on join and after uplink attempts for visual confirmation.
