import React, { useEffect, useState, useCallback } from "react";
import {
  LineChart, Line, XAxis, YAxis, Tooltip,
  ResponsiveContainer, CartesianGrid, Legend,
} from "recharts";

const API = ""; // Vite proxy forwards /api/* → http://127.0.0.1:8000/api/*

/* ─── helpers ─────────────────────────────────────────────── */
function Badge({ children, type = "" }) {
  return <span className={"badge " + type}>{children}</span>;
}

function Card({ title, value, unit, sub }) {
  return (
    <div className="card">
      <span className="card-label">{title}</span>
      <strong className="card-value">{value}</strong>
      <div className="card-footer">
        <small className="card-unit">{unit}</small>
        {sub && <small className="card-sub">{sub}</small>}
      </div>
    </div>
  );
}

function RangeBar({ label, value, unit, min, max, zones = [] }) {
  const hasRange = min !== undefined && max !== undefined && !isNaN(value);
  const numericVal = Number(value);
  const pct = hasRange ? Math.min(Math.max(((numericVal - min) / (max - min)) * 100, 0), 100) : 0;
  
  // Determine current color based on value falling into a zone
  let currentColor = 'var(--accent)';
  if (hasRange) {
    for (const z of zones) {
      if (numericVal >= z.min && numericVal <= z.max) {
        if (z.type === 'safe') currentColor = '#10b981';
        if (z.type === 'warn') currentColor = '#f59e0b';
        if (z.type === 'danger') currentColor = '#ef4444';
      }
    }
  }

  return (
    <div className="range-bar-row">
      <div className="range-info">
        <span className="range-title">{label}</span>
        <span className="range-val">{value} {unit}</span>
      </div>
      {hasRange && (
        <div className="range-container" title={`Range: ${min}-${max}`}>
          <div className="range-track-wrapper">
            <div className="range-track">
               {zones.map((z, i) => (
                 <div key={i} className={`range-zone zone-${z.type}`} style={{
                   left: `${((z.min - min) / (max - min)) * 100}%`,
                   width: `${((z.max - z.min) / (max - min)) * 100}%`
                 }} />
               ))}
            </div>
            <div 
              className="range-thumb" 
              style={{ left: `${pct}%`, backgroundColor: currentColor }} 
            />
          </div>
          <div className="range-labels">
            <span>{min}</span>
            <span>{max}</span>
          </div>
        </div>
      )}
    </div>
  );
}

const STATUS_COLOR = {
  NORMAL: "good",
  EARLY_WARNING: "warn",
  CRITICAL_WARNING: "danger",
  SENSOR_WARNING: "warn",
  SPIKE: "warn",
  DRIFT: "warn",
  STUCK: "danger",
  DISCONNECT: "danger",
  NOISE: "warn",
};

/* ─── main component ──────────────────────────────────────── */
export default function App() {
  const [rows, setRows] = useState([]);
  const [connected, setConnected] = useState(null); // null=unknown, true, false
  const [lastUpdated, setLastUpdated] = useState(null);
  const [theme, setTheme] = useState(
    () => localStorage.getItem("wq-theme") || "dark"
  );

  /* theme */
  useEffect(() => {
    document.documentElement.dataset.theme = theme;
    localStorage.setItem("wq-theme", theme);
  }, [theme]);

  /* polling */
  const load = useCallback(async () => {
    try {
      const r = await fetch(API + "/api/readings");
      if (!r.ok) throw new Error("non-2xx");
      const data = await r.json();
      setRows(data);
      setConnected(true);
      setLastUpdated(new Date());
    } catch {
      setConnected(false);
    }
  }, []);

  useEffect(() => {
    load();
    const t = setInterval(load, 1000); // refresh every second
    return () => clearInterval(t);
  }, [load]);

  const latest = rows[0];
  const chartData = [...rows].reverse().slice(-30); // oldest → newest, max 30 pts

  const handleReset = async () => {
    const pwd = window.prompt("Enter password to reset data:");
    if (pwd) {
      try {
        const res = await fetch(API + "/api/reset", {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({ password: pwd })
        });
        if (res.ok) {
          alert("Data reset successfully!");
          load();
        } else {
          alert("Incorrect password or error resetting data.");
        }
      } catch (e) {
        alert("Failed to reset data: " + e.message);
      }
    }
  };

  /* ─── render ─── */
  return (
    <div className="app">
      {/* HEADER */}
      <header>
        <div className="header-left">
          <div className="eyebrow">EDGE AI · IoT · EARLY WARNING</div>
          <h1>Water Quality Management System</h1>
          <p className="subtitle">
            Three physical sensors → AI validation → pH estimation → water-quality alert
          </p>
        </div>

        <div className="header-actions">
          <button
            id="theme-toggle-btn"
            className="theme-toggle"
            onClick={() => setTheme(t => (t === "dark" ? "light" : "dark"))}
            aria-label={`Switch to ${theme === "dark" ? "light" : "dark"} mode`}
          >
            <span className="theme-icon">{theme === "dark" ? "☀" : "☾"}</span>
            {theme === "dark" ? "Light" : "Dark"}
          </button>
          
          <button className="theme-toggle" onClick={handleReset} style={{ marginLeft: "8px", marginRight: "8px" }}>
            ⚠ Reset Data
          </button>

          <Badge type={connected === true ? "live" : connected === false ? "danger" : ""}>
            {connected === true ? "● LIVE" : connected === false ? "✕ OFFLINE" : "… CONNECTING"}
          </Badge>
        </div>
      </header>

      {/* LAST UPDATED */}
      {lastUpdated && (
        <div className="last-updated">
          Last updated: {lastUpdated.toLocaleTimeString()} · {rows.length} readings
        </div>
      )}

      {/* NO DATA BANNER */}
      {connected === false && (
        <div className="alert-banner danger">
          ⚠ Cannot reach backend at {API} — make sure the FastAPI server is running on port 8000.
        </div>
      )}
      {connected === true && rows.length === 0 && (
        <div className="alert-banner warn">
          ℹ Backend connected but no readings yet. Post data to <code>POST /api/ingest</code> or run <code>demo_post.py</code>.
        </div>
      )}

      {/* KPI CARDS */}
      <section className="cards" aria-label="Sensor readings">
        <Card
          title="Temperature"
          value={latest?.temperature?.toFixed(2) ?? "—"}
          unit="°C"
        />
        <Card
          title="TDS"
          value={latest?.tds?.toFixed(1) ?? "—"}
          unit="ppm"
        />
        <Card
          title="Turbidity"
          value={latest?.turbidity?.toFixed(2) ?? "—"}
          unit="NTU"
        />
        <Card
          title="Estimated pH"
          value={latest?.estimated_ph?.toFixed(2) ?? "—"}
          unit="ML-inferred"
        />
      </section>

      {/* RANGE ANALYSIS */}
      <section className="panel range-panel" aria-label="Range Analysis">
        <h2>Range Analysis</h2>
        <div className="range-grid">
          <RangeBar
            label="Temperature"
            value={latest?.temperature?.toFixed(2) ?? "—"}
            unit="°C"
            min={0} max={50}
            zones={[
              { min: 0, max: 20, type: 'warn' },
              { min: 20, max: 35, type: 'safe' },
              { min: 35, max: 45, type: 'warn' },
              { min: 45, max: 50, type: 'danger' }
            ]}
          />
          <RangeBar
            label="TDS"
            value={latest?.tds?.toFixed(1) ?? "—"}
            unit="ppm"
            min={0} max={1000}
            zones={[
              { min: 0, max: 300, type: 'safe' },
              { min: 300, max: 600, type: 'warn' },
              { min: 600, max: 1000, type: 'danger' }
            ]}
          />
          <RangeBar
            label="Turbidity"
            value={latest?.turbidity?.toFixed(2) ?? "—"}
            unit="NTU"
            min={0} max={15}
            zones={[
              { min: 0, max: 5, type: 'safe' },
              { min: 5, max: 10, type: 'warn' },
              { min: 10, max: 15, type: 'danger' }
            ]}
          />
          <RangeBar
            label="Estimated pH"
            value={latest?.estimated_ph?.toFixed(2) ?? "—"}
            unit="pH"
            min={0} max={14}
            zones={[
              { min: 0, max: 6.5, type: 'danger' },
              { min: 6.5, max: 8.5, type: 'safe' },
              { min: 8.5, max: 14, type: 'danger' }
            ]}
          />
        </div>
      </section>

      {/* MAIN GRID */}
      <section className="grid">
        {/* AI decision panel */}
        <div className="panel" aria-label="AI Decision">
          <h2>AI Decision</h2>

          <div className="decision">
            <span>Sensor health</span>
            <Badge type={STATUS_COLOR[latest?.sensor_status] ?? ""}>
              {latest?.sensor_status ?? "WAITING"}
            </Badge>
          </div>

          <div className="decision">
            <span>Water status</span>
            <Badge type={STATUS_COLOR[latest?.water_status] ?? ""}>
              {latest?.water_status ?? "WAITING"}
            </Badge>
          </div>

          <div className="decision">
            <span>Sensor confidence</span>
            <b>{latest ? Math.round(latest.sensor_confidence * 100) + "%" : "—"}</b>
          </div>

          <div className="decision">
            <span>Water confidence</span>
            <b>{latest ? Math.round(latest.water_confidence * 100) + "%" : "—"}</b>
          </div>

          <div className="decision">
            <span>Water body</span>
            <b style={{ fontSize: 12, textAlign: "right", maxWidth: 180 }}>
              {latest?.water_body ?? "—"}
            </b>
          </div>
        </div>

        {/* Chart */}
        <div className="panel chart-panel" aria-label="Sensor trend chart">
          <h2>Sensor Trend (last {chartData.length} readings)</h2>
          {chartData.length === 0 ? (
            <div className="chart-empty">No data yet — post readings to see the trend</div>
          ) : (
            <ResponsiveContainer width="100%" height={260}>
              <LineChart data={chartData} margin={{ top: 5, right: 20, left: 0, bottom: 5 }}>
                <CartesianGrid strokeDasharray="3 3" stroke="var(--border)" />
                <XAxis dataKey="id" tick={{ fontSize: 11, fill: "var(--muted)" }} />
                <YAxis tick={{ fontSize: 11, fill: "var(--muted)" }} />
                <Tooltip
                  contentStyle={{
                    background: "var(--surface)",
                    border: "1px solid var(--border)",
                    borderRadius: 10,
                    color: "var(--text)",
                  }}
                />
                <Legend wrapperStyle={{ fontSize: 12, color: "var(--muted)" }} />
                <Line
                  type="monotone"
                  dataKey="turbidity"
                  stroke="#7dd3fc"
                  strokeWidth={2}
                  dot={false}
                  activeDot={{ r: 4 }}
                />
                <Line
                  type="monotone"
                  dataKey="tds"
                  stroke="#86efac"
                  strokeWidth={2}
                  dot={false}
                  activeDot={{ r: 4 }}
                />
                <Line
                  type="monotone"
                  dataKey="temperature"
                  stroke="#fcd34d"
                  strokeWidth={2}
                  dot={false}
                  activeDot={{ r: 4 }}
                />
              </LineChart>
            </ResponsiveContainer>
          )}
        </div>
      </section>

      {/* SYSTEM FLOW */}
      <section className="panel flow-panel" aria-label="System data flow">
        <h2>System Pipeline</h2>
        <div className="flow">
          {["Sensors", "ESP8266", "Pre-process", "Sensor ML", "pH ML", "Water ML", "MQTT / API"].map(
            (step, i, arr) => (
              <React.Fragment key={step}>
                <span className="flow-step">{step}</span>
                {i < arr.length - 1 && <i className="flow-arrow">→</i>}
              </React.Fragment>
            )
          )}
        </div>
      </section>

      {/* READINGS TABLE */}
      {rows.length > 0 && (
        <section className="panel table-panel" aria-label="Recent readings table">
          <h2>Recent Readings</h2>
          <div className="table-scroll">
            <table>
              <thead>
                <tr>
                  <th>#</th>
                  <th>Timestamp</th>
                  <th>Temp (°C)</th>
                  <th>TDS (ppm)</th>
                  <th>Turb. (NTU)</th>
                  <th>pH</th>
                  <th>Sensor</th>
                  <th>Water</th>
                </tr>
              </thead>
              <tbody>
                {rows.slice(0, 10).map(r => (
                  <tr key={r.id}>
                    <td>{r.id}</td>
                    <td className="ts">{r.timestamp ? new Date(r.timestamp).toLocaleTimeString() : "—"}</td>
                    <td>{r.temperature?.toFixed(1)}</td>
                    <td>{r.tds?.toFixed(0)}</td>
                    <td>{r.turbidity?.toFixed(2)}</td>
                    <td>{r.estimated_ph?.toFixed(2)}</td>
                    <td>
                      <Badge type={STATUS_COLOR[r.sensor_status] ?? ""}>{r.sensor_status}</Badge>
                    </td>
                    <td>
                      <Badge type={STATUS_COLOR[r.water_status] ?? ""}>{r.water_status}</Badge>
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </section>
      )}
    </div>
  );
}
