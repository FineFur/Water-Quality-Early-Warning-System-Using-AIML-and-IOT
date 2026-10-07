
from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel
from datetime import datetime
from backend.db import init_db, insert_reading, recent, reset_db
from backend.ml_engine import infer

app=FastAPI(title="AI Water Quality Early Warning API")
app.add_middleware(CORSMiddleware,allow_origins=["*"],allow_methods=["*"],allow_headers=["*"])
init_db()
history=[]

class ResetRequest(BaseModel):
    password: str

class Reading(BaseModel):
    temperature: float
    tds: float
    turbidity: float
    water_body: str="Khadakwasla Reservoir/Dam"
    timestamp: str|None=None

@app.get("/api/health")
def health(): return {"status":"ok"}

@app.get("/api/readings")
def readings(): return recent(50)

@app.post("/api/ingest")
def ingest(r:Reading):
    current=r.model_dump()
    current["timestamp"]=current["timestamp"] or datetime.now().isoformat()
    pred=infer(current,history)
    out={**current,**pred}
    history.append(current)
    del history[:-10]
    insert_reading(out)
    return out

@app.post("/api/reset")
def reset(req: ResetRequest):
    if req.password != "Water":
        from fastapi import HTTPException
        raise HTTPException(status_code=401, detail="Incorrect password")
    global history
    history.clear()
    reset_db()
    return {"status": "ok", "message": "Readings reset successfully"}
