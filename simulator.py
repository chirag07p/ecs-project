"""
Smart Library Intelligence System (SLIS) - ESP32 Full Framework Simulator
Simulates the complete 5-layer SLIS architecture:
- Layer 1 (Perception): 7 Seat GPIO Sensors (Pins 22, 23, 32, 33, 25, 26, 27) with bag-detection flags
- Layer 2 (Edge): ESP32 state machine, 16x2 LCD, internal pull-ups, zone-based energy automation
- Layer 3 (Communication): SoftAP + HTTP REST Endpoints (/data, /api/reserve, /api/release, /api/toggle)
- Layer 4 (Analytics): Real-time occupancy curve, space utilization rate, peak hour forecasting
- Layer 5 (Application): Modern glassmorphism dark-mode floor plan, reservation engine & eco-manager
"""

import http.server
import socketserver
import json
import urllib.parse
import threading
import time
from datetime import datetime

PORT = 5000

# Seat definitions and state matching the ESP32 hardware
SEAT_CONFIG = [
    {"id": 1, "gpio": 22, "zone": 1, "zoneName": "Zone A: Silent Study Pod"},
    {"id": 2, "gpio": 23, "zone": 1, "zoneName": "Zone A: Silent Study Pod"},
    {"id": 3, "gpio": 32, "zone": 1, "zoneName": "Zone A: Silent Study Pod"},
    {"id": 4, "gpio": 33, "zone": 1, "zoneName": "Zone A: Silent Study Pod"},
    {"id": 5, "gpio": 25, "zone": 2, "zoneName": "Zone B: Research & Tech Bay"},
    {"id": 6, "gpio": 26, "zone": 2, "zoneName": "Zone B: Research & Tech Bay"},
    {"id": 7, "gpio": 27, "zone": 2, "zoneName": "Zone B: Research & Tech Bay"},
]

# Runtime state
seats_state = {
    s["id"]: {
        "id": s["id"],
        "gpio": s["gpio"],
        "zone": s["zone"],
        "zoneName": s["zoneName"],
        "hardwareTriggered": False,   # Pin LOW = True
        "reserved": False,
        "reservedBy": "",
        "reserveExpiry": 0,           # timestamp
        "bagAlert": False,
    }
    for s in SEAT_CONFIG
}

serial_logs = []

def log_serial(msg):
    timestamp = datetime.now().strftime("%H:%M:%S")
    entry = f"[{timestamp}] {msg}"
    serial_logs.append(entry)
    if len(serial_logs) > 60:
        serial_logs.pop(0)
    print(entry)

def get_seat_status(s):
    now = time.time()
    # If reservation expired, clear it
    if s["reserved"] and now > s["reserveExpiry"] and not s["hardwareTriggered"]:
        s["reserved"] = False
        s["reservedBy"] = ""
        log_serial(f"Reservation EXPIRED for Seat {s['id']} (Auto-released)")

    if s["hardwareTriggered"]:
        return "OCCUPIED"
    elif s["reserved"]:
        return "RESERVED"
    else:
        return "AVAILABLE"

def get_stats():
    now = time.time()
    occ = 0
    res = 0
    avail = 0
    
    for s in seats_state.values():
        st = get_seat_status(s)
        if st == "OCCUPIED":
            occ += 1
        elif st == "RESERVED":
            res += 1
        else:
            avail += 1
            
    utilization = round(((occ + res) / 7.0) * 100, 1)
    
    # Zone 1 (Seats 1-4)
    z1_active = sum(1 for sid in [1, 2, 3, 4] if get_seat_status(seats_state[sid]) != "AVAILABLE")
    z1_eco = (z1_active == 0)
    
    # Zone 2 (Seats 5-7)
    z2_active = sum(1 for sid in [5, 6, 7] if get_seat_status(seats_state[sid]) != "AVAILABLE")
    z2_eco = (z2_active == 0)
    
    power_saved_w = (180 if z1_eco else 0) + (150 if z2_eco else 0)
    
    return {
        "total": 7,
        "occupied": occ,
        "available": avail,
        "reserved": res,
        "utilization": utilization,
        "powerSavedWatts": power_saved_w,
        "zone1": {
            "name": "Zone A: Silent Study Pod",
            "activeSeats": z1_active,
            "totalSeats": 4,
            "ecoMode": z1_eco,
            "lighting": "30% (Ambient Eco)" if z1_eco else "100% (Full Task)",
            "hvac": "STANDBY" if z1_eco else "ACTIVE (22°C)"
        },
        "zone2": {
            "name": "Zone B: Research & Tech Bay",
            "activeSeats": z2_active,
            "totalSeats": 3,
            "ecoMode": z2_eco,
            "lighting": "25% (Ambient Eco)" if z2_eco else "100% (Full Task)",
            "hvac": "STANDBY" if z2_eco else "ACTIVE (22°C)"
        }
    }

def get_lcd_lines():
    def char_for(sid):
        st = get_seat_status(seats_state[sid])
        if st == "OCCUPIED":
            return "O"
        elif st == "RESERVED":
            return "R"
        else:
            return "A"
            
    line1 = f"1:{char_for(1)} 2:{char_for(2)} 3:{char_for(3)} 4:{char_for(4)}"
    line2 = f"5:{char_for(5)} 6:{char_for(6)} 7:{char_for(7)}"
    return line1.ljust(16)[:16], line2.ljust(16)[:16]

def log_hardware_status():
    s_str = " ".join([f"S{i}:{1 if seats_state[i]['hardwareTriggered'] else 0}" for i in range(1, 8)])
    st = get_stats()
    log_serial(f"{s_str} | Avail:{st['available']} Occ:{st['occupied']} Res:{st['reserved']}")

# Complete Modern Glassmorphic SLIS Single-Page Dashboard
HTML_PAGE = """<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>SLIS • Smart Library Intelligence System</title>
<style>
:root {
  --bg-main: #0b0f19;
  --bg-card: rgba(18, 24, 38, 0.85);
  --bg-card-hover: rgba(28, 38, 58, 0.95);
  --border-color: rgba(255, 255, 255, 0.08);
  --border-highlight: rgba(56, 189, 248, 0.3);
  --primary: #38bdf8;
  --primary-glow: rgba(56, 189, 248, 0.2);
  --accent: #818cf8;
  --success: #10b981;
  --success-glow: rgba(16, 185, 129, 0.25);
  --danger: #ef4444;
  --danger-glow: rgba(239, 68, 68, 0.25);
  --warning: #f59e0b;
  --warning-glow: rgba(245, 158, 11, 0.25);
  --text-main: #f1f5f9;
  --text-muted: #94a3b8;
  --radius-lg: 16px;
  --radius-md: 12px;
  --radius-sm: 8px;
}

* {
  box-sizing: border-box;
  margin: 0;
  padding: 0;
  font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
  -webkit-font-smoothing: antialiased;
}

body {
  background-color: var(--bg-main);
  background-image: 
    radial-gradient(at 0% 0%, rgba(56, 189, 248, 0.08) 0px, transparent 50%),
    radial-gradient(at 100% 100%, rgba(129, 140, 248, 0.08) 0px, transparent 50%);
  background-attachment: fixed;
  color: var(--text-main);
  padding: 16px;
  min-height: 100vh;
}

.container {
  max-width: 1200px;
  margin: 0 auto;
}

/* Header & Top Bar */
.header-bar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-wrap: wrap;
  gap: 16px;
  padding: 18px 24px;
  background: var(--bg-card);
  backdrop-filter: blur(16px);
  border: 1px solid var(--border-color);
  border-radius: var(--radius-lg);
  margin-bottom: 20px;
  box-shadow: 0 10px 30px rgba(0,0,0,0.3);
}

.brand-wrapper {
  display: flex;
  align-items: center;
  gap: 14px;
}

.brand-icon {
  width: 44px;
  height: 44px;
  background: linear-gradient(135deg, #0284c7, #6366f1);
  border-radius: 12px;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 22px;
  box-shadow: 0 0 18px rgba(2, 132, 199, 0.4);
}

.brand-title h1 {
  font-size: 20px;
  font-weight: 700;
  letter-spacing: -0.5px;
  background: linear-gradient(90deg, #fff, #93c5fd);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
}

.brand-title p {
  font-size: 12px;
  color: var(--text-muted);
  display: flex;
  align-items: center;
  gap: 6px;
}

.live-pill {
  display: inline-block;
  width: 8px;
  height: 8px;
  background: var(--success);
  border-radius: 50%;
  box-shadow: 0 0 8px var(--success);
  animation: pulse-dot 2s infinite;
}

@keyframes pulse-dot {
  0% { transform: scale(0.95); opacity: 0.8; }
  50% { transform: scale(1.3); opacity: 1; }
  100% { transform: scale(0.95); opacity: 0.8; }
}

.header-badges {
  display: flex;
  gap: 10px;
  align-items: center;
  flex-wrap: wrap;
}

.badge {
  background: rgba(255, 255, 255, 0.05);
  border: 1px solid var(--border-color);
  padding: 6px 12px;
  border-radius: 20px;
  font-size: 12px;
  color: var(--text-muted);
  display: flex;
  align-items: center;
  gap: 6px;
}

.badge strong {
  color: var(--primary);
}

/* Nav Tabs */
.nav-tabs {
  display: flex;
  gap: 8px;
  margin-bottom: 20px;
  overflow-x: auto;
  padding-bottom: 4px;
}

.tab-btn {
  background: rgba(255,255,255,0.03);
  border: 1px solid var(--border-color);
  color: var(--text-muted);
  padding: 10px 18px;
  border-radius: var(--radius-md);
  font-size: 13px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.2s ease;
  white-space: nowrap;
  display: flex;
  align-items: center;
  gap: 8px;
}

.tab-btn:hover {
  background: rgba(255,255,255,0.08);
  color: #fff;
}

.tab-btn.active {
  background: linear-gradient(135deg, rgba(56, 189, 248, 0.15), rgba(99, 102, 241, 0.15));
  border-color: var(--primary);
  color: #fff;
  box-shadow: 0 0 16px rgba(56, 189, 248, 0.15);
}

/* KPI Cards Grid */
.kpi-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
  gap: 14px;
  margin-bottom: 24px;
}

.kpi-card {
  background: var(--bg-card);
  backdrop-filter: blur(12px);
  border: 1px solid var(--border-color);
  border-radius: var(--radius-md);
  padding: 18px;
  display: flex;
  align-items: center;
  gap: 16px;
  transition: transform 0.2s, border-color 0.2s;
}

.kpi-card:hover {
  transform: translateY(-2px);
  border-color: var(--border-highlight);
}

.kpi-icon {
  width: 48px;
  height: 48px;
  border-radius: 12px;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 22px;
}

.kpi-icon.avail { background: rgba(16, 185, 129, 0.15); color: var(--success); border: 1px solid var(--success-glow); }
.kpi-icon.occ { background: rgba(239, 68, 68, 0.15); color: var(--danger); border: 1px solid var(--danger-glow); }
.kpi-icon.res { background: rgba(245, 158, 11, 0.15); color: var(--warning); border: 1px solid var(--warning-glow); }
.kpi-icon.eco { background: rgba(56, 189, 248, 0.15); color: var(--primary); border: 1px solid var(--primary-glow); }

.kpi-info h3 {
  font-size: 11px;
  color: var(--text-muted);
  text-transform: uppercase;
  letter-spacing: 0.6px;
  margin-bottom: 4px;
}

.kpi-info .kpi-num {
  font-size: 28px;
  font-weight: 800;
  line-height: 1.1;
  color: #fff;
}

.kpi-info p {
  font-size: 11px;
  color: var(--text-muted);
  margin-top: 4px;
}

/* Floor Plan Layout */
.section-title {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 14px;
}

.section-title h2 {
  font-size: 16px;
  font-weight: 700;
  color: #fff;
  display: flex;
  align-items: center;
  gap: 8px;
}

.floorplan-zones {
  display: grid;
  grid-template-columns: 1.2fr 1fr;
  gap: 18px;
  margin-bottom: 24px;
}

@media(max-width: 900px) {
  .floorplan-zones {
    grid-template-columns: 1fr;
  }
}

.zone-container {
  background: var(--bg-card);
  backdrop-filter: blur(12px);
  border: 1px solid var(--border-color);
  border-radius: var(--radius-lg);
  padding: 20px;
}

.zone-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 16px;
  padding-bottom: 12px;
  border-bottom: 1px solid var(--border-color);
}

.zone-header h3 {
  font-size: 15px;
  color: var(--primary);
  display: flex;
  align-items: center;
  gap: 8px;
}

.zone-eco-badge {
  font-size: 11px;
  padding: 4px 10px;
  border-radius: 12px;
  font-weight: 600;
}

.eco-on {
  background: rgba(16, 185, 129, 0.15);
  color: var(--success);
  border: 1px solid rgba(16, 185, 129, 0.3);
}

.eco-off {
  background: rgba(56, 189, 248, 0.15);
  color: var(--primary);
  border: 1px solid rgba(56, 189, 248, 0.3);
}

/* Desks Grid */
.desks-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(130px, 1fr));
  gap: 14px;
}

.desk-card {
  background: rgba(255, 255, 255, 0.02);
  border: 2px solid var(--border-color);
  border-radius: var(--radius-md);
  padding: 14px 10px;
  text-align: center;
  cursor: pointer;
  transition: all 0.25s cubic-bezier(0.4, 0, 0.2, 1);
  position: relative;
  overflow: hidden;
}

.desk-card:hover {
  transform: translateY(-4px);
  box-shadow: 0 10px 20px rgba(0,0,0,0.3);
}

.desk-card.available {
  border-color: rgba(16, 185, 129, 0.4);
  background: rgba(16, 185, 129, 0.04);
}
.desk-card.available:hover {
  border-color: var(--success);
  box-shadow: 0 0 20px var(--success-glow);
}

.desk-card.occupied {
  border-color: rgba(239, 68, 68, 0.5);
  background: rgba(239, 68, 68, 0.08);
}
.desk-card.occupied:hover {
  border-color: var(--danger);
  box-shadow: 0 0 20px var(--danger-glow);
}

.desk-card.reserved {
  border-color: rgba(245, 158, 11, 0.5);
  background: rgba(245, 158, 11, 0.08);
  animation: pulse-border 2s infinite;
}

@keyframes pulse-border {
  0% { box-shadow: 0 0 0 rgba(245, 158, 11, 0); }
  50% { box-shadow: 0 0 14px var(--warning-glow); }
  100% { box-shadow: 0 0 0 rgba(245, 158, 11, 0); }
}

.desk-top-bar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  font-size: 11px;
  color: var(--text-muted);
  margin-bottom: 8px;
}

.pin-pill {
  background: rgba(255,255,255,0.06);
  padding: 2px 6px;
  border-radius: 6px;
  font-size: 10px;
  font-family: monospace;
}

.desk-icon {
  font-size: 32px;
  margin: 6px 0;
  transition: transform 0.2s;
}

.desk-card:hover .desk-icon {
  transform: scale(1.1);
}

.desk-name {
  font-size: 14px;
  font-weight: 700;
  color: #fff;
  margin-bottom: 6px;
}

.status-badge {
  display: inline-block;
  padding: 4px 8px;
  border-radius: 8px;
  font-size: 10px;
  font-weight: 700;
  letter-spacing: 0.5px;
  text-transform: uppercase;
}

.desk-card.available .status-badge {
  background: rgba(16, 185, 129, 0.2);
  color: #34d399;
}
.desk-card.occupied .status-badge {
  background: rgba(239, 68, 68, 0.2);
  color: #f87171;
}
.desk-card.reserved .status-badge {
  background: rgba(245, 158, 11, 0.2);
  color: #fbbf24;
}

.bag-alert-tag {
  margin-top: 6px;
  font-size: 9px;
  background: rgba(249, 115, 22, 0.25);
  color: #fb923c;
  padding: 2px 4px;
  border-radius: 4px;
  font-weight: 600;
}

/* Hardware Simulation Console (Bottom Dual Panels) */
.hardware-strip {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 18px;
  margin-bottom: 24px;
}

@media(max-width: 800px) {
  .hardware-strip {
    grid-template-columns: 1fr;
  }
}

.hw-box {
  background: var(--bg-card);
  backdrop-filter: blur(12px);
  border: 1px solid var(--border-color);
  border-radius: var(--radius-lg);
  padding: 18px;
}

.hw-box h3 {
  font-size: 13px;
  color: var(--text-muted);
  text-transform: uppercase;
  letter-spacing: 0.8px;
  margin-bottom: 12px;
  display: flex;
  justify-content: space-between;
  align-items: center;
}

/* 16x2 LCD Simulation */
.lcd-display {
  background: #608820;
  color: #0b1a03;
  font-family: 'Courier New', Courier, monospace;
  font-size: 20px;
  font-weight: 900;
  letter-spacing: 3px;
  padding: 14px 18px;
  border-radius: 8px;
  border: 4px solid #364d12;
  box-shadow: inset 0 0 12px rgba(0,0,0,0.6);
  text-shadow: 0 1px 1px rgba(255,255,255,0.2);
  user-select: none;
}

.lcd-row {
  height: 26px;
  line-height: 26px;
  white-space: pre;
}

/* Sensor Clickers */
.sensor-switches {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(90px, 1fr));
  gap: 8px;
  margin-top: 8px;
}

.hw-switch-btn {
  background: rgba(255,255,255,0.04);
  border: 1px solid var(--border-color);
  border-radius: 8px;
  color: var(--text-main);
  padding: 8px 4px;
  font-size: 12px;
  cursor: pointer;
  transition: all 0.2s;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 2px;
}

.hw-switch-btn:hover {
  border-color: var(--primary);
  background: rgba(56, 189, 248, 0.1);
}

.hw-switch-btn.active {
  background: rgba(239, 68, 68, 0.2);
  border-color: var(--danger);
  color: #fca5a5;
  box-shadow: 0 0 10px rgba(239, 68, 68, 0.3);
}

.hw-switch-btn span.pin-id {
  font-size: 9px;
  color: var(--text-muted);
}

/* Analytics Tab Content */
.analytics-grid {
  display: grid;
  grid-template-columns: 2fr 1fr;
  gap: 18px;
  margin-bottom: 24px;
}

@media(max-width: 850px) {
  .analytics-grid {
    grid-template-columns: 1fr;
  }
}

.chart-card {
  background: var(--bg-card);
  backdrop-filter: blur(12px);
  border: 1px solid var(--border-color);
  border-radius: var(--radius-lg);
  padding: 20px;
}

.chart-card h3 {
  font-size: 15px;
  color: #fff;
  margin-bottom: 14px;
}

.svg-chart {
  width: 100%;
  height: 180px;
}

/* Modal Popup */
.modal-overlay {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: rgba(0,0,0,0.7);
  backdrop-filter: blur(6px);
  display: none;
  align-items: center;
  justify-content: center;
  z-index: 1000;
  padding: 16px;
}

.modal-content {
  background: #111827;
  border: 1px solid var(--border-highlight);
  border-radius: var(--radius-lg);
  padding: 24px;
  max-width: 440px;
  width: 100%;
  box-shadow: 0 20px 40px rgba(0,0,0,0.5);
  animation: modal-enter 0.25s ease-out;
}

@keyframes modal-enter {
  from { transform: scale(0.9); opacity: 0; }
  to { transform: scale(1); opacity: 1; }
}

.modal-content h3 {
  font-size: 18px;
  color: #fff;
  margin-bottom: 8px;
}

.modal-content p {
  font-size: 13px;
  color: var(--text-muted);
  margin-bottom: 18px;
  line-height: 1.5;
}

.form-group {
  margin-bottom: 16px;
  text-align: left;
}

.form-group label {
  display: block;
  font-size: 12px;
  color: var(--text-muted);
  margin-bottom: 6px;
  font-weight: 600;
}

.form-group input {
  width: 100%;
  padding: 10px 14px;
  background: rgba(255,255,255,0.05);
  border: 1px solid var(--border-color);
  border-radius: var(--radius-sm);
  color: #fff;
  font-size: 14px;
  outline: none;
}

.form-group input:focus {
  border-color: var(--primary);
  box-shadow: 0 0 10px rgba(56, 189, 248, 0.2);
}

.modal-actions {
  display: flex;
  gap: 10px;
  justify-content: flex-end;
  margin-top: 20px;
}

.btn {
  padding: 9px 16px;
  border-radius: var(--radius-sm);
  font-size: 13px;
  font-weight: 600;
  cursor: pointer;
  border: none;
  transition: all 0.2s;
}

.btn-primary {
  background: linear-gradient(135deg, #0284c7, #4f46e5);
  color: #fff;
}
.btn-primary:hover {
  opacity: 0.9;
  transform: translateY(-1px);
}

.btn-secondary {
  background: rgba(255,255,255,0.1);
  color: var(--text-muted);
}
.btn-secondary:hover {
  background: rgba(255,255,255,0.15);
  color: #fff;
}

.btn-danger {
  background: rgba(239, 68, 68, 0.2);
  color: #f87171;
  border: 1px solid rgba(239, 68, 68, 0.4);
}
.btn-danger:hover {
  background: var(--danger);
  color: #fff;
}

/* Serial Log Box */
.serial-feed {
  background: #030712;
  border: 1px solid var(--border-color);
  border-radius: var(--radius-md);
  padding: 12px;
  height: 120px;
  overflow-y: auto;
  font-family: 'Courier New', Courier, monospace;
  font-size: 11px;
  color: #4ade80;
  white-space: pre-wrap;
}

.tab-pane {
  display: none;
}
.tab-pane.active {
  display: block;
}
</style>
</head>

<body>
<div class="container">

  <!-- Header -->
  <header class="header-bar">
    <div class="brand-wrapper">
      <div class="brand-icon">📚</div>
      <div class="brand-title">
        <h1>SLIS • Smart Library Intelligence System</h1>
        <p><span class="live-pill"></span> Edge Node 01 • ESP32 Physical Sensing Active</p>
      </div>
    </div>
    
    <div class="header-badges">
      <div class="badge">🌐 Wi-Fi: <strong>Smart_Library (AP)</strong></div>
      <div class="badge">📡 IP: <strong>192.168.4.1</strong></div>
      <div class="badge" id="clockDisplay">⏰ 12:00:00</div>
    </div>
  </header>

  <!-- Navigation Tabs -->
  <nav class="nav-tabs">
    <button class="tab-btn active" onclick="switchTab('floorplan')">🗺️ Interactive Floor Map</button>
    <button class="tab-btn" onclick="switchTab('analytics')">📊 Real-Time Analytics & Trends</button>
    <button class="tab-btn" onclick="switchTab('energy')">⚡ Smart Energy & Eco-Automation</button>
    <button class="tab-btn" onclick="switchTab('telemetry')">📟 Hardware & LCD Mirror</button>
  </nav>

  <!-- KPI Row -->
  <div class="kpi-grid">
    <div class="kpi-card">
      <div class="kpi-icon avail">🟢</div>
      <div class="kpi-info">
        <h3>Available Desks</h3>
        <div class="kpi-num" id="kpi-avail">7</div>
        <p>Ready for study</p>
      </div>
    </div>

    <div class="kpi-card">
      <div class="kpi-icon occ">🔴</div>
      <div class="kpi-info">
        <h3>Occupied Desks</h3>
        <div class="kpi-num" id="kpi-occ">0</div>
        <p>Sensor detected</p>
      </div>
    </div>

    <div class="kpi-card">
      <div class="kpi-icon res">⏳</div>
      <div class="kpi-info">
        <h3>Reserved (On Hold)</h3>
        <div class="kpi-num" id="kpi-res">0</div>
        <p>15-min grace window</p>
      </div>
    </div>

    <div class="kpi-card">
      <div class="kpi-icon eco">⚡</div>
      <div class="kpi-info">
        <h3>Eco Power Saved</h3>
        <div class="kpi-num" id="kpi-eco">330 W</div>
        <p id="kpi-util">Utilization: 0%</p>
      </div>
    </div>
  </div>

  <!-- TAB 1: Interactive Floor Map -->
  <div id="tab-floorplan" class="tab-pane active">
    <div class="section-title">
      <h2>📍 Live Floor Plan • Level 2 East Wing</h2>
      <span style="font-size:12px; color:var(--text-muted);">Click any available desk to reserve, or occupied desk for details</span>
    </div>

    <div class="floorplan-zones">
      <!-- Zone 1 -->
      <div class="zone-container">
        <div class="zone-header">
          <h3>📖 Zone A: Silent Study Pod</h3>
          <span class="zone-eco-badge eco-on" id="zone1-badge">ECO-SAVER ACTIVE</span>
        </div>
        <div class="desks-grid">
          <div class="desk-card available" id="desk-card-1" onclick="handleDeskClick(1)">
            <div class="desk-top-bar">
              <span>Desk 01</span>
              <span class="pin-pill">P22</span>
            </div>
            <div class="desk-icon">🪑</div>
            <div class="desk-name">Seat 1</div>
            <div class="status-badge" id="desk-status-1">AVAILABLE</div>
            <div id="desk-tag-1"></div>
          </div>

          <div class="desk-card available" id="desk-card-2" onclick="handleDeskClick(2)">
            <div class="desk-top-bar">
              <span>Desk 02</span>
              <span class="pin-pill">P23</span>
            </div>
            <div class="desk-icon">🪑</div>
            <div class="desk-name">Seat 2</div>
            <div class="status-badge" id="desk-status-2">AVAILABLE</div>
            <div id="desk-tag-2"></div>
          </div>

          <div class="desk-card available" id="desk-card-3" onclick="handleDeskClick(3)">
            <div class="desk-top-bar">
              <span>Desk 03</span>
              <span class="pin-pill">P32</span>
            </div>
            <div class="desk-icon">🪑</div>
            <div class="desk-name">Seat 3</div>
            <div class="status-badge" id="desk-status-3">AVAILABLE</div>
            <div id="desk-tag-3"></div>
          </div>

          <div class="desk-card available" id="desk-card-4" onclick="handleDeskClick(4)">
            <div class="desk-top-bar">
              <span>Desk 04</span>
              <span class="pin-pill">P33</span>
            </div>
            <div class="desk-icon">🪑</div>
            <div class="desk-name">Seat 4</div>
            <div class="status-badge" id="desk-status-4">AVAILABLE</div>
            <div id="desk-tag-4"></div>
          </div>
        </div>
      </div>

      <!-- Zone 2 -->
      <div class="zone-container">
        <div class="zone-header">
          <h3>🔬 Zone B: Research & Tech Bay</h3>
          <span class="zone-eco-badge eco-on" id="zone2-badge">ECO-SAVER ACTIVE</span>
        </div>
        <div class="desks-grid">
          <div class="desk-card available" id="desk-card-5" onclick="handleDeskClick(5)">
            <div class="desk-top-bar">
              <span>Desk 05</span>
              <span class="pin-pill">P25</span>
            </div>
            <div class="desk-icon">💻</div>
            <div class="desk-name">Seat 5</div>
            <div class="status-badge" id="desk-status-5">AVAILABLE</div>
            <div id="desk-tag-5"></div>
          </div>

          <div class="desk-card available" id="desk-card-6" onclick="handleDeskClick(6)">
            <div class="desk-top-bar">
              <span>Desk 06</span>
              <span class="pin-pill">P26</span>
            </div>
            <div class="desk-icon">💻</div>
            <div class="desk-name">Seat 6</div>
            <div class="status-badge" id="desk-status-6">AVAILABLE</div>
            <div id="desk-tag-6"></div>
          </div>

          <div class="desk-card available" id="desk-card-7" onclick="handleDeskClick(7)">
            <div class="desk-top-bar">
              <span>Desk 07</span>
              <span class="pin-pill">P27</span>
            </div>
            <div class="desk-icon">💻</div>
            <div class="desk-name">Seat 7</div>
            <div class="status-badge" id="desk-status-7">AVAILABLE</div>
            <div id="desk-tag-7"></div>
          </div>
        </div>
      </div>
    </div>
  </div>

  <!-- TAB 2: Analytics & Trends -->
  <div id="tab-analytics" class="tab-pane">
    <div class="analytics-grid">
      <div class="chart-card">
        <h3>📈 Daily Occupancy Curve (Historical vs Today)</h3>
        <svg class="svg-chart" viewBox="0 0 500 160">
          <defs>
            <linearGradient id="chartGrad" x1="0" y1="0" x2="0" y2="1">
              <stop offset="0%" stop-color="#38bdf8" stop-opacity="0.5"/>
              <stop offset="100%" stop-color="#38bdf8" stop-opacity="0.0"/>
            </linearGradient>
          </defs>
          <!-- Grid lines -->
          <line x1="40" y1="20" x2="480" y2="20" stroke="rgba(255,255,255,0.05)" />
          <line x1="40" y1="60" x2="480" y2="60" stroke="rgba(255,255,255,0.05)" />
          <line x1="40" y1="100" x2="480" y2="100" stroke="rgba(255,255,255,0.05)" />
          <line x1="40" y1="140" x2="480" y2="140" stroke="rgba(255,255,255,0.1)" />
          <!-- Y Labels -->
          <text x="15" y="24" fill="#64748b" font-size="10">100%</text>
          <text x="20" y="64" fill="#64748b" font-size="10">60%</text>
          <text x="20" y="104" fill="#64748b" font-size="10">30%</text>
          <text x="25" y="144" fill="#64748b" font-size="10">0%</text>
          <!-- Area Path -->
          <path d="M 40 140 Q 90 120 140 70 T 240 30 T 340 50 T 440 90 L 480 120 L 480 140 Z" fill="url(#chartGrad)" />
          <!-- Line Path -->
          <path d="M 40 140 Q 90 120 140 70 T 240 30 T 340 50 T 440 90 L 480 120" fill="none" stroke="#38bdf8" stroke-width="3" />
          <!-- Current Live Point Marker -->
          <circle id="chartLivePoint" cx="240" cy="30" r="5" fill="#f43f5e" stroke="#fff" stroke-width="2">
            <animate attributeName="r" values="4;7;4" dur="2s" repeatCount="indefinite"/>
          </circle>
          <!-- X Axis Labels -->
          <text x="40" y="155" fill="#64748b" font-size="9">08:00</text>
          <text x="140" y="155" fill="#64748b" font-size="9">11:00</text>
          <text x="240" y="155" fill="#38bdf8" font-weight="bold" font-size="9">14:00 (Peak)</text>
          <text x="340" y="155" fill="#64748b" font-size="9">17:00</text>
          <text x="440" y="155" fill="#64748b" font-size="9">20:00</text>
        </svg>
      </div>

      <div class="chart-card">
        <h3>📊 Space Intelligence Metrics</h3>
        <div style="margin-top:14px; display:flex; flex-direction:column; gap:12px;">
          <div>
            <div style="display:flex; justify-content:space-between; font-size:12px; margin-bottom:4px;">
              <span style="color:var(--text-muted);">Current Utilization Rate</span>
              <strong id="analytics-util-pct" style="color:var(--primary);">0%</strong>
            </div>
            <div style="height:6px; background:rgba(255,255,255,0.08); border-radius:3px; overflow:hidden;">
              <div id="analytics-util-bar" style="width:0%; height:100%; background:linear-gradient(90deg, #38bdf8, #818cf8); transition:width 0.5s;"></div>
            </div>
          </div>
          <div style="background:rgba(255,255,255,0.03); padding:10px; border-radius:8px; border:1px solid var(--border-color);">
            <div style="font-size:11px; color:var(--text-muted);">Peak Congestion Window</div>
            <div style="font-size:13px; font-weight:700; color:#fff; margin-top:2px;">1:30 PM – 4:45 PM (88% Avg)</div>
          </div>
          <div style="background:rgba(255,255,255,0.03); padding:10px; border-radius:8px; border:1px solid var(--border-color);">
            <div style="font-size:11px; color:var(--text-muted);">Avg. Desk Dwell Time</div>
            <div style="font-size:13px; font-weight:700; color:#fff; margin-top:2px;">1 Hour 48 Minutes</div>
          </div>
        </div>
      </div>
    </div>
  </div>

  <!-- TAB 3: Smart Energy Automation -->
  <div id="tab-energy" class="tab-pane">
    <div style="display:grid; grid-template-columns:1fr 1fr; gap:18px; margin-bottom:24px;">
      <div class="zone-container">
        <h3 style="color:var(--primary); font-size:16px; margin-bottom:12px;">💡 Zone A Lighting & HVAC Relay</h3>
        <p style="font-size:13px; color:var(--text-muted); margin-bottom:16px;">Automated by Seats 1–4 sensor state. Automatically dims luminaires and drops HVAC to standby when no students are present.</p>
        <div style="background:rgba(255,255,255,0.03); padding:14px; border-radius:10px; border:1px solid var(--border-color); display:flex; flex-direction:column; gap:10px;">
          <div style="display:flex; justify-content:space-between; font-size:13px;">
            <span>Zone Mode:</span>
            <strong id="energy-z1-mode" style="color:var(--success);">ECO-SAVER (0 Occupants)</strong>
          </div>
          <div style="display:flex; justify-content:space-between; font-size:13px;">
            <span>Luminaire Task Light:</span>
            <strong id="energy-z1-light">30% (Ambient Eco)</strong>
          </div>
          <div style="display:flex; justify-content:space-between; font-size:13px;">
            <span>Ventilation / AC:</span>
            <strong id="energy-z1-hvac">STANDBY</strong>
          </div>
        </div>
      </div>

      <div class="zone-container">
        <h3 style="color:var(--primary); font-size:16px; margin-bottom:12px;">💡 Zone B Lighting & HVAC Relay</h3>
        <p style="font-size:13px; color:var(--text-muted); margin-bottom:16px;">Automated by Seats 5–7 sensor state. Dynamically activates task lighting upon physical presence.</p>
        <div style="background:rgba(255,255,255,0.03); padding:14px; border-radius:10px; border:1px solid var(--border-color); display:flex; flex-direction:column; gap:10px;">
          <div style="display:flex; justify-content:space-between; font-size:13px;">
            <span>Zone Mode:</span>
            <strong id="energy-z2-mode" style="color:var(--success);">ECO-SAVER (0 Occupants)</strong>
          </div>
          <div style="display:flex; justify-content:space-between; font-size:13px;">
            <span>Luminaire Task Light:</span>
            <strong id="energy-z2-light">25% (Ambient Eco)</strong>
          </div>
          <div style="display:flex; justify-content:space-between; font-size:13px;">
            <span>Ventilation / AC:</span>
            <strong id="energy-z2-hvac">STANDBY</strong>
          </div>
        </div>
      </div>
    </div>
  </div>

  <!-- TAB 4: Telemetry & Hardware Console -->
  <div id="tab-telemetry" class="tab-pane">
    <div class="hardware-strip">
      <!-- 16x2 LCD Mirror -->
      <div class="hw-box">
        <h3>Virtual LiquidCrystal (16×2) <span>Pins: 15, 2, 4, 16, 17, 5</span></h3>
        <div class="lcd-display">
          <div class="lcd-row" id="lcd-line1">1:A 2:A 3:A 4:A </div>
          <div class="lcd-row" id="lcd-line2">5:A 6:A 7:A     </div>
        </div>
        <p style="font-size:11px; color:var(--text-muted); margin-top:8px;">Live mirror of physical on-table LCD display (A = Available, O = Occupied, R = Reserved).</p>
      </div>

      <!-- Hardware Switch Controls -->
      <div class="hw-box">
        <h3>Hardware Sensor Switches <span>Click to trigger Pin LOW/HIGH</span></h3>
        <div class="sensor-switches">
          <button class="hw-switch-btn" id="hw-btn-1" onclick="toggleSensor(1)">
            <strong>Seat 1</strong>
            <span class="pin-id">GPIO 22: HIGH</span>
          </button>
          <button class="hw-switch-btn" id="hw-btn-2" onclick="toggleSensor(2)">
            <strong>Seat 2</strong>
            <span class="pin-id">GPIO 23: HIGH</span>
          </button>
          <button class="hw-switch-btn" id="hw-btn-3" onclick="toggleSensor(3)">
            <strong>Seat 3</strong>
            <span class="pin-id">GPIO 32: HIGH</span>
          </button>
          <button class="hw-switch-btn" id="hw-btn-4" onclick="toggleSensor(4)">
            <strong>Seat 4</strong>
            <span class="pin-id">GPIO 33: HIGH</span>
          </button>
          <button class="hw-switch-btn" id="hw-btn-5" onclick="toggleSensor(5)">
            <strong>Seat 5</strong>
            <span class="pin-id">GPIO 25: HIGH</span>
          </button>
          <button class="hw-switch-btn" id="hw-btn-6" onclick="toggleSensor(6)">
            <strong>Seat 6</strong>
            <span class="pin-id">GPIO 26: HIGH</span>
          </button>
          <button class="hw-switch-btn" id="hw-btn-7" onclick="toggleSensor(7)">
            <strong>Seat 7</strong>
            <span class="pin-id">GPIO 27: HIGH</span>
          </button>
        </div>
        <button onclick="resetAllHardware()" style="margin-top:10px; width:100%; padding:8px; background:rgba(255,255,255,0.06); border:1px solid var(--border-color); color:var(--text-muted); border-radius:6px; cursor:pointer; font-size:12px;">Reset All Sensors to Idle (HIGH)</button>
      </div>
    </div>

    <!-- Serial Stream -->
    <div class="hw-box" style="margin-bottom:24px;">
      <h3>ESP32 Serial Stream (115200 Baud)</h3>
      <div class="serial-feed" id="serialLogs">Waiting for telemetry...</div>
    </div>
  </div>

</div>

<!-- Reservation / Action Modal -->
<div class="modal-overlay" id="reserveModal">
  <div class="modal-content">
    <h3 id="modalTitle">Reserve Desk 01</h3>
    <p id="modalDesc">Enter student credentials to place a temporary 15-minute grace-hold on this desk. If physical presence is not sensed within 15 minutes, the seat auto-releases.</p>
    
    <div id="modalForm">
      <div class="form-group">
        <label>Student Roll / ID Number</label>
        <input type="text" id="studentIdInput" placeholder="e.g. STU-2024-8841" value="STU-2024-001">
      </div>
      <div class="form-group">
        <label>Study Session Purpose</label>
        <input type="text" id="studentPurposeInput" placeholder="e.g. ECS Project Prep" value="ECS Viva Study">
      </div>
    </div>

    <div id="modalReservedInfo" style="display:none; background:rgba(245,158,11,0.1); border:1px solid rgba(245,158,11,0.3); border-radius:8px; padding:12px; margin-bottom:14px; font-size:12px;">
      <div style="color:#fbbf24; font-weight:700; margin-bottom:4px;">⏳ ACTIVE RESERVATION</div>
      <div id="reservedByText">Reserved by: STU-2024-001</div>
      <div id="reservedTimeLeft" style="margin-top:4px; font-weight:600;">Time Remaining: 14:52</div>
    </div>

    <div class="modal-actions" id="modalActions">
      <button class="btn btn-secondary" onclick="closeModal()">Cancel</button>
      <button class="btn btn-primary" id="btnModalConfirm" onclick="confirmReservation()">Confirm Reservation</button>
    </div>
  </div>
</div>

<script>
let selectedSeatId = null;
let currentSeatData = {};
const pinMap = {1: 22, 2: 23, 3: 32, 4: 33, 5: 25, 6: 26, 7: 27};

function switchTab(tabId) {
  document.querySelectorAll('.tab-btn').forEach(btn => btn.classList.remove('active'));
  document.querySelectorAll('.tab-pane').forEach(p => p.classList.remove('active'));
  
  event.currentTarget.classList.add('active');
  document.getElementById('tab-' + tabId).classList.add('active');
}

function updateClock() {
  const now = new Date();
  document.getElementById('clockDisplay').innerText = "⏰ " + now.toLocaleTimeString();
}
setInterval(updateClock, 1000);
updateClock();

function handleDeskClick(seatId) {
  selectedSeatId = seatId;
  const s = currentSeatData[seatId];
  if (!s) return;

  const modal = document.getElementById('reserveModal');
  const modalTitle = document.getElementById('modalTitle');
  const modalDesc = document.getElementById('modalDesc');
  const modalForm = document.getElementById('modalForm');
  const modalReservedInfo = document.getElementById('modalReservedInfo');
  const modalActions = document.getElementById('modalActions');

  modalTitle.innerText = `Desk 0${seatId} (GPIO ${pinMap[seatId]})`;

  if (s.status === 'AVAILABLE') {
    modalDesc.innerText = "Place a temporary 15-minute reservation hold on this desk. If the physical seat sensor does not detect occupancy within 15 minutes, the seat auto-releases.";
    modalForm.style.display = 'block';
    modalReservedInfo.style.display = 'none';
    modalActions.innerHTML = `
      <button class="btn btn-secondary" onclick="closeModal()">Cancel</button>
      <button class="btn btn-primary" onclick="confirmReservation()">Reserve 15 Min</button>
    `;
  } else if (s.status === 'RESERVED') {
    modalDesc.innerText = "This desk is currently held under a reservation.";
    modalForm.style.display = 'none';
    modalReservedInfo.style.display = 'block';
    document.getElementById('reservedByText').innerText = "Reserved by: " + (s.reservedBy || "Student");
    const rem = Math.max(0, Math.floor(s.reserveRemaining || 0));
    const mins = Math.floor(rem / 60);
    const secs = rem % 60;
    document.getElementById('reservedTimeLeft').innerText = `Grace Window Remaining: ${mins}:${secs < 10 ? '0' : ''}${secs}`;
    modalActions.innerHTML = `
      <button class="btn btn-secondary" onclick="closeModal()">Close</button>
      <button class="btn btn-danger" onclick="cancelReservation(${seatId})">Release Reservation</button>
      <button class="btn btn-primary" onclick="toggleSensor(${seatId}); closeModal();">Check-In Now</button>
    `;
  } else {
    // OCCUPIED
    modalDesc.innerText = "This seat is actively occupied (detected by physical seat sensor).";
    modalForm.style.display = 'none';
    modalReservedInfo.style.display = 'none';
    modalActions.innerHTML = `
      <button class="btn btn-secondary" onclick="closeModal()">Close</button>
      <button class="btn btn-danger" onclick="toggleSensor(${seatId}); closeModal();">Force Vacate (Test)</button>
    `;
  }

  modal.style.display = 'flex';
}

function closeModal() {
  document.getElementById('reserveModal').style.display = 'none';
  selectedSeatId = null;
}

function confirmReservation() {
  if (!selectedSeatId) return;
  const stuId = document.getElementById('studentIdInput').value || 'STU-Guest';
  
  fetch('/api/reserve', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({seat: selectedSeatId, studentId: stuId})
  })
  .then(() => {
    closeModal();
    fetchData();
  });
}

function cancelReservation(seatId) {
  fetch('/api/release', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({seat: seatId})
  })
  .then(() => {
    closeModal();
    fetchData();
  });
}

function toggleSensor(seatId) {
  fetch('/api/toggle', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({seat: seatId})
  })
  .then(() => fetchData());
}

function resetAllHardware() {
  fetch('/api/reset', {method: 'POST'})
  .then(() => fetchData());
}

function fetchData() {
  fetch('/data')
  .then(res => res.json())
  .then(data => {
    // Update KPIs
    document.getElementById('kpi-avail').innerText = data.available;
    document.getElementById('kpi-occ').innerText = data.occupied;
    document.getElementById('kpi-res').innerText = data.reserved;
    document.getElementById('kpi-eco').innerText = data.powerSavedWatts + ' W';
    document.getElementById('kpi-util').innerText = `Utilization: ${data.utilization}%`;
    document.getElementById('analytics-util-pct').innerText = data.utilization + '%';
    document.getElementById('analytics-util-bar').style.width = data.utilization + '%';

    // Update Zones
    const z1Badge = document.getElementById('zone1-badge');
    if (data.zone1.ecoMode) {
      z1Badge.className = 'zone-eco-badge eco-on';
      z1Badge.innerText = 'ECO-SAVER ACTIVE';
    } else {
      z1Badge.className = 'zone-eco-badge eco-off';
      z1Badge.innerText = `${data.zone1.activeSeats}/4 OCCUPIED (TASK MODE)`;
    }

    const z2Badge = document.getElementById('zone2-badge');
    if (data.zone2.ecoMode) {
      z2Badge.className = 'zone-eco-badge eco-on';
      z2Badge.innerText = 'ECO-SAVER ACTIVE';
    } else {
      z2Badge.className = 'zone-eco-badge eco-off';
      z2Badge.innerText = `${data.zone2.activeSeats}/3 OCCUPIED (TASK MODE)`;
    }

    // Update Energy tab texts
    document.getElementById('energy-z1-mode').innerText = data.zone1.ecoMode ? "ECO-SAVER (0 Occupants)" : "TASK ACTIVE (" + data.zone1.activeSeats + " Occupants)";
    document.getElementById('energy-z1-light').innerText = data.zone1.lighting;
    document.getElementById('energy-z1-hvac').innerText = data.zone1.hvac;

    document.getElementById('energy-z2-mode').innerText = data.zone2.ecoMode ? "ECO-SAVER (0 Occupants)" : "TASK ACTIVE (" + data.zone2.activeSeats + " Occupants)";
    document.getElementById('energy-z2-light').innerText = data.zone2.lighting;
    document.getElementById('energy-z2-hvac').innerText = data.zone2.hvac;

    // Update Desks
    data.seats.forEach(s => {
      currentSeatData[s.id] = s;
      const card = document.getElementById('desk-card-' + s.id);
      const statusBadge = document.getElementById('desk-status-' + s.id);
      const tag = document.getElementById('desk-tag-' + s.id);
      const hwBtn = document.getElementById('hw-btn-' + s.id);

      card.className = 'desk-card ' + s.status.toLowerCase();
      statusBadge.innerText = s.status;

      if (s.status === 'RESERVED') {
        const rem = Math.max(0, Math.floor(s.reserveRemaining || 0));
        const mins = Math.floor(rem / 60);
        const secs = rem % 60;
        tag.innerHTML = `<div class="bag-alert-tag">⏳ ${mins}:${secs < 10 ? '0' : ''}${secs} left</div>`;
      } else if (s.bagAlert) {
        tag.innerHTML = `<div class="bag-alert-tag">⚠️ Suspected Bag</div>`;
      } else {
        tag.innerHTML = '';
      }

      // Hardware switch button
      if (s.hardwareTriggered) {
        hwBtn.className = 'hw-switch-btn active';
        hwBtn.querySelector('.pin-id').innerText = `GPIO ${s.gpio}: LOW`;
      } else {
        hwBtn.className = 'hw-switch-btn';
        hwBtn.querySelector('.pin-id').innerText = `GPIO ${s.gpio}: HIGH`;
      }
    });

    // Update LCD and Serial
    document.getElementById('lcd-line1').innerText = data.lcd.line1;
    document.getElementById('lcd-line2').innerText = data.lcd.line2;

    const logBox = document.getElementById('serialLogs');
    logBox.innerText = data.serial_logs.join("\\n");
    logBox.scrollTop = logBox.scrollHeight;
  })
  .catch(err => console.error("Poll error:", err));
}

setInterval(fetchData, 1000);
fetchData();
</script>
</body>
</html>
"""

class SLISHandler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        parsed = urllib.parse.urlparse(self.path)
        
        if parsed.path == "/" or parsed.path == "/index.html":
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.end_headers()
            self.wfile.write(HTML_PAGE.encode("utf-8"))
            
        elif parsed.path == "/data":
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            
            now = time.time()
            st = get_stats()
            l1, l2 = get_lcd_lines()
            
            seats_list = []
            for sid in range(1, 8):
                s = seats_state[sid]
                status_str = get_seat_status(s)
                rem = max(0, s["reserveExpiry"] - now) if s["reserved"] else 0
                seats_list.append({
                    "id": s["id"],
                    "gpio": s["gpio"],
                    "zone": s["zone"],
                    "hardwareTriggered": s["hardwareTriggered"],
                    "reserved": s["reserved"],
                    "reservedBy": s["reservedBy"],
                    "reserveRemaining": rem,
                    "bagAlert": s["bagAlert"],
                    "status": status_str
                })
                
            response_payload = {
                **st,
                "seats": seats_list,
                "lcd": {"line1": l1, "line2": l2},
                "serial_logs": serial_logs[-15:]
            }
            self.wfile.write(json.dumps(response_payload).encode("utf-8"))
        else:
            self.send_response(404)
            self.end_headers()

    def do_POST(self):
        parsed = urllib.parse.urlparse(self.path)
        content_length = int(self.headers.get('Content-Length', 0))
        body = self.rfile.read(content_length) if content_length > 0 else b""
        
        try:
            payload = json.loads(body.decode("utf-8")) if body else {}
        except Exception:
            payload = {}
            
        if parsed.path == "/api/toggle":
            sid = payload.get("seat")
            if sid in seats_state:
                seats_state[sid]["hardwareTriggered"] = not seats_state[sid]["hardwareTriggered"]
                # If physically occupied, confirm reservation
                if seats_state[sid]["hardwareTriggered"] and seats_state[sid]["reserved"]:
                    log_serial(f"Presence detected on reserved Seat {sid}! Reservation confirmed as OCCUPIED.")
                pin = seats_state[sid]["gpio"]
                val = "LOW (Occupied)" if seats_state[sid]["hardwareTriggered"] else "HIGH (Free)"
                log_serial(f"GPIO Pin {pin} (Seat {sid}) changed to {val}")
                log_hardware_status()
                
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(b'{"status":"ok"}')
            
        elif parsed.path == "/api/reserve":
            sid = payload.get("seat")
            student_id = payload.get("studentId", "STU-Guest")
            if sid in seats_state:
                seats_state[sid]["reserved"] = True
                seats_state[sid]["reservedBy"] = student_id
                seats_state[sid]["reserveExpiry"] = time.time() + 900 # 15 minutes
                log_serial(f"Seat {sid} RESERVED by {student_id} (15 min grace window)")
                log_hardware_status()
                
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(b'{"status":"ok"}')
            
        elif parsed.path == "/api/release":
            sid = payload.get("seat")
            if sid in seats_state:
                seats_state[sid]["reserved"] = False
                seats_state[sid]["reservedBy"] = ""
                seats_state[sid]["reserveExpiry"] = 0
                log_serial(f"Seat {sid} reservation CANCELLED/RELEASED")
                log_hardware_status()
                
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(b'{"status":"ok"}')
            
        elif parsed.path == "/api/reset":
            for sid in seats_state:
                seats_state[sid]["hardwareTriggered"] = False
                seats_state[sid]["reserved"] = False
                seats_state[sid]["reservedBy"] = ""
                seats_state[sid]["reserveExpiry"] = 0
                seats_state[sid]["bagAlert"] = False
            log_serial("All seats and sensors RESET to AVAILABLE")
            log_hardware_status()
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(b'{"status":"ok"}')
        else:
            self.send_response(404)
            self.end_headers()

    def log_message(self, format, *args):
        pass

def run():
    log_serial("SLIS Edge Controller Starting...")
    log_serial("ESP32 SoftAP Initialized: SSID=Smart_Library")
    log_serial(f"HTTP Server Listening on http://localhost:{PORT}")
    log_hardware_status()
    
    server = socketserver.TCPServer(("", PORT), SLISHandler)
    server.allow_reuse_address = True
    print("\n=======================================================")
    print(f"  SLIS Framework Server: http://localhost:{PORT}")
    print("=======================================================\n")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nSimulator stopped.")

if __name__ == "__main__":
    run()
