/*
 * CircuitNotion Sensor Hub Example
 *
 * This example demonstrates advanced sensor management with the CircuitNotion library.
 * It shows how to:
 * - Setup multiple sensor types (temperature, humidity, light, motion)
 * - Configure change detection to optimize data transmission
 * - Handle sensor enable/disable dynamically
 * - Use custom sensors with different data types
 *
 * Hardware Requirements:
 * - ESP8266 or ESP32
 * - DHT22 sensor (temperature & humidity) on pin D4
 * - LDR (Light Dependent Resistor) on pin A0
 * - PIR motion sensor on pin D5
 * - Optional: Pressure sensor on pin D6
 *
 * Dashboard Setup:
 * Create these devices in your CircuitNotion dashboard:
 * - "TEMP_001" (Temperature Sensor)
 * - "HUM_001" (Humidity Sensor)
 * - "LIGHT_001" (Light Sensor)
 * - "MOTION_001" (Motion Sensor)
 * - "PRESSURE_001" (Custom Sensor)
 */

#include <CircuitNotion.h>

// WiFi Configuration
const char *ssid = "Your_WiFi_SSID";
const char *password = "Your_WiFi_Password";

// CircuitNotion Configuration
const char *api_key = "your-api-key-here";
const char *microcontroller_name = "Sensor Hub";

// Device Serials
const char *temp_device = "TEMP_001";
const char *humidity_device = "HUM_001";
const char *light_device = "LIGHT_001";
const char *motion_device = "MOTION_001";
const char *pressure_device = "PRESSURE_001";

// Pin Definitions
const int DHT_PIN = D4;
const int LDR_PIN = A0;
const int PIR_PIN = D5;
const int PRESSURE_PIN = D6;

// Sensor variables
unsigned long lastMotionTime = 0;
float lastPressureReading = 0;

void setup()
{
    Serial.begin(115200);
    Serial.println("\n=== CircuitNotion Sensor Hub ===");

    // Initialize pins
    pinMode(PIR_PIN, INPUT);
    pinMode(PRESSURE_PIN, INPUT);

    // Configure WiFi and CircuitNotion
    CN.setWiFi(ssid, password);
    CN.begin("iot.circuitnotion.com", 443, "/ws", api_key, microcontroller_name);

    // === TEMPERATURE SENSOR ===
    // Read every 30 seconds with 0.5°C change threshold
    auto *tempSensor = CN.addTemperatureSensor(temp_device, "Living Room", 30000, []()
                                               {
        // Simulate DHT22 reading (replace with actual DHT code)
        float temp = 22.0 + sin(millis() / 10000.0) * 3.0; // 19-25°C simulation
        
        // For real DHT22:
        // #include <DHT.h>
        // DHT dht(DHT_PIN, DHT22);
        // float temp = dht.readTemperature();
        
        return SensorValue(temp, "°C"); });
    tempSensor->setChangeThreshold(0.5); // Only send if changed by 0.5°C
    tempSensor->enableChangeDetection(true);

    // === HUMIDITY SENSOR ===
    // Read every 45 seconds with 2% change threshold
    auto *humiditySensor = CN.addHumiditySensor(humidity_device, "Living Room", 45000, []()
                                                {
        // Simulate humidity reading
        float humidity = 55.0 + cos(millis() / 8000.0) * 10.0; // 45-65% simulation
        
        // For real DHT22:
        // DHT dht(DHT_PIN, DHT22);
        // float humidity = dht.readHumidity();
        
        return SensorValue(humidity, "%"); });
    humiditySensor->setChangeThreshold(2.0); // Only send if changed by 2%
    humiditySensor->enableChangeDetection(true);

    // === LIGHT SENSOR ===
    // Read every 10 seconds with 5% change threshold
    auto *lightSensor = CN.addLightSensor(light_device, "Living Room", 10000, []()
                                          {
        // Read LDR value and convert to percentage
        int rawValue = analogRead(LDR_PIN);
        float lightPercent = map(rawValue, 0, 1024, 0, 100);
        
        return SensorValue(lightPercent, "%"); });
    lightSensor->setChangeThreshold(5.0); // Only send if changed by 5%
    lightSensor->enableChangeDetection(true);

    // === MOTION SENSOR ===
    // Check every 5 seconds, immediate transmission on motion
    CN.addMotionSensor(motion_device, "Living Room", 5000, []()
                       {
        bool motionDetected = digitalRead(PIR_PIN);
        
        if (motionDetected) {
            lastMotionTime = millis();
        }
        
        // Return true if motion detected in last 30 seconds
        bool recentMotion = (millis() - lastMotionTime) < 30000;
        return SensorValue(recentMotion ? 1.0 : 0.0, ""); });

    // === CUSTOM PRESSURE SENSOR ===
    // Read every 60 seconds with custom logic
    auto *pressureSensor = CN.addCustomSensor("pressure", pressure_device, "Living Room", 60000, []()
                                              {
        // Simulate pressure sensor reading
        float pressure = 1013.25 + sin(millis() / 20000.0) * 10.0; // 1003-1023 hPa
        
        // For real pressure sensor (e.g., BMP280):
        // #include <Adafruit_BMP280.h>
        // Adafruit_BMP280 bmp;
        // float pressure = bmp.readPressure() / 100.0; // Convert Pa to hPa
        
        lastPressureReading = pressure;
        return SensorValue(pressure, "hPa"); });
    pressureSensor->setChangeThreshold(1.0); // Only send if changed by 1 hPa
    pressureSensor->enableChangeDetection(true);

    // === CALLBACKS ===
    CN.onConnection([](bool connected)
                    {
        if (connected) {
            Serial.println("✓ Sensor Hub Connected!");
            Serial.println("✓ All sensors active and reporting");
            
            // Print sensor status
            CN.printSensorStatus();
        } else {
            Serial.println("✗ Sensor Hub Disconnected");
        } });

    CN.onLog([](String level, String message)
             { Serial.println("[" + level + "] " + message); });

    // Enable auto-reconnection with 15 second intervals
    CN.enableAutoReconnect(true, 15000);

    Serial.println("Starting Sensor Hub...");
    Serial.println("Sensors: Temperature, Humidity, Light, Motion, Pressure");
    CN.connect();
}

void loop()
{
    CN.loop();

    // Print diagnostics every 2 minutes
    static unsigned long lastDiagnostics = 0;
    if (millis() - lastDiagnostics > 120000)
    {
        lastDiagnostics = millis();

        Serial.println("\n=== Sensor Hub Status ===");
        Serial.printf("Uptime: %lu seconds\n", millis() / 1000);
        Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
        Serial.printf("Last Pressure: %.2f hPa\n", lastPressureReading);
        Serial.printf("Total Sensor Readings: %lu\n", CN.getTotalSensorReadings());

        CN.printSensorStatus();
        Serial.println("========================\n");
    }

    // Example: Disable humidity sensor during night (simulated)
    static bool nightModeActive = false;
    bool shouldBeNightMode = (millis() / 10000) % 2 == 0; // Toggle every 10 seconds for demo

    if (shouldBeNightMode && !nightModeActive)
    {
        Serial.println("🌙 Night mode: Disabling humidity sensor");
        CN.disableSensor("humidity", humidity_device);
        nightModeActive = true;
    }
    else if (!shouldBeNightMode && nightModeActive)
    {
        Serial.println("☀️ Day mode: Enabling humidity sensor");
        CN.enableSensor("humidity", humidity_device);
        nightModeActive = false;
    }

    delay(100);
}

/*
 * Advanced Features Demonstrated:
 *
 * 1. Change Detection: Sensors only transmit when values change significantly
 * 2. Multiple Sensor Types: Temperature, humidity, light, motion, custom
 * 3. Dynamic Control: Enable/disable sensors based on conditions
 * 4. Custom Intervals: Different reading frequencies for different sensors
 * 5. Diagnostics: Monitor system health and sensor performance
 *
 * Dashboard Features to Try:
 * - View real-time sensor data charts
 * - Set up alerts for sensor thresholds
 * - Export historical data
 * - Create automation rules based on sensor values
 *
 * Optimization Tips:
 * - Use change detection to reduce bandwidth
 * - Adjust reading intervals based on sensor characteristics
 * - Monitor memory usage for large sensor deployments
 * - Use custom sensors for specialized hardware
 */
