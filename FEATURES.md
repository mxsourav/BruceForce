# BruceForce Feature Catalog

BruceForce is built for authorized offensive and defensive wireless security research, vulnerability testing, and network behavior monitoring in controlled lab environments. The descriptions below are intentionally short and focus on what each module is for, not on operational attack instructions.

## Wi-Fi / Network Research

| Feature | Wikipedia reference | BruceForce purpose | Ethical use |
| --- | --- | --- | --- |
| [Wi-Fi](https://en.wikipedia.org/wiki/Wi-Fi) | Wireless LAN standard | Core scan, connect, and lab testing workflows | Test only networks you own or have written permission to assess |
| [Wi-Fi deauthentication attack](https://en.wikipedia.org/wiki/Wi-Fi_deauthentication_attack) | Disconnect / re-association research | Controlled disconnect testing and recovery validation | Use only in a lab or explicitly authorized assessment |
| [Beacon frame](https://en.wikipedia.org/wiki/Beacon_frame) | Periodic Wi-Fi management frame | Beacon generation and frame-behavior experiments | Keep output inside a contained test environment |
| [Evil twin (wireless networks)](https://en.wikipedia.org/wiki/Evil_twin_%28wireless_networks%29) | Rogue AP / phishing concept | Captive-portal and rogue-AP lab demonstrations | Do not use to trick real users or capture real credentials |
| [Captive portal](https://en.wikipedia.org/wiki/Captive_portal) | Portal shown before full access | Local login-page experiments and onboarding flows | Use for controlled demos, not credential harvesting |
| [Wardriving](https://en.wikipedia.org/wiki/Wardriving) | Wi-Fi discovery with mobile gear | Site survey, mapping, and network-behavior observation | Respect privacy, consent, and local laws |
| [Wi-Fi Protected Setup](https://en.wikipedia.org/wiki/Wi-Fi_Protected_Setup) | Router pairing standard | Compatibility and security-validation workflows | Only for devices in your own lab |

## Bluetooth / BLE

| Feature | Wikipedia reference | BruceForce purpose | Ethical use |
| --- | --- | --- | --- |
| [Bluetooth](https://en.wikipedia.org/wiki/Bluetooth) | Short-range wireless standard | Classic Bluetooth behavior inspection and device research | Use only for owned or authorized devices |
| [Bluetooth Low Energy](https://en.wikipedia.org/wiki/Bluetooth_Low_Energy) | Low-power Bluetooth profile | Beacons, advertisements, and low-power device testing | Keep tests contained and non-disruptive |

## Wired / Traffic Monitoring

| Feature | Wikipedia reference | BruceForce purpose | Ethical use |
| --- | --- | --- | --- |
| [Ethernet](https://en.wikipedia.org/wiki/Ethernet) | Wired LAN standard | Wired network telemetry and bridge experiments | Use only on trusted infrastructure |
| [Sniffer](https://en.wikipedia.org/wiki/Sniffer) | Packet analyzer / traffic capture concept | Network behavior monitoring and packet analysis | Capture only traffic you are allowed to inspect |
| [Global Positioning System](https://en.wikipedia.org/wiki/Global_Positioning_System) | Satellite navigation system | Location tagging for wardriving and field notes | Do not track people without consent |

## Radio / Field Modules

| Feature | Wikipedia reference | BruceForce purpose | Ethical use |
| --- | --- | --- | --- |
| [Infrared](https://en.wikipedia.org/wiki/Infrared) | IR electromagnetic band | Remote-control and IR signaling modules | Use only with devices you own or are permitted to test |
| [Near-field communication](https://en.wikipedia.org/wiki/Near-field_communication) | Short-range contactless communication | NFC discovery and tag interaction experiments | Respect access controls and privacy rules |
| [Radio-frequency identification](https://en.wikipedia.org/wiki/Radio-frequency_identification) | RFID tag and reader system | RFID card, tag, and reader research | Only test tags and readers you own or are authorized to use |
| [LoRa](https://en.wikipedia.org/wiki/LoRa) | Long-range low-power radio | Long-range telemetry and module integration | Use legal regional frequencies and power limits |

## Utility / UI Modules

| Feature | Wikipedia reference | BruceForce purpose | Ethical use |
| --- | --- | --- | --- |
| [App store](https://en.wikipedia.org/wiki/App_store) | Digital software marketplace | BruceForce module browser and on-device add-on manager | Install only trusted modules and review code carefully |
| [File manager](https://en.wikipedia.org/wiki/File_manager) | File browsing and management UI | Local storage browsing, transfers, and maintenance | Protect sensitive captures and logs |
| [QR code](https://en.wikipedia.org/wiki/QR_code) | 2D barcode format | Displaying quick links, setup data, or project references | Avoid placing sensitive credentials in public QR codes |

## What Is Already Working

- ESP32-S3 master TFT UI on the 1.8" ST7735 display
- Generic ESP32 slave OLED dashboard on the 1.3" SH1106 display
- Four-button master navigation
- UART bridge between master and slave
- Boot branding, theme handling, and version logging

## What Is Still Planned

- IR transmit and receive hardware support
- NFC and RFID hardware support
- Additional radio module integrations
- More BruceForce UI polish and telemetry screens

## Safety Note

BruceForce is intended for authorized research, internal testing, and defensive validation. Do not use it against networks, devices, or users you do not own or explicitly have permission to test.
