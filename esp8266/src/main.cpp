/*
  ESP8266 -> MQTT Water Quality Prototype
  ----------------------------------------
  Reads DS18B20 (temperature), ADS1115 (TDS + turbidity),
  publishes JSON telemetry to MQTT broker on laptop.
  PC-side AI engine handles preprocessing, deep-learning inference,
  SQLite storage, and React dashboard serving.

  Libraries (managed by PlatformIO):
    - PubSubClient (MQTT)
    - Adafruit ADS1X15
    - OneWire + DallasTemperature

  Pin wiring:
    DS18B20 DATA  -> D5 (GPIO14)  + 4.7k pull-up to 3.3V
    ADS1115 SDA   -> D2 (GPIO4)
    ADS1115 SCL   -> D1 (GPIO5)
    TDS sensor    -> ADS1115 A1
    Turbidity     -> ADS1115 A0
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// =================================================================
// CONFIGURATION — UPDATE THESE FOR YOUR NETWORK
// =================================================================
const char* WIFI_SSID = "  ";        // Your WiFi SSID
const char* WIFI_PASS = "QAZWSX123578";   // Your WiFi password
const char* MQTT_HOST = "10.79.24.1";   // Updated Laptop WiFi IP
const uint16_t MQTT_PORT = 1884;
const char* MQTT_TOPIC = "water/telemetry";
const char* WATER_BODY = "Test";

// =================================================================
// SENSOR CALIBRATION PROFILES
// =================================================================
// TDS Sensor Calibration
// Calibrated for Pune Municipal Corporation (PMC) Tap Water
// Pune tap water (Khadakwasla dam) typically has a TDS of ~80 to 120 ppm.
const float TDS_CALIBRATION_VOLTAGE = 0.130f; // Typical live voltage reading for this sensor in tap water
const float TDS_CALIBRATION_PPM = 100.0f;     // Average PMC tap water TDS

// Turbidity Sensor Calibration
// Calibrated for Pune Tap Water (very clear, usually < 1.0 NTU)
const float TURBIDITY_CLEAR_VOLTAGE = 1.708f; // Voltage currently read by your sensor
const float TURBIDITY_CLEAR_NTU = 1.0f;       // Clean tap water baseline
const float TURBIDITY_MUDDY_VOLTAGE = 0.5f;   // Estimate for muddy/monsoon water
const float TURBIDITY_MUDDY_NTU = 100.0f;


// =================================================================
// HARDWARE
// =================================================================
#define ONE_WIRE_BUS D5

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature tempSensor(&oneWire);

Adafruit_ADS1115 ads;
bool adsFound = false;

WiFiClient espClient;
PubSubClient mqtt(espClient);

// =================================================================
// NON-BLOCKING TIMER
// =================================================================
unsigned long lastPublish = 0;
const unsigned long PUBLISH_INTERVAL = 1000;  // 1 second — matches dashboard refresh rate

// =================================================================
// SETUP WIFI
// =================================================================
void setup_wifi() {
  Serial.print("\n[WiFi] Connecting to: ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    yield();
    if (++attempts > 60) {  // 30-second timeout
      Serial.println("\n[WiFi] FAILED — restarting ESP...");
      ESP.restart();
    }
  }

  Serial.println();
  Serial.print("[WiFi] Connected! IP: ");
  Serial.println(WiFi.localIP());
}

// =================================================================
// MQTT RECONNECT
// =================================================================
void mqtt_reconnect() {
  int retries = 0;
  while (!mqtt.connected() && retries < 5) {
    Serial.printf("[MQTT] Connecting to %s:%d ...", MQTT_HOST, MQTT_PORT);
    if (mqtt.connect("ESP8266-WaterNode")) {
      Serial.println(" connected!");
      return;
    }
    Serial.printf(" failed (rc=%d), retry in 3s\n", mqtt.state());
    retries++;
    delay(3000);
    yield();
  }
}

// =================================================================
// SETUP
// =================================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("=================================");
  Serial.println("ESP8266 Water Quality Sensor Test");
  Serial.println("  + MQTT Telemetry for Dashboard");
  Serial.println("=================================");

  // I2C bus: SDA = D2, SCL = D1
  Wire.begin(D2, D1);

  Serial.println("\n--- I2C SCANNER ---");
  byte count = 0;
  for (byte i = 8; i < 120; i++) {
    Wire.beginTransmission(i);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found I2C device at address: 0x");
      Serial.println(i, HEX);
      count++;
    }
  }
  if (count == 0) Serial.println("No I2C devices found!");
  Serial.println("-------------------\n");

  // ADS1115 ADC
  adsFound = ads.begin();
  if (adsFound) {
    ads.setGain(GAIN_ONE);  // +/- 4.096V range
    Serial.println("[ADS1115] Initialized successfully");
  } else {
    Serial.println("[ADS1115] WARNING: Not found at 0x48 — TDS/Turbidity will read 0");
  }

  // DS18B20 temperature
  tempSensor.begin();
  tempSensor.setWaitForConversion(false);  // Non-blocking conversion
  tempSensor.requestTemperatures();        // Trigger first conversion

  Serial.print("[DS18B20] Sensors found: ");
  Serial.println(tempSensor.getDeviceCount());

  // WiFi + MQTT
  setup_wifi();
  mqtt.setServer(MQTT_HOST, MQTT_PORT);

  Serial.println();
  Serial.println("[READY] Starting telemetry loop...");
  Serial.println();
}

// =================================================================
// LOOP
// =================================================================
void loop() {
  // Keep MQTT connection alive
  if (!mqtt.connected()) {
    mqtt_reconnect();
  }
  mqtt.loop();
  yield();

  // Non-blocking publish timer
  unsigned long now = millis();
  if (now - lastPublish < PUBLISH_INTERVAL) {
    delay(10);
    return;
  }
  lastPublish = now;

  // ----- 1. Temperature (non-blocking read) -----
  float temperatureC = tempSensor.getTempCByIndex(0);
  tempSensor.requestTemperatures();  // Trigger next conversion for next cycle
  yield();

  // ----- 2. ADC channels (TDS + Turbidity) -----
  int16_t tdsRaw = 0;
  int16_t turbidityRaw = 0;
  float tdsVoltage = 0.0f;
  float turbidityVoltage = 0.0f;

  if (adsFound) {
    tdsRaw = ads.readADC_SingleEnded(1);
    yield();
    turbidityRaw = ads.readADC_SingleEnded(0);
    yield();
    tdsVoltage = ads.computeVolts(tdsRaw);
    turbidityVoltage = ads.computeVolts(turbidityRaw);
  }

  // =================================================================
  // 3. SENSOR CALIBRATION ALGORITHMS
  // =================================================================
  
  // TDS Calculation (Linear Interpolation based on calibration fluid)
  // Assumes 0V = 0ppm.
  float tds = 0.0f;
  if (tdsVoltage > 0) {
      tds = (tdsVoltage / TDS_CALIBRATION_VOLTAGE) * TDS_CALIBRATION_PPM;
  }

  // Turbidity Calculation (Linear Interpolation between Clear and Muddy)
  float turbidity = 0.0f;
  if (turbidityVoltage >= TURBIDITY_CLEAR_VOLTAGE) {
      turbidity = TURBIDITY_CLEAR_NTU; // Max clarity
  } else if (turbidityVoltage <= TURBIDITY_MUDDY_VOLTAGE) {
      turbidity = TURBIDITY_MUDDY_NTU; // Max muddiness
  } else {
      // Map voltage linearly between clear and muddy thresholds
      float voltageRange = TURBIDITY_CLEAR_VOLTAGE - TURBIDITY_MUDDY_VOLTAGE;
      float ntuRange = TURBIDITY_MUDDY_NTU - TURBIDITY_CLEAR_NTU;
      float voltageDrop = TURBIDITY_CLEAR_VOLTAGE - turbidityVoltage;
      
      turbidity = TURBIDITY_CLEAR_NTU + ((voltageDrop / voltageRange) * ntuRange);
  }

  // ----- 3. Serial debug output (same format as your test code) -----
  Serial.println("---------------------------------");

  if (temperatureC <= -126.0f) {
    Serial.println("Water Temperature: ERROR - Sensor disconnected");
  } else {
    Serial.print("Water Temperature: ");
    Serial.print(temperatureC);
    Serial.println(" C");
  }

  Serial.print("Turbidity Raw: ");
  Serial.print(turbidityRaw);
  Serial.print("  Voltage: ");
  Serial.print(turbidityVoltage, 3);
  Serial.println(" V");

  Serial.print("TDS Raw:       ");
  Serial.print(tdsRaw);
  Serial.print("  Voltage: ");
  Serial.print(tdsVoltage, 3);
  Serial.println(" V");

  // ----- 4. Build JSON and publish via MQTT -----
  String payload = "{";
  payload += "\"temperature\":" + String(temperatureC, 2) + ",";
  payload += "\"tds\":" + String(tds, 2) + ",";
  payload += "\"turbidity\":" + String(turbidity, 2) + ",";
  payload += "\"water_body\":\"" + String(WATER_BODY) + "\"";
  payload += "}";

  if (mqtt.publish(MQTT_TOPIC, payload.c_str())) {
    Serial.print("[MQTT SENT] ");
    Serial.println(payload);
  } else {
    Serial.println("[MQTT ERROR] Publish failed");
  }
}
