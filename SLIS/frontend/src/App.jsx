import { useState, useEffect } from "react";
import {
  Armchair,
  Activity,
  RefreshCw,
  Shield,
  GraduationCap,
  Zap,
  BarChart3,
  Database,
  Laptop,
  CheckCircle2,
  XCircle,
  Clock,
  RotateCcw,
  AlertTriangle
} from "lucide-react";
import "./App.css";

const API = "http://127.0.0.1:5001/api";

export default function App() {
  const [role, setRole] = useState("student"); // 'student' | 'librarian'
  const [data, setData] = useState(null);
  const [analytics, setAnalytics] = useState(null);
  const [error, setError] = useState("");
  const [modalSeat, setModalSeat] = useState(null);
  const [studentId, setStudentId] = useState("STU-2024-001");
  const [purpose, setPurpose] = useState("ECS Project Study");
  const [showHwConsole, setShowHwConsole] = useState(true);

  // Fetch seat status
  async function loadSeats() {
    try {
      const res = await fetch(`${API}/seats`);
      if (!res.ok) throw new Error("API request failed");
      const json = await res.json();
      setData(json);
      setError("");
    } catch {
      setError("Backend unavailable. Ensure Flask is running on http://127.0.0.1:5001.");
    }
  }

  // Fetch librarian analytics
  async function loadAnalytics() {
    try {
      const res = await fetch(`${API}/analytics`);
      if (res.ok) {
        const json = await res.json();
        setAnalytics(json);
      }
    } catch {
      // Handled by main error
    }
  }

  useEffect(() => {
    loadSeats();
    loadAnalytics();
    const interval = setInterval(() => {
      loadSeats();
      if (role === "librarian") loadAnalytics();
    }, 1000);
    return () => clearInterval(interval);
  }, [role]);

  // Handle student reservation
  async function handleReserve() {
    if (!modalSeat) return;
    try {
      const res = await fetch(`${API}/reserve`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          seat_id: modalSeat.id,
          student_id: studentId,
          purpose: purpose
        })
      });
      if (!res.ok) {
        const err = await res.json();
        alert(err.error || "Reservation failed");
      }
      setModalSeat(null);
      loadSeats();
    } catch (e) {
      alert("Error contacting backend: " + e.message);
    }
  }

  // Release reservation
  async function handleRelease(seatId) {
    try {
      await fetch(`${API}/release`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ seat_id: seatId })
      });
      loadSeats();
    } catch (e) {
      console.error(e);
    }
  }

  // Simulate hardware sensor toggle (Pins 22, 23, 32, 33, 25, 26, 27)
  async function toggleHardwareSensor(seat) {
    try {
      await fetch(`${API}/occupancy`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          seat_id: seat.id,
          occupied: !seat.occupied
        })
      });
      loadSeats();
    } catch (e) {
      console.error(e);
    }
  }

  // Toggle bag alert for librarian
  async function toggleBagAlert(seat) {
    try {
      await fetch(`${API}/bag-alert`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          seat_id: seat.id,
          alert: !seat.bag_alert
        })
      });
      loadSeats();
    } catch (e) {
      console.error(e);
    }
  }

  // Reset entire system for clean demo
  async function handleResetAll() {
    if (!confirm("Reset all 7 seats to Available in SQLite?")) return;
    try {
      await fetch(`${API}/reset`, { method: "POST" });
      loadSeats();
      loadAnalytics();
    } catch (e) {
      console.error(e);
    }
  }

  return (
    <main className="app">
      {/* Topbar */}
      <header className="topbar">
        <div className="brand">
          <div className="brand-icon">
            <Armchair size={24} />
          </div>
          <div>
            <h1>SLIS • Smart Library</h1>
            <p>Intelligence System & Space Automation</p>
          </div>
        </div>

        <div className="topbar-right">
          {/* Role Switcher */}
          <nav className="role-nav">
            <button
              className={`role-btn ${role === "student" ? "active" : ""}`}
              onClick={() => setRole("student")}
            >
              <GraduationCap size={15} /> Student View
            </button>
            <button
              className={`role-btn ${role === "librarian" ? "active" : ""}`}
              onClick={() => {
                setRole("librarian");
                loadAnalytics();
              }}
            >
              <Shield size={15} /> Librarian Portal
            </button>
          </nav>

          <div className="api-pill">
            <span className="live-dot" />
            <Database size={13} />
            <span>SQLite: <strong>slis.db</strong></span>
          </div>

          <button className="icon-btn" onClick={loadSeats} title="Refresh API">
            <RefreshCw size={14} />
          </button>
          <button className="icon-btn" onClick={handleResetAll} title="Reset All (Demo)">
            <RotateCcw size={14} />
          </button>
        </div>
      </header>

      {/* Error notification banner if Flask is off */}
      {error && (
        <div className="error-banner">
          <AlertTriangle size={18} />
          <div>
            <strong>Backend Connection Offline:</strong> {error}
          </div>
        </div>
      )}

      {/* Role-Specific Hero Header */}
      <section className="hero">
        <div>
          <p className="eyebrow">
            {role === "student"
              ? "STUDENT SEAT LOCATOR / LIVE CAMPUS RADAR"
              : "LIBRARY OPERATIONS / ADMINISTRATIVE TELEMETRY"}
          </p>
          <h2>
            {role === "student"
              ? "Find your next study desk in real time."
              : "Real-Time Seating & Eco-Automation Command Center"}
          </h2>
          <p>
            {role === "student"
              ? "Live occupancy sensed from ESP32 edge nodes and persisted in our SQLite records."
              : "Monitor seat utilization, enforce check-in policies, view audit trails, and manage zone lighting."}
          </p>
        </div>
      </section>

      {/* Key Metric KPI Cards */}
      <section className="stats-grid">
        <article className="stat-card">
          <div className="stat-icon avail">
            <CheckCircle2 size={24} />
          </div>
          <div className="stat-info">
            <span>Available Desks</span>
            <strong>{data?.available ?? "—"}</strong>
            <small>Ready for study</small>
          </div>
        </article>

        <article className="stat-card">
          <div className="stat-icon occ">
            <XCircle size={24} />
          </div>
          <div className="stat-info">
            <span>Occupied Desks</span>
            <strong>{data?.occupied ?? "—"}</strong>
            <small>Sensor confirmed</small>
          </div>
        </article>

        <article className="stat-card">
          <div className="stat-icon res">
            <Clock size={24} />
          </div>
          <div className="stat-info">
            <span>Reserved (On Hold)</span>
            <strong>{data?.reserved ?? "—"}</strong>
            <small>15-min grace window</small>
          </div>
        </article>

        <article className="stat-card">
          <div className="stat-icon eco">
            <Zap size={24} />
          </div>
          <div className="stat-info">
            <span>Eco Power Saved</span>
            <strong>{data?.power_saved_watts ?? 0} W</strong>
            <small>Utilization: {data?.utilization ?? 0}%</small>
          </div>
        </article>
      </section>

      {/* STUDENT VIEW: Interactive 2D Seat Map */}
      {role === "student" && (
        <section>
          <div className="floor-zones">
            {/* Zone A */}
            <div className="zone-box">
              <div className="zone-header">
                <h3>📖 Zone A: Silent Study Pod</h3>
                <span
                  className={`zone-badge ${
                    data?.zone1?.eco_mode ? "eco" : "active"
                  }`}
                >
                  {data?.zone1?.eco_mode
                    ? "ECO-SAVER ACTIVE"
                    : `${data?.zone1?.active_seats}/4 OCCUPIED (TASK MODE)`}
                </span>
              </div>
              <div className="seats-grid">
                {(data?.seats?.slice(0, 4) ?? []).map((seat) => (
                  <div
                    key={seat.id}
                    className={`seat-card ${seat.status.toLowerCase()}`}
                    onClick={() => {
                      if (seat.status === "AVAILABLE") {
                        setModalSeat(seat);
                      }
                    }}
                  >
                    <div className="seat-header">
                      <span>Desk 0{seat.id}</span>
                      <span className="pin-tag">P{seat.gpio}</span>
                    </div>
                    <div className="seat-icon">
                      <Armchair size={32} />
                    </div>
                    <div className="seat-title">Seat {seat.id}</div>
                    <div className="status-pill">{seat.status}</div>

                    {seat.status === "RESERVED" && (
                      <div className="countdown-tag">
                        ⏳ {Math.floor((seat.reserve_remaining || 0) / 60)}:
                        {String((seat.reserve_remaining || 0) % 60).padStart(2, "0")} left
                      </div>
                    )}
                    {seat.bag_alert && (
                      <div className="countdown-tag" style={{ background: "#7c2d12", color: "#fdba74" }}>
                        ⚠️ Bag Alert
                      </div>
                    )}
                  </div>
                ))}
              </div>
            </div>

            {/* Zone B */}
            <div className="zone-box">
              <div className="zone-header">
                <h3>🔬 Zone B: Research & Tech Bay</h3>
                <span
                  className={`zone-badge ${
                    data?.zone2?.eco_mode ? "eco" : "active"
                  }`}
                >
                  {data?.zone2?.eco_mode
                    ? "ECO-SAVER ACTIVE"
                    : `${data?.zone2?.active_seats}/3 OCCUPIED (TASK MODE)`}
                </span>
              </div>
              <div className="seats-grid">
                {(data?.seats?.slice(4, 7) ?? []).map((seat) => (
                  <div
                    key={seat.id}
                    className={`seat-card ${seat.status.toLowerCase()}`}
                    onClick={() => {
                      if (seat.status === "AVAILABLE") {
                        setModalSeat(seat);
                      }
                    }}
                  >
                    <div className="seat-header">
                      <span>Desk 0{seat.id}</span>
                      <span className="pin-tag">P{seat.gpio}</span>
                    </div>
                    <div className="seat-icon">
                      <Laptop size={32} />
                    </div>
                    <div className="seat-title">Seat {seat.id}</div>
                    <div className="status-pill">{seat.status}</div>

                    {seat.status === "RESERVED" && (
                      <div className="countdown-tag">
                        ⏳ {Math.floor((seat.reserve_remaining || 0) / 60)}:
                        {String((seat.reserve_remaining || 0) % 60).padStart(2, "0")} left
                      </div>
                    )}
                    {seat.bag_alert && (
                      <div className="countdown-tag" style={{ background: "#7c2d12", color: "#fdba74" }}>
                        ⚠️ Bag Alert
                      </div>
                    )}
                  </div>
                ))}
              </div>
            </div>
          </div>
        </section>
      )}

      {/* LIBRARIAN VIEW: Management & Space Intelligence */}
      {role === "librarian" && (
        <section>
          <div className="admin-grid">
            {/* Real-Time Seat Control Table */}
            <div className="admin-card">
              <h3>
                <Activity size={18} /> Active Seat Telemetry & Staff Overrides
              </h3>
              <table className="data-table">
                <thead>
                  <tr>
                    <th>Desk</th>
                    <th>GPIO</th>
                    <th>Status</th>
                    <th>Hold / User</th>
                    <th>Energy Mode</th>
                    <th>Actions</th>
                  </tr>
                </thead>
                <tbody>
                  {(data?.seats ?? []).map((s) => (
                    <tr key={s.id}>
                      <td>
                        <strong>Desk 0{s.id}</strong>
                      </td>
                      <td>
                        <code>GPIO {s.gpio}</code>
                      </td>
                      <td>
                        <span
                          className="status-pill"
                          style={{
                            background:
                              s.status === "AVAILABLE"
                                ? "rgba(16, 185, 129, 0.2)"
                                : s.status === "OCCUPIED"
                                ? "rgba(239, 68, 68, 0.2)"
                                : "rgba(245, 158, 11, 0.2)",
                            color:
                              s.status === "AVAILABLE"
                                ? "#34d399"
                                : s.status === "OCCUPIED"
                                ? "#f87171"
                                : "#fbbf24"
                          }}
                        >
                          {s.status}
                        </span>
                      </td>
                      <td>
                        {s.reserved ? (
                          <span style={{ color: "#fbbf24" }}>
                            {s.reserved_by} (⏳ {Math.floor((s.reserve_remaining || 0) / 60)}m)
                          </span>
                        ) : (
                          <span style={{ color: "var(--text-muted)" }}>—</span>
                        )}
                      </td>
                      <td>
                        {s.zone === 1
                          ? data?.zone1?.eco_mode ? "Eco (Dimmed)" : "Task (100%)"
                          : data?.zone2?.eco_mode ? "Eco (Dimmed)" : "Task (100%)"}
                      </td>
                      <td>
                        <div style={{ display: "flex", gap: "6px" }}>
                          {s.reserved && (
                            <button
                              className="btn btn-danger"
                              style={{ padding: "4px 8px", fontSize: "11px" }}
                              onClick={() => handleRelease(s.id)}
                            >
                              Cancel Hold
                            </button>
                          )}
                          <button
                            className="btn btn-secondary"
                            style={{ padding: "4px 8px", fontSize: "11px" }}
                            onClick={() => toggleBagAlert(s)}
                          >
                            {s.bag_alert ? "Clear Bag" : "Flag Bag"}
                          </button>
                        </div>
                      </td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>

            {/* Historical Analytics & Peak Hours */}
            <div className="admin-card">
              <h3>
                <BarChart3 size={18} /> Space Intelligence & Trends
              </h3>
              <div style={{ display: "flex", flexDirection: "column", gap: "14px" }}>
                <div style={{ background: "rgba(255,255,255,0.03)", padding: "12px", borderRadius: "8px" }}>
                  <div style={{ fontSize: "11px", color: "var(--text-muted)" }}>Peak Congestion Window</div>
                  <div style={{ fontSize: "15px", fontWeight: "700", color: "#fff", marginTop: "4px" }}>
                    {analytics?.peak_window || "1:30 PM – 4:45 PM"}
                  </div>
                </div>

                <div style={{ background: "rgba(255,255,255,0.03)", padding: "12px", borderRadius: "8px" }}>
                  <div style={{ fontSize: "11px", color: "var(--text-muted)" }}>Total State Changes Recorded</div>
                  <div style={{ fontSize: "20px", fontWeight: "800", color: "var(--primary)", marginTop: "4px" }}>
                    {analytics?.recorded_state_changes ?? 0} Events
                  </div>
                  <small style={{ color: "var(--text-muted)" }}>Logged in SQLite occupancy_logs</small>
                </div>

                {/* SVG Mini Hourly Trend */}
                <div>
                  <div style={{ fontSize: "12px", color: "var(--text-muted)", marginBottom: "8px" }}>
                    Hourly Utilization Curve
                  </div>
                  <svg viewBox="0 0 300 80" style={{ width: "100%", height: "80px" }}>
                    <path
                      d="M 10 70 Q 50 60 100 35 T 200 15 T 290 50 L 290 75 L 10 75 Z"
                      fill="rgba(56, 189, 248, 0.15)"
                    />
                    <path
                      d="M 10 70 Q 50 60 100 35 T 200 15 T 290 50"
                      fill="none"
                      stroke="#38bdf8"
                      strokeWidth="2.5"
                    />
                    <circle cx="200" cy="15" r="4" fill="#f43f5e" />
                    <text x="180" y="32" fill="#fff" fontSize="9" fontWeight="bold">Peak 14:00</text>
                  </svg>
                </div>
              </div>
            </div>
          </div>

          {/* Live SQLite Audit Log Table */}
          <div className="admin-card" style={{ marginBottom: "28px" }}>
            <h3>
              <Database size={18} /> SQLite Database Audit Trail (slis.db)
            </h3>
            <table className="data-table">
              <thead>
                <tr>
                  <th>Log ID</th>
                  <th>Seat #</th>
                  <th>Action Event</th>
                  <th>Occupied</th>
                  <th>UTC Timestamp</th>
                </tr>
              </thead>
              <tbody>
                {(analytics?.recent_logs ?? []).map((log) => (
                  <tr key={log.id}>
                    <td>#{log.id}</td>
                    <td>Desk 0{log.seat_id}</td>
                    <td>
                      <code style={{ color: "#38bdf8" }}>{log.action}</code>
                    </td>
                    <td>
                      {log.occupied ? (
                        <span style={{ color: "#f87171" }}>TRUE</span>
                      ) : (
                        <span style={{ color: "#34d399" }}>FALSE</span>
                      )}
                    </td>
                    <td style={{ color: "var(--text-muted)" }}>{log.timestamp}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </section>
      )}

      {/* Floating / Collapsible Hardware Console & 16x2 LCD Mirror */}
      <section className="hardware-bar">
        <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center" }}>
          <h4 style={{ fontSize: "14px", color: "var(--text-muted)", textTransform: "uppercase", letterSpacing: "1px" }}>
            Edge Node Hardware Integration & 16×2 LCD Mirror
          </h4>
          <button
            className="icon-btn"
            style={{ fontSize: "11px", padding: "4px 8px" }}
            onClick={() => setShowHwConsole(!showHwConsole)}
          >
            {showHwConsole ? "Collapse" : "Expand"}
          </button>
        </div>

        {showHwConsole && (
          <div style={{ display: "grid", gridTemplateColumns: "1fr 1.2fr", gap: "20px", marginTop: "14px" }}>
            {/* Virtual 16x2 LCD */}
            <div>
              <div style={{ fontSize: "11px", color: "var(--text-muted)" }}>
                Physical LCD Screen Preview (Pins: 15, 2, 4, 16, 17, 5)
              </div>
              <div className="lcd-box">
                <div className="lcd-line">{data?.lcd?.line1 || "1:A 2:A 3:A 4:A"}</div>
                <div className="lcd-line">{data?.lcd?.line2 || "5:A 6:A 7:A    "}</div>
              </div>
              <small style={{ color: "var(--text-muted)", fontSize: "10px", marginTop: "4px", display: "block" }}>
                A = Available • O = Occupied (Sensor Grounded) • R = Reserved (15-min hold)
              </small>
            </div>

            {/* Interactive Sensor Switch Triggers */}
            <div>
              <div style={{ fontSize: "11px", color: "var(--text-muted)" }}>
                Click to simulate physical sensor trigger (Pin LOW / HIGH)
              </div>
              <div className="switches-grid">
                {(data?.seats ?? []).map((s) => (
                  <button
                    key={s.id}
                    className={`switch-btn ${s.occupied ? "active" : ""}`}
                    onClick={() => toggleHardwareSensor(s)}
                  >
                    <strong>Seat {s.id}</strong>
                    <span style={{ fontSize: "9px" }}>
                      P{s.gpio}: {s.occupied ? "LOW (OCC)" : "HIGH (FREE)"}
                    </span>
                  </button>
                ))}
              </div>
            </div>
          </div>
        )}
      </section>

      {/* Reservation Modal */}
      {modalSeat && (
        <div className="modal-overlay">
          <div className="modal">
            <h3>Reserve Desk 0{modalSeat.id}</h3>
            <p>
              Place a temporary 15-minute hold on this desk. If physical presence is not sensed within 15 minutes,
              the seat automatically returns to the public pool.
            </p>

            <div className="form-group">
              <label>Student Roll / ID Number</label>
              <input
                type="text"
                value={studentId}
                onChange={(e) => setStudentId(e.target.value)}
                placeholder="e.g. STU-2024-8841"
              />
            </div>

            <div className="form-group">
              <label>Study Session Purpose</label>
              <input
                type="text"
                value={purpose}
                onChange={(e) => setPurpose(e.target.value)}
                placeholder="e.g. ECS Viva Study"
              />
            </div>

            <div className="modal-actions">
              <button className="btn btn-secondary" onClick={() => setModalSeat(null)}>
                Cancel
              </button>
              <button className="btn btn-primary" onClick={handleReserve}>
                Confirm 15-Min Reservation
              </button>
            </div>
          </div>
        </div>
      )}
    </main>
  );
}
