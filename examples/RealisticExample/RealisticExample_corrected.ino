/*
  CircuitNotion Library - Realistic Example

  This example demonstrates the CORRECT workflow for CircuitNotion:

  1. Devices are registered in the CircuitNotion dashboard FIRST
  2. This microcontroller connects using an API key
  3. Sensors are attached to devices via this microcontroller
  4. Local device control is handled through pin mapping

  This reflects the actual architecture where devices exist in the system
  independently and microcontrollers act as sensor/control interfaces.

  Hardware Setup:
  - ESP8266 with WiFi connection
  - DHT22 sensor on pin D4
  - LED on pin D2 (mapped to device from dashboard)
  - Light sensor (LDR) on analog pin A0

  Prerequisites:
  1. Create devices in CircuitNotion dashboard
  2. Get API key for this microcontroller
  3. Note the device serials for mapping
*/

#include <CircuitNotion.h>
#include <DHT.h>

// WiFi credentials
const char *ssid = "YourWiFiNetwork";
const char *password = "YourWiFiPassword";

// CircuitNotion configuration
const char *cn_host = "circuitnotion.com";
const int cn_port = 443;
const char *cn_path = "/ws";
const char *cn_api_key = "your-microcontroller-api-key";
const char *microcontroller_name = "Home_Sensor_Hub";

// Hardware pins
#define DHT_PIN D4
#define LED_PIN D2
#define LDR_PIN A0

// Sensor setup
DHT dht(DHT_PIN, DHT22);

// Device serials from dashboard (these would be real serials from your dashboard)
const char *temperature_device_serial = "TEMP_SENSOR_001";
const char *humidity_device_serial = "HUMIDITY_SENSOR_001";
const char *light_device_serial = "LIGHT_SENSOR_001";
const char *led_device_serial = "LED_DEVICE_001";

void setup()
{
    Serial.begin(115200);

    // Initialize hardware
    dht.begin();
    pinMode(LDR_PIN, INPUT);

    Serial.println("CircuitNotion Realistic Example Starting...");
    Serial.println("This demonstrates the CORRECT workflow:");
    Serial.println("1. Devices registered in dashboard first");
    Serial.println("2. Microcontroller connects with API key");
    Serial.println("3. Sensors attached to existing devices");
    Serial.println("4. Local device control via pin mapping");

    // Setup WiFi
    CN.setWiFi(ssid, password);

    // Configure CircuitNotion connection
    CN.begin(cn_host, cn_port, cn_path, cn_api_key, microcontroller_name);

    // Setup device mappings for local control
    // These map dashboard devices to local pins for control
    CN.mapDigitalDevice(led_device_serial, LED_PIN, "Status LED");

    // Setup sensors attached to dashboard devices
    // These sensors belong to devices that already exist in the dashboard
    CN.addTemperatureSensor(temperature_device_serial, "Living Room", 30000, readTemperature);
    CN.addHumiditySensor(humidity_device_serial, "Living Room", 30000, readHumidity);
    CN.addLightSensor(light_device_serial, "Living Room", 10000, readLightLevel);

    // Setup callbacks
    CN.onDeviceControl([](String deviceSerial, String state)
                       {
                           Serial.println("Device Control: " + deviceSerial + " -> " + state);
                           // Local device control is automatically handled by device mappings
                           // Additional custom logic can go here
                       });

    CN.onConnection([](bool connected)
                    {
        if (connected) {
            Serial.println("✓ Connected and authenticated to CircuitNotion!");
            CN.printDiagnostics();
        } else {
            Serial.println("✗ Disconnected from CircuitNotion");
        } });

    CN.onLog([](String message)
             { Serial.println("[CN] " + message); });

    // Enable auto-reconnect
    CN.enableAutoReconnect(true, 10000);

    // Connect to CircuitNotion
    CN.connect();

    Serial.println("Setup complete! Sensor readings will start once connected.");
}

void loop()
{
    // Main CircuitNotion loop - handles WebSocket, sensors, and device control
    CN.loop();

    // Optional: Print diagnostics every 5 minutes
    static unsigned long lastDiagnostics = 0;
    if (millis() - lastDiagnostics > 300000)
    {
        lastDiagnostics = millis();
        CN.printDiagnostics();
        CN.printSensorStatus();
        CN.printDeviceMappings();
    }

    delay(100);
}

// Sensor reading functions
SensorValue readTemperature()
{
    float temp = dht.readTemperature();
    if (isnan(temp))
    {
        Serial.println("Failed to read temperature from DHT sensor!");
        temp = 0.0;
    }
    return SensorValue(temp, "°C");
}

SensorValue readHumidity()
{
    float humidity = dht.readHumidity();
    if (isnan(humidity))
    {
        Serial.println("Failed to read humidity from DHT sensor!");
        humidity = 0.0;
    }
    return SensorValue(humidity, "%");
}

SensorValue readLightLevel()
{
    int rawValue = analogRead(LDR_PIN);
    // Convert to percentage (0-100)
    float lightPercent = (rawValue / 1024.0) * 100.0;
    return SensorValue(lightPercent, "%");
}
