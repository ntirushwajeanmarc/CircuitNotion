# CircuitNotion Arduino Library - Corrected Architecture

## 🏗️ **Actual System Architecture**

Understanding the real CircuitNotion workflow is crucial for proper library usage:

### **System Entities & Relationships:**

```
User Account (Web Dashboard)
├── Devices (Registered in Dashboard)
│   ├── GATE-001 (Smart Gate)
│   ├── LIGHT-001 (Smart Light)
│   └── FAN-001 (Smart Fan)
│
├── Microcontrollers (Added in Dashboard)
│   ├── ESP8266_Controller_1 (API Key: abc123...)
│   └── ESP8266_Controller_2 (API Key: def456...)
│
└── Sensor Data (From Microcontrollers)
    ├── Temperature readings → linked to devices
    ├── Humidity readings → linked to devices
    └── Motion readings → linked to devices
```

### **Data Flow:**

1. **Device Registration**: User registers devices in web dashboard
2. **Microcontroller Registration**: User adds microcontroller in dashboard → Gets API key
3. **Sensor Data Flow**: Microcontroller → WebSocket → Database → Dashboard
4. **Device Control Flow**: Dashboard → WebSocket → Microcontroller → Local pins

## 🔧 **Corrected Library Usage**

### **Step 1: Web Dashboard Setup**

Before writing any Arduino code:

1. **Register at CircuitNotion** web dashboard
2. **Add your devices** (e.g., "GATE-001", "LIGHT-001", "FAN-001")
3. **Add your microcontroller** → Get the API key
4. **Note down device serial numbers** for Arduino code

### **Step 2: Arduino Code**

```cpp
#include <CircuitNotion.h>

void setup() {
    // Configure with your microcontroller API key (from dashboard)
    CN.begin("server.com", 443, "/ws", "your-api-key", "My Controller");
    CN.setWiFi("WiFi", "password");

    // Map devices to pins (devices must exist in dashboard first!)
    CN.mapDigitalDevice("GATE-001", D2);    // Dashboard device → Local pin
    CN.mapDigitalDevice("LIGHT-001", D4);   // Dashboard device → Local pin

    // Add sensors (will appear in dashboard automatically)
    CN.addTemperatureSensor("GATE-001", "entrance", 60000, readTemp);
    CN.addMotionSensor("GATE-001", "entrance", 5000, readMotion);

    // Handle device control from dashboard
    CN.onDeviceControl([](String deviceSerial, String state) {
        // Automatic pin control + custom logic
        Serial.println("Dashboard controlled: " + deviceSerial + " → " + state);
    });

    CN.connect();
}

void loop() {
    CN.loop(); // Handles everything automatically
}
```

## 📊 **Key Corrections Made**

### **Before (Incorrect):**

- ❌ Library tried to create devices
- ❌ Mixed sensor management with device management
- ❌ Assumed devices could be created from Arduino code
- ❌ Didn't reflect the web dashboard workflow

### **After (Correct):**

- ✅ Library works with existing dashboard devices
- ✅ Sensors belong to microcontroller (not devices directly)
- ✅ Device mapping for local pin control
- ✅ Reflects actual user workflow
- ✅ Dashboard-first approach

## 🎯 **Correct API Methods**

### **Device Mapping (Local Control)**

```cpp
// Map dashboard devices to local pins for control
CN.mapDigitalDevice("GATE-001", D2);           // Digital on/off
CN.mapAnalogDevice("LIGHT-001", D4);           // PWM control
CN.mapDevice("FAN-001", D5, "Smart Fan");      // Custom mapping
```

### **Sensor Management (Data Collection)**

```cpp
// Sensors belong to this microcontroller
CN.addTemperatureSensor("GATE-001", "entrance", 60000, callback);
CN.addMotionSensor("GATE-001", "entrance", 5000, callback);
CN.addCustomSensor("pressure", "GATE-001", "entrance", 120000, callback);
```

### **Device Control Handling**

```cpp
// Respond to dashboard commands
CN.onDeviceControl([](String deviceSerial, String state) {
    // Library automatically controls mapped pins
    // Add custom logic here if needed

    if (deviceSerial == "GATE-001") {
        if (state == "on") {
            // Custom gate opening sequence
        }
    }
});
```

## 🔄 **Complete Workflow Example**

### **1. Dashboard Setup:**

```
User Dashboard Actions:
1. Login to CircuitNotion dashboard
2. Navigate to "Devices" → Add devices:
   - Name: "Front Gate", Serial: "GATE-001", Type: "switch"
   - Name: "Living Light", Serial: "LIGHT-001", Type: "light"
3. Navigate to "Microcontrollers" → Add microcontroller:
   - Name: "Home Controller" → Get API key: "abc123..."
```

### **2. Arduino Implementation:**

```cpp
#include <CircuitNotion.h>
#include <DHT.h>

DHT dht(D1, DHT22);

void setup() {
    CN.begin("home.circuitnotion.com", 443, "/ws", "abc123...", "Home Controller");
    CN.setWiFi("WiFi", "password");

    // Map dashboard devices to pins
    CN.mapDigitalDevice("GATE-001", D2);    // Gate control via D2
    CN.mapDigitalDevice("LIGHT-001", D4);   // Light control via D4

    // Add sensors (linked to devices)
    CN.addTemperatureSensor("GATE-001", "entrance", 60000, []() {
        return {dht.readTemperature(), "°C"};
    });

    CN.addMotionSensor("GATE-001", "entrance", 5000, []() {
        return {digitalRead(D3) ? 1.0f : 0.0f, ""};
    });

    // Handle dashboard commands
    CN.onDeviceControl([](String deviceSerial, String state) {
        Serial.println("Dashboard command: " + deviceSerial + " → " + state);
        // Pin control happens automatically
    });

    CN.connect();
}

void loop() {
    CN.loop();
}
```

### **3. Dashboard Experience:**

```
Dashboard Shows:
- Devices: GATE-001 (controllable), LIGHT-001 (controllable)
- Sensors: Temperature (from GATE-001), Motion (from GATE-001)
- Microcontrollers: "Home Controller" (online/offline status)

User Actions:
- Click GATE-001 "ON" → Microcontroller receives command → Pin D2 HIGH
- View sensor data in real-time from microcontroller
```

## 🚀 **Benefits of Corrected Architecture**

1. **🎯 Accurate Workflow**: Matches actual CircuitNotion system
2. **📱 Dashboard-First**: Devices created in dashboard, not Arduino
3. **🔌 Clean Separation**: Device control vs sensor data collection
4. **🛡️ Secure**: API keys managed through dashboard
5. **📊 Real-time**: Sensor data flows properly to dashboard
6. **🔧 Flexible**: Can map any device to any pin locally

This corrected library now properly reflects how CircuitNotion actually works! 🎉
