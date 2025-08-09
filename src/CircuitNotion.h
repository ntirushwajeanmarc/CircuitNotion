#ifndef CIRCUITNOTION_H
#define CIRCUITNOTION_H

#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <functional>

// Version information
#define CIRCUITNOTION_VERSION "1.0.0"
#define CIRCUITNOTION_VERSION_MAJOR 1
#define CIRCUITNOTION_VERSION_MINOR 0
#define CIRCUITNOTION_VERSION_PATCH 0

// Forward declarations
class CircuitNotionSensor;

// Callback function types
typedef std::function<void(String deviceSerial, String state)> DeviceControlCallback;
typedef std::function<void(String message)> LogCallback;
typedef std::function<void(bool connected)> ConnectionCallback;

// Sensor value structure
struct SensorValue
{
    float value;
    String unit;
    JsonObject metadata;

    // Constructor for easy creation
    SensorValue(float val, String unit_str) : value(val), unit(unit_str) {}
    SensorValue(float val, String unit_str, JsonObject meta) : value(val), unit(unit_str), metadata(meta) {}
};

// Sensor reading callback
typedef std::function<SensorValue()> SensorReadCallback;

// Connection status enum
enum ConnectionStatus
{
    DISCONNECTED = 0,
    CONNECTING = 1,
    CONNECTED = 2,
    AUTHENTICATED = 3
};

// Device control mapping structure (for local pin control)
struct DeviceMapping
{
    String serial;
    uint8_t pin;
    bool isDigital;
    String name;
    bool inverted;

    DeviceMapping(String ser, uint8_t p, bool digital, String n = "", bool inv = false)
        : serial(ser), pin(p), isDigital(digital), name(n), inverted(inv) {}
};

class CircuitNotionSensor
{
private:
    String _type;
    String _deviceSerial;
    String _location;
    unsigned long _interval;
    unsigned long _lastReading;
    SensorReadCallback _readCallback;
    float _threshold;
    float _lastValue;
    bool _changeDetection;
    bool _enabled;

public:
    CircuitNotionSensor(String type, String deviceSerial, String location,
                        unsigned long interval, SensorReadCallback callback);

    void setChangeThreshold(float threshold);
    void enableChangeDetection(bool enabled);
    void setEnabled(bool enabled);
    bool shouldRead();
    bool shouldSend(float newValue);
    SensorValue read();
    String getType() { return _type; }
    String getDeviceSerial() { return _deviceSerial; }
    String getLocation() { return _location; }
    unsigned long getInterval() { return _interval; }
    bool isEnabled() { return _enabled; }
};

class CircuitNotion
{
private:
    // WebSocket client
    WebSocketsClient _webSocket;

    // Configuration
    String _host;
    int _port;
    String _path;
    String _apiKey;
    String _microcontrollerName;
    bool _useSSL;

    // Status
    ConnectionStatus _status;
    bool _isAuthenticated;
    unsigned long _lastPing;
    unsigned long _reconnectInterval;
    bool _autoReconnect;
    unsigned long _lastReconnectAttempt;

    // Collections
    std::vector<DeviceMapping> _deviceMappings;
    std::vector<CircuitNotionSensor *> _sensors;

    // Callbacks
    DeviceControlCallback _deviceControlCallback;
    LogCallback _logCallback;
    ConnectionCallback _connectionCallback;

    // Statistics
    unsigned long _totalSensorReadings;
    unsigned long _totalMessagesReceived;
    unsigned long _lastConnectionTime;

    // Internal methods
    void webSocketEvent(WStype_t type, uint8_t *payload, size_t length);
    void handleMessage(const char *msg);
    void sendAuth();
    void sendPong();
    void sendSensorReading(CircuitNotionSensor *sensor, SensorValue value);
    void log(String message);
    void handleDeviceStateUpdate(String deviceSerial, String state);
    DeviceMapping *findDeviceMapping(String serial);
    void attemptReconnect();
    static CircuitNotion *_instance; // For static callback

public:
    CircuitNotion();
    ~CircuitNotion();

    // Configuration methods
    void begin(String host, int port, String path, String apiKey, String microcontrollerName, bool useSSL = true);
    void setWiFi(String ssid, String password);

    // Callback setters
    void onDeviceControl(DeviceControlCallback callback);
    void onLog(LogCallback callback);
    void onConnection(ConnectionCallback callback);

    // Device mapping management (for local control)
    void mapDevice(String deviceSerial, uint8_t pin, String deviceName = "", bool isDigital = true);
    void mapDigitalDevice(String deviceSerial, uint8_t pin, String deviceName = "", bool inverted = false);
    void mapAnalogDevice(String deviceSerial, uint8_t pin, String deviceName = "");
    void mapPWMDevice(String deviceSerial, uint8_t pin, String deviceName = "");
    void controlLocalDevice(String deviceSerial, String state);
    void controlLocalDevice(String deviceSerial, int value);

    // Sensor management (sensors are attached to this microcontroller)
    CircuitNotionSensor *addSensor(String type, String deviceSerial, String location,
                                   unsigned long interval, SensorReadCallback callback);
    CircuitNotionSensor *addTemperatureSensor(String deviceSerial, String location,
                                              unsigned long interval, SensorReadCallback callback);
    CircuitNotionSensor *addHumiditySensor(String deviceSerial, String location,
                                           unsigned long interval, SensorReadCallback callback);
    CircuitNotionSensor *addLightSensor(String deviceSerial, String location,
                                        unsigned long interval, SensorReadCallback callback);
    CircuitNotionSensor *addMotionSensor(String deviceSerial, String location,
                                         unsigned long interval, SensorReadCallback callback);
    CircuitNotionSensor *addCustomSensor(String sensorType, String deviceSerial, String location,
                                         unsigned long interval, SensorReadCallback callback);

    // Sensor management
    void enableSensor(String sensorType, String deviceSerial);
    void disableSensor(String sensorType, String deviceSerial);
    void removeAllSensors();

    // Connection methods
    void connect();
    void disconnect();
    bool isConnected();
    bool isAuthenticated();
    ConnectionStatus getStatus();
    String getStatusString();

    // Main loop method
    void loop();

    // Utility methods
    void sendCustomMessage(JsonDocument &doc);
    void enableAutoReconnect(bool enabled, unsigned long interval = 5000);

    // System info and statistics
    String getMicrocontrollerName() { return _microcontrollerName; }
    String getAPIKey() { return _apiKey; }
    int getSensorCount() { return _sensors.size(); }
    int getDeviceMappingCount() { return _deviceMappings.size(); }
    unsigned long getTotalSensorReadings() { return _totalSensorReadings; }
    unsigned long getTotalMessagesReceived() { return _totalMessagesReceived; }
    unsigned long getUptimeMs() { return _lastConnectionTime > 0 ? millis() - _lastConnectionTime : 0; }
    String getLibraryVersion() { return CIRCUITNOTION_VERSION; }

    // Debug and diagnostics
    void printDiagnostics();
    void printSensorStatus();
    void printDeviceMappings();
};

// Global instance for easy access
extern CircuitNotion CN;

#endif
