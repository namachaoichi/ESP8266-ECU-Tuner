#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include "Config.h"

// ============================================================================
// ENGINE STATE STRUCTURES
// ============================================================================

// Real-time engine sensor readings
typedef struct {
    uint16_t rpm;              // Engine RPM
    float map_kpa;             // Manifold Absolute Pressure (kPa)
    float tps_percent;         // Throttle Position (%)
    float clt_celsius;         // Coolant Temperature (°C)
    float iat_celsius;         // Intake Air Temperature (°C)
    float battery_voltage;     // Battery voltage (V)
    uint32_t timestamp_us;     // Timestamp (microseconds)
} SensorReading_t;

// Calculated engine parameters
typedef struct {
    float spark_advance_deg;   // Spark advance (degrees BTDC)
    float pulse_width_us;      // Fuel injector pulse width (microseconds)
    float dwell_time_ms;       // Ignition dwell time (milliseconds)
    uint8_t fuel_cut;          // Fuel cut flag (0 = inject, 1 = cut)
    uint8_t rev_limit;         // Rev limiter active flag
    uint8_t error_flags;       // Sensor error bitmask
} EngineCalc_t;

// Configuration parameters
typedef struct {
    uint8_t num_cylinders;     // Number of cylinders (4, 6, 8)
    uint8_t ignition_type;     // Ignition configuration (0=wasted spark, 1=direct coil)
    uint8_t fuel_type;         // Fuel injection type (0=batch, 1=sequential)
    float spark_min_deg;       // Minimum spark advance (°BTDC)
    float spark_max_deg;       // Maximum spark advance (°BTDC)
    float dwell_max_ms;        // Maximum dwell time (ms)
    uint16_t pw_min_us;        // Minimum pulse width (µs)
    uint16_t pw_max_us;        // Maximum pulse width (µs)
    float injector_dead_time;  // Injector opening time (ms)
    uint16_t rev_limit_rpm;    // Rev limiter RPM threshold
    uint16_t fuel_cut_rpm;     // Fuel cut RPM threshold
    float map_min_kpa;         // MAP sensor minimum (kPa)
    float map_max_kpa;         // MAP sensor maximum (kPa)
    uint8_t tps_min_percent;   // TPS minimum (%)
    uint8_t tps_max_percent;   // TPS maximum (%)
    float clt_min_celsius;     // CLT minimum (°C)
    float clt_max_celsius;     // CLT maximum (°C)
} EcuConfig_t;

// 3D Tune Table (RPM x MAP)
typedef struct {
    uint8_t rows;              // Number of RPM rows
    uint8_t cols;              // Number of MAP columns
    float data[16][16];        // Table data (max 16x16)
} TuneTable_t;

// Crank trigger tooth info
typedef struct {
    uint16_t tooth_count;      // Total teeth in wheel (e.g., 36)
    uint16_t missing_teeth;    // Count of missing teeth (e.g., 1 for 36-1)
    uint32_t last_tooth_time;  // Timestamp of last tooth edge (µs)
    uint32_t tooth_period;     // Time between teeth (µs)
    uint16_t current_rpm;      // Calculated RPM
} CrankTrigger_t;

// Tuning command structure
typedef struct {
    uint8_t cmd_type;          // Command type (see command constants)
    uint8_t param1;            // Parameter 1 (table index, row, etc.)
    uint8_t param2;            // Parameter 2 (column, etc.)
    float value;               // Float value for tuning
    uint8_t checksum;          // Simple checksum for validation
} TuneCommand_t;

// ============================================================================
// ENUM CONSTANTS
// ============================================================================

// Sensor error flags
#define SENSOR_ERR_MAP         0x01
#define SENSOR_ERR_TPS         0x02
#define SENSOR_ERR_CLT         0x04
#define SENSOR_ERR_IAT         0x08
#define SENSOR_ERR_VBAT        0x10
#define SENSOR_ERR_CRANK       0x20
#define SENSOR_ERR_CAM         0x40

// Tuning command types
#define CMD_GET_STATE          1
#define CMD_SET_TABLE          2
#define CMD_GET_TABLE          3
#define CMD_SET_CONFIG         4
#define CMD_GET_CONFIG         5
#define CMD_SAVE_CONFIG        6
#define CMD_RESET_CONFIG       7
#define CMD_SAVE_TABLES        8
#define CMD_RESET_TABLES       9
#define CMD_REBOOT            10
#define CMD_GET_LOG           11
#define CMD_CLEAR_LOG         12

// WebSocket message types
#define WS_MSG_TYPE_STATE      1
#define WS_MSG_TYPE_CONFIG     2
#define WS_MSG_TYPE_TABLE      3
#define WS_MSG_TYPE_ERROR      4
#define WS_MSG_TYPE_LOG        5

#endif  // TYPES_H
