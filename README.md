# AI + IoT Water Quality Early Warning System — Submission Prototype

## What is included
1. **Model 1 — Sensor Health Deep Neural Network**
   - MLP classifier for NORMAL / SPIKE / DRIFT / STUCK / DISCONNECT / NOISE.
   - Training faults are synthetic injections around the existing prototype data.
2. **Model 2 — pH Estimation Deep Neural Network**
   - Temperature + TDS + turbidity → estimated pH.
   - Trained on `synthetic_pH_target`, so it is a prototype only.
3. **Model 3 — Water Early-Warning Deep Neural Network**
   - Uses sensor features + estimated pH.
   - Prototype warning labels are derived from multi-parameter deviations, not field-validated contamination labels.
4. **FastAPI + SQLite**
5. **MQTT bridge**
6. **ESP8266 Arduino sketch**
7. **Simple React/Vite dashboard**
8. **Mosquitto configuration**

## Run on a laptop

### 1. Install Python dependencies
`pip install -r backend/requirements.txt`

### 2. Start MQTT broker
Install Eclipse Mosquitto and run it with:
`mosquitto -c mqtt/mosquitto.conf`

Mosquitto is the MQTT broker used in this prototype. Official documentation: https://mosquitto.org/man/

### 3. Start API
From the project root:
`uvicorn backend.app:app --reload --host 0.0.0.0 --port 8000`

### 4. Start MQTT bridge
`python mqtt/mqtt_bridge.py`

### 5. Start React dashboard
From `frontend`:
`npm install`
`npm run dev`

### 6. ESP8266
Install PubSubClient, Adafruit ADS1X15, OneWire and DallasTemperature in Arduino IDE.
Set Wi-Fi credentials and `MQTT_HOST` to the laptop's LAN IP, then upload `esp8266/water_monitor.ino`.

## Demo without ESP8266
POST a reading to:
`http://127.0.0.1:8000/api/ingest`

Example JSON:
{"temperature":25.4,"tds":122,"turbidity":4.3,"water_body":"Khadakwasla Reservoir/Dam"}

## Important scientific limitation
This submission prototype is not a certified drinking-water safety system. The pH model uses synthetic pH targets and the water-warning model uses prototype synthetic labels. For research validation, replace those with paired reference-pH measurements and experimentally labelled sensor-fault/contamination events.

## Architecture
Sensors → ESP8266 → preprocessing → Sensor Health ML → pH Estimation ML → Water Early Warning ML → MQTT/Wi-Fi → FastAPI → SQLite → React dashboard.