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
- **Flash size:** 1MB minimum (2MB+ recommended)
- **Storage:** LittleFS or SPIFFS (built-in flash filesystem)
- **Serial interface:** USB-to-UART @ 115200 baud

## Quick Start

### Prerequisites
- PlatformIO (CLI or VS Code extension)
- ESP8266 board (NodeMCU, Wemos D1 Mini, etc.)
- USB cable with data lines

### Build & Upload

```bash
git clone https://github.com/namachaoichi/ESP8266-ECU-Tuner.git
cd ESP8266-ECU-Tuner

# Build
pio run -e nodemcu

# Upload firmware
pio run -e nodemcu -t upload

# Monitor serial
pio run -e nodemcu -t monitor
```

### Use Tuning Interface

1. Connect ESP8266 via USB
2. Open Serial terminal @ **115200 baud**
3. Access embedded HTML dashboard
4. Edit 16×16 tune tables in real-time
5. Auto-save to LittleFS

## Architecture

### Dual-Loop Design

**Main Loop (Core 0):**
- 50ms sensor update interval
- 100ms tune table calculation
- WebSocket frame processing
- JSON serialization/deserialization

**Real-Time Tasks:**
- Ignition dwell/spark timing (timer-based)
- Fuel injector pulse width control
- Crank trigger decoding (ISR)

### Memory Layout

```
ESP8266 Flash (1MB):
  ├─ Bootloader (4KB)
  ├─ Firmware (250KB)
  ├─ LittleFS (650KB)
  │  ├─ index.html (50KB)
  │  ├─ config.json (2KB)
  │  ├─ tune.json (20KB)
  │  └─ logs/ (free)
  └─ OTA buffer (96KB)

ESP8266 RAM (160KB):
  ├─ Stack (32KB)
  ├─ Heap (80KB)
  ├─ Tune tables (16KB)
  └─ Free (32KB)
```

## Pin Configuration

| Function | GPIO | Type | Notes |
|----------|------|------|-------|
| **Crank Sensor** | D5 (GPIO14) | Digital input | 36-1 trigger wheel |
| **Ignition Coil** | D1 (GPIO5) | PWM output | Dwell + spark timing |
| **Fuel Injector** | D2 (GPIO4) | PWM output | Pulse width control |
| **MAP Sensor** | A0 (ADC) | Analog input | 0-3.3V |
| **TPS Sensor** | D8 (GPIO15) | Analog input | Throttle position |
| **CLT Sensor** | A0 (multiplex) | Analog input | Coolant temperature |

All pins configurable in `firmware/include/Pins.h` without reflashing.

## 3D Tune Tables

### Spark Advance Table
- **Rows:** RPM (0-7000 in 400 RPM steps)
- **Cols:** MAP (0-200 kPa in 12.5 kPa steps)
- **Values:** -5 to +40° BTDC
- **Uses:** Calculate spark timing from engine speed & load

### Volumetric Efficiency (VE) Table
- **Rows:** RPM bins
- **Cols:** MAP bins
- **Values:** 50-120% cylinder fill
- **Uses:** Fuel pulse width calculation (speed-density)

### AFR Target Table
- **Rows:** RPM bins
- **Cols:** MAP bins
- **Values:** 11-15.5 lambda ratio
- **Uses:** Closed-loop O2 feedback (if wideband sensor added)

## Web Dashboard Tabs

### Dashboard
- Live sensor gauges (RPM, MAP, TPS, CLT)
- Calculated values (spark advance, pulse width)
- Engine state indicator (running, stopped, error)

### Tune Tables
- 16×16 editable cells for each table
- Real-time cell editing
- Download/upload as JSON
- Reset to factory defaults
- Auto-save on change

### Configuration
- Engine setup (cylinders, ignition type, fuel type)
- Spark limits (min/max advance, max dwell)
- Fuel limits (min/max pulse width, dead time)
- Sensor calibration (MAP/TPS/CLT min/max values)

### Log Viewer
- Real-time serial log stream
- Download as text file
- Clear log buffer

## Serial Protocol

### WebSocket Frame Format

```json
{
  "cmd": "get_state",
  "data": { }
}
```

### Response Format

```json
{
  "type": "state",
  "data": {
    "rpm": 2500,
    "map": 75.5,
    "tps": 45.0,
    "clt": 85.5,
    "advance": 28.5,
    "pw": 5250,
    "state": 2
  }
}
```

### Available Commands

| Command | Purpose |
|---------|----------|
| `get_state` | Read current sensor values and calculated parameters |
| `get_config` | Retrieve all configuration settings |
| `set_config` | Update configuration (requires JSON payload) |
| `save_config` | Persist config to LittleFS |
| `reset_config` | Reset to factory defaults |
| `get_table` | Download tune table (spark/ve/afr) |
| `set_table` | Upload tune table modifications |
| `save_tables` | Persist tune tables to LittleFS |
| `reset_tables` | Reset tables to factory defaults |
| `get_log` | Retrieve system log buffer |
| `clear_log` | Clear log entries |
| `reboot` | Restart ESP8266 |

## Configuration Files

### config_default.json

```json
{
  "cylinders": 4,
  "ignition_type": 1,
  "fuel_type": 1,
  "adv_min": -5,
  "adv_max": 40,
  "dwell_max": 5.0,
  "pw_min": 1000,
  "pw_max": 25000,
  "dead_time": 1.0,
  "map_min": 10,
  "map_max": 200,
  "tps_min": 0,
  "tps_max": 100,
  "clt_min": -40,
  "clt_max": 120,
  "rev_limit_rpm": 7000,
  "fuel_cut_rpm": 6900
}
```

## Memory Usage

| Component | Size | Notes |
|-----------|------|-------|
| Firmware binary | 250KB | Arduino core + libraries |
| Web dashboard | 50KB | HTML/CSS/JS embedded |
| Config JSON | 2KB | Runtime config |
| Tune tables (3×16×16) | 20KB | Spark, VE, AFR |
| Log buffer | 10KB | Circular ringbuffer |
| Runtime heap | 50KB | Task stack + temp buffers |
| **Total used** | **382KB** | |
| **Available** | **618KB** | Expansion room |

## Troubleshooting

### Issue: LittleFS full
**Solution:** Reduce tune table resolution (8×8 instead of 16×16) in `Config.h`, or use 2MB+ flash board.

### Issue: Slow serial response
**Solution:** Check baud rate is 115200, reduce table update frequency, minimize log verbosity.

### Issue: Config not persisting
**Solution:** Verify LittleFS mount succeeded in Serial output. Run `pio run -t uploadfs` to format and upload defaults.

### Issue: Memory crash during tuning
**Solution:** Reduce WebSocket buffer size or disable high-frequency sensor logging.

### Issue: Crank trigger not detected
**Solution:** Check GPIO14 (D5) wiring, verify interrupt handler is attached, use pull-up resistor (10kΩ).

## Development Tips

### Adding a New Sensor

1. Add ADC channel in `Sensor_Manager.cpp`
2. Define min/max calibration values in config
3. Add JSON field in `/api/state` response
4. Update dashboard gauge in `index.html`
5. Save config to LittleFS

### Tuning the Ignition

1. Open **Dashboard** tab
2. Switch to **Tune Tables** tab
3. Select **Spark Advance** table
4. Edit cells for your RPM/MAP range
5. Click **Save to ECU**
6. Observe changes in Dashboard

### Backing Up Your Setup

```bash
# Download config
curl http://esp-ecu.local/api/config > my_config.json

# Download tune tables
curl http://esp-ecu.local/api/tables > my_tables.json

# Restore by uploading via web UI
```

## Performance Characteristics

- **Sensor update rate:** 20 Hz (50ms)
- **Tune calculation:** 10 Hz (100ms)
- **Table interpolation:** Bilinear (smooth)
- **WebSocket latency:** <100ms typical
- **Serial baud rate:** 115200 (115.2 kbps)

## Future Enhancements

- [ ] Wideband O2 sensor support (closed-loop tuning)
- [ ] Knock sensor integration
- [ ] Data logging to LittleFS
- [ ] Multi-profile support (street/race/economy)
- [ ] Live dyno curve plotting
- [ ] Firmware OTA updates
- [ ] CAN bus support (OBD-II)
- [ ] Python desktop tuning app

## License

MIT License — Free for personal and educational use.

## Contributing

Pull requests welcome! Please follow Arduino coding style and include documentation.

## References

- [PlatformIO Docs](https://docs.platformio.org/)
- [ESP8266 Arduino Core](https://github.com/esp8266/Arduino)
- [LittleFS Documentation](https://github.com/littlefs-project/littlefs)
- [ArduinoJson](https://arduinojson.org/)
- [Engine Control Unit Theory](https://en.wikipedia.org/wiki/Engine_control_unit)
