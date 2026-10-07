
"""
Basic MQTT bridge:
ESP8266 -> MQTT broker -> this bridge -> FastAPI /api/ingest
Install: pip install paho-mqtt requests
"""
import json, requests
import paho.mqtt.client as mqtt

BROKER="127.0.0.1"  # Connect to local mosquitto service
PORT=1884
TOPIC="water/telemetry"
API="http://127.0.0.1:8000/api/ingest"

def on_connect(client, userdata, flags, reason_code, properties=None):
    print(f"MQTT connected with result code {reason_code}", flush=True)
    client.subscribe(TOPIC)

def on_message(client, userdata, msg):
    try:
        payload = json.loads(msg.payload.decode())
        r = requests.post(API, json=payload, timeout=3)
        print(f"MQTT -> API: {r.status_code} {r.json()}", flush=True)
    except Exception as e:
        print(f"Bridge error: {e}", flush=True)

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.on_connect = on_connect
client.on_message = on_message

while True:
    try:
        print(f"Connecting to MQTT Broker at {BROKER}:{PORT}...", flush=True)
        client.connect(BROKER, PORT, 60)
        client.loop_forever()
    except Exception as e:
        print(f"Connection lost: {e} — retrying in 5s", flush=True)
        import time; time.sleep(5)
