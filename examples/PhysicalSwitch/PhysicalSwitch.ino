#include <CircuitNotion.h>

// Physical switch synchronization example (ESP8266/ESP32).
//
// This example handles:
// 1) Offline local toggles (offline button toggles always flip the relay locally)
// 2) Resync after reconnect/auth (onConnection(true) reports current relay state)
// 3) Remote state while pressing (device_control updates currentStateOn too)
// 4) ISR safety (ISR only sets a flag; network reporting happens in loop())
// 5) Debounce + report throttling
// 6) Optional relay feedback pin + active-low relay modules

const char *ssid = "Your_WiFi_SSID";
const char *password = "Your_WiFi_Password";
const char *api_key = "your-api-key-from-dashboard";
const char *microcontroller_name = "ESP32-PhysicalSwitch";

const String DEVICE_SERIAL = "LIGHT-001";
const uint8_t RELAY_PIN = 5;
const uint8_t BUTTON_PIN = 4;

// If your relay module is active-low, set to true.
const bool RELAY_ACTIVE_LOW = false;

// Optional: relay feedback pin (if you have a real feedback signal).
// Set to 255 if you do NOT have a feedback pin.
const uint8_t RELAY_FEEDBACK_PIN = 255;

// Debounce + reporting throttling.
const unsigned long BUTTON_DEBOUNCE_MS = 200;
const unsigned long REPORT_THROTTLE_MS = 300;

bool currentStateOn = false;
unsigned long lastPressMs = 0;
unsigned long lastReportMs = 0;
volatile bool buttonPressed = false;

bool readActualRelayOn() {
  if (RELAY_FEEDBACK_PIN != 255) {
    int lvl = digitalRead(RELAY_FEEDBACK_PIN);
    return RELAY_ACTIVE_LOW ? (lvl == LOW) : (lvl == HIGH);
  }
  // No feedback pin: treat what we drive as the truth.
  int lvl = digitalRead(RELAY_PIN);
  return RELAY_ACTIVE_LOW ? (lvl == LOW) : (lvl == HIGH);
}

void applyRelayState(bool on) {
  if (RELAY_ACTIVE_LOW) {
    digitalWrite(RELAY_PIN, on ? LOW : HIGH);
  } else {
    digitalWrite(RELAY_PIN, on ? HIGH : LOW);
  }
}

void reportCurrentState(const char *source) {
  // Rate-limit reports to reduce bounce noise.
  unsigned long now = millis();
  if (now - lastReportMs < REPORT_THROTTLE_MS) return;
  lastReportMs = now;

  String state = currentStateOn ? "on" : "off";
  applyRelayState(currentStateOn);
  CN.reportPhysicalState(DEVICE_SERIAL, state, source);
  Serial.println("Physical state -> " + state + " (source=" + String(source) + ")");
}

void IRAM_ATTR onButtonInterrupt() {
  // ISR safety: only set a flag.
  buttonPressed = true;
}

void resyncAfterAuth() {
  // Scenario: device was toggled while offline, so server might be stale.
  // When websocket auth succeeds again, push the current relay state.
  currentStateOn = readActualRelayOn();
  lastReportMs = 0; // ensure reconnect resync is not throttled
  reportCurrentState("reconnect_resync");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(RELAY_PIN, OUTPUT);
  if (RELAY_FEEDBACK_PIN != 255) {
    pinMode(RELAY_FEEDBACK_PIN, INPUT);
  }
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Boot-time scenarios:
  // - If feedback pin exists: we recover actual relay state after reboot.
  // - Else: we start from a safe default (OFF) and assume MCU controls relay output.
  currentStateOn = readActualRelayOn();
  applyRelayState(currentStateOn);
  lastReportMs = 0;

  CN.begin(api_key, microcontroller_name);
  CN.setWiFi(ssid, password);

  // Remote control updates: keep currentStateOn in sync so the NEXT physical press toggles correctly.
  CN.onDeviceControl([](String serial, String state, JsonObject data) {
    if (serial != DEVICE_SERIAL) return;
    bool newOn = state.equalsIgnoreCase("on");
    currentStateOn = newOn;
    applyRelayState(currentStateOn);
    Serial.println("Remote state -> " + state + " (source=device_control)");
  });

  // Reconnect/auth scenario: when websocket becomes authenticated again, push state immediately.
  CN.onConnection([](bool connected) {
    if (!connected) return;
    resyncAfterAuth();
  });

  CN.connect();
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), onButtonInterrupt, FALLING);
}

void loop() {
  CN.loop();

  if (!buttonPressed) return;
  buttonPressed = false;

  unsigned long now = millis();
  if (now - lastPressMs < BUTTON_DEBOUNCE_MS) return;
  lastPressMs = now;

  // Physical toggle:
  // - If wiring has feedback: toggle based on actual relay state.
  // - Else: toggle based on currentStateOn variable.
  bool baseOn = (RELAY_FEEDBACK_PIN != 255) ? readActualRelayOn() : currentStateOn;
  currentStateOn = !baseOn;
  applyRelayState(currentStateOn);

  // Scenario: if offline, CN.reportPhysicalState will fail (library checks websocket/auth).
  // We still update local hardware; when reconnect happens, onConnection(true) will resync.
  reportCurrentState("physical_button");
}
