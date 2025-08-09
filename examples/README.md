# CircuitNotion Arduino Library Examples

This directory contains comprehensive examples demonstrating various use cases for the CircuitNotion Arduino library. Each example includes detailed comments, setup instructions, and hardware requirements.

## 📁 Available Examples

### 1. 🔰 BasicUsage

**File:** `BasicUsage/BasicUsage.ino`

Perfect for getting started with CircuitNotion. Demonstrates:

- WiFi connection setup
- Basic device mapping (LED control)
- Simple temperature sensor
- Connection event handling

**Hardware:** ESP8266/ESP32, LED, optional DHT22 sensor

---

### 2. 📊 SensorHub

**File:** `SensorHub/SensorHub.ino`

Advanced sensor management example showing:

- Multiple sensor types (temperature, humidity, light, motion)
- Change detection optimization
- Dynamic sensor enable/disable
- Custom sensors with different data types
- Sensor diagnostics and monitoring

**Hardware:** ESP8266/ESP32, DHT22, LDR, PIR sensor, optional pressure sensor

---

### 3. 🏠 SmartHome

**File:** `SmartHome/SmartHome.ino`

Complete home automation system featuring:

- Multi-room device control (lights, fans, blinds)
- Environmental monitoring and automation
- Security features (motion, door sensors)
- Energy monitoring and management
- Intelligent automation based on occupancy and time

**Hardware:** ESP32 (recommended), relay modules, DHT22, PIR sensors, servo motors, current sensors

---

### 4. 🏭 IndustrialMonitoring

**File:** `IndustrialMonitoring/IndustrialMonitoring.ino`

Industrial IoT monitoring solution with:

- Machine health monitoring (temperature, vibration)
- Production counting and rate tracking
- Multi-level alert system
- Predictive maintenance indicators
- Operating hours tracking
- Equipment status monitoring

**Hardware:** ESP32, temperature sensors, accelerometer, proximity sensors, status LEDs, buzzer

---

## 🚀 Quick Start Guide

### 1. Choose Your Example

Select the example that best matches your project needs:

- **New to IoT?** Start with `BasicUsage`
- **Multiple sensors?** Use `SensorHub`
- **Home automation?** Try `SmartHome`
- **Industrial application?** Use `IndustrialMonitoring`

### 2. Dashboard Setup

Before uploading any example:

1. **Create Account:** Visit [iot.circuitnotion.com](https://iot.circuitnotion.com)
2. **Add Microcontroller:** Get your API key
3. **Create Devices:** Add devices matching the example's device serials
4. **Note Device Serials:** Copy them for use in Arduino code

### 3. Hardware Setup

- Connect sensors and actuators according to pin definitions in each example
- Ensure proper power supply for your ESP8266/ESP32
- Test individual components before integration

### 4. Code Configuration

In each example, update these values:

```cpp
const char* ssid = "Your_WiFi_SSID";
const char* password = "Your_WiFi_Password";
const char* api_key = "your-api-key-from-dashboard";
```

### 5. Upload and Test

1. Upload the sketch to your microcontroller
2. Open Serial Monitor (115200 baud)
3. Verify connection to CircuitNotion
4. Test device control from dashboard
5. Monitor sensor data in real-time

---

## 🔧 Common Setup Steps

### Library Dependencies

Install these libraries via Arduino Library Manager:

- `ArduinoJson` (6.21.3+)
- `WebSocketsClient` (2.3.7+)
- `DHT sensor library` (for temperature examples)
- `Servo` (for SmartHome blinds control)

### WiFi Configuration

All examples use the same WiFi setup pattern:

```cpp
CN.setWiFi(ssid, password);
CN.begin("iot.circuitnotion.com", 443, "/ws", api_key, microcontroller_name);
```

### Device Mapping Pattern

Map dashboard devices to local pins:

```cpp
// Digital devices (lights, relays)
CN.mapDigitalDevice("DEVICE_SERIAL", pin, "Display Name");

// Analog/PWM devices (dimmers, servos)
CN.mapPWMDevice("DEVICE_SERIAL", pin, "Display Name");
```

### Sensor Addition Pattern

Add sensors with callbacks:

```cpp
CN.addTemperatureSensor("SENSOR_SERIAL", "Location", interval_ms, []() {
    float value = readYourSensor();
    return SensorValue(value, "unit");
});
```

---

## 📋 Example Comparison

| Feature           | BasicUsage | SensorHub | SmartHome | Industrial |
| ----------------- | ---------- | --------- | --------- | ---------- |
| **Complexity**    | ⭐         | ⭐⭐      | ⭐⭐⭐    | ⭐⭐⭐     |
| **Sensors**       | 1          | 5+        | 5+        | 8+         |
| **Actuators**     | 1          | 0         | 6+        | 3+         |
| **Automation**    | None       | Basic     | Advanced  | Critical   |
| **Hardware Cost** | $          | $$        | $$$       | $$$        |
| **Setup Time**    | 15 min     | 30 min    | 2 hours   | 3 hours    |

---

## 🛠️ Troubleshooting

### Connection Issues

1. **Check WiFi credentials** - Ensure SSID and password are correct
2. **Verify API key** - Copy exactly from dashboard
3. **Device serials** - Must match between code and dashboard
4. **Network firewall** - Ensure WebSocket connections allowed

### Sensor Issues

1. **Hardware connections** - Verify wiring and power
2. **Sensor initialization** - Check sensor-specific setup
3. **Reading intervals** - Don't set too frequent (min 5 seconds)
4. **Data validation** - Handle sensor read errors

### Performance Issues

1. **Memory usage** - Monitor with `ESP.getFreeHeap()`
2. **Connection stability** - Enable auto-reconnect
3. **Change detection** - Use to reduce bandwidth
4. **Sensor optimization** - Adjust intervals based on needs

---

## 📖 Additional Resources

- **Main Documentation:** See `README.md` in library root
- **API Reference:** Complete method documentation in README
- **Dashboard Guide:** [iot.circuitnotion.com/docs](https://iot.circuitnotion.com/docs)
- **Hardware Guides:** Wiring diagrams and component selection
- **Community Forum:** Share projects and get help

---

## 🎯 Next Steps

After trying these examples:

1. **Modify for your hardware** - Adapt pin assignments and sensors
2. **Create custom devices** - Add your own device types in dashboard
3. **Build automation rules** - Use dashboard to create smart behaviors
4. **Scale your project** - Add more sensors and locations
5. **Share your project** - Contribute back to the community

Happy building with CircuitNotion! 🚀
