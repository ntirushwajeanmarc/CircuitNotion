# CircuitNotion Arduino Library

A powerful Arduino library for connecting ESP8266/ESP32 microcontrollers to the CircuitNotion IoT platform. This library enables seamless sensor data collection, device control, and real-time communication with your CircuitNotion dashboard.

## 🚀 Features

- **Real-time Communication**: WebSocket-based bidirectional communication with CircuitNotion servers
- **Dashboard Integration**: Full integration with the CircuitNotion web dashboard at [iot.circuitnotion.com](https://iot.circuitnotion.com)
- **Sensor Management**: Easy setup for temperature, humidity, light, motion, and custom sensors
- **Device Control**: Map dashboard devices to local pins for automatic control
- **Auto-Reconnection**: Robust connection handling with automatic reconnection
- **Change Detection**: Intelligent sensor reading optimization to reduce bandwidth
- **Memory Efficient**: Optimized for resource-constrained microcontrollers
- **Production Ready**: Comprehensive error handling and diagnostics

## 📋 Getting Started - Complete Setup Guide

### Step 1: Dashboard Account Setup

1. **Visit the CircuitNotion Dashboard**

   - Go to [iot.circuitnotion.com](https://iot.circuitnotion.com)
   - Click "Sign Up" to create a new account
   - Fill in your details (name, email, password)
   - Verify your email address

2. **Initial Dashboard Setup**
   - Log in to your new account
   - Complete your profile setup
   - Choose your subscription plan (free tier available)

### Step 2: Create Your Devices

Before writing any Arduino code, you need to create devices in your dashboard:

1. **Navigate to Devices**

   - In the dashboard, click "Devices" in the main navigation
   - Click "Add New Device"

2. **Create Your First Device**

   ```
   Device Name: Living Room Light
   Device Type: Light/LED
   Location: Living Room
   Description: Main ceiling light
   ```

   - Click "Create Device"
   - **Note the Device Serial** (e.g., `LIGHT_001`) - you'll need this for Arduino code

3. **Create More Devices** (repeat for each physical device)

   ```
   Temperature Sensor Device:
   Name: Living Room Temperature
   Type: Sensor
   Location: Living Room
   Serial: TEMP_001

   Humidity Sensor Device:
   Name: Living Room Humidity
   Type: Sensor
   Location: Living Room
   Serial: HUM_001
   ```

### Step 3: Add Your Microcontroller

1. **Navigate to Microcontrollers**

   - Click "Microcontrollers" in the dashboard navigation
   - Click "Add New Microcontroller"

2. **Configure Your Microcontroller**
   ```
   Name: Home Sensor Hub
   Description: ESP8266 controlling living room devices
   Location: Living Room
   Board Type: ESP8266
   ```
   - Click "Create Microcontroller"
   - **Copy the API Key** - this is crucial for Arduino code

### Step 4: Arduino Library Installation

#### Option A: Arduino Library Manager

1. Open Arduino IDE
2. Go to **Tools** → **Manage Libraries**
3. Search for "CircuitNotion"
4. Click **Install**

#### Option B: Manual Installation

1. Download the library ZIP from GitHub
2. In Arduino IDE: **Sketch** → **Include Library** → **Add .ZIP Library**
3. Select the downloaded ZIP file

#### Install Dependencies

The library requires these dependencies (install via Library Manager):

- `ArduinoJson` (6.21.3 or later)
- `WebSocketsClient` (2.3.7 or later)
- `DHT sensor library` (1.4.4 or later) - for temperature/humidity sensors

### Step 5: Arduino Code Setup

#### Basic Example

```cpp
#include <CircuitNotion.h>

// WiFi credentials
const char* ssid = "YourWiFiNetwork";
const char* password = "YourWiFiPassword";

// CircuitNotion configuration (from your dashboard)
const char* api_key = "your-microcontroller-api-key";  // From Step 3
const char* microcontroller_name = "Home Sensor Hub";   // From Step 3

// Device serials (from Step 2)
const char* light_device_serial = "LIGHT_001";
const char* temp_device_serial = "TEMP_001";

void setup() {
    Serial.begin(115200);

    // Setup WiFi
    CN.setWiFi(ssid, password);

    // Configure CircuitNotion connection
    CN.begin("iot.circuitnotion.com", 443, "/ws", api_key, microcontroller_name);

    // Map dashboard device to local pin for control
    CN.mapDigitalDevice(light_device_serial, D2, "Living Room Light");

    // Add sensor for dashboard device
    CN.addTemperatureSensor(temp_device_serial, "Living Room", 30000, []() {
        // Read your actual sensor here
        float temp = 23.5; // Replace with real sensor reading
        return SensorValue(temp, "°C");
    });

    // Setup callbacks
    CN.onConnection([](bool connected) {
        Serial.println(connected ? "✓ Connected!" : "✗ Disconnected");
    });

    CN.onDeviceControl([](String deviceSerial, String state) {
        Serial.println("Device control: " + deviceSerial + " -> " + state);
    });

    // Connect to CircuitNotion
    CN.connect();
}

void loop() {
    CN.loop();
    delay(100);
}
```

### Step 6: Verify Everything Works

1. **Upload Your Code**

   - Upload the Arduino sketch to your ESP8266/ESP32
   - Open Serial Monitor (115200 baud)
   - Look for connection messages

2. **Check Dashboard**
   - Return to [iot.circuitnotion.com](https://iot.circuitnotion.com)
   - Go to "Microcontrollers" - your device should show as "Connected"
   - Go to "Devices" - you should see real-time sensor data
   - Try controlling devices from the dashboard

## 🏗️ Architecture Overview

CircuitNotion follows a **dashboard-first** architecture:

```
Dashboard (iot.circuitnotion.com) → Device Creation → API Keys → Arduino Connection → Sensor Data
```

### Key Concepts

1. **Devices**: Physical entities registered in your dashboard (lights, sensors, etc.)
2. **Microcontrollers**: Arduino boards that interface with devices
3. **Device Mappings**: Connect dashboard devices to local microcontroller pins
4. **Sensors**: Data collectors attached to devices via microcontrollers

## 🔌 Device Mapping

Map dashboard devices to local pins for automatic control:

```cpp
// Digital devices (LED, relay, etc.)
CN.mapDigitalDevice("LIGHT_001", D2, "Living Room Light");
CN.mapDigitalDevice("RELAY_001", D3, "Water Pump", true); // inverted logic

// Analog/PWM devices
CN.mapAnalogDevice("DIMMER_001", D5, "Dimmable Light");
CN.mapPWMDevice("SERVO_001", D6, "Window Servo");
```

## 📊 Sensor Management

Attach sensors to dashboard devices:

```cpp
// Temperature sensor reading every 30 seconds
CN.addTemperatureSensor("TEMP_001", "Kitchen", 30000, readTemperature);

// Humidity sensor with change detection
auto* humidity = CN.addHumiditySensor("HUM_001", "Kitchen", 15000, readHumidity);
humidity->setChangeThreshold(5.0); // Only send if changed by 5%
humidity->enableChangeDetection(true);

// Custom sensor
CN.addCustomSensor("pressure", "PRESS_001", "Basement", 60000, readPressure);

// Enable/disable sensors dynamically
CN.enableSensor("temperature", "TEMP_001");
CN.disableSensor("humidity", "HUM_001");
```

## 🔄 Dashboard Features You Can Use

### Real-time Device Control

- Toggle lights, fans, pumps from the web dashboard
- Set analog values (dimmer levels, servo positions)
- Create automation rules and schedules

### Sensor Data Visualization

- Real-time charts and graphs
- Historical data analysis
- Export data to CSV/JSON
- Set up alerts and notifications

### Device Management

- Organize devices by location/room
- Set device names and descriptions
- Monitor device status and connectivity
- Manage device permissions

### User Management

- Share access with family/team members
- Set user permissions per device
- Activity logs and audit trails

## 🔧 Advanced Configuration

### Custom Server (Enterprise)

```cpp
// For self-hosted CircuitNotion instances
CN.begin("your-server.com", 443, "/ws", api_key, microcontroller_name);
```

### SSL/TLS Configuration

```cpp
// Enable/disable SSL (default: enabled)
CN.begin(host, 443, path, api_key, name, true);  // SSL enabled
CN.begin(host, 80, path, api_key, name, false);  // Plain HTTP
```

### Auto-reconnection

```cpp
// Enable auto-reconnection with custom interval
CN.enableAutoReconnect(true, 10000); // 10 second intervals
```

## 📈 Monitoring and Diagnostics

```cpp
// Print comprehensive diagnostics
CN.printDiagnostics();
CN.printSensorStatus();
CN.printDeviceMappings();

// Get statistics
int sensorCount = CN.getSensorCount();
unsigned long totalReadings = CN.getTotalSensorReadings();
String version = CN.getLibraryVersion();
```

## 🌐 Complete API Reference

### Main Class: `CircuitNotion`

#### Configuration

- `begin(host, port, path, apiKey, microcontrollerName, useSSL=true)`
- `setWiFi(ssid, password)`
- `enableAutoReconnect(enabled, interval=5000)`

#### Connection

- `connect()`
- `disconnect()`
- `bool isConnected()`
- `bool isAuthenticated()`
- `ConnectionStatus getStatus()`

#### Device Mapping

- `mapDigitalDevice(deviceSerial, pin, name="", inverted=false)`
- `mapAnalogDevice(deviceSerial, pin, name="")`
- `mapPWMDevice(deviceSerial, pin, name="")`
- `controlLocalDevice(deviceSerial, state/value)`

#### Sensor Management

- `addTemperatureSensor(deviceSerial, location, interval, callback)`
- `addHumiditySensor(deviceSerial, location, interval, callback)`
- `addLightSensor(deviceSerial, location, interval, callback)`
- `addMotionSensor(deviceSerial, location, interval, callback)`
- `addCustomSensor(type, deviceSerial, location, interval, callback)`

#### Callbacks

- `onDeviceControl(callback)`
- `onConnection(callback)`
- `onLog(callback)`

## 🛠️ Hardware Requirements

- **ESP8266** or **ESP32** microcontroller
- **WiFi connection**
- **Sensors** (DHT22, LDR, etc.) - optional
- **Actuators** (LEDs, relays, servos) - optional

## 📝 Complete Example Projects

### Smart Home Hub

Monitor temperature, humidity, and light levels while controlling lights and fans.

### Garden Monitoring

Track soil moisture, temperature, and automate watering systems.

### Security System

Motion detection, door sensors, and automatic lighting control.

### Industrial Monitoring

Machine temperature monitoring, production counters, and alert systems.

## 🐛 Troubleshooting

### Dashboard Issues

1. **Can't access iot.circuitnotion.com**

   - Check your internet connection
   - Try clearing browser cache
   - Contact support if site is down

2. **Device not appearing in dashboard**
   - Verify device creation in dashboard
   - Check device serial matches Arduino code
   - Ensure microcontroller is connected

### Connection Issues

1. **Arduino won't connect**

   - Verify WiFi credentials
   - Check API key copied correctly from dashboard
   - Ensure microcontroller name matches dashboard
   - Monitor Serial output for error messages

2. **Sensors not sending data**
   - Verify device serials match dashboard
   - Check sensor hardware connections
   - Confirm sensor initialization in code
   - Test sensor readings independently

### Performance Issues

1. **Memory problems**

   - Reduce sensor reading frequency
   - Enable change detection to reduce transmissions
   - Monitor free heap with `ESP.getFreeHeap()`

2. **Connection drops**
   - Enable auto-reconnect: `CN.enableAutoReconnect(true)`
   - Check WiFi signal strength
   - Verify power supply stability

## 🔒 Security

- Uses **WSS (WebSocket Secure)** for encrypted communication
- **API key authentication** for device authorization
- **Dashboard access control** with user permissions
- **SSL/TLS encryption** for all data transmission
- **Secure device registration** through web dashboard

## 📞 Support and Community

- **Dashboard**: [iot.circuitnotion.com](https://iot.circuitnotion.com)
- **Documentation**: [docs.circuitnotion.com](https://docs.circuitnotion.com)
- **Community Forum**: [community.circuitnotion.com](https://community.circuitnotion.com)
- **GitHub Issues**: [github.com/circuitnotion/arduino-library/issues](https://github.com/circuitnotion/arduino-library/issues)
- **Email Support**: support@circuitnotion.com

## 📄 License

This library is released under the MIT License. See LICENSE file for details.

---

**Start your IoT journey today!** 🚀

1. Sign up at [iot.circuitnotion.com](https://iot.circuitnotion.com)
2. Create your devices
3. Install this library
4. Connect your Arduino
5. Control everything from your dashboard!
