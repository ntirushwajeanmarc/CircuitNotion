/*
 * CircuitNotion Industrial Monitoring Example
 *
 * This example demonstrates industrial IoT monitoring capabilities:
 * - Machine temperature and vibration monitoring
 * - Production counter tracking
 * - Alert system for anomalies
 * - Equipment status monitoring
 * - Predictive maintenance indicators
 *
 * Hardware Requirements:
 * - ESP32 (for industrial-grade reliability)
 * - Temperature sensors (DS18B20 or thermocouple)
 * - Vibration sensor (accelerometer)
 * - Proximity sensors for counting
 * - Status indicator LEDs
 * - Buzzer for local alerts
 *
 * Dashboard Setup:
 * Create these devices in your CircuitNotion dashboard:
 * - "MACHINE_TEMP_01" (Temperature Sensor)
 * - "VIBRATION_01" (Vibration Sensor)
 * - "PRODUCTION_COUNT_01" (Counter)
 * - "MACHINE_STATUS_01" (Status Indicator)
 * - "ALERT_SYSTEM_01" (Alert Control)
 */

#include <CircuitNotion.h>

// WiFi Configuration
const char *ssid = "Industrial_WiFi";
const char *password = "Industrial_Password";

// CircuitNotion Configuration
const char *api_key = "your-industrial-api-key";
const char *microcontroller_name = "Production Line Monitor";

// Device Serials
const char *machine_temp = "MACHINE_TEMP_01";
const char *vibration_sensor = "VIBRATION_01";
const char *production_counter = "PRODUCTION_COUNT_01";
const char *machine_status = "MACHINE_STATUS_01";
const char *alert_system = "ALERT_SYSTEM_01";

// Pin Definitions
const int TEMP_SENSOR_PIN = 4;
const int VIBRATION_PIN = 35;
const int PROXIMITY_PIN = 14;
const int STATUS_LED_PIN = 2;
const int ALERT_LED_PIN = 18;
const int BUZZER_PIN = 19;

// Industrial monitoring parameters
struct IndustrialState
{
    float currentTemp = 0;
    float vibrationLevel = 0;
    unsigned long productionCount = 0;
    bool machineRunning = false;
    bool alertActive = false;
    unsigned long lastProductionTime = 0;
    unsigned long operatingHours = 0;
    float avgTemperature = 0;
    int tempReadingCount = 0;
} industrial;

// Alert thresholds
const float TEMP_WARNING_THRESHOLD = 75.0;  // °C
const float TEMP_CRITICAL_THRESHOLD = 85.0; // °C
const float VIBRATION_WARNING_THRESHOLD = 5.0;
const float VIBRATION_CRITICAL_THRESHOLD = 8.0;
const unsigned long PRODUCTION_TIMEOUT = 300000; // 5 minutes without production = issue

void setup()
{
    Serial.begin(115200);
    Serial.println("\n=== CircuitNotion Industrial Monitor ===");

    // Initialize pins
    pinMode(PROXIMITY_PIN, INPUT_PULLUP);
    pinMode(STATUS_LED_PIN, OUTPUT);
    pinMode(ALERT_LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    // Configure CircuitNotion
    CN.setWiFi(ssid, password);
    CN.begin("iot.circuitnotion.com", 443, "/ws", api_key, microcontroller_name);

    // === MACHINE TEMPERATURE MONITORING ===
    // Critical for equipment health - read every 15 seconds
    CN.addTemperatureSensor(machine_temp, "Production Line A", 15000, []()
                            {
        // Simulate temperature sensor reading
        float baseTemp = 45.0; // Normal operating temperature
        float variation = random(-100, 300) / 10.0; // Add some realistic variation
        float temp = baseTemp + variation;
        
        // For real DS18B20 sensor:
        // #include <OneWire.h>
        // #include <DallasTemperature.h>
        // OneWire oneWire(TEMP_SENSOR_PIN);
        // DallasTemperature sensors(&oneWire);
        // sensors.requestTemperatures();
        // float temp = sensors.getTempCByIndex(0);
        
        industrial.currentTemp = temp;
        
        // Update running average
        industrial.avgTemperature = ((industrial.avgTemperature * industrial.tempReadingCount) + temp) / (industrial.tempReadingCount + 1);
        industrial.tempReadingCount++;
        
        // Temperature-based alerts
        if (temp >= TEMP_CRITICAL_THRESHOLD) {
            triggerAlert("CRITICAL", "Machine temperature critical: " + String(temp) + "°C");
            setMachineStatus(false); // Emergency shutdown signal
        } else if (temp >= TEMP_WARNING_THRESHOLD) {
            triggerAlert("WARNING", "Machine temperature high: " + String(temp) + "°C");
        }
        
        return SensorValue(temp, "°C"); });

    // === VIBRATION MONITORING ===
    // Detect mechanical issues - read every 10 seconds
    CN.addCustomSensor("vibration", vibration_sensor, "Production Line A", 10000, []()
                       {
        // Read accelerometer or vibration sensor
        int rawVibration = analogRead(VIBRATION_PIN);
        float vibration = (rawVibration / 4095.0) * 10.0; // 0-10 scale
        
        industrial.vibrationLevel = vibration;
        
        // Vibration-based alerts
        if (vibration >= VIBRATION_CRITICAL_THRESHOLD) {
            triggerAlert("CRITICAL", "Excessive vibration detected: " + String(vibration));
            setMachineStatus(false); // Potential mechanical failure
        } else if (vibration >= VIBRATION_WARNING_THRESHOLD) {
            triggerAlert("WARNING", "High vibration detected: " + String(vibration));
        }
        
        return SensorValue(vibration, "G"); });

    // === PRODUCTION COUNTER ===
    // Track production output - read every 30 seconds
    CN.addCustomSensor("production", production_counter, "Production Line A", 30000, []()
                       {
        // Count products using proximity sensor
        static bool lastProximityState = false;
        static unsigned long lastCountTime = 0;
        
        bool currentProximity = !digitalRead(PROXIMITY_PIN); // Inverted for pull-up
        
        // Detect product passing (rising edge)
        if (currentProximity && !lastProximityState) {
            industrial.productionCount++;
            industrial.lastProductionTime = millis();
            
            // Calculate production rate
            if (lastCountTime > 0) {
                unsigned long timeBetween = millis() - lastCountTime;
                float rate = 3600000.0 / timeBetween; // Products per hour
                Serial.printf("📦 Product #%lu produced (Rate: %.1f/hr)\n", 
                             industrial.productionCount, rate);
            }
            lastCountTime = millis();
        }
        
        lastProximityState = currentProximity;
        
        // Check for production stoppage
        if (industrial.machineRunning && 
            (millis() - industrial.lastProductionTime) > PRODUCTION_TIMEOUT) {
            triggerAlert("WARNING", "Production stopped - No output detected");
        }
        
        return SensorValue(industrial.productionCount, "units"); });

    // === MACHINE STATUS MONITORING ===
    // Overall machine health - read every 60 seconds
    CN.addCustomSensor("status", machine_status, "Production Line A", 60000, []()
                       {
        // Calculate overall machine health score (0-100)
        float healthScore = 100.0;
        
        // Temperature factor
        if (industrial.currentTemp > TEMP_WARNING_THRESHOLD) {
            healthScore -= (industrial.currentTemp - TEMP_WARNING_THRESHOLD) * 2;
        }
        
        // Vibration factor
        if (industrial.vibrationLevel > VIBRATION_WARNING_THRESHOLD) {
            healthScore -= (industrial.vibrationLevel - VIBRATION_WARNING_THRESHOLD) * 5;
        }
        
        // Production factor
        if (industrial.machineRunning && 
            (millis() - industrial.lastProductionTime) > PRODUCTION_TIMEOUT) {
            healthScore -= 30; // Significant impact for stopped production
        }
        
        healthScore = max(0.0f, healthScore);
        
        // Update machine status based on health score
        if (healthScore >= 80) {
            setMachineStatus(true);  // Good condition
        } else if (healthScore >= 60) {
            // Maintenance recommended
        } else {
            setMachineStatus(false); // Poor condition
        }
        
        return SensorValue(healthScore, "%"); });

    // === OPERATING HOURS COUNTER ===
    // Track total operating time for maintenance scheduling
    CN.addCustomSensor("operating_hours", "OPERATING_HOURS_01", "Production Line A", 300000, []()
                       {
        static unsigned long lastOperatingUpdate = 0;
        
        if (industrial.machineRunning) {
            if (lastOperatingUpdate > 0) {
                industrial.operatingHours += (millis() - lastOperatingUpdate);
            }
        }
        lastOperatingUpdate = millis();
        
        float hours = industrial.operatingHours / 3600000.0; // Convert to hours
        
        // Predictive maintenance alerts
        if (fmod(hours, 1000.0) < 0.1 && hours > 0) { // Every 1000 hours
            triggerAlert("INFO", "Scheduled maintenance due - " + String(hours) + " operating hours");
        }
        
        return SensorValue(hours, "hours"); });

    // === DEVICE MAPPINGS ===
    // Map alert system for dashboard control
    CN.mapDigitalDevice(alert_system, ALERT_LED_PIN, "Alert System");

    // === CALLBACKS ===
    CN.onConnection([](bool connected)
                    {
        if (connected) {
            Serial.println("🏭 Industrial Monitor Connected!");
            Serial.println("✓ Production line monitoring active");
            digitalWrite(STATUS_LED_PIN, HIGH);
        } else {
            Serial.println("🏭 Industrial Monitor Disconnected");
            digitalWrite(STATUS_LED_PIN, LOW);
        } });

    CN.onDeviceControl([](String deviceSerial, String state)
                       {
        if (deviceSerial == alert_system) {
            if (state == "ON") {
                acknowledgeAlert();
            }
        } });

    // Enable auto-reconnection for industrial reliability
    CN.enableAutoReconnect(true, 5000);

    Serial.println("🚀 Starting Industrial Monitor...");
    Serial.println("🏭 Monitoring: Temperature, Vibration, Production, Status");

    // Initialize machine as running
    setMachineStatus(true);

    CN.connect();
}

void loop()
{
    CN.loop();

    // Update operating hours if machine is running
    static unsigned long lastHourUpdate = 0;
    if (millis() - lastHourUpdate > 60000)
    { // Update every minute
        lastHourUpdate = millis();

        if (industrial.machineRunning)
        {
            industrial.operatingHours += 60000; // Add 1 minute
        }
    }

    // Status reporting every 10 minutes
    static unsigned long lastReport = 0;
    if (millis() - lastReport > 600000)
    {
        lastReport = millis();

        Serial.println("\n=== Industrial Status Report ===");
        Serial.printf("🏭 Machine Running: %s\n", industrial.machineRunning ? "Yes" : "No");
        Serial.printf("🌡️ Current Temp: %.1f°C (Avg: %.1f°C)\n",
                      industrial.currentTemp, industrial.avgTemperature);
        Serial.printf("📳 Vibration: %.1fG\n", industrial.vibrationLevel);
        Serial.printf("📦 Production Count: %lu units\n", industrial.productionCount);
        Serial.printf("⏰ Operating Hours: %.1f\n", industrial.operatingHours / 3600000.0);
        Serial.printf("🚨 Alert Status: %s\n", industrial.alertActive ? "ACTIVE" : "Clear");
        Serial.printf("📊 Total Readings: %lu\n", CN.getTotalSensorReadings());
        Serial.println("================================\n");
    }

    // Local alert handling
    if (industrial.alertActive)
    {
        // Flash alert LED
        static unsigned long lastFlash = 0;
        static bool ledState = false;

        if (millis() - lastFlash > 500)
        {
            lastFlash = millis();
            ledState = !ledState;
            digitalWrite(ALERT_LED_PIN, ledState);
        }
    }

    delay(100);
}

// Helper Functions
void triggerAlert(String level, String message)
{
    Serial.println("🚨 ALERT [" + level + "]: " + message);

    industrial.alertActive = true;
    digitalWrite(ALERT_LED_PIN, HIGH);

    // Sound buzzer for critical alerts
    if (level == "CRITICAL")
    {
        for (int i = 0; i < 3; i++)
        {
            digitalWrite(BUZZER_PIN, HIGH);
            delay(200);
            digitalWrite(BUZZER_PIN, LOW);
            delay(200);
        }
    }

    // Send to dashboard (you could create an alert device for this)
    // CN.sendCustomData("ALERT_LOG", level + ": " + message);
}

void acknowledgeAlert()
{
    Serial.println("✅ Alert acknowledged via dashboard");
    industrial.alertActive = false;
    digitalWrite(ALERT_LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
}

void setMachineStatus(bool running)
{
    industrial.machineRunning = running;
    digitalWrite(STATUS_LED_PIN, running ? HIGH : LOW);

    Serial.println(running ? "✅ Machine Status: RUNNING" : "❌ Machine Status: STOPPED");

    if (!running && industrial.alertActive == false)
    {
        triggerAlert("WARNING", "Machine status changed to STOPPED");
    }
}

/*
 * Industrial IoT Features Demonstrated:
 *
 * 🏭 PRODUCTION MONITORING:
 * - Real-time production counting
 * - Production rate calculation
 * - Stoppage detection and alerts
 * - Operating hours tracking
 *
 * 🌡️ EQUIPMENT HEALTH:
 * - Temperature monitoring with thresholds
 * - Vibration analysis for mechanical health
 * - Overall health scoring system
 * - Predictive maintenance indicators
 *
 * 🚨 ALERT SYSTEM:
 * - Multi-level alerts (INFO, WARNING, CRITICAL)
 * - Local visual and audio alerts
 * - Dashboard integration for remote acknowledgment
 * - Automatic threshold monitoring
 *
 * 📊 DATA ANALYTICS:
 * - Running averages and trends
 * - Historical data collection
 * - Performance metrics
 * - Maintenance scheduling data
 *
 * 🔧 MAINTENANCE FEATURES:
 * - Operating hours tracking
 * - Scheduled maintenance reminders
 * - Equipment health scoring
 * - Performance degradation detection
 *
 * DASHBOARD CAPABILITIES:
 * - Real-time monitoring dashboards
 * - Historical trend analysis
 * - Alert management and acknowledgment
 * - Performance reporting
 * - Maintenance scheduling
 * - Multi-site monitoring
 *
 * EXPANSION IDEAS:
 * - Energy consumption monitoring
 * - Quality control sensors
 * - Environmental monitoring (dust, humidity)
 * - Operator interface integration
 * - SCADA system integration
 * - Mobile notifications for critical alerts
 */
