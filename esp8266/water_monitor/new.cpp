#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// -------------------------
// DS18B20
// -------------------------
#define ONE_WIRE_BUS D5

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature tempSensor(&oneWire);

// -------------------------
// ADS1115
// -------------------------
Adafruit_ADS1115 ads;

void setup() {

  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("=================================");
  Serial.println("ESP8266 Water Quality Sensor Test");
  Serial.println("=================================");

  // I2C
  // SDA = D2
  // SCL = D1
  Wire.begin(D2, D1);

  // Start ADS1115
  if (!ads.begin()) {
    Serial.println("ERROR: ADS1115 not found!");
    while (1) {
      delay(1000);
    }
  }

  Serial.println("ADS1115 detected.");

  // Gain = +/-4.096 V
  ads.setGain(GAIN_ONE);

  // Start DS18B20
  tempSensor.begin();

  Serial.print("DS18B20 sensors found: ");
  Serial.println(tempSensor.getDeviceCount());

  Serial.println();
  Serial.println("Starting readings...");
  Serial.println();
}

void loop() {

  // =================================
  // TEMPERATURE
  // =================================

  tempSensor.requestTemperatures();

  float temperatureC = tempSensor.getTempCByIndex(0);

  // =================================
  // ADS1115
  // =================================

  int16_t turbidityRaw = ads.readADC_SingleEnded(0);
  int16_t tdsRaw = ads.readADC_SingleEnded(1);

  // Convert ADC counts to voltage
  float turbidityVoltage = ads.computeVolts(turbidityRaw);
  float tdsVoltage = ads.computeVolts(tdsRaw);

  // =================================
  // DISPLAY
  // =================================

  Serial.println("---------------------------------");

  // Temperature
  Serial.print("Water Temperature: ");

  if (temperatureC == DEVICE_DISCONNECTED_C) {
    Serial.println("ERROR - Sensor disconnected");
  } else {
    Serial.print(temperatureC, 2);
    Serial.println(" °C");
  }

  // Turbidity
  Serial.print("Turbidity Raw: ");
  Serial.println(turbidityRaw);

  Serial.print("Turbidity Voltage: ");
  Serial.print(turbidityVoltage, 3);
  Serial.println(" V");

  // TDS
  Serial.print("TDS Raw: ");
  Serial.println(tdsRaw);

  Serial.print("TDS Voltage: ");
  Serial.print(tdsVoltage, 3);
  Serial.println(" V");

  delay(2000);
}