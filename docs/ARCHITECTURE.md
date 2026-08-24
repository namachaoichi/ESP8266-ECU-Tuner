# ESP8266 ECU Tuner Architecture

## System Overview

The ESP8266 ECU Tuner is a lightweight, real-time engine control system designed for simplicity and flexibility. It separates concerns into distinct layers:

```
┌─────────────────────────────────────┐
│   Web Dashboard (HTML/CSS/JS)       │
│   - Tune tables editor              │
│   - Live sensors                    │
│   - Configuration UI                │
└──────────────┬──────────────────────┘
               │
        WebSocket/Serial
               │
┌──────────────▼──────────────────────┐
│   WebSocket Handler                 │
│   - Parses JSON commands            │
│   - Routes to handlers              │
│   - Serializes responses            │
└──────────────┬──────────────────────┘
               │
    ┌──────────┼──────────┐
    ▼          ▼          ▼
┌────────┐ ┌────────┐ ┌────────────┐
│ Config │ │ Sensor │ │ Tune Table │
│Manager │ │Manager │ │  Engine    │
└────┬───┘ └────┬───┘ └────┬───────┘
     │          │          │
┌────▼──────────▼──────────▼────────┐
│   Real-Time Engine Controller     │
│   - RPM calculation               │
│   - Spark timing (dwell + fire)   │
│   - Injector pulse width          │
│   - Rev limiter                   │
└─────────────────────────────────────┘
               │
    ┌──────────┼──────────┐
    ▼          ▼          ▼
┌────────┐ ┌────────┐ ┌────────────┐
│ Ignition│ │ Fuel   │ │  Crank/    │
│ Driver  │ │ Driver │ │  Cam ISR   │
└────────┘ └────────┘ └────────────┘
```

## Core Components

### 1. Sensor Manager (`Sensor_Manager.cpp`)

**Responsibility:** Read and filter analog sensor inputs.

**Inputs:**
- MAP sensor (Manifold Absolute Pressure) — A0 ADC
- TPS sensor (Throttle Position) — D8 analog
- CLT sensor (Coolant Temperature) — ADC multiplexed
- Battery voltage

**Processing:**
- Exponential moving average (EMA) smoothing
- Min/max validation
- Fault detection

**Output:** Filtered sensor values to ECU state.

**Update Rate:** 20 Hz (50ms)

### 2. Tune Table Engine (`Tune_Table.cpp`)

**Responsibility:** Bilinear interpolation of 3D lookup tables.

**Inputs:**
- RPM value (0-7000)
- MAP value (0-200 kPa)
- Active tune table (spark/VE/AFR)

**Processing:**
- Find surrounding grid cells
- Interpolate between values
- Apply limits (min/max clamp)

**Outputs:**
- Spark advance (degrees)
- VE modifier (for fuel calc)
- AFR target (for closed-loop)

**Accuracy:** Better than ±5% between grid points.

### 3. Ignition Manager (`Ignition.cpp`)

**Responsibility:** Calculate and execute ignition timing.

**Inputs:**
- RPM, MAP (from sensors)
- Spark table value (from tune engine)
- Dwell time limit (from config)

**Processing:**
1. Look up spark advance from table
2. Calculate dwell time (5-10ms typical)
3. Emit dwell pulse to coil (GPIO5)
4. Calculate fire angle from crank position
5. Emit spark pulse

**Output:** PWM signals to ignition coil.

**Timing Precision:** ±1° crank angle.

### 4. Fuel Manager (`Fuel.cpp`)

**Responsibility:** Calculate and execute fuel injection.

**Inputs:**
- RPM, MAP (from sensors)
- VE table value (from tune engine)
- AFR target (from tune engine)
- Injector dead time (from config)

**Processing:**
1. Calculate air mass from MAP + VE table
2. Calculate fuel mass from AFR target
3. Convert to injector pulse width
4. Add dead time offset
5. Apply min/max limits
6. Emit pulse to injector

**Output:** PWM signal to fuel injector.

**Precision:** ±0.1ms pulse width.

### 5. Crank Trigger Decoder (`CrankSensor.cpp`)

**Responsibility:** Decode 36-1 crank wheel and calculate RPM.

**Inputs:**
- GPIO14 interrupt (crank sensor signal)

**Processing:**
1. Measure time between teeth
2. Detect missing tooth gap
3. Calculate tooth frequency
4. Convert to RPM
5. Detect sync (36-1 pattern)

**Outputs:**
- Reliable RPM reading
- Crank angle position (0-720° at 0.5° resolution)

**Error Detection:**
- Outlier filtering (timeout detection)
- Pattern validation
- Stall detection (0 RPM after running)

### 6. Config Manager (`Config_Manager.cpp`)

**Responsibility:** Load/save persistent configuration.

**Storage:** LittleFS `/config.json`

**Format:** JSON with human-readable keys.

**Operations:**
- `loadConfig()` — Read from flash at startup
- `saveConfig()` — Persist changed values
- `resetDefaults()` — Restore factory settings

**Thread Safety:** All file I/O on Core 0 (async).

## Data Structures

### EngineState_t (Real-Time)

```c
struct {
    uint16_t rpm;              // 0-7000 RPM
    float map_kpa;             // 0-200 kPa
    float tps_percent;         // 0-100 %
    float clt_celsius;         // -40 to +120 °C
    float battery_voltage;     // 8-16V
    
    float spark_advance;       // -5 to +40 °BTDC
    uint32_t pulse_width_us;   // 1000-25000 µs
    float dwell_time_ms;       // 1-10 ms
    
    uint8_t engine_state;      // STOPPED, CRANKING, RUNNING
    uint32_t timestamp_us;     // µs since boot
} EngineState_t;
```

### TuneTable_t (Config)

```c
struct {
    uint8_t rows;              // 16
    uint8_t cols;              // 16
    float data[16][16];        // Interpolation grid
} TuneTable_t;
```

### EcuConfig_t (Persistent)

```c
struct {
    uint8_t num_cylinders;     // 4, 6, or 8
    uint8_t ignition_type;     // 0=wasted, 1=direct
    uint8_t fuel_type;         // 0=batch, 1=sequential
    float spark_min_deg;       // Safety lower limit
    float spark_max_deg;       // Safety upper limit
    uint16_t pw_min_us;        // Min injector pulse
    uint16_t pw_max_us;        // Max injector pulse
    // ... sensor calibration values
} EcuConfig_t;
```

## Timing & Scheduling

### Main Loop (50ms cycle)

```cpp
void loop() {
    // 1. Update sensors (20 Hz)
    if (time_since(last_sensor) >= 50ms) {
        updateSensors();      // Read ADC, filter
        last_sensor = now();
    }
    
    // 2. Tune calculation (10 Hz)
    if (time_since(last_tune) >= 100ms) {
        updateTuneCalcs();    // Interpolate tables
        last_tune = now();
    }
    
    // 3. WebSocket frame handling (async)
    webSocket.loop();
    
    // 4. Safety checks
    checkLimits();            // Rev limiter, fuel cut
}
```

### ISR (Interrupt Service Routine)

```cpp
void crankISR() {
    // Called on rising edge of crank sensor
    // Must complete in <10µs
    
    uint32_t now = micros();
    tooth_period = now - last_tooth_time;
    last_tooth_time = now;
    
    // Calculate RPM (averaging over 2 teeth)
    rpm = (60 * 1000000) / (tooth_period * 2);
    
    // Schedule ignition event (via timer)  
    scheduleSparkEvent(crank_angle);
}
```

## LittleFS Layout

```
/
├── index.html          (50 KB)  — Web dashboard
├── config.json         (2 KB)   — Runtime config
├── tune.json           (20 KB)  — Tune tables (3×16×16)
└── logs/               (variable)
    └── ecu_YYYYMMDD.log
```

## Error Handling

### Sensor Faults
- Out-of-range values trigger **SENSOR_ERROR** flag
- Limp mode activated (reduced rev limit, fixed timing)
- Error logged to circular buffer

### Memory Exhaustion
- Circular ringbuffer prevents overflow
- Oldest log entries discarded when full
- WebSocket frames rejected if heap < 10KB

### Communication Errors
- JSON parse failure → skip command, send error response
- CRC mismatch (optional) → retransmit
- Timeout → client reconnect with exponential backoff

## Power Consumption

| Component | Idle | Running | Notes |
|-----------|------|---------|-------|
| ESP8266 | 80mA | 150mA | WiFi off |
| GPIO outputs | 0mA | 50mA | Coil + injector |
| Sensors (ADC) | 5mA | 5mA | Continuous |
| **Total** | **85mA** | **205mA** | 12V supply |

## Performance Metrics

- **Sensor update latency:** 50ms
- **Tune calculation latency:** 100ms
- **Total loop iteration:** 10-20ms
- **WebSocket frame processing:** <10ms
- **JSON serialization:** <5ms (for state)
- **Table interpolation time:** <1ms

## Future Scalability

### For Larger Tables
- Increase table resolution from 16×16 → 32×32
- Use external EEPROM or SD card
- Implement caching/memoization

### For More Sensors
- Add I2C ADC (ADS1115) for 4+ analog inputs
- Integrate CAN bus (OBD-II style messages)
- Add wideband O2 sensor (SPI interface)

### For Advanced Features
- Closed-loop AFR correction
- Knock detection + retard
- Anti-lag system (ALS) for turbo
- Data logging to SD card
