# CircuitNotion (Arduino / ESP8266 / ESP32)

Connect your ESP8266 or ESP32 to the **CircuitNotion Gate** server over WebSocket: receive device control (on/off, servo angle, volume, mute), send sensor readings, report physical button changes, and trigger email notifications.

## Requirements

- **Board**: ESP8266 or ESP32
- **Libraries**: [ArduinoJson](https://arduinojson.org/) 6.x, [WebSockets](https://github.com/Links2004/arduinoWebSockets)

## Installation

1. Install dependencies via Library Manager: **ArduinoJson**, **WebSockets**.
2. Install this library: clone or download into your Arduino `libraries` folder as `CircuitNotion`, or use **Sketch → Include Library → Add .ZIP Library** if you zip the `CircuitNotion` folder.

## Quick start

```cpp
#include <CircuitNotion.h>

CircuitNotion CN;

void setup() {
  Serial.begin(115200);
  CN.begin("your-api-key", "MyESP");   // minimal: default host/port/path
  CN.setWiFi("YourSSID", "YourPassword");
  CN.onDeviceControl([](String serial, String state, JsonObject data) {
    if (!data.isNull() && data.containsKey("angle")) {
      int angle = data["angle"].as<int>();
      // e.g. servo.write(angle);
    }
  });
  CN.connect();
}

void loop() {
  CN.loop();
}
```

## API summary

| Method | Description |
|--------|-------------|
| `begin(apiKey, name)` | Use default Gate server (recommended). |
| `begin(host, port, path, apiKey, name, useSSL)` | Custom server. |
| `setWiFi(ssid, password)` | Connect WiFi before `connect()`. |
| `connect()` / `disconnect()` | Start/stop WebSocket. |
| `loop()` | Call in `loop()`; handles ping, sensors, reconnect. |
| `onDeviceControl(callback)` | Callback(deviceSerial, state, data). Use `data["angle"]`, `data["volume"]`, `data["muted"]`. |
| `reportPhysicalState(deviceSerial, state, source)` | Report local physical on/off changes so server/dashboard state is updated immediately. |
| `mapDevice(serial, pin, name, isDigital)` | Map device to GPIO (digital: relay/light; analog: PWM/servo value). |
| `addSensor(type, deviceSerial, location, intervalMs, readCallback)` | Send sensor readings to Gate. |
| `sendNotification(template, variables)` | POST to `/api/notify` (uses stored host + API key). Templates: `threshold_alert`, `device_alert`, `custom`. |

See the main [iot_library README](../README.md) and [Notify Email API](../../docs/NOTIFY_EMAIL_API.md) for details and examples.

## Version

1.2.0 — Adds `reportPhysicalState(...)` for physical switch sync, plus previous minimal begin/device_control/sendNotification support.
