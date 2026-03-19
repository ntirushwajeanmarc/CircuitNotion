#include "CircuitNotion.h"

#if defined(ESP8266)
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#elif defined(ESP32)
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#endif

// Global instance
CircuitNotion CN;

// Static instance pointer for callbacks
CircuitNotion* CircuitNotion::_instance = nullptr;

// =====================================
// CircuitNotionSensor Implementations
// =====================================

CircuitNotionSensor::CircuitNotionSensor(String type, String deviceSerial, String location,
                                         unsigned long interval, SensorReadCallback callback)
    : _type(type), _deviceSerial(deviceSerial), _location(location),
      _interval(interval), _readCallback(callback), _lastReading(0),
      _threshold(0.0), _lastValue(0.0), _changeDetection(false), _enabled(true) {
}

void CircuitNotionSensor::setChangeThreshold(float threshold) {
    _threshold = threshold;
}

void CircuitNotionSensor::enableChangeDetection(bool enabled) {
    _changeDetection = enabled;
}

void CircuitNotionSensor::setEnabled(bool enabled) {
    _enabled = enabled;
}

bool CircuitNotionSensor::shouldRead() {
    if (!_enabled) return false;
    return (millis() - _lastReading) >= _interval;
}

bool CircuitNotionSensor::shouldSend(float newValue) {
    if (!_changeDetection) return true;
    
    float difference = abs(newValue - _lastValue);
    bool shouldSend = difference >= _threshold;
    _lastValue = newValue;
    return shouldSend;
}

SensorValue CircuitNotionSensor::read() {
    _lastReading = millis();
    return _readCallback();
}

// =====================================
// CircuitNotion Main Class Implementations
// =====================================

CircuitNotion::CircuitNotion() 
    : _port(443), _useSSL(true), _status(DISCONNECTED), 
      _isAuthenticated(false), _lastPing(0), _reconnectInterval(5000),
      _autoReconnect(true), _lastReconnectAttempt(0),
      _totalSensorReadings(0), _totalMessagesReceived(0), _lastConnectionTime(0) {
    _instance = this;
}

CircuitNotion::~CircuitNotion() {
    // Clean up sensors
    for (auto* sensor : _sensors) {
        delete sensor;
    }
    _sensors.clear();
    
    // Device mappings are value types, no cleanup needed
    _deviceMappings.clear();
}

void CircuitNotion::begin(String host, int port, String path, String apiKey, String microcontrollerName, bool useSSL) {
    _host = host;
    _port = port;
    _path = path;
    _apiKey = apiKey;
    _microcontrollerName = microcontrollerName;
    _useSSL = useSSL;
    
    log("CircuitNotion library initialized v" + String(CIRCUITNOTION_VERSION));
    log("Host: " + _host + ":" + String(_port));
    log("Microcontroller: " + _microcontrollerName);
}

void CircuitNotion::begin(String apiKey, String microcontrollerName) {
    begin(String(CIRCUITNOTION_DEFAULT_HOST), CIRCUITNOTION_DEFAULT_PORT,
          String(CIRCUITNOTION_DEFAULT_PATH), apiKey, microcontrollerName, true);
}

void CircuitNotion::setWiFi(String ssid, String password) {
    log("Connecting to WiFi: " + ssid);
    WiFi.begin(ssid.c_str(), password.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        attempts++;
        log(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        log("WiFi connected! IP: " + WiFi.localIP().toString());
    } else {
        log("WiFi connection failed!");
    }
}

void CircuitNotion::onDeviceControl(DeviceControlCallback callback) {
    _deviceControlCallback = callback;
}

void CircuitNotion::onLog(LogCallback callback) {
    _logCallback = callback;
}

void CircuitNotion::onConnection(ConnectionCallback callback) {
    _connectionCallback = callback;
}

// Device mapping methods for local control
void CircuitNotion::mapDevice(String deviceSerial, uint8_t pin, String deviceName, bool isDigital) {
    DeviceMapping mapping(deviceSerial, pin, isDigital, deviceName);
    _deviceMappings.push_back(mapping);
    pinMode(pin, OUTPUT);
    if (isDigital) {
        digitalWrite(pin, LOW);
    }
    log("Mapped device: " + deviceName + " (" + deviceSerial + ") to pin " + String(pin));
}

void CircuitNotion::mapDigitalDevice(String deviceSerial, uint8_t pin, String deviceName, bool inverted) {
    DeviceMapping mapping(deviceSerial, pin, true, deviceName, inverted);
    _deviceMappings.push_back(mapping);
    pinMode(pin, OUTPUT);
    digitalWrite(pin, inverted ? HIGH : LOW);
    log("Mapped digital device: " + deviceName + " (" + deviceSerial + ") to pin " + String(pin));
}

void CircuitNotion::mapAnalogDevice(String deviceSerial, uint8_t pin, String deviceName) {
    mapDevice(deviceSerial, pin, deviceName, false);
}

void CircuitNotion::mapPWMDevice(String deviceSerial, uint8_t pin, String deviceName) {
    mapDevice(deviceSerial, pin, deviceName, false);
}

void CircuitNotion::controlLocalDevice(String deviceSerial, String state) {
    DeviceMapping* mapping = findDeviceMapping(deviceSerial);
    if (!mapping) return;
    
    if (mapping->isDigital) {
        bool on = (state.equalsIgnoreCase("on") || state.equalsIgnoreCase("true") || state == "1");
        digitalWrite(mapping->pin, mapping->inverted ? !on : on);
        log("Set device " + deviceSerial + " to " + state);
    }
}

void CircuitNotion::controlLocalDevice(String deviceSerial, int value) {
    DeviceMapping* mapping = findDeviceMapping(deviceSerial);
    if (!mapping) return;
    
    if (!mapping->isDigital) {
        analogWrite(mapping->pin, value);
        log("Set device " + deviceSerial + " to " + String(value));
    }
}

bool CircuitNotion::reportPhysicalState(String deviceSerial, String state, String source) {
    if (_status != AUTHENTICATED) {
        log("reportPhysicalState: not authenticated");
        return false;
    }

    String normalized = state;
    normalized.toLowerCase();
    if (normalized != "on" && normalized != "off") {
        log("reportPhysicalState: state must be 'on' or 'off'");
        return false;
    }

    JsonDocument doc;
    doc["type"] = "physical_state_changed";
    doc["device_serial"] = deviceSerial;
    doc["state"] = normalized;
    doc["source"] = source;

    String message;
    serializeJson(doc, message);
    _webSocket.sendTXT(message);
    log("Reported physical state change: " + deviceSerial + " -> " + normalized);
    return true;
}

DeviceMapping* CircuitNotion::findDeviceMapping(String serial) {
    for (auto& mapping : _deviceMappings) {
        if (mapping.serial == serial) {
            return &mapping;
        }
    }
    return nullptr;
}

// Sensor management methods
CircuitNotionSensor* CircuitNotion::addSensor(String type, String deviceSerial, String location,
                                              unsigned long interval, SensorReadCallback callback) {
    auto* sensor = new CircuitNotionSensor(type, deviceSerial, location, interval, callback);
    _sensors.push_back(sensor);
    log("Added " + type + " sensor for device " + deviceSerial + " at " + location);
    return sensor;
}

CircuitNotionSensor* CircuitNotion::addTemperatureSensor(String deviceSerial, String location,
                                                         unsigned long interval, SensorReadCallback callback) {
    return addSensor("temperature", deviceSerial, location, interval, callback);
}

CircuitNotionSensor* CircuitNotion::addHumiditySensor(String deviceSerial, String location,
                                                      unsigned long interval, SensorReadCallback callback) {
    return addSensor("humidity", deviceSerial, location, interval, callback);
}

CircuitNotionSensor* CircuitNotion::addLightSensor(String deviceSerial, String location,
                                                   unsigned long interval, SensorReadCallback callback) {
    return addSensor("light", deviceSerial, location, interval, callback);
}

CircuitNotionSensor* CircuitNotion::addMotionSensor(String deviceSerial, String location,
                                                    unsigned long interval, SensorReadCallback callback) {
    return addSensor("motion", deviceSerial, location, interval, callback);
}

CircuitNotionSensor* CircuitNotion::addCustomSensor(String sensorType, String deviceSerial, String location,
                                                    unsigned long interval, SensorReadCallback callback) {
    return addSensor(sensorType, deviceSerial, location, interval, callback);
}

void CircuitNotion::enableSensor(String sensorType, String deviceSerial) {
    for (auto* sensor : _sensors) {
        if (sensor->getType() == sensorType && sensor->getDeviceSerial() == deviceSerial) {
            sensor->setEnabled(true);
            log("Enabled sensor: " + sensorType + " for device " + deviceSerial);
            break;
        }
    }
}

void CircuitNotion::disableSensor(String sensorType, String deviceSerial) {
    for (auto* sensor : _sensors) {
        if (sensor->getType() == sensorType && sensor->getDeviceSerial() == deviceSerial) {
            sensor->setEnabled(false);
            log("Disabled sensor: " + sensorType + " for device " + deviceSerial);
            break;
        }
    }
}

void CircuitNotion::removeAllSensors() {
    for (auto* sensor : _sensors) {
        delete sensor;
    }
    _sensors.clear();
    log("Removed all sensors");
}

void CircuitNotion::connect() {
    if (WiFi.status() != WL_CONNECTED) {
        log("Error: WiFi not connected");
        return;
    }
    
    _status = CONNECTING;
    log("Connecting to CircuitNotion server...");
    
    if (_connectionCallback) {
        _connectionCallback(false);
    }
    
    // Setup WebSocket event handler
    _webSocket.onEvent([this](WStype_t type, uint8_t* payload, size_t length) {
        this->webSocketEvent(type, payload, length);
    });
    
    // Connect to server
    if (_useSSL) {
        _webSocket.beginSSL(_host.c_str(), _port, _path.c_str());
    } else {
        _webSocket.begin(_host.c_str(), _port, _path.c_str());
    }
    
    if (_autoReconnect) {
        _webSocket.setReconnectInterval(_reconnectInterval);
        // Keep connection alive so server/MC don't drop it (prevents "broken pipe" / "unexpected EOF")
        _webSocket.enableHeartbeat(15000, 3000, 2);
    }
}

void CircuitNotion::disconnect() {
    _webSocket.disconnect();
    _status = DISCONNECTED;
    _isAuthenticated = false;
    
    if (_connectionCallback) {
        _connectionCallback(false);
    }
    
    log("Disconnected from CircuitNotion server");
}

bool CircuitNotion::isConnected() {
    return _status == CONNECTED || _status == AUTHENTICATED;
}

bool CircuitNotion::isAuthenticated() {
    return _isAuthenticated;
}

ConnectionStatus CircuitNotion::getStatus() {
    return _status;
}

String CircuitNotion::getStatusString() {
    switch (_status) {
        case DISCONNECTED: return "Disconnected";
        case CONNECTING: return "Connecting";
        case CONNECTED: return "Connected";
        case AUTHENTICATED: return "Authenticated";
        default: return "Unknown";
    }
}

void CircuitNotion::loop() {
    _webSocket.loop();
    
    // Handle auto-reconnect
    if (_autoReconnect && _status == DISCONNECTED && 
        (millis() - _lastReconnectAttempt) > _reconnectInterval) {
        _lastReconnectAttempt = millis();
        attemptReconnect();
    }
    
    // Handle ping/pong
    if (_status == AUTHENTICATED && (millis() - _lastPing) > 30000) {
        // Send ping every 30 seconds
        _webSocket.sendTXT("{\"type\":\"ping\"}");
        _lastPing = millis();
    }
    
    // Process sensor readings
    for (auto* sensor : _sensors) {
        if (sensor->shouldRead()) {
            SensorValue value = sensor->read();
            if (sensor->shouldSend(value.value)) {
                sendSensorReading(sensor, value);
                _totalSensorReadings++;
            }
        }
    }
}

void CircuitNotion::sendCustomMessage(JsonDocument& doc) {
    String message;
    serializeJson(doc, message);
    _webSocket.sendTXT(message);
}

void CircuitNotion::enableAutoReconnect(bool enabled, unsigned long interval) {
    _autoReconnect = enabled;
    _reconnectInterval = interval;
    
    if (enabled) {
        _webSocket.enableHeartbeat(15000, 3000, 2);
        _webSocket.setReconnectInterval(interval);
        log("Auto-reconnect enabled with " + String(interval) + "ms interval");
    } else {
        log("Auto-reconnect disabled");
    }
}

void CircuitNotion::printDiagnostics() {
    log("=== CircuitNotion Diagnostics ===");
    log("Library Version: " + String(CIRCUITNOTION_VERSION));
    log("Status: " + getStatusString());
    log("Microcontroller: " + _microcontrollerName);
    log("Host: " + _host + ":" + String(_port));
    log("Sensors: " + String(_sensors.size()));
    log("Device Mappings: " + String(_deviceMappings.size()));
    log("Total Sensor Readings: " + String(_totalSensorReadings));
    log("Total Messages Received: " + String(_totalMessagesReceived));
    log("Uptime: " + String(getUptimeMs()) + "ms");
    log("Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
}

void CircuitNotion::printSensorStatus() {
    log("=== Sensor Status ===");
    for (auto* sensor : _sensors) {
        log(sensor->getType() + " (" + sensor->getDeviceSerial() + ") at " + 
            sensor->getLocation() + " - " + (sensor->isEnabled() ? "Enabled" : "Disabled") +
            " - Interval: " + String(sensor->getInterval()) + "ms");
    }
}

void CircuitNotion::printDeviceMappings() {
    log("=== Device Mappings ===");
    for (auto& mapping : _deviceMappings) {
        log(mapping.name + " (" + mapping.serial + ") -> Pin " + String(mapping.pin) +
            " (" + (mapping.isDigital ? "Digital" : "Analog") + 
            (mapping.inverted ? ", Inverted" : "") + ")");
    }
}

// Private methods

void CircuitNotion::attemptReconnect() {
    log("Attempting to reconnect...");
    connect();
}

void CircuitNotion::webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            _status = DISCONNECTED;
            _isAuthenticated = false;
            log("WebSocket Disconnected");
            if (_connectionCallback) {
                _connectionCallback(false);
            }
            break;
            
        case WStype_CONNECTED:
            _status = CONNECTED;
            _lastConnectionTime = millis();
            log("WebSocket Connected to: " + String((char*)payload));
            sendAuth();
            break;
            
        case WStype_TEXT: {
            String message = String((char*)payload);
            _totalMessagesReceived++;
            handleMessage(message.c_str());
            break;
        }
        
        case WStype_ERROR:
            log("WebSocket Error");
            break;
            
        default:
            break;
    }
}

void CircuitNotion::handleMessage(const char* msg) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, msg);
    
    if (error) {
        log("Failed to parse message: " + String(msg));
        return;
    }
    
    String type = doc["type"];
    
    if (type == "auth_success") {
        _status = AUTHENTICATED;
        _isAuthenticated = true;
        log("Authentication successful");
        if (_connectionCallback) {
            _connectionCallback(true);
        }
    }
    else if (type == "auth_error") {
        log("Authentication failed: " + String(doc["message"]));
        disconnect();
    }
    else if (type == "device_control") {
        String deviceSerial = doc["device_serial"];
        String state = doc["state"];
        JsonObject data = doc["data"].as<JsonObject>();
        
        handleDeviceStateUpdate(deviceSerial, state, data);
    }
    else if (type == "ping") {
        sendPong();
    }
    else if (type == "pong") {
        // Pong received, connection is healthy
    }
    else {
        log("Unknown message type: " + type);
    }
}

void CircuitNotion::handleDeviceStateUpdate(String deviceSerial, String state, JsonObject data) {
    // Control local device if mapped (use angle for servo when present)
    if (!data.isNull() && data.containsKey("angle")) {
        int angle = data["angle"].as<int>();
        controlLocalDevice(deviceSerial, angle);
    } else {
        controlLocalDevice(deviceSerial, state);
    }
    
    // Call user callback (data may contain angle, volume, muted, etc.)
    if (_deviceControlCallback) {
        _deviceControlCallback(deviceSerial, state, data);
    }
    
    log("Device control: " + deviceSerial + " -> " + state);
}

void CircuitNotion::sendAuth() {
    JsonDocument doc;
    doc["type"] = "auth";
    doc["api_key"] = _apiKey;
    doc["microcontroller_name"] = _microcontrollerName;
    
    String message;
    serializeJson(doc, message);
    _webSocket.sendTXT(message);
    
    log("Sent authentication request for microcontroller: " + _microcontrollerName);
}

void CircuitNotion::sendPong() {
    _webSocket.sendTXT("{\"type\":\"pong\"}");
}

void CircuitNotion::sendSensorReading(CircuitNotionSensor* sensor, SensorValue value) {
    JsonDocument doc;
    doc["type"] = "sensor_reading";
    doc["sensor_type"] = sensor->getType();
    doc["device_serial"] = sensor->getDeviceSerial();
    doc["location"] = sensor->getLocation();
    doc["value"] = value.value;
    doc["unit"] = value.unit;
    doc["timestamp"] = millis();
    doc["microcontroller"] = _microcontrollerName;
    
    String message;
    serializeJson(doc, message);
    _webSocket.sendTXT(message);
    
    log("Sent " + sensor->getType() + " reading: " + String(value.value) + " " + value.unit + 
        " for device " + sensor->getDeviceSerial());
}

void CircuitNotion::log(String message) {
    if (_logCallback) {
        _logCallback(message);
    } else {
        Serial.println("[CircuitNotion] " + message);
    }
}

bool CircuitNotion::sendNotification(String templateName) {
    JsonDocument emptyDoc;
    JsonObject emptyVars = emptyDoc.to<JsonObject>();
    return sendNotification(templateName, emptyVars);
}

bool CircuitNotion::sendNotification(String templateName, JsonObject variables) {
    return sendNotificationRequest(templateName, variables);
}

bool CircuitNotion::sendNotificationRequest(String templateName, JsonObject variables) {
    JsonDocument doc;
    doc["template"] = templateName;
    JsonObject vars = doc["variables"].to<JsonObject>();
    for (JsonPair kv : variables) {
        vars[kv.key()] = kv.value();
    }
    String body;
    serializeJson(doc, body);

#if defined(ESP8266) || defined(ESP32)
    int port = _port;
    String url = String(_useSSL ? "https" : "http") + "://" + _host;
    if ((_useSSL && port != 443) || (!_useSSL && port != 80)) {
        url += ":" + String(port);
    }
    url += "/api/notify";
    WiFiClientSecure client;
    client.setInsecure();
#if defined(ESP8266)
    HTTPClient http;
    if (!http.begin(client, url)) {
        log("sendNotification: failed to begin HTTP");
        return false;
    }
#else
    HTTPClient http;
    if (!http.begin(client, url)) {
        log("sendNotification: failed to begin HTTP");
        return false;
    }
#endif
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-API-Key", _apiKey);
    int code = http.POST(body);
    bool ok = (code == 200);
    if (!ok) {
        log("sendNotification: HTTP " + String(code));
    }
    http.end();
    return ok;
#else
    (void)body;
    log("sendNotification: not implemented for this platform");
    return false;
#endif
}
