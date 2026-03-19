#include <CircuitNotion.h>

const char *ssid = "Your_WiFi_SSID";
const char *password = "Your_WiFi_Password";
const char *api_key = "your-api-key-from-dashboard";
const char *microcontroller_name = "ESP32-PhysicalSwitch";

const String DEVICE_SERIAL = "LIGHT-001";
const uint8_t RELAY_PIN = 5;
const uint8_t BUTTON_PIN = 4;

bool currentStateOn = false;
unsigned long lastPressMs = 0;
volatile bool buttonPressed = false;

void reportCurrentState(const char *source) {
  String state = currentStateOn ? "on" : "off";
  digitalWrite(RELAY_PIN, currentStateOn ? HIGH : LOW);
  CN.reportPhysicalState(DEVICE_SERIAL, state, source);
  Serial.println("Physical state -> " + state);
}

void IRAM_ATTR onButtonInterrupt() {
  buttonPressed = true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  CN.begin(api_key, microcontroller_name);
  CN.setWiFi(ssid, password);

  CN.onDeviceControl([](String serial, String state, JsonObject data) {
    if (serial != DEVICE_SERIAL) {
      return;
    }
    currentStateOn = state.equalsIgnoreCase("on");
    digitalWrite(RELAY_PIN, currentStateOn ? HIGH : LOW);
    Serial.println("Remote state -> " + state);
  });

  CN.connect();
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), onButtonInterrupt, FALLING);
}

void loop() {
  CN.loop();

  if (buttonPressed) {
    buttonPressed = false;
    unsigned long now = millis();
    if (now - lastPressMs >= 200) { // debounce in main loop (safe place)
      lastPressMs = now;
      currentStateOn = !currentStateOn;
      reportCurrentState("physical_button");
    }
  }
}
