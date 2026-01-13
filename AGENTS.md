# Repository Guidelines

## Project Structure & Module Organization
This repository is currently empty. When adding code, keep a clear, predictable layout so new contributors can navigate quickly. A common Arduino layout is:

- `lorawan_tank_level/`: main Arduino sketch (e.g., `lorawan_tank_level.ino`).
- `lib/`: reusable modules and drivers.
- `tests/`: unit or hardware-in-the-loop tests.
- `docs/`: wiring diagrams, calibration notes, and deployment steps.
- `assets/`: static artifacts like JSON configs or reference images.

If you choose a different structure, document it here and keep directories consistent across features.

## Build, Test, and Development Commands
Use `arduino-cli` to compile and upload sketches. The target board is Arduino UNO R4 WiFi. Use the USB identifiers below to confirm the exact FQBN before building:

- Board: Arduino UNO R4 WiFi
- Port: `/dev/cu.usbmodem9C139EEBDB442`
- FQBN: `arduino:renesas_uno:unor4wifi`
- Core: `arduino:renesas_uno`
- `arduino-cli board list`: identify the port and FQBN for the attached board; re-run after reconnecting USB.

- `arduino-cli compile --fqbn arduino:renesas_uno:unor4wifi lorawan_tank_level/`: build firmware for the Arduino UNO R4 WiFi.
- `arduino-cli upload --fqbn arduino:renesas_uno:unor4wifi -p /dev/cu.usbmodem9C139EEBDB442 lorawan_tank_level/`: flash to the UNO R4 WiFi on the listed port.

## Coding Style & Naming Conventions
Keep formatting consistent across new files:

- Indentation: 2 spaces (or match existing files if different).
- Filenames: lower_snake_case for modules (e.g., `tank_level_sensor.cpp`).
- Symbols: `PascalCase` for types, `camelCase` for functions, `kConstant` for constants.

Document any formatter or linter you add (e.g., `clang-format`) and include the config in the repo.

## Testing Guidelines
No test framework is present. If you add tests, prefer a lightweight Arduino-friendly framework and:

- Place tests under `tests/`.
- Name test files by module (e.g., `test_sensor.cpp`).
- Document required hardware or simulators.

## Hardware Configuration
This project targets a Duinotech Long Range LoRa Shield XC4392 with the SX1276 radio at 915 MHz. Note the following in code and docs:

- Set LoRa radio frequency to `915000000`.
- Verify SPI wiring/pins match the shield defaults for the Arduino UNO R4 WiFi.
- Document any required antenna or region-specific settings.

## Commit & Pull Request Guidelines
This repository has no Git history yet. When you introduce version control:

- Use clear, imperative commit messages (e.g., `Add ultrasonic sensor driver`).
- Keep PRs focused; include a short summary and any hardware or calibration steps.
- If UI or documentation changes are included, add screenshots or photos.

## Configuration & Secrets
Do not commit device keys, LoRaWAN credentials, or personal access tokens. Store secrets in local config files (e.g., `config.local.h`) and add them to `.gitignore`.
