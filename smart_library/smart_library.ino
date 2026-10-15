#include <WiFi.h>
#include <WebServer.h>
#include <LiquidCrystal.h>

/*
 * Smart Library Intelligence System (SLIS) - Edge Microcontroller Firmware
 * 5-Layer Complete Architecture Implementation:
 * - Layer 1 (Perception): 7x Seat Digital Occupancy Sensors (GPIO 22, 23, 32, 33, 25, 26, 27)
 * - Layer 2 (Edge): Dual-Zone Energy Automation (Zone A & B), 16x2 Glanceable LCD, 15-min Reservation Engine
 * - Layer 3 (Communication): Standalone SoftAP ("Smart_Library") + High-Speed HTTP REST API (/data, /api/reserve, /api/release)
 * - Layer 4 (Analytics): Real-Time Utilization % and Watts Saved telemetry
 * - Layer 5 (Application): Self-Contained Glassmorphic SPA Dashboard served straight from ESP32 Flash
 */

LiquidCrystal lcd(15, 2, 4, 16, 17, 5);
WebServer server(80);

const char* ssid = "Smart_Library";
const char* password = "12345678";

struct Seat {
  int id;
  int pin;
  int zone;
  bool occupied;
  bool reserved;
  unsigned long reserveExpiry;
  String reservedBy;
};

Seat seats[7] = {
  {1, 22, 1, false, false, 0, ""},
  {2, 23, 1, false, false, 0, ""},
  {3, 32, 1, false, false, 0, ""},
  {4, 33, 1, false, false, 0, ""},
  {5, 25, 2, false, false, 0, ""},
  {6, 26, 2, false, false, 0, ""},
  {7, 27, 2, false, false, 0, ""}
};

int totalOccupied = 0;
int totalReserved = 0;
int totalAvailable = 7;

// Function declarations
void handleRoot();
void handleData();
void handleReserve();
void handleRelease();
void handleReset();
void readSeats();
void updateLCD();

// Offline-ready, self-contained modern SLIS web dashboard
const char HTML_PAGE[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>SLIS • Smart Library Intelligence System</title>
<style>
:root {
  --bg-main: #0b0f19;
  --bg-card: rgba(18, 24, 38, 0.85);
  --border-color: rgba(255, 255, 255, 0.08);
  --border-highlight: rgba(56, 189, 248, 0.3);
  --primary: #38bdf8;
  --primary-glow: rgba(56, 189, 248, 0.2);
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
  font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
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
.container { max-width: 1200px; margin: 0 auto; }
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
.brand-wrapper { display: flex; align-items: center; gap: 14px; }
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
  0%, 100% { transform: scale(0.95); opacity: 0.8; }
  50% { transform: scale(1.3); opacity: 1; }
}
.header-badges { display: flex; gap: 10px; align-items: center; flex-wrap: wrap; }
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
.badge strong { color: var(--primary); }
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
}
.tab-btn:hover { background: rgba(255,255,255,0.08); color: #fff; }
.tab-btn.active {
  background: linear-gradient(135deg, rgba(56, 189, 248, 0.15), rgba(99, 102, 241, 0.15));
  border-color: var(--primary);
  color: #fff;
  box-shadow: 0 0 16px rgba(56, 189, 248, 0.15);
}
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
  transition: transform 0.2s;
}
.kpi-card:hover { transform: translateY(-2px); border-color: var(--border-highlight); }
.kpi-icon {
  width: 48px;
  height: 48px;
  border-radius: 12px;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 22px;
}
.kpi-icon.avail { background: rgba(16, 185, 129, 0.15); color: var(--success); }
.kpi-icon.occ { background: rgba(239, 68, 68, 0.15); color: var(--danger); }
.kpi-icon.res { background: rgba(245, 158, 11, 0.15); color: var(--warning); }
.kpi-icon.eco { background: rgba(56, 189, 248, 0.15); color: var(--primary); }
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
  color: #fff;
}
.kpi-info p { font-size: 11px; color: var(--text-muted); margin-top: 4px; }
.section-title {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 14px;
}
.section-title h2 { font-size: 16px; font-weight: 700; color: #fff; }
.floorplan-zones {
  display: grid;
  grid-template-columns: 1.2fr 1fr;
  gap: 18px;
  margin-bottom: 24px;
}
@media(max-width: 900px) { .floorplan-zones { grid-template-columns: 1fr; } }
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
.zone-header h3 { font-size: 15px; color: var(--primary); }
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
  transition: all 0.25s ease;
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
  0%, 100% { box-shadow: 0 0 0 rgba(245, 158, 11, 0); }
  50% { box-shadow: 0 0 14px var(--warning-glow); }
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
.desk-icon { font-size: 32px; margin: 6px 0; }
.desk-name { font-size: 14px; font-weight: 700; color: #fff; margin-bottom: 6px; }
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
.hardware-strip {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 18px;
  margin-bottom: 24px;
}
@media(max-width: 800px) { .hardware-strip { grid-template-columns: 1fr; } }
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
}
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
  user-select: none;
}
.lcd-row { height: 26px; line-height: 26px; white-space: pre; }
.analytics-grid {
  display: grid;
  grid-template-columns: 2fr 1fr;
  gap: 18px;
  margin-bottom: 24px;
}
@media(max-width: 850px) { .analytics-grid { grid-template-columns: 1fr; } }
.chart-card {
  background: var(--bg-card);
  backdrop-filter: blur(12px);
  border: 1px solid var(--border-color);
  border-radius: var(--radius-lg);
  padding: 20px;
}
.chart-card h3 { font-size: 15px; color: #fff; margin-bottom: 14px; }
.svg-chart { width: 100%; height: 180px; }
.modal-overlay {
  position: fixed;
  top: 0; left: 0; right: 0; bottom: 0;
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
}
.modal-content h3 { font-size: 18px; color: #fff; margin-bottom: 8px; }
.modal-content p { font-size: 13px; color: var(--text-muted); margin-bottom: 18px; line-height: 1.5; }
.form-group { margin-bottom: 16px; text-align: left; }
.form-group label { display: block; font-size: 12px; color: var(--text-muted); margin-bottom: 6px; font-weight: 600; }
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
.modal-actions { display: flex; gap: 10px; justify-content: flex-end; margin-top: 20px; }
.btn {
  padding: 9px 16px;
  border-radius: var(--radius-sm);
  font-size: 13px;
  font-weight: 600;
  cursor: pointer;
  border: none;
}
.btn-primary { background: linear-gradient(135deg, #0284c7, #4f46e5); color: #fff; }
.btn-secondary { background: rgba(255,255,255,0.1); color: var(--text-muted); }
.btn-danger { background: rgba(239, 68, 68, 0.2); color: #f87171; border: 1px solid rgba(239, 68, 68, 0.4); }
.tab-pane { display: none; }
.tab-pane.active { display: block; }
</style>
</head>
<body>
<div class="container">
  <header class="header-bar">
    <div class="brand-wrapper">
      <div class="brand-icon">📚</div>
      <div class="brand-title">
        <h1>SLIS • Smart Library Intelligence System</h1>
        <p><span class="live-pill"></span> Edge Node 01 • ESP32 Hardware Sensing Active</p>
      </div>
    </div>
    <div class="header-badges">
      <div class="badge">🌐 Wi-Fi: <strong>Smart_Library</strong></div>
      <div class="badge">📡 IP: <strong>192.168.4.1</strong></div>
      <div class="badge" id="clockDisplay">⏰ 12:00:00</div>
    </div>
  </header>

  <nav class="nav-tabs">
    <button class="tab-btn active" onclick="switchTab('floorplan')">🗺️ Interactive Floor Map</button>
    <button class="tab-btn" onclick="switchTab('analytics')">📊 Real-Time Analytics & Trends</button>
    <button class="tab-btn" onclick="switchTab('energy')">⚡ Smart Energy & Eco-Automation</button>
    <button class="tab-btn" onclick="switchTab('telemetry')">📟 Hardware & LCD Mirror</button>
  </nav>

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

  <!-- TAB 1: Floor Map -->
  <div id="tab-floorplan" class="tab-pane active">
    <div class="section-title">
      <h2>📍 Live Floor Plan • Level 2 East Wing</h2>
      <span style="font-size:12px; color:var(--text-muted);">Click any available desk to reserve, or occupied desk for details</span>
    </div>
    <div class="floorplan-zones">
      <div class="zone-container">
        <div class="zone-header">
          <h3>📖 Zone A: Silent Study Pod</h3>
          <span class="zone-eco-badge eco-on" id="zone1-badge">ECO-SAVER ACTIVE</span>
        </div>
        <div class="desks-grid">
          <div class="desk-card available" id="desk-card-1" onclick="handleDeskClick(1)">
            <div class="desk-top-bar"><span>Desk 01</span><span class="pin-pill">P22</span></div>
            <div class="desk-icon">🪑</div>
            <div class="desk-name">Seat 1</div>
            <div class="status-badge" id="desk-status-1">AVAILABLE</div>
            <div id="desk-tag-1"></div>
          </div>
          <div class="desk-card available" id="desk-card-2" onclick="handleDeskClick(2)">
            <div class="desk-top-bar"><span>Desk 02</span><span class="pin-pill">P23</span></div>
            <div class="desk-icon">🪑</div>
            <div class="desk-name">Seat 2</div>
            <div class="status-badge" id="desk-status-2">AVAILABLE</div>
            <div id="desk-tag-2"></div>
          </div>
          <div class="desk-card available" id="desk-card-3" onclick="handleDeskClick(3)">
            <div class="desk-top-bar"><span>Desk 03</span><span class="pin-pill">P32</span></div>
            <div class="desk-icon">🪑</div>
            <div class="desk-name">Seat 3</div>
            <div class="status-badge" id="desk-status-3">AVAILABLE</div>
            <div id="desk-tag-3"></div>
          </div>
          <div class="desk-card available" id="desk-card-4" onclick="handleDeskClick(4)">
            <div class="desk-top-bar"><span>Desk 04</span><span class="pin-pill">P33</span></div>
            <div class="desk-icon">🪑</div>
            <div class="desk-name">Seat 4</div>
            <div class="status-badge" id="desk-status-4">AVAILABLE</div>
            <div id="desk-tag-4"></div>
          </div>
        </div>
      </div>

      <div class="zone-container">
        <div class="zone-header">
          <h3>🔬 Zone B: Research & Tech Bay</h3>
          <span class="zone-eco-badge eco-on" id="zone2-badge">ECO-SAVER ACTIVE</span>
        </div>
        <div class="desks-grid">
          <div class="desk-card available" id="desk-card-5" onclick="handleDeskClick(5)">
            <div class="desk-top-bar"><span>Desk 05</span><span class="pin-pill">P25</span></div>
            <div class="desk-icon">💻</div>
            <div class="desk-name">Seat 5</div>
            <div class="status-badge" id="desk-status-5">AVAILABLE</div>
            <div id="desk-tag-5"></div>
          </div>
          <div class="desk-card available" id="desk-card-6" onclick="handleDeskClick(6)">
            <div class="desk-top-bar"><span>Desk 06</span><span class="pin-pill">P26</span></div>
            <div class="desk-icon">💻</div>
            <div class="desk-name">Seat 6</div>
            <div class="status-badge" id="desk-status-6">AVAILABLE</div>
            <div id="desk-tag-6"></div>
          </div>
          <div class="desk-card available" id="desk-card-7" onclick="handleDeskClick(7)">
            <div class="desk-top-bar"><span>Desk 07</span><span class="pin-pill">P27</span></div>
            <div class="desk-icon">💻</div>
            <div class="desk-name">Seat 7</div>
            <div class="status-badge" id="desk-status-7">AVAILABLE</div>
            <div id="desk-tag-7"></div>
          </div>
        </div>
      </div>
    </div>
  </div>

  <!-- TAB 2: Analytics -->
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
          <line x1="40" y1="20" x2="480" y2="20" stroke="rgba(255,255,255,0.05)" />
          <line x1="40" y1="60" x2="480" y2="60" stroke="rgba(255,255,255,0.05)" />
          <line x1="40" y1="100" x2="480" y2="100" stroke="rgba(255,255,255,0.05)" />
          <line x1="40" y1="140" x2="480" y2="140" stroke="rgba(255,255,255,0.1)" />
          <text x="15" y="24" fill="#64748b" font-size="10">100%</text>
          <text x="20" y="64" fill="#64748b" font-size="10">60%</text>
          <text x="20" y="104" fill="#64748b" font-size="10">30%</text>
          <text x="25" y="144" fill="#64748b" font-size="10">0%</text>
          <path d="M 40 140 Q 90 120 140 70 T 240 30 T 340 50 T 440 90 L 480 120 L 480 140 Z" fill="url(#chartGrad)" />
          <path d="M 40 140 Q 90 120 140 70 T 240 30 T 340 50 T 440 90 L 480 120" fill="none" stroke="#38bdf8" stroke-width="3" />
          <circle cx="240" cy="30" r="5" fill="#f43f5e" stroke="#fff" stroke-width="2" />
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

  <!-- TAB 3: Energy Automation -->
  <div id="tab-energy" class="tab-pane">
    <div style="display:grid; grid-template-columns:1fr 1fr; gap:18px; margin-bottom:24px;">
      <div class="zone-container">
        <h3 style="color:var(--primary); font-size:16px; margin-bottom:12px;">💡 Zone A Lighting & HVAC Relay</h3>
        <p style="font-size:13px; color:var(--text-muted); margin-bottom:16px;">Automated by Seats 1–4 sensor state. Automatically dims luminaires and puts HVAC to standby when no students are present.</p>
        <div style="background:rgba(255,255,255,0.03); padding:14px; border-radius:10px; border:1px solid var(--border-color); display:flex; flex-direction:column; gap:10px;">
          <div style="display:flex; justify-content:space-between; font-size:13px;"><span>Zone Mode:</span><strong id="energy-z1-mode" style="color:var(--success);">ECO-SAVER (0 Occupants)</strong></div>
          <div style="display:flex; justify-content:space-between; font-size:13px;"><span>Luminaire Task Light:</span><strong id="energy-z1-light">30% (Ambient Eco)</strong></div>
          <div style="display:flex; justify-content:space-between; font-size:13px;"><span>Ventilation / AC:</span><strong id="energy-z1-hvac">STANDBY</strong></div>
        </div>
      </div>
      <div class="zone-container">
        <h3 style="color:var(--primary); font-size:16px; margin-bottom:12px;">💡 Zone B Lighting & HVAC Relay</h3>
        <p style="font-size:13px; color:var(--text-muted); margin-bottom:16px;">Automated by Seats 5–7 sensor state. Dynamically activates task lighting upon physical presence.</p>
        <div style="background:rgba(255,255,255,0.03); padding:14px; border-radius:10px; border:1px solid var(--border-color); display:flex; flex-direction:column; gap:10px;">
          <div style="display:flex; justify-content:space-between; font-size:13px;"><span>Zone Mode:</span><strong id="energy-z2-mode" style="color:var(--success);">ECO-SAVER (0 Occupants)</strong></div>
          <div style="display:flex; justify-content:space-between; font-size:13px;"><span>Luminaire Task Light:</span><strong id="energy-z2-light">25% (Ambient Eco)</strong></div>
          <div style="display:flex; justify-content:space-between; font-size:13px;"><span>Ventilation / AC:</span><strong id="energy-z2-hvac">STANDBY</strong></div>
        </div>
      </div>
    </div>
  </div>

  <!-- TAB 4: Telemetry -->
  <div id="tab-telemetry" class="tab-pane">
    <div class="hardware-strip">
      <div class="hw-box">
        <h3>Virtual LiquidCrystal (16×2) <span>Pins: 15, 2, 4, 16, 17, 5</span></h3>
        <div class="lcd-display">
          <div class="lcd-row" id="lcd-line1">1:A 2:A 3:A 4:A </div>
          <div class="lcd-row" id="lcd-line2">5:A 6:A 7:A     </div>
        </div>
        <p style="font-size:11px; color:var(--text-muted); margin-top:8px;">Live mirror of physical on-table LCD display (A = Available, O = Occupied, R = Reserved).</p>
      </div>
      <div class="hw-box">
        <h3>System Status & Hardware Info</h3>
        <div style="display:flex; flex-direction:column; gap:10px; font-size:13px; margin-top:8px;">
          <div style="display:flex; justify-content:space-between;"><span>Controller:</span><strong>ESP32-D0WD-V3 (Dual Core 240MHz)</strong></div>
          <div style="display:flex; justify-content:space-between;"><span>Access Point:</span><strong>Smart_Library (WPA2)</strong></div>
          <div style="display:flex; justify-content:space-between;"><span>Internal Pull-Ups:</span><strong>INPUT_PULLUP Active</strong></div>
          <div style="display:flex; justify-content:space-between;"><span>Sensor Logic:</span><strong>LOW = Occupied, HIGH = Free</strong></div>
        </div>
      </div>
    </div>
  </div>
</div>

<!-- Modal -->
<div class="modal-overlay" id="reserveModal">
  <div class="modal-content">
    <h3 id="modalTitle">Reserve Desk</h3>
    <p id="modalDesc">Enter student ID to place a 15-minute hold on this desk.</p>
    <div id="modalForm">
      <div class="form-group">
        <label>Student Roll / ID Number</label>
        <input type="text" id="studentIdInput" placeholder="e.g. STU-2024-8841" value="STU-2024-001">
      </div>
    </div>
    <div id="modalReservedInfo" style="display:none; background:rgba(245,158,11,0.1); border:1px solid rgba(245,158,11,0.3); border-radius:8px; padding:12px; margin-bottom:14px; font-size:12px;">
      <div style="color:#fbbf24; font-weight:700; margin-bottom:4px;">⏳ ACTIVE RESERVATION</div>
      <div id="reservedByText">Reserved by: STU-2024-001</div>
      <div id="reservedTimeLeft" style="margin-top:4px; font-weight:600;">Time Remaining: 14:52</div>
    </div>
    <div class="modal-actions" id="modalActions">
      <button class="btn btn-secondary" onclick="closeModal()">Cancel</button>
      <button class="btn btn-primary" onclick="confirmReservation()">Confirm Reservation</button>
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
    modalDesc.innerText = "Place a temporary 15-minute reservation hold on this desk. If physical occupancy is not sensed within 15 minutes, the seat auto-releases.";
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
    `;
  } else {
    modalDesc.innerText = "This seat is actively occupied (detected by physical seat sensor).";
    modalForm.style.display = 'none';
    modalReservedInfo.style.display = 'none';
    modalActions.innerHTML = `<button class="btn btn-secondary" onclick="closeModal()">Close</button>`;
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
    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
    body: 'seat=' + selectedSeatId + '&studentId=' + encodeURIComponent(stuId)
  })
  .then(() => { closeModal(); fetchData(); });
}

function cancelReservation(seatId) {
  fetch('/api/release', {
    method: 'POST',
    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
    body: 'seat=' + seatId
  })
  .then(() => { closeModal(); fetchData(); });
}

function fetchData() {
  fetch('/data')
  .then(res => res.json())
  .then(data => {
    document.getElementById('kpi-avail').innerText = data.available;
    document.getElementById('kpi-occ').innerText = data.occupied;
    document.getElementById('kpi-res').innerText = data.reserved;
    document.getElementById('kpi-eco').innerText = data.powerSavedWatts + ' W';
    document.getElementById('kpi-util').innerText = `Utilization: ${data.utilization}%`;
    document.getElementById('analytics-util-pct').innerText = data.utilization + '%';
    document.getElementById('analytics-util-bar').style.width = data.utilization + '%';

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

    document.getElementById('energy-z1-mode').innerText = data.zone1.ecoMode ? "ECO-SAVER (0 Occupants)" : "TASK ACTIVE (" + data.zone1.activeSeats + " Occupants)";
    document.getElementById('energy-z1-light').innerText = data.zone1.lighting;
    document.getElementById('energy-z1-hvac').innerText = data.zone1.hvac;

    document.getElementById('energy-z2-mode').innerText = data.zone2.ecoMode ? "ECO-SAVER (0 Occupants)" : "TASK ACTIVE (" + data.zone2.activeSeats + " Occupants)";
    document.getElementById('energy-z2-light').innerText = data.zone2.lighting;
    document.getElementById('energy-z2-hvac').innerText = data.zone2.hvac;

    data.seats.forEach(s => {
      currentSeatData[s.id] = s;
      const card = document.getElementById('desk-card-' + s.id);
      const statusBadge = document.getElementById('desk-status-' + s.id);
      const tag = document.getElementById('desk-tag-' + s.id);

      card.className = 'desk-card ' + s.status.toLowerCase();
      statusBadge.innerText = s.status;

      if (s.status === 'RESERVED') {
        const rem = Math.max(0, Math.floor(s.reserveRemaining || 0));
        const mins = Math.floor(rem / 60);
        const secs = rem % 60;
        tag.innerHTML = `<div class="bag-alert-tag">⏳ ${mins}:${secs < 10 ? '0' : ''}${secs} left</div>`;
      } else {
        tag.innerHTML = '';
      }
    });

    document.getElementById('lcd-line1').innerText = data.lcd.line1;
    document.getElementById('lcd-line2').innerText = data.lcd.line2;
  })
  .catch(err => console.error("Poll error:", err));
}

setInterval(fetchData, 1000);
fetchData();
</script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);

  // Initialize digital seat sensor inputs with internal pull-ups
  for (int i = 0; i < 7; i++) {
    pinMode(seats[i].pin, INPUT_PULLUP);
  }

  // Initialize LCD display
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SLIS Framework");
  lcd.setCursor(0, 1);
  lcd.print("Starting AP...");

  // Start Wi-Fi Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  Serial.println();
  Serial.println("=========================================");
  Serial.println("  SLIS - Smart Library Intelligence System");
  Serial.println("=========================================");
  Serial.print("Access Point SSID: ");
  Serial.println(ssid);
  Serial.print("Access Point IP  : ");
  Serial.println(WiFi.softAPIP());
  Serial.println("Hardware Channels: 7 Seats (Pins: 22,23,32,33,25,26,27)");
  Serial.println("Dual Zone Energy : Zone A (1-4), Zone B (5-7)");
  Serial.println("=========================================");

  // Web Server Endpoints
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/api/reserve", HTTP_POST, handleReserve);
  server.on("/api/release", HTTP_POST, handleRelease);
  server.on("/api/reset", HTTP_POST, handleReset);

  server.begin();
  delay(800);
  lcd.clear();
}

void loop() {
  server.handleClient();

  static unsigned long lastSensorPoll = 0;
  if (millis() - lastSensorPoll >= 250) {
    lastSensorPoll = millis();
    readSeats();
    updateLCD();
  }
}

void readSeats() {
  unsigned long now = millis();

  for (int i = 0; i < 7; i++) {
    // LOW means sensor triggered (circuit grounded by switch/IR)
    bool isTriggered = (digitalRead(seats[i].pin) == LOW);
    seats[i].occupied = isTriggered;

    // Auto-release expired reservations
    if (seats[i].reserved && now > seats[i].reserveExpiry && !seats[i].occupied) {
      seats[i].reserved = false;
      seats[i].reservedBy = "";
      Serial.print("[EXPIRED] Reservation auto-released for Seat ");
      Serial.println(seats[i].id);
    }
  }

  // Recalculate totals
  int occ = 0;
  int res = 0;
  for (int i = 0; i < 7; i++) {
    if (seats[i].occupied) occ++;
    else if (seats[i].reserved) res++;
  }
  totalOccupied = occ;
  totalReserved = res;
  totalAvailable = 7 - (occ + res);
}

void updateLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);

  // Line 1: Seats 1 to 4
  for (int i = 0; i < 4; i++) {
    lcd.print(String(i + 1) + ":");
    if (seats[i].occupied) lcd.print("O ");
    else if (seats[i].reserved) lcd.print("R ");
    else lcd.print("A ");
  }

  // Line 2: Seats 5 to 7
  lcd.setCursor(0, 1);
  for (int i = 4; i < 7; i++) {
    lcd.print(String(i + 1) + ":");
    if (seats[i].occupied) lcd.print("O ");
    else if (seats[i].reserved) lcd.print("R ");
    else lcd.print("A ");
  }

  // Telemetry output to Serial
  Serial.print("S1:"); Serial.print(seats[0].occupied);
  Serial.print(" S2:"); Serial.print(seats[1].occupied);
  Serial.print(" S3:"); Serial.print(seats[2].occupied);
  Serial.print(" S4:"); Serial.print(seats[3].occupied);
  Serial.print(" S5:"); Serial.print(seats[4].occupied);
  Serial.print(" S6:"); Serial.print(seats[5].occupied);
  Serial.print(" S7:"); Serial.print(seats[6].occupied);
  Serial.print(" | Avail:"); Serial.print(totalAvailable);
  Serial.print(" Occ:"); Serial.print(totalOccupied);
  Serial.print(" Res:"); Serial.println(totalReserved);
}

void handleRoot() {
  server.send(200, "text/html", HTML_PAGE);
}

void handleData() {
  unsigned long now = millis();

  // Zone 1 (Seats 0-3) and Zone 2 (Seats 4-6)
  int z1_active = 0;
  for (int i = 0; i < 4; i++) {
    if (seats[i].occupied || seats[i].reserved) z1_active++;
  }
  bool z1_eco = (z1_active == 0);

  int z2_active = 0;
  for (int i = 4; i < 7; i++) {
    if (seats[i].occupied || seats[i].reserved) z2_active++;
  }
  bool z2_eco = (z2_active == 0);

  int powerSaved = (z1_eco ? 180 : 0) + (z2_eco ? 150 : 0);
  int utilPct = (int)(((totalOccupied + totalReserved) * 100.0) / 7.0);

  String json = "{";
  json += "\"total\":7,";
  json += "\"occupied\":" + String(totalOccupied) + ",";
  json += "\"available\":" + String(totalAvailable) + ",";
  json += "\"reserved\":" + String(totalReserved) + ",";
  json += "\"utilization\":" + String(utilPct) + ",";
  json += "\"powerSavedWatts\":" + String(powerSaved) + ",";

  // Backward compatibility keys for original scripts
  for (int i = 0; i < 7; i++) {
    json += "\"seat" + String(i + 1) + "Occupied\":" + (seats[i].occupied ? "true," : "false,");
  }

  // Zone 1
  json += "\"zone1\":{";
  json += "\"name\":\"Zone A: Silent Study Pod\",";
  json += "\"activeSeats\":" + String(z1_active) + ",";
  json += "\"totalSeats\":4,";
  json += "\"ecoMode\":" + String(z1_eco ? "true" : "false") + ",";
  json += "\"lighting\":\"" + String(z1_eco ? "30% (Ambient Eco)" : "100% (Full Task)") + "\",";
  json += "\"hvac\":\"" + String(z1_eco ? "STANDBY" : "ACTIVE (22°C)") + "\"";
  json += "},";

  // Zone 2
  json += "\"zone2\":{";
  json += "\"name\":\"Zone B: Research & Tech Bay\",";
  json += "\"activeSeats\":" + String(z2_active) + ",";
  json += "\"totalSeats\":3,";
  json += "\"ecoMode\":" + String(z2_eco ? "true" : "false") + ",";
  json += "\"lighting\":\"" + String(z2_eco ? "25% (Ambient Eco)" : "100% (Full Task)") + "\",";
  json += "\"hvac\":\"" + String(z2_eco ? "STANDBY" : "ACTIVE (22°C)") + "\"";
  json += "},";

  // Seats detailed array
  json += "\"seats\":[";
  for (int i = 0; i < 7; i++) {
    String st = "AVAILABLE";
    long remSec = 0;
    if (seats[i].occupied) {
      st = "OCCUPIED";
    } else if (seats[i].reserved) {
      st = "RESERVED";
      if (seats[i].reserveExpiry > now) {
        remSec = (seats[i].reserveExpiry - now) / 1000;
      }
    }

    json += "{";
    json += "\"id\":" + String(seats[i].id) + ",";
    json += "\"gpio\":" + String(seats[i].pin) + ",";
    json += "\"zone\":" + String(seats[i].zone) + ",";
    json += "\"hardwareTriggered\":" + String(seats[i].occupied ? "true" : "false") + ",";
    json += "\"reserved\":" + String(seats[i].reserved ? "true" : "false") + ",";
    json += "\"reservedBy\":\"" + seats[i].reservedBy + "\",";
    json += "\"reserveRemaining\":" + String(remSec) + ",";
    json += "\"bagAlert\":false,";
    json += "\"status\":\"" + st + "\"";
    json += "}";
    if (i < 6) json += ",";
  }
  json += "],";

  // LCD Line representation
  String l1 = "1:" + String(seats[0].occupied ? "O" : (seats[0].reserved ? "R" : "A")) + " ";
  l1 += "2:" + String(seats[1].occupied ? "O" : (seats[1].reserved ? "R" : "A")) + " ";
  l1 += "3:" + String(seats[2].occupied ? "O" : (seats[2].reserved ? "R" : "A")) + " ";
  l1 += "4:" + String(seats[3].occupied ? "O" : (seats[3].reserved ? "R" : "A"));

  String l2 = "5:" + String(seats[4].occupied ? "O" : (seats[4].reserved ? "R" : "A")) + " ";
  l2 += "6:" + String(seats[5].occupied ? "O" : (seats[5].reserved ? "R" : "A")) + " ";
  l2 += "7:" + String(seats[6].occupied ? "O" : (seats[6].reserved ? "R" : "A"));

  json += "\"lcd\":{\"line1\":\"" + l1 + "\",\"line2\":\"" + l2 + "\"}";
  json += "}";

  server.send(200, "application/json", json);
}

void handleReserve() {
  int seatId = 0;
  String studentId = "STU-Guest";

  if (server.hasArg("seat")) {
    seatId = server.arg("seat").toInt();
  }
  if (server.hasArg("studentId")) {
    studentId = server.arg("studentId");
  }

  if (seatId >= 1 && seatId <= 7) {
    seats[seatId - 1].reserved = true;
    seats[seatId - 1].reservedBy = studentId;
    seats[seatId - 1].reserveExpiry = millis() + 900000; // 15 mins (900s)
    Serial.print("[RESERVE] Seat ");
    Serial.print(seatId);
    Serial.print(" held by ");
    Serial.println(studentId);
  }

  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handleRelease() {
  int seatId = 0;
  if (server.hasArg("seat")) {
    seatId = server.arg("seat").toInt();
  }

  if (seatId >= 1 && seatId <= 7) {
    seats[seatId - 1].reserved = false;
    seats[seatId - 1].reservedBy = "";
    seats[seatId - 1].reserveExpiry = 0;
    Serial.print("[RELEASE] Reservation canceled for Seat ");
    Serial.println(seatId);
  }

  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handleReset() {
  for (int i = 0; i < 7; i++) {
    seats[i].reserved = false;
    seats[i].reservedBy = "";
    seats[i].reserveExpiry = 0;
  }
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}
