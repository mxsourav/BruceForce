# BruceForce Version Log

## v1.0.0 - BruceForce repo migration

- Renamed the GitHub-facing project identity from DeautherNet to BruceForce.
- Removed unrelated root web assets from the repository root.
- Added a professional README with hardware tables, pin maps, and roadmap notes.
- Added an MIT License for the repository.
- Added a maintained version log folder for future releases.

## v0.9.0 - Device integration baseline

- Master ESP32-S3 firmware focused on the TFT UI and button input handling.
- Slave ESP32 firmware used as an OLED co-processor over UART and I2C.
- Custom radio hooks and Wi-Fi scan display work established.

## v0.8.0 - Legacy BruceForce UI work

- Master and slave display layouts refined for 128x64 and ST7735 targets.
- Initial boot branding and menu cleanup.
- Early serial bridge and status reporting support.

## Imported legacy note

Older development logs and firmware iteration notes should continue to live here as the BruceForce project matures. Keep each release note focused on:

- what changed
- why it changed
- what hardware it affects
- any flashing or wiring warnings
