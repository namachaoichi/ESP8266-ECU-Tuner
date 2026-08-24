# ESP8266 ECU Tuner — Serial Protocol Specification

## Overview

The ECU communicates via **WebSocket-over-Serial** at **115200 baud**. All commands and responses are JSON-formatted for human readability and easy parsing.

## Connection Setup

1. Connect ESP8266 via USB (DTR/RTS for auto-reset)
2. Open Serial terminal at **115200 baud, 8N1**
3. Wait for boot message
4. Send WebSocket upgrade request (optional for direct JSON)

## Message Format

### Command (Host → ESP8266)

```json
{
  "cmd": "<command_name>",
  "param": "<optional_value>",
  "data": { "nested": "payload" }
}
```

### Response (ESP8266 → Host)

```json
{
  "type": "<response_type>",
  "status": "ok|error",
  "data": { "result": "value" },
  "error": "<error_message>"
}
```

## Commands

### 1. Get Engine State

**Request:**
```json
{"cmd": "get_state"}
```

**Response:**
```json
{
  "type": "state",
  "status": "ok",
  "data": {
    "rpm": 2500,
    "map": 75.5,
    "tps": 45.0,
    "clt": 85.5,
    "iat": 22.0,
    "vbat": 13.8,
    "advance": 28.5,
    "pw": 5250,
    "dwell": 5.5,
    "state": 2,
    "timestamp_us": 123456789
  }
}
```

**Fields:**
- `rpm` (uint16) — Engine speed (0-7000)
- `map` (float) — Manifold pressure (0-200 kPa)
- `tps` (float) — Throttle position (0-100 %)
- `clt` (float) — Coolant temperature (-40 to +120 °C)
- `iat` (float) — Intake air temperature
- `vbat` (float) — Battery voltage (8-16V)
- `advance` (float) — Spark advance (-5 to +40 °BTDC)
- `pw` (uint32) — Fuel pulse width (1000-25000 µs)
- `dwell` (float) — Ignition dwell (1-10 ms)
- `state` (uint8) — Engine state (0=stopped, 1=cranking, 2=running)
- `timestamp_us` (uint32) — Uptime microseconds

---

### 2. Get Configuration

**Request:**
```json
{"cmd": "get_config"}
```

**Response:**
```json
{
  "type": "config",
  "status": "ok",
  "data": {
    "cylinders": 4,
    "ignition_type": 1,
    "fuel_type": 1,
    "adv_min": -5.0,
    "adv_max": 40.0,
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
}
```

---

### 3. Set Configuration

**Request:**
```json
{
  "cmd": "set_config",
  "data": {
    "cylinders": 4,
    "adv_max": 42.0,
    "pw_max": 26000
  }
}
```

**Response:**
```json
{"type": "config", "status": "ok"}
```

**Notes:**
- Only send fields that changed
- Server will update and return current state on next `get_config`
- Invalid values are rejected with error response

---

### 4. Save Configuration

**Request:**
```json
{"cmd": "save_config"}
```

**Response:**
```json
{
  "type": "config",
  "status": "ok",
  "message": "Configuration saved to LittleFS"
}
```

**Notes:**
- Persists all runtime config changes to `/config.json`
- Automatically called after `set_config` in typical workflows
- Non-blocking operation (~10ms)

---

### 5. Reset Configuration

**Request:**
```json
{"cmd": "reset_config"}
```

**Response:**
```json
{
  "type": "config",
  "status": "ok",
  "message": "Configuration reset to factory defaults"
}
```

**Notes:**
- Restores defaults from compiled constants
- Does NOT save to LittleFS (call `save_config` to persist)

---

### 6. Get Tune Table

**Request:**
```json
{
  "cmd": "get_table",
  "param": "spark"
}
```

**Response:**
```json
{
  "type": "table",
  "status": "ok",
  "table": "spark",
  "rows": 16,
  "cols": 16,
  "data": [
    [25.0, 24.5, 24.0, ...],
    [25.5, 25.0, 24.5, ...],
    [...]
  ],
  "rpm_bins": [0, 400, 800, ..., 6000],
  "map_bins": [10, 22.5, 35, ..., 200]
}
```

**Parameters:**
- `spark` — Spark advance table
- `ve` — Volumetric efficiency table
- `afr` — AFR target table

**RPM Bins:** 0 to 6000 in 400 RPM steps (16 rows)
**MAP Bins:** 10 to 200 kPa in 12.5 kPa steps (16 cols)

---

### 7. Set Tune Table

**Request:**
```json
{
  "cmd": "set_table",
  "param": "spark",
  "data": [
    [25.0, 24.5, 24.0, ...],
    [25.5, 25.0, 24.5, ...],
    [...]
  ]
}
```

**Response:**
```json
{
  "type": "table",
  "status": "ok",
  "table": "spark",
  "message": "Table updated (not yet saved)"
}
```

**Notes:**
- Updates runtime table only (not saved to LittleFS)
- Table data must be 16×16 array of floats
- Call `save_tables` to persist
- Validation: values clamped to min/max limits

---

### 8. Save Tune Tables

**Request:**
```json
{"cmd": "save_tables"}
```

**Response:**
```json
{
  "type": "table",
  "status": "ok",
  "message": "All tables saved to LittleFS"
}
```

**Notes:**
- Persists all 3 tables (spark, VE, AFR) to `/tune.json`
- Non-blocking (~20ms for JSON serialization)

---

### 9. Reset Tune Tables

**Request:**
```json
{
  "cmd": "reset_tables",
  "param": "spark"
}
```

**Response:**
```json
{
  "type": "table",
  "status": "ok",
  "table": "spark",
  "message": "Table reset to factory defaults"
}
```

**Notes:**
- Omit `param` to reset ALL tables
- Does not save to LittleFS

---

### 10. Get System Log

**Request:**
```json
{"cmd": "get_log"}
```

**Response:**
```json
{
  "type": "log",
  "status": "ok",
  "entries": 150,
  "data": [
    "[12345] System initialized",
    "[12456] LittleFS mounted (650 KB free)",
    "[12567] Config loaded from /config.json",
    "[12678] Engine running at 2500 RPM",
    ...
  ]
}
```

**Notes:**
- Circular ringbuffer (max 1000 entries)
- Each entry timestamped in milliseconds
- Oldest entries discarded when buffer full

---

### 11. Clear System Log

**Request:**
```json
{"cmd": "clear_log"}
```

**Response:**
```json
{
  "type": "log",
  "status": "ok",
  "message": "Log buffer cleared"
}
```

---

### 12. Reboot System

**Request:**
```json
{"cmd": "reboot"}
```

**Response:**
```json
{
  "type": "system",
  "status": "ok",
  "message": "Rebooting in 1 second..."
}
```

**Notes:**
- ESP8266 restarts after 1 second delay
- All unsaved changes are lost
- Call `save_config` and `save_tables` before reboot if needed

---

## Error Responses

### Format

```json
{
  "type": "error",
  "status": "error",
  "error": "<error_message>",
  "code": <error_code>
}
```

### Common Errors

| Code | Message | Cause |
|------|---------|-------|
| 100 | Invalid command | Unknown `cmd` field |
| 101 | Missing parameter | Required field not provided |
| 102 | Invalid JSON | Parse error |
| 103 | Value out of range | Sensor reading or limit exceeded |
| 104 | File I/O error | LittleFS read/write failed |
| 105 | Memory error | Heap exhausted |
| 200 | Table size mismatch | Uploaded table not 16×16 |
| 201 | Config validation failed | Invalid config combination |

### Example Error

```json
{
  "type": "error",
  "status": "error",
  "error": "Invalid table size: expected 16x16, got 8x8",
  "code": 200
}
```

---

## Asynchronous Messages (Server → Host)

The server may send unsolicited messages to notify of state changes:

### Engine State Change

```json
{
  "type": "event",
  "event": "engine_state_change",
  "data": {
    "old_state": 0,
    "new_state": 2,
    "timestamp_ms": 12345
  }
}
```

### Sensor Fault

```json
{
  "type": "event",
  "event": "sensor_fault",
  "data": {
    "sensor": "MAP",
    "error": "Out of range (>200 kPa)",
    "value": 250.5
  }
}
```

### LittleFS Full Warning

```json
{
  "type": "event",
  "event": "storage_warning",
  "data": {
    "used_bytes": 900000,
    "free_bytes": 100000,
    "percent_used": 90
  }
}
```

---

## Example Session

### 1. Connect and initialize

```
→ {"cmd": "get_config"}
← {"type": "config", "status": "ok", "data": {...}}
```

### 2. Download current tune table

```
→ {"cmd": "get_table", "param": "spark"}
← {"type": "table", "status": "ok", "data": [...]}
```

### 3. Modify and upload

```
→ {"cmd": "set_table", "param": "spark", "data": [...]}
← {"type": "table", "status": "ok"}
```

### 4. Save changes

```
→ {"cmd": "save_tables"}
← {"type": "table", "status": "ok"}
```

### 5. Monitor live engine state

```
→ {"cmd": "get_state"}
← {"type": "state", "status": "ok", "data": {"rpm": 2500, ...}}
→ {"cmd": "get_state"}
← {"type": "state", "status": "ok", "data": {"rpm": 2750, ...}}
```

---

## Best Practices

1. **Always save after changes**
   ```json
   {"cmd": "set_config", "data": {...}}
   {"cmd": "save_config"}
   ```

2. **Check for errors**
   ```json
   if (response.status === "error") {
     console.error(response.error);
   }
   ```

3. **Rate limit state polls**
   - Don't poll faster than 100ms (risk of buffer overflow)
   - Typical: 200-500ms for UI updates

4. **Use timeout for requests**
   - Expect response within 500ms
   - Retry with backoff on timeout

5. **Validate received data**
   - Check array dimensions before use
   - Verify min/max ranges
   - Handle missing fields gracefully

---

## Troubleshooting

### No response from ECU
- Check baud rate (must be 115200)
- Verify USB cable has data lines (not power-only)
- Power-cycle ESP8266
- Check for garbage characters in terminal

### JSON parse errors
- Ensure commands are valid JSON
- Check for trailing commas
- Verify string quotes are escaped in JSON

### Timeout when saving large tables
- LittleFS write may take 50-100ms
- Increase timeout to 1000ms for `save_tables`
- Reduce table size if SPIFFS is full

---

## See Also

- [Protocol Implementation (main.cpp)](../firmware/src/main.cpp)
- [Data Structures (Types.h)](../firmware/include/Types.h)
- [Web Dashboard (index.html)](../firmware/data/index.html)
