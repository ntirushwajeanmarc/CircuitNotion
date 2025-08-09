#include <CircuitNotion.h>
#include <DHT.h>

// WiFi credentials
const char *ssid = "YourWiFi";
const char *password = "YourPassword";

// CircuitNotion configuration
const char *server = "home.circuitnotion.com";
const char *apiKey = "your-api-key-here";

// Sensors
DHT dht(D1, DHT22);
unsigned long lastMotionTime = 0;
bool lastMotionState = false;

void setup()
{
    Serial.begin(115200);

    // Initialize sensors
    dht.begin();
    pinMode(D3, INPUT); // Motion sensor

    // Configure CircuitNotion with custom intervals and thresholds
    CN.begin(server, 443, "/ws", apiKey, "Advanced IoT Controller", true);
    CN.setWiFi(ssid, password);

    // Add devices
    auto *gate = CN.addDevice("GATE-001", "Smart Gate", D2, "entrance");
    auto *light = CN.addDevice("LIGHT-001", "Smart Light", D4, "living_room");
    auto *fan = CN.addDevice("FAN-001", "Smart Fan", D5, "bedroom");

    // Add temperature sensor with custom threshold (only send if temperature changes by 0.3°C)
    auto *tempSensor = CN.addTemperatureSensor("GATE-001", "entrance", 120000, []() -> SensorValue
                                               {
        float temp = dht.readTemperature();
        SensorValue sv = {temp, "°C"};
        // Add custom metadata
        StaticJsonDocument<64> metadata;
        metadata["sensor_model"] = "DHT22";
        metadata["calibration"] = 0.0;
        sv.metadata = metadata.as<JsonObject>();
        return sv; });
    tempSensor->setChangeThreshold(0.3); // Custom threshold

    // Add humidity sensor
    CN.addHumiditySensor("GATE-001", "entrance", 120000, []() -> SensorValue
                         { return {dht.readHumidity(), "%"}; });

    // Add light sensor with custom logic
    CN.addLightSensor("LIGHT-001", "living_room", 30000, []() -> SensorValue
                      {
        int raw = analogRead(A0);
        float lux = map(raw, 0, 1024, 0, 1000); // Convert to lux
        return {lux, "lux"}; });

    // Add motion sensor with event-driven updates
    auto *motionSensor = CN.addMotionSensor("GATE-001", "entrance", 1000, []() -> SensorValue
                                            {
        bool motion = digitalRead(D3);
        return {motion ? 1.0f : 0.0f, ""}; });
    motionSensor->enableChangeDetection(true); // Only send when motion state changes

    // Custom device control with advanced logic
    CN.onDeviceControl([](String serial, String state)
                       {
        Serial.println("Received command - Device: " + serial + ", State: " + state);
        
        // Custom logic for different devices
        if (serial == "GATE-001") {
            if (state == "on") {
                // Open gate sequence
                Serial.println("Opening gate...");
                // Add your gate opening logic here
            } else {
                // Close gate sequence
                Serial.println("Closing gate...");
            }
        }
        else if (serial == "LIGHT-001") {
            // Smart light with dimming
            if (state.startsWith("dim_")) {
                int brightness = state.substring(4).toInt();
                Serial.println("Setting light brightness to " + String(brightness) + "%");
                // Add PWM dimming logic here
            }
        }
        else if (serial == "FAN-001") {
            // Smart fan with speed control
            if (state.startsWith("speed_")) {
                int speed = state.substring(6).toInt();
                Serial.println("Setting fan speed to " + String(speed));
                // Add fan speed control logic here
            }
        } });

    // Connection status monitoring
    CN.onConnection([](bool connected)
                    {
        if (connected) {
            Serial.println("✅ Connected to CircuitNotion Cloud!");
            digitalWrite(LED_BUILTIN, LOW); // Turn on LED (inverted logic)
        } else {
            Serial.println("❌ Disconnected from CircuitNotion Cloud");
            digitalWrite(LED_BUILTIN, HIGH); // Turn off LED
        } });

    // Custom logging
    CN.onLog([](String message)
             {
                 Serial.println("[LOG] " + message);
                 // You could also send logs to SD card, EEPROM, etc.
             });

    // Connect to server
    CN.connect();

    Serial.println("Advanced CircuitNotion setup complete!");
}

void loop()
{
    // Main CircuitNotion loop
    CN.loop();

    // Custom application logic
    checkSecuritySystem();
    handleEmergencyStop();

    delay(50);
}

void checkSecuritySystem()
{
    // Example: If motion detected at night, turn on lights
    static unsigned long lastSecurityCheck = 0;
    if (millis() - lastSecurityCheck > 5000)
    { // Check every 5 seconds

        bool motion = digitalRead(D3);
        int lightLevel = analogRead(A0);

        // If motion detected and it's dark
        if (motion && lightLevel < 200)
        {
            CN.updateDeviceState("LIGHT-001", DEVICE_ON);
            Serial.println("🔒 Security: Motion detected in dark - turning on light");
        }

        lastSecurityCheck = millis();
    }
}

void handleEmergencyStop()
{
    // Example: Emergency stop button
    static bool lastEmergencyState = false;
    bool emergencyPressed = digitalRead(D6); // Assuming emergency button on D6

    if (emergencyPressed && !lastEmergencyState)
    {
        // Emergency stop - turn off all devices
        Serial.println("🚨 EMERGENCY STOP ACTIVATED!");
        CN.updateDeviceState("GATE-001", DEVICE_OFF);
        CN.updateDeviceState("LIGHT-001", DEVICE_OFF);
        CN.updateDeviceState("FAN-001", DEVICE_OFF);

        // Send emergency notification
        StaticJsonDocument<256> emergency;
        emergency["type"] = "emergency_stop";
        emergency["timestamp"] = millis();
        emergency["location"] = "entrance";
        CN.sendCustomMessage(emergency);
    }

    lastEmergencyState = emergencyPressed;
}
