/*
 * CircuitNotion Basic Usage Example
 *
 * This example demonstrates the basic setup and usage of the CircuitNotion library.
 * It shows how to:
 * - Connect to WiFi and CircuitNotion platform
 * - Map a digital device (LED) for dashboard control
 * - Add a temperature sensor
 * - Handle connection events
 *
 * Hardware Requirements:
 * - ESP8266 or ESP32
 * - LED connected to pin D2 (or built-in LED)
 * - DHT22 temperature sensor connected to pin D4 (optional)
 *
 * Setup Instructions:
 * 1. Create an account at iot.circuitnotion.com
 * 2. Add a new microcontroller and copy the API key
 * 3. Create devices in dashboard:
 *    - Device Serial: "LIGHT_001" (Type: Light/LED)
 *    - Device Serial: "TEMP_001" (Type: Temperature Sensor)
 * 4. Update WiFi credentials and API key below
 * 5. Upload to your ESP8266/ESP32
 */

#include <CircuitNotion.h>

// WiFi Configuration
const char *ssid = "Your_WiFi_SSID";         // Replace with your WiFi network name
const char *password = "Your_WiFi_Password"; // Replace with your WiFi password

// CircuitNotion Configuration
const char *api_key = "your-api-key-here";          // Get from iot.circuitnotion.com
const char *microcontroller_name = "Basic Example"; // Name for your microcontroller

// Device Serials (must match devices created in dashboard)
const char *light_device_serial = "LIGHT_001";
const char *temp_device_serial = "TEMP_001";

// Pin Definitions
const int LED_PIN = D2; // Pin for LED control (use LED_BUILTIN for built-in LED)
const int DHT_PIN = D4; // Pin for DHT22 sensor (if using)

void setup()
{
    Serial.begin(115200);
    Serial.println("\n=== CircuitNotion Basic Example ===");

    // Initialize pins
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // Configure WiFi credentials
    CN.setWiFi(ssid, password);

    // Configure CircuitNotion connection (uses default host/port/path)
    CN.begin(api_key, microcontroller_name);

    // Map dashboard device to local pin for automatic control
    // When you toggle this device in the dashboard, the LED will respond
    CN.mapDigitalDevice(light_device_serial, LED_PIN, "Living Room Light");

    // Add temperature sensor that reads every 30 seconds
    CN.addTemperatureSensor(temp_device_serial, "Living Room", 30000, []()
                            {
        // Simulate temperature reading (replace with actual sensor code)
        float temperature = 20.0 + (millis() % 10000) / 1000.0; // Simulated 20-30°C
        
        // For real DHT22 sensor, use:
        // #include <DHT.h>
        // DHT dht(DHT_PIN, DHT22);
        // float temperature = dht.readTemperature();
        
        return SensorValue(temperature, "°C"); });

    // Setup connection callback
    CN.onConnection([](bool connected)
                    {
        if (connected) {
            Serial.println("✓ Connected to CircuitNotion!");
            Serial.println("✓ Check your dashboard at iot.circuitnotion.com");
            digitalWrite(LED_BUILTIN, LOW); // Turn on built-in LED (usually inverted)
        } else {
            Serial.println("✗ Disconnected from CircuitNotion");
            digitalWrite(LED_BUILTIN, HIGH); // Turn off built-in LED
        } });

    // Setup device control callback
    CN.onDeviceControl([](String deviceSerial, String state)
                       {
        Serial.println("Device Control: " + deviceSerial + " -> " + state);
        
        if (deviceSerial == light_device_serial) {
            if (state == "ON" || state == "1") {
                digitalWrite(LED_PIN, HIGH);
                Serial.println("✓ LED turned ON");
            } else {
                digitalWrite(LED_PIN, LOW);
                Serial.println("✓ LED turned OFF");
            }
        } });

    // Setup logging callback for debugging
    CN.onLog([](String level, String message)
             { Serial.println("[" + level + "] " + message); });

    Serial.println("Connecting to CircuitNotion...");
    Serial.println("Dashboard: https://iot.circuitnotion.com");

    // Connect to CircuitNotion platform
    CN.connect();
}

void loop()
{
    // This must be called regularly to maintain connection and handle events
    CN.loop();

    // Small delay to prevent overwhelming the system
    delay(100);
}

/*
 * Usage Instructions:
 *
 * 1. Upload this sketch to your ESP8266/ESP32
 * 2. Open Serial Monitor (115200 baud) to see connection status
 * 3. Go to iot.circuitnotion.com and log into your dashboard
 * 4. Navigate to "Devices" and find your light device
 * 5. Toggle the device ON/OFF to control the LED
 * 6. Check temperature readings in the sensor device
 *
 * Troubleshooting:
 * - Ensure WiFi credentials are correct
 * - Verify API key is copied correctly from dashboard
 * - Check that device serials match between code and dashboard
 * - Monitor Serial output for error messages
 */
