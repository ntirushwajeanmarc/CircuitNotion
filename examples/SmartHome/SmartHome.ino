/*
 * CircuitNotion Smart Home Example
 *
 * This comprehensive example demonstrates a complete smart home setup with:
 * - Multiple room control (lights, fans, blinds)
 * - Environmental monitoring (temperature, humidity, air quality)
 * - Security features (motion detection, door sensors)
 * - Energy monitoring (power consumption tracking)
 * - Automated responses based on sensor data
 *
 * Hardware Requirements:
 * - ESP32 (recommended for more GPIO pins)
 * - Relay modules for AC devices
 * - DHT22 sensors for temperature/humidity
 * - PIR motion sensors
 * - Reed switches for door/window sensors
 * - Current sensors for power monitoring
 * - Servo motors for automated blinds
 *
 * Dashboard Setup:
 * Create these devices in your CircuitNotion dashboard:
 * Lights: LIGHT_LR, LIGHT_BR, LIGHT_KT
 * Fans: FAN_LR, FAN_BR
 * Blinds: BLIND_LR, BLIND_BR
 * Sensors: TEMP_LR, HUM_LR, MOTION_LR, DOOR_MAIN, POWER_MAIN
 */

#include <CircuitNotion.h>
#include <Servo.h>

// WiFi Configuration
const char *ssid = "Your_WiFi_SSID";
const char *password = "Your_WiFi_Password";

// CircuitNotion Configuration
const char *api_key = "your-api-key-here";
const char *microcontroller_name = "Smart Home Hub";

// Device Serials - Lights
const char *living_room_light = "LIGHT_LR";
const char *bedroom_light = "LIGHT_BR";
const char *kitchen_light = "LIGHT_KT";

// Device Serials - Fans
const char *living_room_fan = "FAN_LR";
const char *bedroom_fan = "FAN_BR";

// Device Serials - Blinds
const char *living_room_blinds = "BLIND_LR";
const char *bedroom_blinds = "BLIND_BR";

// Device Serials - Sensors
const char *temp_sensor = "TEMP_LR";
const char *humidity_sensor = "HUM_LR";
const char *motion_sensor = "MOTION_LR";
const char *door_sensor = "DOOR_MAIN";
const char *power_sensor = "POWER_MAIN";

// Pin Definitions - Outputs
const int LIGHT_LR_PIN = 2;
const int LIGHT_BR_PIN = 4;
const int LIGHT_KT_PIN = 5;
const int FAN_LR_PIN = 18;
const int FAN_BR_PIN = 19;
const int BLIND_LR_PIN = 21;
const int BLIND_BR_PIN = 22;

// Pin Definitions - Sensors
const int DHT_PIN = 15;
const int PIR_PIN = 14;
const int DOOR_PIN = 12;
const int CURRENT_PIN = 35;

// Servo objects for automated blinds
Servo livingRoomBlinds;
Servo bedroomBlinds;

// Smart home state variables
struct SmartHomeState
{
    bool homeOccupied = false;
    bool nightMode = false;
    bool awayMode = false;
    float currentTemp = 0;
    float currentHumidity = 0;
    unsigned long lastMotionTime = 0;
    unsigned long lastDoorOpenTime = 0;
    float dailyPowerConsumption = 0;
} homeState;

void setup()
{
    Serial.begin(115200);
    Serial.println("\n=== CircuitNotion Smart Home Hub ===");

    // Initialize output pins
    pinMode(LIGHT_LR_PIN, OUTPUT);
    pinMode(LIGHT_BR_PIN, OUTPUT);
    pinMode(LIGHT_KT_PIN, OUTPUT);
    pinMode(FAN_LR_PIN, OUTPUT);
    pinMode(FAN_BR_PIN, OUTPUT);

    // Initialize input pins
    pinMode(PIR_PIN, INPUT);
    pinMode(DOOR_PIN, INPUT_PULLUP);

    // Initialize servos for blinds
    livingRoomBlinds.attach(BLIND_LR_PIN);
    bedroomBlinds.attach(BLIND_BR_PIN);

    // Configure CircuitNotion
    CN.setWiFi(ssid, password);
    CN.begin("home.circuitnotion.com", 443, "/ws", api_key, microcontroller_name);

    // === DEVICE MAPPINGS ===
    // Map lights for dashboard control
    CN.mapDigitalDevice(living_room_light, LIGHT_LR_PIN, "Living Room Light");
    CN.mapDigitalDevice(bedroom_light, LIGHT_BR_PIN, "Bedroom Light");
    CN.mapDigitalDevice(kitchen_light, LIGHT_KT_PIN, "Kitchen Light");

    // Map fans with PWM control
    CN.mapPWMDevice(living_room_fan, FAN_LR_PIN, "Living Room Fan");
    CN.mapPWMDevice(bedroom_fan, FAN_BR_PIN, "Bedroom Fan");

    // Map blinds as analog devices (servo position)
    CN.mapAnalogDevice(living_room_blinds, BLIND_LR_PIN, "Living Room Blinds");
    CN.mapAnalogDevice(bedroom_blinds, BLIND_BR_PIN, "Bedroom Blinds");

    // === ENVIRONMENTAL SENSORS ===
    // Temperature monitoring every 30 seconds
    CN.addTemperatureSensor(temp_sensor, "Living Room", 30000, []()
                            {
        // Simulate DHT22 reading
        float temp = 22.0 + random(-50, 50) / 10.0; // 17-27°C range
        homeState.currentTemp = temp;
        
        // Auto-control fans based on temperature
        if (temp > 26.0 && homeState.homeOccupied) {
            CN.controlLocalDevice(living_room_fan, "75"); // 75% fan speed
            Serial.println("🌡️ Auto: Fan increased due to high temperature");
        } else if (temp < 20.0) {
            CN.controlLocalDevice(living_room_fan, "0"); // Turn off fan
            Serial.println("🌡️ Auto: Fan turned off due to low temperature");
        }
        
        return SensorValue(temp, "°C"); });

    // Humidity monitoring every 45 seconds
    CN.addHumiditySensor(humidity_sensor, "Living Room", 45000, []()
                         {
        float humidity = 55.0 + random(-200, 200) / 10.0; // 35-75% range
        homeState.currentHumidity = humidity;
        
        // Auto-control based on humidity
        if (humidity > 70.0) {
            CN.controlLocalDevice(bedroom_fan, "50"); // Circulate air
            Serial.println("💧 Auto: Fan activated due to high humidity");
        }
        
        return SensorValue(humidity, "%"); });

    // === SECURITY SENSORS ===
    // Motion detection every 5 seconds
    CN.addMotionSensor(motion_sensor, "Living Room", 5000, []()
                       {
        bool motion = digitalRead(PIR_PIN);
        
        if (motion) {
            homeState.lastMotionTime = millis();
            homeState.homeOccupied = true;
            
            // Auto-lighting when motion detected in night mode
            if (homeState.nightMode && !homeState.awayMode) {
                CN.controlLocalDevice(living_room_light, "ON");
                Serial.println("🚶 Auto: Lights activated by motion");
            }
        }
        
        // Update occupancy status (no motion for 30 minutes = unoccupied)
        homeState.homeOccupied = (millis() - homeState.lastMotionTime) < 1800000;
        
        return SensorValue(motion ? 1.0 : 0.0, ""); });

    // Door sensor monitoring every 10 seconds
    CN.addCustomSensor("door", door_sensor, "Main Entrance", 10000, []()
                       {
        bool doorOpen = !digitalRead(DOOR_PIN); // Inverted for pull-up
        
        if (doorOpen) {
            homeState.lastDoorOpenTime = millis();
            
            // Security alert if door opens in away mode
            if (homeState.awayMode) {
                Serial.println("🚨 SECURITY ALERT: Door opened while in away mode!");
                // Turn on all lights as deterrent
                CN.controlLocalDevice(living_room_light, "ON");
                CN.controlLocalDevice(bedroom_light, "ON");
                CN.controlLocalDevice(kitchen_light, "ON");
            }
        }
        
        return SensorValue(doorOpen ? 1.0 : 0.0, ""); });

    // === ENERGY MONITORING ===
    // Power consumption monitoring every 60 seconds
    CN.addCustomSensor("power", power_sensor, "Main Panel", 60000, []()
                       {
        // Simulate current sensor reading
        int rawCurrent = analogRead(CURRENT_PIN);
        float current = (rawCurrent / 4095.0) * 30.0; // 0-30A range
        float power = current * 230.0; // Watts (assuming 230V)
        
        // Accumulate daily consumption
        static unsigned long lastPowerTime = 0;
        unsigned long now = millis();
        if (lastPowerTime > 0) {
            float hours = (now - lastPowerTime) / 3600000.0;
            homeState.dailyPowerConsumption += power * hours / 1000.0; // kWh
        }
        lastPowerTime = now;
        
        // Alert on high power consumption
        if (power > 5000.0) { // 5kW threshold
            Serial.println("⚡ WARNING: High power consumption detected!");
        }
        
        return SensorValue(power, "W"); });

    // === EVENT CALLBACKS ===
    CN.onConnection([](bool connected)
                    {
        if (connected) {
            Serial.println("🏠 Smart Home Hub Connected!");
            Serial.println("✓ All systems operational");
            
            // Initialize home state
            homeState.nightMode = false;
            homeState.awayMode = false;
            
        } else {
            Serial.println("🏠 Smart Home Hub Disconnected");
        } });

    CN.onDeviceControl([](String deviceSerial, String state)
                       {
        Serial.println("🎛️ Device Control: " + deviceSerial + " -> " + state);
        
        // Handle blind control (servo positioning)
        if (deviceSerial == living_room_blinds || deviceSerial == bedroom_blinds) {
            int position = state.toInt(); // 0-100%
            int servoAngle = map(position, 0, 100, 0, 180);
            
            if (deviceSerial == living_room_blinds) {
                livingRoomBlinds.write(servoAngle);
            } else {
                bedroomBlinds.write(servoAngle);
            }
            
            Serial.printf("🪟 Blinds positioned to %d%% (%d°)\n", position, servoAngle);
        } });

    // Enable auto-reconnection
    CN.enableAutoReconnect(true, 10000);

    Serial.println("🚀 Starting Smart Home Hub...");
    Serial.println("🌐 Dashboard: https://iot.circuitnotion.com");
    CN.connect();
}

void loop()
{
    CN.loop();

    // === SMART HOME AUTOMATION LOGIC ===

    // Night mode detection (simulated - use RTC in real implementation)
    static bool lastNightMode = false;
    homeState.nightMode = (millis() / 20000) % 2 == 0; // Toggle every 20 seconds for demo

    if (homeState.nightMode != lastNightMode)
    {
        lastNightMode = homeState.nightMode;

        if (homeState.nightMode)
        {
            Serial.println("🌙 Entering night mode");
            // Dim lights, lower fan speeds, close blinds partially
            if (homeState.homeOccupied)
            {
                CN.controlLocalDevice(living_room_fan, "25");    // Low speed
                CN.controlLocalDevice(living_room_blinds, "20"); // Mostly closed
            }
        }
        else
        {
            Serial.println("☀️ Entering day mode");
            // Open blinds, reset fan speeds
            CN.controlLocalDevice(living_room_blinds, "80"); // Mostly open
            CN.controlLocalDevice(bedroom_blinds, "80");
        }
    }

    // Away mode simulation (press a button or use dashboard control)
    static bool lastAwayMode = false;
    // In real implementation, this would be controlled via dashboard or schedule

    if (homeState.awayMode != lastAwayMode)
    {
        lastAwayMode = homeState.awayMode;

        if (homeState.awayMode)
        {
            Serial.println("🚶‍♂️ Entering away mode - Security active");
            // Turn off non-essential devices
            CN.controlLocalDevice(living_room_light, "OFF");
            CN.controlLocalDevice(living_room_fan, "0");
            CN.controlLocalDevice(bedroom_fan, "0");
        }
        else
        {
            Serial.println("🏠 Exiting away mode - Welcome home!");
            // Restore comfortable settings
            if (homeState.currentTemp > 24.0)
            {
                CN.controlLocalDevice(living_room_fan, "50");
            }
        }
    }

    // Status reporting every 5 minutes
    static unsigned long lastStatusReport = 0;
    if (millis() - lastStatusReport > 300000)
    {
        lastStatusReport = millis();

        Serial.println("\n=== Smart Home Status ===");
        Serial.printf("🏠 Home Occupied: %s\n", homeState.homeOccupied ? "Yes" : "No");
        Serial.printf("🌙 Night Mode: %s\n", homeState.nightMode ? "Yes" : "No");
        Serial.printf("🚶‍♂️ Away Mode: %s\n", homeState.awayMode ? "Yes" : "No");
        Serial.printf("🌡️ Temperature: %.1f°C\n", homeState.currentTemp);
        Serial.printf("💧 Humidity: %.1f%%\n", homeState.currentHumidity);
        Serial.printf("⚡ Daily Power: %.2f kWh\n", homeState.dailyPowerConsumption);
        Serial.printf("📊 Total Readings: %lu\n", CN.getTotalSensorReadings());
        Serial.println("========================\n");
    }

    delay(100);
}

/*
 * Smart Home Features Demonstrated:
 *
 * 🏠 AUTOMATION:
 * - Temperature-based fan control
 * - Motion-activated lighting
 * - Humidity-based ventilation
 * - Day/night mode automation
 *
 * 🔒 SECURITY:
 * - Motion detection
 * - Door/window monitoring
 * - Away mode with alerts
 * - Automatic deterrent lighting
 *
 * ⚡ ENERGY MANAGEMENT:
 * - Power consumption monitoring
 * - High usage alerts
 * - Away mode energy saving
 * - Daily consumption tracking
 *
 * 🎛️ DEVICE CONTROL:
 * - Lights (on/off)
 * - Fans (variable speed)
 * - Blinds (position control)
 * - Remote dashboard control
 *
 * 📱 Dashboard Integration:
 * - Real-time monitoring
 * - Historical data
 * - Remote control
 * - Automation rules
 * - Mobile notifications
 *
 * EXPANSION IDEAS:
 * - Add weather integration for blind control
 * - Implement voice control via smart speakers
 * - Add irrigation system for gardens
 * - Integrate security cameras
 * - Add air quality sensors
 * - Implement scheduling system
 */
