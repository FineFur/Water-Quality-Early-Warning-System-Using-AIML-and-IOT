
import requests, time
samples=[
 {"temperature":25.4,"tds":122,"turbidity":4.3},
 {"temperature":25.5,"tds":124,"turbidity":5.0},
 {"temperature":25.6,"tds":180,"turbidity":35.0},
 {"temperature":25.6,"tds":185,"turbidity":42.0},
]
for s in samples:
    print(requests.post("http://127.0.0.1:8000/api/ingest",json=s).json())
    time.sleep(1)
