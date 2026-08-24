#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stddef.h>

// ============================================================================
// FIRMWARE VERSION
// ============================================================================
#define FW_VERSION_MAJOR    1
#define FW_VERSION_MINOR    0
#define FW_VERSION_PATCH    0

// ============================================================================
// HARDWARE CONFIGURATION
// ============================================================================

// Number of cylinders
#define NUM_CYLINDERS       4

// Number of ignition coils (typically = cylinders)
#define NUM_COILS           NUM_CYLINDERS

// Number of fuel injectors
#define NUM_INJECTORS       NUM_CYLINDERS

// ============================================================================
// SERIAL & COMMUNICATION
// ============================================================================

// Serial baud rate for tuning interface
#define SERIAL_BAUD         115200

// WebSocket-over-Serial frame size (max command size)
#define WS_FRAME_MAX        512

// Command timeout (ms) — how long to wait for response
#define CMD_TIMEOUT_MS      5000

// ============================================================================
// TUNE TABLE CONFIGURATION
// ============================================================================

// Spark advance table dimensions (RPM x MAP)
#define TUNE_ROWS           16
#define TUNE_COLS           16

// Default table values
#define SPARK_MIN_DEG       -5.0f
#define SPARK_MAX_DEG       40.0f
#define SPARK_DEFAULT_DEG   25.0f

// VE (volumetric efficiency) table
#define VE_MIN_PCT          50.0f
#define VE_MAX_PCT          120.0f
#define VE_DEFAULT_PCT      90.0f

// AFR target table
#define AFR_MIN             11.0f
#define AFR_MAX             15.5f
#define AFR_DEFAULT         14.7f

// ============================================================================
// SENSOR CONFIGURATION
// ============================================================================

// ADC input pins (configured in Pins.h)
#define NUM_SENSORS         6

// Sensor smoothing (exponential moving average alpha)
#define SENSOR_SMOOTH_ALPHA 0.1f

// ============================================================================
// STORAGE (LittleFS)
// ============================================================================

// Config file path
#define CONFIG_FILE_PATH    "/config.json"

// Tune tables file path
#define TUNE_FILE_PATH      "/tune.json"

// Max file size for config (should fit in memory)
#define CONFIG_MAX_SIZE     4096  // 4 KB

// Tune table JSON size estimate
#define TUNE_MAX_SIZE       16384  // 16 KB (3x 16x16 tables + metadata)

// ============================================================================
// ENGINE LIMITS
// ============================================================================

// RPM limits
#define RPM_MAX             7000
#define RPM_MIN             0

// Spark advance limits (safety)
#define ADVANCE_CLAMP_MIN   -10.0f
#define ADVANCE_CLAMP_MAX   50.0f

// Fuel pulse width limits (microseconds)
#define PULSE_WIDTH_MIN_US  1000
#define PULSE_WIDTH_MAX_US  25000

// ============================================================================
// TIMING CONFIGURATION
// ============================================================================

// Crank trigger tooth count (36-1 wheel = 35 teeth + 1 gap)
#define CRANK_TEETH         35

// Sensor update interval (ms)
#define SENSOR_UPDATE_INTERVAL_MS   50

// Tune table interpolation update (ms)
#define TUNE_UPDATE_INTERVAL_MS     100

// ============================================================================
// MEMORY & RUNTIME
// ============================================================================

// Stack size for background tasks
#define BACKGROUND_STACK_SIZE  2048

// Max number of WebSocket clients (usually 1 over Serial)
#define MAX_WS_CLIENTS      1

// ============================================================================
// DEBUG SETTINGS
// ============================================================================

// Enable debug logging
#define DEBUG_ENABLED       1

// Print sensor values to Serial every N updates
#define DEBUG_SENSOR_RATE   50  // Every 2.5 seconds (50 * 50ms)

// ============================================================================
// ENUM DEFINITIONS
// ============================================================================

// Cylinder configuration enum
typedef enum {
    CYLE_CONFIG_4CYL = 4,
    CYLE_CONFIG_6CYL = 6,
    CYLE_CONFIG_8CYL = 8,
} CylinderConfig_t;

// Ignition type
typedef enum {
    IGNITION_TYPE_WASTED_SPARK = 0,
    IGNITION_TYPE_DIRECT_COIL   = 1,
} IgnitionType_t;

// Fuel injection type
typedef enum {
    FUEL_TYPE_BATCH     = 0,
    FUEL_TYPE_SEQUENTIAL = 1,
} FuelType_t;

// Engine state
typedef enum {
    ENGINE_STATE_STOPPED  = 0,
    ENGINE_STATE_CRANKING = 1,
    ENGINE_STATE_RUNNING  = 2,
} EngineState_t;

#endif  // CONFIG_H
