#ifndef PINS_H
#define PINS_H

#include <Arduino.h>

// ============================================================================
// GPIO PIN DEFINITIONS (ESP8266)
// ============================================================================
// Note: ESP8266 pins can be referenced as D0, D1, etc. or GPIO numbers
// D0  = GPIO16 (no PWM, wake only)
// D1  = GPIO5  (I2C SCL)
// D2  = GPIO4  (I2C SDA)
// D3  = GPIO0  (FLASH button)
// D4  = GPIO2  (onboard LED, UART TX)
// D5  = GPIO14 (SPI CLK)
// D6  = GPIO12 (SPI MISO)
// D7  = GPIO13 (SPI MOSI)
// D8  = GPIO15 (SPI CS, pulled to GND at boot)
// A0  = ADC (analog input, 0-3.3V)

// ============================================================================
// SENSOR INPUTS (DIGITAL)
// ============================================================================

// Crank sensor input (must support interrupts)
#define CRANK_SENSOR_PIN    D5  // GPIO14

// Cam sensor input (optional, for sequential fuel injection)
#define CAM_SENSOR_PIN      D6  // GPIO12

// ============================================================================
// OUTPUT PINS (IGNITION & FUEL)
// ============================================================================

// Ignition coil control pins (outputs)
#define COIL_1_PIN          D1  // GPIO5
#define COIL_2_PIN          D2  // GPIO4
#define COIL_3_PIN          D7  // GPIO13  (if using 4+ cylinders)
#define COIL_4_PIN          D8  // GPIO15  (if using 4+ cylinders)

// For 6-8 cylinders, may need additional pins or use expander

// Fuel injector control pins (outputs)
#define INJECTOR_1_PIN      D3  // GPIO0   (optional: use FLASH for GPIO0 mux)
#define INJECTOR_2_PIN      D4  // GPIO2   (optional: onboard LED — beware!)
#define INJECTOR_3_PIN      A0  // Cannot use as digital output (ADC only)
#define INJECTOR_4_PIN      -1  // Not available without expander

// For more injectors, consider using I2C/SPI expander (MCP23017, etc.)

// ============================================================================
// ANALOG SENSOR INPUTS (ADC)
// ============================================================================

// ESP8266 has single ADC on pin A0 (GPIO17)
// For multiple analog sensors, use external ADC (ADS1115, MCP3204)

#define ADC_MAP_PIN         A0  // Map sensor (can multiplex via MUX or external ADC)
#define ADC_TPS_PIN         -1  // Throttle Position Sensor (requires external ADC)
#define ADC_CLT_PIN         -1  // Coolant Temperature (requires external ADC)
#define ADC_IAT_PIN         -1  // Intake Air Temperature (requires external ADC)

// For best practice, use external I2C ADC:
// ADS1115 I2C addresses: 0x48, 0x49, 0x4A, 0x4B
// - Channel 0: MAP
// - Channel 1: TPS
// - Channel 2: CLT
// - Channel 3: IAT
// or MCP3204 SPI ADC

// ============================================================================
// I2C CONFIGURATION
// ============================================================================

#define I2C_SDA_PIN         D2  // GPIO4  (standard SDA)
#define I2C_SCL_PIN         D1  // GPIO5  (standard SCL)
#define I2C_CLOCK_HZ        100000  // 100 kHz

// ============================================================================
// SPI CONFIGURATION (optional, for SD card or SPI ADC)
// ============================================================================

// Note: ESP8266 SPI pins are fixed:
// CLK  = GPIO14 (D5)
// MOSI = GPIO13 (D7)
// MISO = GPIO12 (D6)
// CS   = GPIO15 (D8) or GPIO4 (D2) or GPIO0 (D3) - software selectable

#define SPI_CLOCK_HZ        4000000  // 4 MHz for external devices

// ============================================================================
// FILESYSTEM CONFIGURATION
// ============================================================================

// LittleFS mount point (ESP8266 uses SPIFFS or LittleFS on flash)
#define SPIFFS_MOUNT_POINT  "/spiffs"

// ============================================================================
// SERIAL CONFIGURATION
// ============================================================================

// Serial port for tuning (always UART0 at GPIO1/GPIO3)
// RX = GPIO3 (RX0)
// TX = GPIO1 (TX0)
// Baud rate defined in Config.h

// ============================================================================
// HELPER MACROS
// ============================================================================

// Convert GPIO number to pin name (D0-D8)
#define GPIO_TO_DPIN(gpio) \
    ((gpio == 16) ? D0 : \
     (gpio == 5)  ? D1 : \
     (gpio == 4)  ? D2 : \
     (gpio == 0)  ? D3 : \
     (gpio == 2)  ? D4 : \
     (gpio == 14) ? D5 : \
     (gpio == 12) ? D6 : \
     (gpio == 13) ? D7 : \
     (gpio == 15) ? D8 : -1)

#endif  // PINS_H
