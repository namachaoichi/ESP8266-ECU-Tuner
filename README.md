# ESP8266 Serial-Based ECU Tuner

Lightweight ECU tuning interface for ESP8266 with embedded web dashboard over Serial. Real-time 3D tune table editing, sensor monitoring, and configuration management—all stored in LittleFS/SPIFFS without SD card dependency.

## Features

✅ **Web-based tuning interface** — Responsive HTML/CSS/JS dashboard
✅ **3D Tune Tables** — Editable RPM x MAP spark advance, VE, AFR target tables
✅ **Real-time Serial communication** — WebSocket-over-Serial protocol
✅ **LittleFS storage** — All config, tune tables, and web assets in flash
✅ **Minimal memory footprint** — Designed for ESP8266 (1MB-4MB)
✅ **Multiple cylinder support** — 4, 6, or 8 cylinders (configurable)
✅ **Ignition & Fuel control** — Basic coil/injector driver support
✅ **Live sensor dashboard** — RPM, MAP, TPS, CLT, IAT, VBAT monitoring
✅ **Configuration persistence** — JSON-based config storage
✅ **No external dependencies** — Standalone firmware, minimal libraries

## Hardware Support

- **Microcontroller:** ESP8266 (12E, 12F, NodeMCU, Wemos D1 Mini)
- **Flash size:** 1MB minimum (2MB+ recommended for more tune tables)
- **Storage:** LittleFS or SPIFFS (built-in flash filesystem)
- **Serial interface:** USB-to-UART for tuning connection

## Project Structure

```
ESP8266-ECU-Tuner/
├── firmware/
│   ├── src/
│   │   ├── main.cpp              # Entry point, setup/loop
│   │   ├── ECU.h/cpp             # Core ECU controller
│   │   ├── Serial_Handler.h/cpp  # WebSocket-over-Serial protocol
│   │   ├── Web_Server.h/cpp      # Minimal embedded web server (alternative)
│   │   ├── Config_Manager.h/cpp  # JSON config storage/load
│   │   ├── Tune_Table.h/cpp      # 3D table interpolation
│   │   ├── Sensor_Manager.h/cpp  # ADC sensor reads
│   │   ├── Ignition.h/cpp        # Coil control
│   │   └── Fuel.h/cpp            # Injector control
│   ├── include/
│   │   ├── Config.h              # Constants and config structs
│   │   ├── Types.h               # ECU data structures
│   │   └── Pins.h                # GPIO pin mapping
│   ├── data/
│   │   ├── index.html            # Main dashboard (embedded)
│   │   ├── config_default.json   # Default configuration
│   │   └── tune_default.json     # Default tune tables
│   ├── platformio.ini
│   └── README.md
└── docs/
    ├── ARCHITECTURE.md           # System design
    ├── SERIAL_PROTOCOL.md        # Communication format
    ├── PINOUT.md                 # GPIO mapping
    └── TUNING_GUIDE.md
```

## Quick Start

### Prerequisites
- PlatformIO (CLI or VS Code extension)
- ESP8266 board (NodeMCU v2 or similar)
- USB cable with data lines

### Build & Upload

```bash
# Clone and enter directory
git clone https://github.com/namachaoichi/ESP8266-ECU-Tuner.git
cd ESP8266-ECU-Tuner

# Build and upload to ESP8266
pio run -e nodemcu -t upload

# Monitor serial output
pio run -e nodemcu -t monitor
```

### Access Tuning Interface

1. **Connect via Serial monitor** at **115200 baud**
2. Open **HTML dashboard** embedded in firmware
3. **Real-time tuning** of tables, sensor readings, and config
4. **Auto-save** to LittleFS on changes

## Communication Protocol

**WebSocket-over-Serial** protocol for tuning commands:

```
Host → ESP8266:
  GET /ws HTTP/1.1
  Host: ecu.local
  Upgrade: websocket
  ...

Command Format (JSON):
  {
    "cmd": "get_state",           // Sensor readings
    "cmd": "set_table",           // Update tune table cell
    "cmd": "save_config",         // Persist config
    "cmd": "reboot"               // Restart ECU
  }

Response Format:
  {
    "status": "ok",
    "data": { ... }
  }
```

## Pin Configuration (Default)

| Function | GPIO | Type |
|----------|------|------|
| Crank sensor | D5 (GPIO14) | Digital input |
| Ignition coil | D1 (GPIO5) | Digital output |
| Fuel injector | D2 (GPIO4) | Digital output |
| MAP sensor | A0 | Analog input |
| TPS sensor | D8 (GPIO15) | Analog input |
| CLT sensor | ADC (onboard) | Analog |

Configurable via `pins.json` without reflashing.

## Memory Usage

| Component | RAM | Flash |
|-----------|-----|-------|
| Firmware | ~250KB | 500KB |
| Web assets | ~50KB | 100KB |
| Config/tables | ~20KB | 50KB |
| Free for runtime | ~200KB | 350KB |

**Total available:** 1MB Flash, 160KB RAM (ESP8266)

## Development

### Modify tune tables in firmware

Edit `data/tune_default.json`:

```json
{
  "spark": {
    "rows": 16,
    "cols": 16,
    "data": [[25, 24, 23, ...], ...]
  },
  "ve": { ... },
  "afr": { ... }
}
```

### Add custom sensor input

1. Extend `Sensor_Manager.cpp` with new ADC channel
2. Add JSON key in `/state` endpoint response
3. Update `app.js` dashboard gauge

### Compile web assets

No build step—all HTML/CSS/JS embedded directly in `data/` directory. PlatformIO's `littlefs` uploader packages them automatically.

## Serial Commands

Connect at **115200 baud** and send:

```
T,16,0,25      # Set table row 16, col 0 to value 25
G              # Get current engine state (JSON)
S              # Save config to LittleFS
R              # Reboot ECU
```

See `SERIAL_PROTOCOL.md` for full reference.

## Troubleshooting

| Issue | Solution |
|-------|----------|
| LittleFS full | Reduce tune table resolution (8x8 instead of 16x16) or increase flash partition |
| Slow serial response | Check baud rate (must be 115200), reduce table size updates |
| Config not persisting | Verify LittleFS partition mounted correctly via web UI |
| Memory crash during tuning | Reduce concurrent WebSocket connections or table update rate |

## License

MIT License — Free for personal and educational use.

## References

- [PlatformIO ESP8266 Docs](https://docs.platformio.org/en/latest/boards/espressif8266/nodemcu.html)
- [LittleFS for ESP8266](https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html)
- [ArduinoJson Library](https://arduinojson.org/)
