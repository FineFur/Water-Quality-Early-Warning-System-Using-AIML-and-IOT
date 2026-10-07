# AI + IoT Water Quality Early Warning System

![Project Status](https://img.shields.io/badge/status-prototype-orange)
![Primary Language](https://img.shields.io/badge/primary%20language-C%2B%2B-blue)
![Backend](https://img.shields.io/badge/backend-FastAPI-009688)
![Frontend](https://img.shields.io/badge/frontend-React%20%2B%20Vite-61DAFB)
![ML](https://img.shields.io/badge/ML-PyTorch-EE4C2C)
![IoT](https://img.shields.io/badge/IoT-ESP8266%20%2B%20MQTT-7B68EE)

A complete prototype for **real-time water-quality monitoring and early warning** using low-cost IoT sensors, an ESP8266 edge node, MQTT messaging, a FastAPI backend, PyTorch neural networks, SQLite persistence, and a React dashboard.

The system collects temperature, Total Dissolved Solids (TDS), and turbidity readings; evaluates sensor health; estimates pH using a machine-learning regression model; classifies the water condition; stores readings; and presents live results through a browser-based dashboard.

> **Important:** This repository is an educational and research prototype. It is not a certified drinking-water safety system, medical device, environmental compliance instrument, or replacement for laboratory testing and calibrated water-quality equipment. The pH and water-warning models use prototype/synthetic targets and labels and must be independently validated before any operational deployment.

---

## Table of Contents

- [Project Overview](#project-overview)
- [Objectives](#objectives)
- [Key Features](#key-features)
- [System Architecture](#system-architecture)
- [End-to-End Data Flow](#end-to-end-data-flow)
- [Repository Structure](#repository-structure)
- [Technology Stack](#technology-stack)
- [Hardware](#hardware)
- [Software Prerequisites](#software-prerequisites)
- [Quick Start Without Hardware](#quick-start-without-hardware)
- [Complete Local Setup](#complete-local-setup)
- [Running the ESP8266 Node](#running-the-esp8266-node)
- [Running the Sensor Simulator](#running-the-sensor-simulator)
- [Sending Test Readings](#sending-test-readings)
- [Backend API](#backend-api)
- [Input and Output Schemas](#input-and-output-schemas)
- [Machine-Learning Pipeline](#machine-learning-pipeline)
- [MQTT Integration](#mqtt-integration)
- [Frontend Dashboard](#frontend-dashboard)
- [Database](#database)
- [Configuration](#configuration)
- [Demonstration Workflow](#demonstration-workflow)
- [Troubleshooting](#troubleshooting)
- [Scientific and Operational Limitations](#scientific-and-operational-limitations)
- [Security Considerations](#security-considerations)
- [Future Improvements](#future-improvements)
- [Contributing](#contributing)
- [License](#license)

---

## Project Overview

Water-quality monitoring traditionally relies on laboratory analysis or dedicated chemical probes. Those approaches can be accurate, but they may be expensive, require calibration and maintenance, and can be difficult to scale across distributed water bodies.

This project explores a layered alternative:

1. **Physical sensing** collects temperature, TDS, and turbidity.
2. **Edge hardware** uses an ESP8266 to read sensors and transmit telemetry.
3. **MQTT** provides lightweight publish/subscribe communication between the sensor node and the host computer.
4. **A Python bridge** forwards MQTT messages to the HTTP backend.
5. **FastAPI** validates incoming readings and exposes REST endpoints.
6. **PyTorch models** analyze sensor health, estimate pH, and classify water status.
7. **SQLite** stores processed readings and model outputs.
8. **React and Recharts** provide a live dashboard for operators and demonstrations.

The default demonstration water body is `Khadakwasla Reservoir/Dam`, but the API accepts a configurable `water_body` value.

## Objectives

- Monitor water-quality indicators in near real time.
- Demonstrate IoT telemetry ingestion over MQTT and HTTP.
- Detect abnormal sensor behavior such as spikes, drift, stuck readings, disconnects, and noise.
- Estimate pH from more readily available sensor values.
- Provide early-warning classifications for potentially abnormal water conditions.
- Store a history of readings for trend visualization.
- Present model predictions and confidence values in an operator-friendly dashboard.
- Provide a hardware-free simulator for repeatable demonstrations and development.

## Key Features

### IoT and sensing

- ESP8266/NodeMCU firmware built with PlatformIO and Arduino.
- Temperature sensing through a DS18B20-compatible digital sensor.
- Analog TDS and turbidity sensor support.
- ADS1115 external ADC support for higher-resolution analog acquisition.
- Optional 16x2 I2C LCD output for local status display.
- MQTT telemetry published to the `water/telemetry` topic.

### Machine learning

- **Sensor health classifier:** predicts `NORMAL`, `SPIKE`, `DRIFT`, `STUCK`, `DISCONNECT`, or `NOISE`.
- **pH estimator:** predicts an estimated pH value from current and short-term historical sensor features.
- **Water warning classifier:** predicts `NORMAL`, `EARLY_WARNING`, or `CRITICAL_WARNING`.
- Rolling-window features based on the current reading, recent deltas, means, and standard deviations.
- Saved PyTorch model weights and preprocessing scalers loaded from `ml/models`.

### Backend and storage

- FastAPI REST service.
- CORS enabled for local dashboard development.
- SQLite database for readings and predictions.
- Health, ingestion, history, and reset endpoints.
- Password-protected reset endpoint for the prototype dashboard.

### Frontend

- React single-page dashboard powered by Vite.
- Automatic polling of recent readings every second.
- Live/offline connection indicator.
- Temperature, TDS, turbidity, and estimated-pH KPI cards.
- Range bars with safe, warning, and danger zones.
- Sensor-health and water-status badges.
- Confidence values for model predictions.
- Recharts trend graph for recent sensor values.
- Recent-readings table.
- Dark/light theme toggle.
- Data reset action.

---

## System Architecture

```text
+-----------------------+
| Water-quality sensors |
| Temperature / TDS /  |
| Turbidity             |
+-----------+-----------+
            |
            v
+-----------------------+
| ESP8266 edge node     |
| ADS1115 + LCD + Wi-Fi |
+-----------+-----------+
            |
            | MQTT: water/telemetry
            v
+-----------------------+       HTTP POST /api/ingest
| Mosquitto MQTT broker | <---------------------------+
| Port 1884             |                             |
+-----------+-----------+                             |
            |                                         |
            v                                         |
+-----------------------+                             |
| mqtt_bridge.py       | ------------------------------+
+-----------------------+                             |
                                                      v
+-----------------------------------------------------+
| FastAPI backend                                     |
| - request validation                                |
| - feature construction                              |
| - sensor-health model                               |
| - pH regression model                               |
| - water-warning model                               |
| - SQLite persistence                                |
+----------------------+------------------------------+
                       |
                       | GET /api/readings
                       v
+-----------------------------------------------------+
| React/Vite dashboard                                |
| - live cards and alerts                             |
| - range analysis                                    |
| - trend chart                                       |
| - readings table                                    |
+-----------------------------------------------------+
```

## End-to-End Data Flow

1. The ESP8266 reads the physical sensors.
2. Sensor values are converted into a JSON telemetry payload.
3. The ESP8266 publishes the payload to the MQTT broker on `water/telemetry`.
4. `mqtt/mqtt_bridge.py` subscribes to that topic.
5. The bridge forwards each message to `POST /api/ingest`.
6. FastAPI validates the reading with a Pydantic model.
7. The ML engine creates a feature vector from the current reading and up to five recent readings.
8. The sensor-health model predicts a fault/health class.
9. The pH model estimates pH.
10. The water-warning model classifies the overall water condition.
11. The backend stores the raw values and model outputs in SQLite.
12. The React dashboard polls `GET /api/readings` and refreshes the interface.

The repository also supports a direct HTTP path. The simulator and `demo_post.py` can send readings straight to the backend without an ESP8266 or MQTT broker.

---

## Repository Structure

```text
.
├── backend/
│   ├── __init__.py
│   ├── app.py                 # FastAPI application and REST endpoints
│   ├── db.py                  # SQLite initialization and data access
│   ├── ml_engine.py           # Model loading, feature creation, inference
│   ├── requirements.txt       # Python dependencies
│   └── water_quality.db       # SQLite database created/used by the backend
├── data/
│   └── pune_drinking_water_source_synthetic_weather_conditioned.csv
├── esp8266/
│   ├── platformio.ini         # PlatformIO environment and libraries
│   ├── src/
│   │   └── main.cpp           # ESP8266 firmware
│   ├── read_serial.py         # Serial-monitor helper
│   └── compile_commands.json
├── frontend/
│   ├── index.html
│   ├── package.json
│   ├── package-lock.json
│   ├── vite.config.js
│   └── src/
│       ├── App.jsx            # Dashboard UI and API polling
│       ├── index.css           # Dashboard styling and themes
│       └── main.jsx            # React entry point
├── ml/
│   └── models/                # Saved weights and preprocessing scalers
├── mqtt/
│   ├── mosquitto.conf         # Local broker configuration
│   └── mqtt_bridge.py         # MQTT-to-FastAPI bridge
├── demo_post.py               # Sends a small sequence of test readings
├── simulate_esp8266.py        # Hardware-free telemetry simulator
├── project_presentation_guide.md
├── README.md
└── .gitignore
```

## Technology Stack

| Layer | Technologies |
|---|---|
| Firmware | C++, Arduino framework, ESP8266, PlatformIO |
| Sensors | DS18B20, TDS sensor, turbidity sensor, ADS1115 ADC |
| Messaging | MQTT, Eclipse Mosquitto, Paho MQTT |
| Backend | Python, FastAPI, Uvicorn, Pydantic |
| Machine learning | PyTorch, NumPy, pandas, scikit-learn, serialized scalers |
| Database | SQLite |
| Frontend | React, Vite, Recharts, CSS |
| Development utilities | Python requests, serial tooling, simulator scripts |

The repository language composition is approximately **C++ 27.6%**, **JavaScript 26.7%**, **Python 22.5%**, **CSS 21.8%**, and **HTML 1.4%**.

## Hardware

The intended edge node can include:

- ESP8266 NodeMCU / ESP-12E development board.
- DS18B20 waterproof digital temperature sensor.
- Analog TDS sensor.
- Analog turbidity sensor.
- ADS1115 16-bit I2C analog-to-digital converter.
- 16x2 I2C LCD.
- Appropriate power supply, wiring, breadboard, and waterproofing.

### Hardware safety

- Never immerse exposed electronics in water.
- Use appropriate isolation and waterproof connectors.
- Confirm sensor operating voltages before connecting them to the ESP8266 or ADS1115.
- Do not connect sensor outputs beyond the voltage range supported by the ADC.
- Use a common ground where required, while following the electrical specifications of every module.
- Calibrate sensors with appropriate reference solutions before interpreting measurements.

## Software Prerequisites

Install the following before running the project:

- Python 3.10 or newer recommended.
- Node.js and npm.
- Git.
- Eclipse Mosquitto if using the MQTT path.
- PlatformIO CLI or PlatformIO IDE if compiling firmware.
- A modern web browser.

The backend dependencies are listed in `backend/requirements.txt`:

- FastAPI
- Uvicorn
- Pydantic
- NumPy
- pandas
- scikit-learn
- PyTorch
- requests
- paho-mqtt

The frontend dependencies are declared in `frontend/package.json` and installed with `npm install`.

## Quick Start Without Hardware

This is the fastest way to launch the API and dashboard.

### 1. Clone the repository

```bash
git clone https://github.com/FineFur/Water-Quality-Early-Warning-System-Using-AIML-and-IOT.git
cd Water-Quality-Early-Warning-System-Using-AIML-and-IOT
```

### 2. Create and activate a Python virtual environment

Linux/macOS:

```bash
python3 -m venv .venv
source .venv/bin/activate
```

Windows PowerShell:

```powershell
py -m venv .venv
.venv\Scripts\Activate.ps1
```

### 3. Install backend dependencies

```bash
python -m pip install --upgrade pip
pip install -r backend/requirements.txt
```

### 4. Start the FastAPI backend

From the repository root:

```bash
uvicorn backend.app:app --reload --host 0.0.0.0 --port 8000
```

The backend is available at:

- API base: `http://127.0.0.1:8000`
- Interactive Swagger UI: `http://127.0.0.1:8000/docs`
- ReDoc: `http://127.0.0.1:8000/redoc`
- Health check: `http://127.0.0.1:8000/api/health`

### 5. Install and start the frontend

Open a second terminal:

```bash
cd frontend
npm install
npm run dev
```

Open the local URL printed by Vite, usually `http://127.0.0.1:5173`.

### 6. Generate readings

Open a third terminal from the repository root and run either:

```bash
python demo_post.py
```

or the continuous simulator:

```bash
python simulate_esp8266.py
```

The dashboard should begin showing readings and predictions within approximately one second of ingestion.

## Complete Local Setup

The complete MQTT-backed workflow uses four processes:

### Terminal 1: MQTT broker

Install Eclipse Mosquitto for your operating system, then run:

```bash
mosquitto -c mqtt/mosquitto.conf
```

The included configuration listens on port `1884` on all interfaces and allows anonymous connections for local prototyping.

### Terminal 2: FastAPI backend

```bash
uvicorn backend.app:app --reload --host 0.0.0.0 --port 8000
```

### Terminal 3: MQTT bridge

```bash
python mqtt/mqtt_bridge.py
```

The bridge connects to `127.0.0.1:1884`, subscribes to `water/telemetry`, and forwards messages to `http://127.0.0.1:8000/api/ingest`.

### Terminal 4: React dashboard

```bash
cd frontend
npm install
npm run dev
```

### Terminal 5: Simulator or ESP8266

For a hardware-free MQTT demonstration, use an MQTT publisher or the ESP8266 firmware. The supplied `simulate_esp8266.py` posts directly to the FastAPI endpoint and is therefore best used for the direct HTTP path unless adapted to publish MQTT messages.

## Running the ESP8266 Node

The firmware is configured as a PlatformIO project in `esp8266/platformio.ini`.

### PlatformIO CLI

```bash
cd esp8266
pio run
pio run --target upload
pio device monitor -b 115200
```

The configured environment is `nodemcuv2` with the Arduino framework and a serial monitor speed of `115200`.

### Declared firmware libraries

PlatformIO installs the libraries declared in `platformio.ini`:

- `knolleary/PubSubClient`
- `adafruit/Adafruit ADS1X15`
- `paulstoffregen/OneWire`
- `milesburton/DallasTemperature`
- `marcoschwartz/LiquidCrystal_I2C`

### Device configuration

Before uploading firmware, review `esp8266/src/main.cpp` and configure:

- Wi-Fi SSID.
- Wi-Fi password.
- MQTT broker hostname or LAN IP address.
- MQTT broker port, matching the Mosquitto listener.
- MQTT topic, matching `water/telemetry`.
- Sensor wiring and calibration constants.
- LCD I2C address if it differs from the configured address.

The ESP8266 must be able to reach the computer running Mosquitto over the local network. `127.0.0.1` on the ESP8266 refers to the device itself and must not be used as the broker address in a physical deployment.

## Running the Sensor Simulator

`simulate_esp8266.py` continuously generates readings with:

- Slow random drift in temperature, TDS, and turbidity.
- Diurnal-style variation in temperature.
- Occasional `spike`, `noise`, and `stuck` fault injection.
- Configurable backend URL.
- Configurable posting interval.
- Configurable water-body name.

Run it with defaults:

```bash
python simulate_esp8266.py
```

Run it with custom options:

```bash
python simulate_esp8266.py \
  --url http://127.0.0.1:8000 \
  --interval 2 \
  --water_body "Khadakwasla Reservoir/Dam"
```

The simulator posts to `/api/ingest` and prints the returned estimated pH, sensor status, and water status.

## Sending Test Readings

### Using `demo_post.py`

```bash
python demo_post.py
```

The script posts normal and abnormal examples, including high TDS and turbidity values.

### Using cURL

```bash
curl -X POST http://127.0.0.1:8000/api/ingest \
  -H "Content-Type: application/json" \
  -d '{
    "temperature": 25.4,
    "tds": 122,
    "turbidity": 4.3,
    "water_body": "Khadakwasla Reservoir/Dam"
  }'
```

### Example abnormal reading

```bash
curl -X POST http://127.0.0.1:8000/api/ingest \
  -H "Content-Type: application/json" \
  -d '{
    "temperature": 25.6,
    "tds": 850,
    "turbidity": 35.0,
    "water_body": "Khadakwasla Reservoir/Dam"
  }'
```

## Backend API

### `GET /api/health`

Returns a basic service-health response.

Example response:

```json
{
  "status": "ok"
}
```

### `POST /api/ingest`

Validates a new reading, runs inference, stores the result, and returns the combined record.

Required fields:

- `temperature`: numeric temperature in degrees Celsius.
- `tds`: numeric Total Dissolved Solids value in ppm.
- `turbidity`: numeric turbidity value in NTU.

Optional fields:

- `water_body`: string; defaults to `Khadakwasla Reservoir/Dam`.
- `timestamp`: ISO-8601 timestamp; generated by the backend when omitted.

Example response shape:

```json
{
  "temperature": 25.4,
  "tds": 122.0,
  "turbidity": 4.3,
  "water_body": "Khadakwasla Reservoir/Dam",
  "timestamp": "2026-10-07T12:00:00",
  "sensor_status": "NORMAL",
  "sensor_confidence": 0.91,
  "estimated_ph": 7.2,
  "water_status": "NORMAL",
  "water_confidence": 0.88
}
```

The exact prediction values depend on the loaded models, scaler files, current history, and input readings.

### `GET /api/readings`

Returns up to the 50 most recent database records, newest first.

The frontend uses this endpoint and displays the newest reading, a trend chart, and a table containing the most recent records.

### `POST /api/reset`

Clears the in-memory feature history and deletes persisted readings.

Prototype request:

```json
{
  "password": "Water"
}
```

> The reset password is currently hardcoded in the prototype. Do not use this endpoint as-is in a production deployment.

## Input and Output Schemas

### Input telemetry

```json
{
  "temperature": 25.4,
  "tds": 122,
  "turbidity": 4.3,
  "water_body": "Khadakwasla Reservoir/Dam",
  "timestamp": "2026-10-07T12:00:00"
}
```

### Model output fields

| Field | Description |
|---|---|
| `sensor_status` | Predicted sensor condition: `NORMAL`, `SPIKE`, `DRIFT`, `STUCK`, `DISCONNECT`, or `NOISE`. |
| `sensor_confidence` | Maximum softmax probability from the sensor-health classifier. |
| `estimated_ph` | pH estimated by the regression model. |
| `water_status` | Predicted water condition: `NORMAL`, `EARLY_WARNING`, or `CRITICAL_WARNING`. |
| `water_confidence` | Maximum softmax probability from the water-warning classifier. |

## Machine-Learning Pipeline

The inference implementation is in `backend/ml_engine.py`.

### Feature construction

For each incoming reading, the engine extracts:

- Current temperature, TDS, and turbidity.
- Difference between the current reading and the previous reading.
- Mean values over the available recent history.
- Standard deviation over the available recent history.

The resulting feature vector contains 12 values:

```text
[current temperature, current TDS, current turbidity,
 delta temperature, delta TDS, delta turbidity,
 mean temperature, mean TDS, mean turbidity,
 standard deviation temperature, standard deviation TDS,
 standard deviation turbidity]
```

The backend keeps a short in-memory history and uses up to the last five historical readings when calculating rolling statistics.

### Model 1: Sensor health classifier

- Architecture: multilayer perceptron classifier.
- Input size: 12 features.
- Output classes: six sensor-health categories.
- Purpose: identify unusual patterns in the sensor stream.

### Model 2: pH estimator

- Architecture: multilayer perceptron regressor.
- Input size: 12 features.
- Output: one continuous pH estimate.
- Purpose: provide a virtual/inferred pH signal when a dedicated pH probe is unavailable.

### Model 3: Water-warning classifier

- Architecture: multilayer perceptron classifier.
- Input size: 13 features: the 12 base features plus estimated pH.
- Output classes: `NORMAL`, `EARLY_WARNING`, and `CRITICAL_WARNING`.
- Purpose: combine sensor information and estimated pH into an overall warning state.

### Saved artifacts

The backend expects model artifacts in `ml/models`, including:

- `sensor_health_mlp.pt`
- `ph_estimation_mlp.pt`
- `water_warning_mlp.pt`
- `scalers.pkl`

The scaler file contains preprocessing objects used to transform model inputs and reverse-transform the pH prediction.

### Demonstration overrides

The current inference code contains explicit demonstration overrides for extreme values and known prototype behavior. For example, very high TDS or turbidity can force `CRITICAL_WARNING`. These rules are useful for predictable presentations, but they should be clearly separated from validated model logic in a production or research-grade system.

## MQTT Integration

### Broker configuration

`mqtt/mosquitto.conf` contains the local prototype configuration:

```conf
listener 1884 0.0.0.0
allow_anonymous true
```

### Topic

Telemetry is published on:

```text
water/telemetry
```

### Bridge behavior

`mqtt/mqtt_bridge.py`:

1. Connects to the Mosquitto broker at `127.0.0.1:1884`.
2. Subscribes to `water/telemetry`.
3. Decodes each MQTT payload as JSON.
4. Sends the JSON to `http://127.0.0.1:8000/api/ingest`.
5. Prints the API response.
6. Retries after connection failures.

### Production MQTT recommendations

Before exposing the broker beyond a trusted local machine:

- Disable anonymous access.
- Configure username/password authentication.
- Use TLS certificates.
- Restrict listener interfaces and firewall rules.
- Validate topic permissions.
- Add message authentication and replay protection where appropriate.
- Add a durable queue or retry strategy for offline backend periods.

## Frontend Dashboard

The dashboard is implemented in `frontend/src/App.jsx` and styled in `frontend/src/index.css`.

### Dashboard behavior

- Polls `GET /api/readings` every second.
- Shows live, offline, or connecting status.
- Displays the latest temperature, TDS, turbidity, and estimated pH.
- Visualizes configured ranges and status zones.
- Shows AI sensor-health and water-warning decisions.
- Displays sensor and water confidence percentages.
- Plots up to the latest 30 readings.
- Lists recent readings in a table.
- Supports dark/light themes using local storage.
- Provides a reset-data control.

### Frontend commands

From `frontend`:

```bash
npm install
npm run dev
npm run build
npm run preview
```

The Vite configuration proxies `/api/*` requests to the backend during local development. If the frontend and backend are hosted separately, configure an appropriate API base URL and production CORS policy.

## Database

The backend uses SQLite at:

```text
backend/water_quality.db
```

The `readings` table stores:

| Column | Description |
|---|---|
| `id` | Auto-incrementing record identifier. |
| `timestamp` | Reading timestamp. |
| `water_body` | Water-body/source name. |
| `temperature` | Temperature value. |
| `tds` | TDS value. |
| `turbidity` | Turbidity value. |
| `estimated_ph` | Model-estimated pH. |
| `sensor_status` | Sensor-health classification. |
| `sensor_confidence` | Sensor-health confidence. |
| `water_status` | Water-warning classification. |
| `water_confidence` | Water-warning confidence. |
| `raw_json` | Serialized combined record. |

The database is initialized automatically when the FastAPI application starts. To reset data, use the dashboard reset action or call the reset endpoint with the prototype password.

## Configuration

The prototype currently uses values embedded in source files. Review these locations before deployment:

| Setting | Location |
|---|---|
| Backend host/port | Uvicorn command |
| MQTT broker host/port | `mqtt/mqtt_bridge.py`, ESP8266 firmware |
| MQTT topic | `mqtt/mqtt_bridge.py`, ESP8266 firmware |
| Backend ingestion URL | `mqtt/mqtt_bridge.py`, simulator |
| Wi-Fi credentials | ESP8266 firmware |
| Model artifact directory | `backend/ml_engine.py` |
| Database path | `backend/db.py` |
| Reset password | `backend/app.py` |
| Frontend API proxy | `frontend/vite.config.js` |

For a production-ready implementation, move credentials, endpoints, ports, thresholds, and secrets into environment variables or a secure configuration system.

## Demonstration Workflow

A reliable classroom or project-panel demonstration can follow this sequence:

1. Start the backend and frontend.
2. Confirm `GET /api/health` returns `{"status":"ok"}`.
3. Start `demo_post.py` or `simulate_esp8266.py`.
4. Show normal temperature, TDS, turbidity, estimated pH, and `NORMAL` status.
5. Introduce a high-TDS/high-turbidity test reading and observe the warning state.
6. Run the simulator long enough to observe injected spike, noise, and stuck patterns.
7. Explain the rolling-window features and three-model inference sequence.
8. Use the dashboard trend chart to show how readings change over time.
9. Demonstrate the reset action using the prototype password `Water`.
10. Explain that real water-safety decisions require calibrated sensors, validated labels, field testing, and laboratory confirmation.

## Troubleshooting

### Backend will not start

- Confirm the virtual environment is active.
- Reinstall dependencies with `pip install -r backend/requirements.txt`.
- Run Uvicorn from the repository root so the `backend` package can be imported.
- Check that the required files exist in `ml/models`.
- Confirm the selected Python version is compatible with the installed PyTorch build.

### Dashboard shows `OFFLINE`

- Confirm the backend is running on port `8000`.
- Open `http://127.0.0.1:8000/api/health` directly.
- Confirm Vite is running from the `frontend` directory.
- Check the browser developer console for proxy or CORS errors.

### Dashboard is connected but contains no readings

- Run `python demo_post.py`.
- Run `python simulate_esp8266.py`.
- Send a manual request with cURL.
- Confirm the request uses the required `temperature`, `tds`, and `turbidity` fields.

### MQTT bridge cannot connect

- Confirm Mosquitto is installed and running.
- Confirm the broker is listening on port `1884`.
- Confirm the bridge uses the same host, port, and topic as the broker and firmware.
- On a physical ESP8266, replace `127.0.0.1` with the LAN IP address of the broker computer.
- Check firewall rules and local network connectivity.

### ESP8266 does not upload

- Confirm the correct USB driver and serial port.
- Check that the board is set to the `nodemcuv2` PlatformIO environment.
- Disconnect peripherals that interfere with bootstrapping pins.
- Review serial output at `115200` baud.

### Model loading fails

- Confirm all expected files are present under `ml/models`.
- Ensure model architecture and saved weights match.
- Ensure `scalers.pkl` contains the expected scaler keys: `sc1`, `sc2x`, `sc2y`, and `sc3`.
- Check file permissions and run the backend from the repository root.

### Predictions look unreliable

- Treat prototype predictions as demonstrations rather than validated measurements.
- Check sensor calibration and wiring.
- Inspect the scale and units of incoming readings.
- Verify that the deployed data distribution resembles the model-training data.
- Evaluate the models against independently labeled field data.

## Scientific and Operational Limitations

This project has important limitations:

1. **Synthetic or prototype targets:** The pH estimator uses a synthetic pH target, and the water-warning labels are prototype labels derived from parameter behavior rather than certified contamination measurements.
2. **No laboratory validation:** Predictions have not been established as a substitute for laboratory analysis.
3. **Sensor dependency:** Incorrect calibration, fouling, temperature effects, electrical noise, drift, or poor placement can invalidate readings.
4. **Limited feature set:** Temperature, TDS, and turbidity cannot fully characterize drinking-water safety.
5. **No chemical specificity:** The system cannot reliably identify particular pathogens, heavy metals, pesticides, or chemical contaminants from these readings alone.
6. **Model distribution shift:** Model performance can degrade when water bodies, seasons, sensor hardware, or environmental conditions differ from training data.
7. **In-memory history:** The rolling history used for inference is lost when the backend restarts.
8. **Prototype thresholds:** Dashboard ranges and demonstration overrides are not universal regulatory limits.
9. **Insecure local defaults:** Anonymous MQTT access, permissive CORS, and a hardcoded reset password are unsuitable for production.
10. **No high-availability guarantees:** The prototype does not provide redundant brokers, durable queues, authentication, authorization, observability, or guaranteed delivery.

Use this system as an educational platform and research starting point. Any real-world deployment should include sensor calibration, quality assurance, secure communications, validated datasets, independent testing, regulatory review, and qualified environmental-science oversight.

## Security Considerations

The local prototype intentionally uses simplified settings. Before deployment:

- Replace `allow_anonymous true` with authenticated MQTT access.
- Enable TLS for MQTT and HTTP traffic.
- Restrict CORS to trusted frontend origins.
- Replace the hardcoded reset password with secure authentication and authorization.
- Store secrets outside source control.
- Validate ranges, timestamps, payload sizes, and message rates.
- Add rate limiting and request logging.
- Avoid exposing SQLite or administrative endpoints publicly.
- Protect the device Wi-Fi credentials.
- Apply least-privilege firewall rules.
- Monitor failed connections and unusual telemetry patterns.
- Back up and protect stored readings.

## Future Improvements

Potential next steps include:

- Add pH, dissolved oxygen, conductivity, oxidation-reduction potential, and water-temperature calibration workflows.
- Replace synthetic labels with independently verified field and laboratory datasets.
- Add a reproducible training pipeline with experiment tracking and evaluation reports.
- Report precision, recall, F1 score, ROC-AUC, calibration, and regression error metrics.
- Version model artifacts and record model version with every prediction.
- Add database migrations and configurable retention policies.
- Persist feature history so inference can resume safely after restart.
- Add automated tests for the API, feature extraction, database layer, MQTT bridge, and frontend.
- Add Docker Compose for the API, broker, and dashboard.
- Add authentication, TLS, role-based access control, and secrets management.
- Add alert delivery through email, SMS, webhooks, or a notification service.
- Add edge buffering for intermittent connectivity.
- Add device registration, telemetry acknowledgements, and firmware update support.
- Improve sensor-fault classification using time-series models and labeled fault experiments.
- Add real-time WebSocket or server-sent-event updates instead of one-second polling.
- Add deployment observability with structured logs, metrics, and health checks.

## Contributing

Contributions are welcome. A useful contribution should:

1. Explain the motivation and scope of the change.
2. Keep hardware, backend, frontend, and ML changes separately understandable.
3. Include tests or reproducible verification steps where practical.
4. Avoid committing credentials, private data, or generated secrets.
5. Document new configuration values and API changes.
6. Clearly identify whether a change affects prototype behavior, scientific validity, or production security.

Suggested workflow:

```bash
git checkout -b feature/your-change
# make and test changes
git add .
git commit -m "Describe the change"
git push origin feature/your-change
```

Then open a pull request with a description of the implementation, test commands, screenshots where relevant, and any scientific or security implications.

## License

No license file is currently included in the repository. Until a license is added, all rights are reserved by the repository owner. If you intend to reuse, distribute, or modify this project, contact the owner or add an explicit open-source license.

## Acknowledgements

This project combines open-source technologies and common academic/engineering patterns for IoT telemetry, machine learning, web APIs, and dashboards. See the repository files and dependency manifests for the specific libraries used by each layer.
