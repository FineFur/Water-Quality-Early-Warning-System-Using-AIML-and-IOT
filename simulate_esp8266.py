"""
simulate_esp8266.py
-------------------
Simulates the ESP8266 sensor node by continuously posting realistic
water-quality readings to the FastAPI backend at /api/ingest.

Runs indefinitely (Ctrl-C to stop).  Mimics:
  - Slow random drift in temperature, TDS, turbidity
  - Occasional fault injection (spike, noise, stuck) so the AI
    models produce interesting outputs on the dashboard.

Usage:
    python simulate_esp8266.py [--url http://127.0.0.1:8000] [--interval 1]
"""

import argparse, math, random, time, requests, sys
from datetime import datetime

# Force UTF-8 output so special chars print cleanly on Windows
sys.stdout.reconfigure(encoding="utf-8", errors="replace")

# -- CLI -------------------------------------------------------------------
parser = argparse.ArgumentParser()
parser.add_argument("--url",       default="http://127.0.0.1:8000")
parser.add_argument("--interval",  default=1.0, type=float)
parser.add_argument("--water_body",default="Khadakwasla Reservoir/Dam")
args = parser.parse_args()

API      = args.url.rstrip("/") + "/api/ingest"
INTERVAL = args.interval

# -- Sensor state (slow random walk) ---------------------------------------
temp      = 25.0   # deg C
tds       = 120.0  # ppm
turbidity = 3.5    # NTU

# Fault injection
FAULT_PROBABILITY = 0.08
fault_remaining   = 0
fault_type        = None
FAULT_MODES       = ["spike", "noise", "stuck"]


def next_reading(tick: int) -> dict:
    global temp, tds, turbidity, fault_remaining, fault_type

    # Normal random walk
    temp      += random.gauss(0, 0.04)
    tds       += random.gauss(0, 0.8)
    turbidity += random.gauss(0, 0.12)

    # Diurnal sine variation
    t = temp + 0.4 * math.sin(tick / 60 * math.pi)
    d = max(80.0, tds)
    u = max(0.5,  turbidity)

    # Fault injection
    if fault_remaining <= 0:
        if random.random() < FAULT_PROBABILITY:
            fault_type      = random.choice(FAULT_MODES)
            fault_remaining = random.randint(3, 8)
            print(f"  [FAULT] Injecting {fault_type.upper()} for {fault_remaining} ticks")
    else:
        fault_remaining -= 1
        if fault_type == "spike":
            d += random.uniform(80, 180)
            u += random.uniform(20, 60)
        elif fault_type == "noise":
            t += random.gauss(0, 2.0)
            d += random.gauss(0, 30)
            u += random.gauss(0, 8)
        # "stuck" -> values unchanged (already captured above)

    return {
        "temperature": round(t, 2),
        "tds":         round(d, 1),
        "turbidity":   round(u, 2),
        "water_body":  args.water_body,
        "timestamp":   datetime.now().isoformat(),
    }


# -- Main loop -------------------------------------------------------------
print(f"[Simulator] Posting to {API} every {INTERVAL}s  |  Ctrl-C to stop\n")

tick = 0
while True:
    tick += 1
    payload = next_reading(tick)
    try:
        resp = requests.post(API, json=payload, timeout=3)
        resp.raise_for_status()
        result = resp.json()
        print(
            f"[{tick:>4}] {payload['timestamp'][11:19]}"
            f"  T={payload['temperature']:6.2f}C"
            f"  TDS={payload['tds']:7.1f}ppm"
            f"  Turb={payload['turbidity']:6.2f}NTU"
            f"  pH={result.get('estimated_ph','?')}"
            f"  Sensor={result.get('sensor_status','?'):<12}"
            f"  Water={result.get('water_status','?')}"
        )
    except requests.exceptions.ConnectionError:
        print(f"[{tick:>4}] CANNOT REACH backend at {API} -- is it running?")
    except Exception as e:
        print(f"[{tick:>4}] ERROR: {e}")

    time.sleep(INTERVAL)
