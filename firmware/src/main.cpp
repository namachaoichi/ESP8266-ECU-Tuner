#include <Arduino.h>
#include <LittleFS.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include "Config.h"
#include "Pins.h"
#include "Types.h"

// Globals
ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);
EngineState_t engine_state = ENGINE_STATE_STOPPED;
uint16_t rpm = 0;
float map_kpa = 0.0f;
float tps_percent = 0.0f;
float spark_advance = 0.0f;
uint32_t pulse_width_us = 0;

// Tune tables (16x16 RPM x MAP)
float spark_table[TUNE_ROWS][TUNE_COLS];
float ve_table[TUNE_ROWS][TUNE_COLS];
float afr_table[TUNE_ROWS][TUNE_COLS];

// Configuration structure
struct {
    uint8_t cylinders = NUM_CYLINDERS;
    uint8_t ignition_type = IGNITION_TYPE_DIRECT_COIL;
    uint8_t fuel_type = FUEL_TYPE_SEQUENTIAL;
    float adv_min = SPARK_MIN_DEG;
    float adv_max = SPARK_MAX_DEG;
    float dwell_max = 5.0f;
    uint16_t pw_min = PULSE_WIDTH_MIN_US;
    uint16_t pw_max = PULSE_WIDTH_MAX_US;
    float dead_time = 1.0f;
    uint16_t map_min = 10;
    uint16_t map_max = 200;
    uint8_t tps_min = 0;
    uint8_t tps_max = 100;
} config;

// Prototypes
void initFilesystem();
void initWebServer();
void initWebSocket();
void loadConfig();
void saveConfig();
void loadTables();
void saveTables();
void handleWebSocket(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
void updateSensors();
void updateEngine();
void broadcastState();

void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(1000);
    Serial.println("\n\nESP8266 ECU Tuner v1.0");
    Serial.println("Initializing...");

    // Initialize pins
    pinMode(CRANK_SENSOR_PIN, INPUT);
    pinMode(COIL_1_PIN, OUTPUT);
    pinMode(COIL_2_PIN, OUTPUT);
    if (NUM_COILS > 2) pinMode(COIL_3_PIN, OUTPUT);
    if (NUM_COILS > 3) pinMode(COIL_4_PIN, OUTPUT);

    // Initialize filesystem
    initFilesystem();

    // Load configuration and tune tables
    loadConfig();
    loadTables();

    // Initialize web server
    initWebServer();
    initWebSocket();

    // Initialize WiFi (AP mode for direct connection)
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ECU-Tuner", "12345678");
    IPAddress ip = WiFi.softAPIP();
    Serial.print("WiFi AP Started: ");
    Serial.println(ip);

    server.begin();
    webSocket.begin();

    Serial.println("Setup complete. Ready for tuning.");
}

void loop() {
    webSocket.loop();
    server.handleClient();
    
    static unsigned long last_sensor_update = 0;
    static unsigned long last_engine_update = 0;
    static unsigned long last_broadcast = 0;

    unsigned long now = millis();

    if (now - last_sensor_update >= SENSOR_UPDATE_INTERVAL_MS) {
        updateSensors();
        last_sensor_update = now;
    }

    if (now - last_engine_update >= TUNE_UPDATE_INTERVAL_MS) {
        updateEngine();
        last_engine_update = now;
    }

    if (now - last_broadcast >= 100) {
        broadcastState();
        last_broadcast = now;
    }
}

// ============================================================================
// FILESYSTEM
// ============================================================================

void initFilesystem() {
    if (!LittleFS.begin()) {
        Serial.println("LittleFS Mount Failed! Formatting...");
        LittleFS.format();
        if (!LittleFS.begin()) {
            Serial.println("LittleFS initialization failed.");
            return;
        }
    }
    Serial.println("LittleFS Mounted Successfully");

    // List files
    Serial.println("Files in LittleFS:");
    Dir dir = LittleFS.openDir("/");
    while (dir.next()) {
        Serial.print("  ");
        Serial.print(dir.fileName());
        Serial.print(" - ");
        Serial.println(dir.fileSize());
    }
}

// ============================================================================
// CONFIGURATION
// ============================================================================

void loadConfig() {
    if (!LittleFS.exists(CONFIG_FILE_PATH)) {
        Serial.println("Config file not found, using defaults.");
        return;
    }

    File file = LittleFS.open(CONFIG_FILE_PATH, "r");
    if (!file) {
        Serial.println("Failed to open config file.");
        return;
    }

    StaticJsonDocument<CONFIG_MAX_SIZE> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.println("JSON parse error in config: " + String(error.c_str()));
        return;
    }

    config.cylinders = doc["cylinders"] | NUM_CYLINDERS;
    config.ignition_type = doc["ignition_type"] | IGNITION_TYPE_DIRECT_COIL;
    config.fuel_type = doc["fuel_type"] | FUEL_TYPE_SEQUENTIAL;
    config.adv_min = doc["adv_min"] | SPARK_MIN_DEG;
    config.adv_max = doc["adv_max"] | SPARK_MAX_DEG;
    config.dwell_max = doc["dwell_max"] | 5.0f;
    config.pw_min = doc["pw_min"] | PULSE_WIDTH_MIN_US;
    config.pw_max = doc["pw_max"] | PULSE_WIDTH_MAX_US;
    config.dead_time = doc["dead_time"] | 1.0f;
    config.map_min = doc["map_min"] | 10;
    config.map_max = doc["map_max"] | 200;
    config.tps_min = doc["tps_min"] | 0;
    config.tps_max = doc["tps_max"] | 100;

    Serial.println("Config loaded successfully.");
}

void saveConfig() {
    StaticJsonDocument<CONFIG_MAX_SIZE> doc;
    doc["cylinders"] = config.cylinders;
    doc["ignition_type"] = config.ignition_type;
    doc["fuel_type"] = config.fuel_type;
    doc["adv_min"] = config.adv_min;
    doc["adv_max"] = config.adv_max;
    doc["dwell_max"] = config.dwell_max;
    doc["pw_min"] = config.pw_min;
    doc["pw_max"] = config.pw_max;
    doc["dead_time"] = config.dead_time;
    doc["map_min"] = config.map_min;
    doc["map_max"] = config.map_max;
    doc["tps_min"] = config.tps_min;
    doc["tps_max"] = config.tps_max;

    File file = LittleFS.open(CONFIG_FILE_PATH, "w");
    if (!file) {
        Serial.println("Failed to open config file for writing.");
        return;
    }

    serializeJson(doc, file);
    file.close();
    Serial.println("Config saved successfully.");
}

// ============================================================================
// TUNE TABLES
// ============================================================================

void loadTables() {
    // Initialize with default values
    for (int i = 0; i < TUNE_ROWS; i++) {
        for (int j = 0; j < TUNE_COLS; j++) {
            spark_table[i][j] = SPARK_DEFAULT_DEG;
            ve_table[i][j] = VE_DEFAULT_PCT;
            afr_table[i][j] = AFR_DEFAULT;
        }
    }

    if (!LittleFS.exists(TUNE_FILE_PATH)) {
        Serial.println("Tune file not found, using defaults.");
        return;
    }

    File file = LittleFS.open(TUNE_FILE_PATH, "r");
    if (!file) return;

    StaticJsonDocument<TUNE_MAX_SIZE> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.println("JSON parse error in tune: " + String(error.c_str()));
        return;
    }

    // Load spark table
    if (doc.containsKey("spark")) {
        JsonArray spark_arr = doc["spark"];
        for (size_t i = 0; i < spark_arr.size() && i < TUNE_ROWS; i++) {
            JsonArray row = spark_arr[i];
            for (size_t j = 0; j < row.size() && j < TUNE_COLS; j++) {
                spark_table[i][j] = row[j];
            }
        }
    }

    Serial.println("Tune tables loaded successfully.");
}

void saveTables() {
    StaticJsonDocument<TUNE_MAX_SIZE> doc;

    JsonArray spark_arr = doc.createNestedArray("spark");
    for (int i = 0; i < TUNE_ROWS; i++) {
        JsonArray row = spark_arr.createNestedArray();
        for (int j = 0; j < TUNE_COLS; j++) {
            row.add(spark_table[i][j]);
        }
    }

    File file = LittleFS.open(TUNE_FILE_PATH, "w");
    if (!file) {
        Serial.println("Failed to open tune file for writing.");
        return;
    }

    serializeJson(doc, file);
    file.close();
    Serial.println("Tune tables saved successfully.");
}

// ============================================================================
// WEB SERVER
// ============================================================================

void initWebServer() {
    // Serve embedded HTML dashboard
    server.on("/", HTTP_GET, []() {
        server.sendHeader("Content-Encoding", "gzip");
        server.send(200, "text/html", (const char*)&__builtin_expect);
    });

    // REST API endpoints
    server.on("/api/state", HTTP_GET, []() {
        StaticJsonDocument<512> doc;
        doc["rpm"] = rpm;
        doc["map"] = map_kpa;
        doc["tps"] = tps_percent;
        doc["advance"] = spark_advance;
        doc["pw"] = pulse_width_us;
        doc["state"] = engine_state;

        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response);
    });

    server.on("/api/config", HTTP_GET, []() {
        StaticJsonDocument<CONFIG_MAX_SIZE> doc;
        doc["cylinders"] = config.cylinders;
        doc["ignition_type"] = config.ignition_type;
        doc["fuel_type"] = config.fuel_type;
        doc["adv_min"] = config.adv_min;
        doc["adv_max"] = config.adv_max;
        doc["dwell_max"] = config.dwell_max;
        doc["pw_min"] = config.pw_min;
        doc["pw_max"] = config.pw_max;
        doc["dead_time"] = config.dead_time;
        doc["map_min"] = config.map_min;
        doc["map_max"] = config.map_max;
        doc["tps_min"] = config.tps_min;
        doc["tps_max"] = config.tps_max;

        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response);
    });
}

void initWebSocket() {
    webSocket.onEvent(handleWebSocket);
}

void handleWebSocket(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
    if (type == WStype_TEXT) {
        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (error) {
            Serial.println("WebSocket JSON error");
            return;
        }

        String cmd = doc["cmd"];

        if (cmd == "get_state") {
            StaticJsonDocument<512> response;
            response["type"] = "state";
            response["data"]["rpm"] = rpm;
            response["data"]["map"] = map_kpa;
            response["data"]["tps"] = tps_percent;
            response["data"]["advance"] = spark_advance;
            response["data"]["pw"] = pulse_width_us;

            String resp;
            serializeJson(response, resp);
            webSocket.sendTXT(num, resp);
        } else if (cmd == "get_config") {
            StaticJsonDocument<CONFIG_MAX_SIZE> response;
            response["type"] = "config";
            response["data"]["cylinders"] = config.cylinders;
            response["data"]["adv_min"] = config.adv_min;
            response["data"]["adv_max"] = config.adv_max;
            // ... add more config fields

            String resp;
            serializeJson(response, resp);
            webSocket.sendTXT(num, resp);
        } else if (cmd == "set_config") {
            config.cylinders = doc["data"]["cylinders"];
            config.adv_min = doc["data"]["adv_min"];
            config.adv_max = doc["data"]["adv_max"];
            // ... update more config fields
            saveConfig();
            webSocket.sendTXT(num, "{\"status\":\"ok\"}");
        } else if (cmd == "save_config") {
            saveConfig();
            webSocket.sendTXT(num, "{\"status\":\"ok\"}");
        }
    }
}

// ============================================================================
// ENGINE CONTROL
// ============================================================================

void updateSensors() {
    // Simulate sensor reads (replace with actual ADC reads)
    static uint32_t tick = 0;
    tick++;

    // Simulate RPM (sawtooth pattern for testing)
    rpm = (tick % 200) * 35;
    map_kpa = 50 + 30 * sin(tick * 0.01);
    tps_percent = 50 + 30 * sin(tick * 0.005);

#if DEBUG_ENABLED
    if (tick % DEBUG_SENSOR_RATE == 0) {
        Serial.print("RPM: ");
        Serial.print(rpm);
        Serial.print(" MAP: ");
        Serial.print(map_kpa);
        Serial.print(" TPS: ");
        Serial.println(tps_percent);
    }
#endif
}

void updateEngine() {
    if (rpm > 300) {
        engine_state = ENGINE_STATE_RUNNING;
        spark_advance = 25.0f;  // Simple fixed value
        pulse_width_us = 5000;   // 5ms
    } else {
        engine_state = ENGINE_STATE_STOPPED;
        spark_advance = 0.0f;
        pulse_width_us = 0;
    }
}

void broadcastState() {
    StaticJsonDocument<256> doc;
    doc["type"] = "state";
    doc["data"]["rpm"] = rpm;
    doc["data"]["map"] = map_kpa;
    doc["data"]["tps"] = tps_percent;
    doc["data"]["advance"] = spark_advance;
    doc["data"]["pw"] = pulse_width_us;
    doc["data"]["state"] = engine_state;

    String message;
    serializeJson(doc, message);
    webSocket.broadcastTXT(message);
}
