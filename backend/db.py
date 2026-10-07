
import sqlite3, json
from pathlib import Path
DB=Path(__file__).resolve().parent/"water_quality.db"
def init_db():
    con=sqlite3.connect(DB)
    con.execute("""CREATE TABLE IF NOT EXISTS readings(
      id INTEGER PRIMARY KEY AUTOINCREMENT, timestamp TEXT, water_body TEXT,
      temperature REAL, tds REAL, turbidity REAL, estimated_ph REAL,
      sensor_status TEXT, sensor_confidence REAL, water_status TEXT,
      water_confidence REAL, raw_json TEXT)""")
    con.commit(); con.close()
def insert_reading(r):
    con=sqlite3.connect(DB)
    con.execute("""INSERT INTO readings(timestamp,water_body,temperature,tds,turbidity,
      estimated_ph,sensor_status,sensor_confidence,water_status,water_confidence,raw_json)
      VALUES(?,?,?,?,?,?,?,?,?,?,?)""",
      (r.get("timestamp"),r.get("water_body","Khadakwasla Reservoir/Dam"),
       r["temperature"],r["tds"],r["turbidity"],r.get("estimated_ph"),
       r.get("sensor_status"),r.get("sensor_confidence"),r.get("water_status"),
       r.get("water_confidence"),json.dumps(r)))
    con.commit(); con.close()
def recent(limit=50):
    con=sqlite3.connect(DB); con.row_factory=sqlite3.Row
    rows=[dict(x) for x in con.execute("SELECT * FROM readings ORDER BY id DESC LIMIT ?",(limit,))]
    con.close(); return rows
def reset_db():
    con=sqlite3.connect(DB)
    con.execute("DELETE FROM readings")
    con.commit(); con.close()
