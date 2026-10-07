
/*
  ESP8266 -> MQTT water-quality system
  Libraries:
    OneWire, DallasTemperature, PubSubClient
    Adafruit ADS1X15
  This sketch publishes raw sensor values. The PC-side AI engine performs
  preprocessing, deep-learning inference, SQLite storage, and dashboard serving.
*/
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <OneWire.h>
#include <DallasTemperature.h>

const char* WIFI_SSID = "  ";
const char* WIFI_PASS = "QAZWSX123578";
const char* MQTT_HOST = "10.118.131.1"; // Your laptop's hotspot/Wi-Fi IP
const uint16_t MQTT_PORT = 1884;

WiFiClient espClient;
PubSubClient mqtt(espClient);
Adafruit_ADS1115 ads;
OneWire oneWire(D5);
DallasTemperature tempSensor(&oneWire);

void setup_wifi(){
  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi Connected! IP: " + WiFi.localIP().toString());
}

void reconnect(){
  while(!mqtt.connected()){
    Serial.print("Attempting MQTT connection to ");
    Serial.print(MQTT_HOST);
    Serial.print(":");
    Serial.print(MQTT_PORT);
    Serial.print("...");
    if(mqtt.connect("ESP8266-WaterNode")){
      Serial.println("connected!");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" retrying in 3s");
      delay(3000);
    }
  }
}

bool adsFound = false;

void setup(){
  Serial.begin(115200);
  Wire.begin(D2, D1); // SDA = D2 (GPIO4), SCL = D1 (GPIO5)
  Wire.setTimeout(100); // 100ms I2C timeout
  adsFound = ads.begin();
  if(adsFound){
    ads.setGain(GAIN_ONE);
    Serial.println("[ADS1115] Initialized successfully");
  } else {
    Serial.println("[ADS1115] Warning: Device not found at 0x48. Using fallback ADC/0 values.");
  }
  tempSensor.begin();
  tempSensor.setWaitForConversion(false); // Non-blocking temperature conversion
  setup_wifi();
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
}

unsigned long lastPublish = 0;
const unsigned long publishInterval = 3000; // publish every 3 seconds

void loop(){
  if(!mqtt.connected()){
    reconnect();
  }
  mqtt.loop();
  yield();

  unsigned long now = millis();
  if(now - lastPublish >= publishInterval){
    lastPublish = now;

    // 1. Read temperature (non-blocking)
    float temperature = tempSensor.getTempCByIndex(0);
    tempSensor.requestTemperatures(); // Trigger next conversion
    yield();

    // 2. Read ADC safely
    int16_t tdsRaw = 0;
    int16_t turbRaw = 0;
    if(adsFound){
      tdsRaw = ads.readADC_SingleEnded(1);
      yield();
      turbRaw = ads.readADC_SingleEnded(0);
      yield();
    }

    // Calibration formula
    float tds = max(0.0f, tdsRaw * 0.02f);
    float turbidity = max(0.0f, turbRaw * 0.02f);

    String payload = "{";
    payload += "\"temperature\":" + String(temperature, 2) + ",";
    payload += "\"tds\":" + String(tds, 2) + ",";
    payload += "\"turbidity\":" + String(turbidity, 2) + ",";
    payload += "\"water_body\":\"Khadakwasla Reservoir/Dam\"";
    payload += "}";

    if(mqtt.publish("water/telemetry", payload.c_str())){
      Serial.print("[SENT]: ");
      Serial.println(payload);
    } else {
      Serial.println("[ERROR]: MQTT publish failed");
    }
  }

  delay(10); // Cooperative delay to yield CPU to WiFi background stack
}
