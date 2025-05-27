# ESP32 DevKitC Bluetooth‑to‑UART Bridge

## Overview

A guide for turning an ESP32‑DevKitC into a transparent Classic Bluetooth (SPP) ↔ hardware UART bridge, plus a quick on‑board loop‑back test for validation.

## Features

* Classic Bluetooth SPP acceptor ― 8 N 1, default 115 200 Bd.
* Bridges UART1 (GPIO17 TX / GPIO16 RX) to SPP.
* Loop‑back self‑test by jumpering TX2 ↔ RX2.
* Minimal ESP‑IDF component set (`driver`, `nvs_flash`, `bt`).

## Hardware References

| Item                              | Link                                                                                                                                                                                                                                                                                                                     |
| --------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| **DevKitC‑V4 Schematic**          | [https://dl.espressif.com/dl/schematics/esp32\_devkitc\_v4-sch.pdf](https://dl.espressif.com/dl/schematics/esp32_devkitc_v4-sch.pdf)                                                                                                                                                                                     |
| **DevKitC‑V4 Pin Tables**         | [https://asset.conrad.com/media10/add/160267/c1/-/gl/002490159DS00/tehnicni-podatki-2383855-espressif-esp32-devkitc-ve-razvojna-plosca-esp32-devkitc-ve.pdf](https://asset.conrad.com/media10/add/160267/c1/-/gl/002490159DS00/tehnicni-podatki-2383855-espressif-esp32-devkitc-ve-razvojna-plosca-esp32-devkitc-ve.pdf) |
| **ESP32‑DevKitC Getting‑Started** | [https://docs.espressif.com/projects/esp-idf/en/latest/esp32/hw-reference/esp32/get-started-devkitc.html](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/hw-reference/esp32/get-started-devkitc.html)                                                                                                       |

### Key Pins (right‑hand header)

| Signal  | GPIO | Silk label |
| ------- | ---- | ---------- |
| UART TX | 17   | TX2        |
| UART RX | 16   | RX2        |
| 3V3     | —    | 3V3        |
| GND     | —    | GND        |

> **Loop‑back**: place a dupont jumper between GPIO17 (TX2) and GPIO16 (RX2).

## Wiring to External MCU

```
ESP32 DevKitC        External Controller
----------------     -------------------
TX2  (GPIO17)  ----> RX
RX2  (GPIO16)  <---- TX
GND ------------●--- GND
```

Add RTS/CTS if flow‑control is required (assign free GPIOs and enable in `uart_param_config`).

## Building with ESP‑IDF

```bash
idf.py set-target esp32
idf.py fullclean
idf.py build flash monitor
```

### `main/CMakeLists.txt`

```cmake
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES driver nvs_flash bt
)
```

## macOS / Linux Loop‑back Test

1. Pair via Bluetooth (PIN `1234`).
2. Click **Connect → Serial Port** if macOS shows extra options.
3. Identify port:

   ```bash
   ls -l /dev/cu.*   # macOS
   ```
4. Open terminal:

   ```bash
   screen /dev/cu.BT-UART-Bridge-SPP 115200
   ```
5. Typed characters should echo while TX2 ↔ RX2 jumper is in place.

## Windows 11 Test

1. Settings → Bluetooth → Add device → *BT‑UART‑Bridge* → PIN `1234`.
2. Note the COM port under *More Bluetooth settings → Ports*.
3. Use PuTTY / TeraTerm at 115 200 Bd.

## License

SPDX‑License‑Identifier: Unlicense OR CC0‑1.0
