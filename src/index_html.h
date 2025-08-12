#ifndef INDEX_HTML_H
#define INDEX_HTML_H

#include <Arduino.h>

// Embedded HTML file as C array
const char index_html[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>Greenhouse Control Dashboard</title>
  <style>
    * {
      margin: 0;
      padding: 0;
      box-sizing: border-box;
    }

    :root {
      --primary-bg: #0a0f1c;
      --secondary-bg: #1a1f2e;
      --card-bg: #242837;
      --accent-green: #00ff88;
      --accent-blue: #0099ff;
      --accent-orange: #ff9500;
      --accent-red: #ff4444;
      --text-primary: #ffffff;
      --text-secondary: #b4bcd0;
      --border-color: #2d3748;
      --shadow-main: 0 8px 32px rgba(0, 255, 136, 0.1);
      --shadow-hover: 0 12px 48px rgba(0, 255, 136, 0.2);
      --gradient-main: linear-gradient(135deg, #0a0f1c 0%, #1a1f2e 100%);
      --gradient-card: linear-gradient(135deg, #242837 0%, #2d3748 100%);
      --gradient-accent: linear-gradient(135deg, #00ff88 0%, #0099ff 100%);
    }

    body {
      background: var(--gradient-main);
      color: var(--text-primary);
      font-family: 'Inter', -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
      min-height: 100vh;
      overflow-x: hidden;
    }

    .container {
      max-width: 1400px;
      margin: 0 auto;
      padding: 2rem;
    }

    .header {
      text-align: center;
      margin-bottom: 3rem;
      position: relative;
    }

    .header::before {
      content: '';
      position: absolute;
      top: -20px;
      left: 50%;
      transform: translateX(-50%);
      width: 100px;
      height: 4px;
      background: var(--gradient-accent);
      border-radius: 2px;
    }

    .header h1 {
      font-size: 3rem;
      font-weight: 700;
      background: var(--gradient-accent);
      -webkit-background-clip: text;
      -webkit-text-fill-color: transparent;
      background-clip: text;
      margin-bottom: 0.5rem;
      letter-spacing: -0.02em;
    }

    .header p {
      color: var(--text-secondary);
      font-size: 1.1rem;
      font-weight: 400;
    }

    .system-status {
      display: flex;
      justify-content: center;
      gap: 1rem;
      margin: 1rem 0;
      flex-wrap: wrap;
    }

    .status-chip {
      padding: 0.5rem 1rem;
      border-radius: 20px;
      font-size: 0.85rem;
      font-weight: 600;
      text-transform: uppercase;
      letter-spacing: 0.05em;
      display: flex;
      align-items: center;
      gap: 0.5rem;
      border: 2px solid transparent;
      transition: all 0.3s ease;
    }

    .status-chip.online {
      background: rgba(0, 255, 136, 0.1);
      border-color: var(--accent-green);
      color: var(--accent-green);
    }

    .status-chip.offline {
      background: rgba(255, 68, 68, 0.1);
      border-color: var(--accent-red);
      color: var(--accent-red);
    }

    .status-chip.connecting {
      background: rgba(255, 149, 0, 0.1);
      border-color: var(--accent-orange);
      color: var(--accent-orange);
    }

    .dashboard-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(320px, 1fr));
      gap: 2rem;
      margin-bottom: 2rem;
    }

    .card {
      background: var(--card-bg);
      border-radius: 16px;
      padding: 2rem;
      border: 1px solid var(--border-color);
      box-shadow: var(--shadow-main);
      backdrop-filter: blur(10px);
      transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
      position: relative;
      overflow: hidden;
    }

    .card::before {
      content: '';
      position: absolute;
      top: 0;
      left: 0;
      right: 0;
      height: 3px;
      background: var(--gradient-accent);
      opacity: 0.7;
    }

    .card:hover {
      transform: translateY(-4px);
      box-shadow: var(--shadow-hover);
      border-color: var(--accent-green);
    }

    .card-title {
      font-size: 1.5rem;
      font-weight: 600;
      margin-bottom: 1.5rem;
      color: var(--text-primary);
      display: flex;
      align-items: center;
      gap: 0.75rem;
    }

    .card-icon {
      width: 32px;
      height: 32px;
      background: var(--gradient-accent);
      border-radius: 8px;
      display: flex;
      align-items: center;
      justify-content: center;
      font-size: 1.2rem;
    }

    .sensor-grid {
      display: flex;
      justify-content: center;
      gap: 2rem;
      flex-wrap: wrap;
    }

    .gauge-container {
      position: relative;
      aspect-ratio: 1;
      width: 180px;
      height: 180px;
      margin: 0 auto;
    }

    .gauge {
      width: 100%;
      height: 100%;
      position: relative;
      border-radius: 50%;
      background: conic-gradient(from 0deg, var(--accent-green) 0%, var(--accent-blue) 70%, var(--border-color) 70%);
      padding: 6px;
      animation: gaugeRotate 4s ease-in-out infinite alternate;
    }

    @keyframes gaugeRotate {
      0% { transform: rotate(-1deg); }
      100% { transform: rotate(1deg); }
    }

    .gauge-inner {
      width: 100%;
      height: 100%;
      background: var(--card-bg);
      border-radius: 50%;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      position: relative;
    }

    .gauge-value {
      font-size: 1.8rem;
      font-weight: 700;
      background: var(--gradient-accent);
      -webkit-background-clip: text;
      -webkit-text-fill-color: transparent;
      background-clip: text;
      line-height: 1;
      min-height: 2rem;
      display: flex;
      align-items: center;
      justify-content: center;
      text-align: center;
    }

    .gauge-label {
      font-size: 0.8rem;
      color: var(--text-secondary);
      margin-top: 0.5rem;
      font-weight: 500;
      text-transform: uppercase;
      letter-spacing: 0.05em;
    }

    .gauge-trend {
      font-size: 0.7rem;
      color: var(--text-secondary);
      margin-top: 0.25rem;
      display: flex;
      align-items: center;
      gap: 0.25rem;
    }

    .temperature-card {
      grid-column: span 2;
    }

    .temperature-grid {
      display: flex;
      justify-content: space-around;
      gap: 1rem;
      flex-wrap: wrap;
    }

    .status-display {
      background: var(--secondary-bg);
      border-radius: 12px;
      padding: 1.5rem;
      border: 1px solid var(--border-color);
    }

    .status-item {
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding: 0.75rem 0;
      border-bottom: 1px solid var(--border-color);
    }

    .status-item:last-child {
      border-bottom: none;
    }

    .status-label {
      color: var(--text-secondary);
      font-weight: 500;
    }

    .status-value {
      color: var(--text-primary);
      font-weight: 600;
      display: flex;
      align-items: center;
      gap: 0.5rem;
    }

    .status-indicator {
      width: 8px;
      height: 8px;
      border-radius: 50%;
      animation: pulse 2s infinite;
    }

    .status-connected { background: var(--accent-green); }
    .status-disconnected { background: var(--accent-red); }
    .status-connecting { background: var(--accent-orange); }

    @keyframes pulse {
      0%, 100% { opacity: 1; transform: scale(1); }
      50% { opacity: 0.7; transform: scale(1.1); }
    }

    .form-group {
      margin-bottom: 1.5rem;
    }

    .form-label {
      display: block;
      margin-bottom: 0.5rem;
      color: var(--text-secondary);
      font-weight: 500;
      font-size: 0.9rem;
      text-transform: uppercase;
      letter-spacing: 0.05em;
    }

    .form-input {
      width: 100%;
      padding: 1rem;
      background: var(--secondary-bg);
      border: 2px solid var(--border-color);
      border-radius: 8px;
      color: var(--text-primary);
      font-size: 1rem;
      transition: all 0.3s ease;
    }

    .form-input:focus {
      outline: none;
      border-color: var(--accent-green);
      box-shadow: 0 0 0 3px rgba(0, 255, 136, 0.1);
    }

    .form-input::placeholder {
      color: var(--text-secondary);
      opacity: 0.7;
    }

    .form-select {
      width: 100%;
      padding: 1rem;
      background: var(--secondary-bg);
      border: 2px solid var(--border-color);
      border-radius: 8px;
      color: var(--text-primary);
      font-size: 1rem;
      transition: all 0.3s ease;
      cursor: pointer;
      appearance: none;
      background-image: url('data:image/svg+xml;charset=US-ASCII,<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 4 5"><path fill="%23b4bcd0" d="M2 5L0 3h4zm0-5L0 2h4z"/></svg>');
      background-repeat: no-repeat;
      background-position: right 1rem center;
      background-size: 12px;
    }

    .form-select:focus {
      outline: none;
      border-color: var(--accent-green);
      box-shadow: 0 0 0 3px rgba(0, 255, 136, 0.1);
    }

    .form-select option {
      background: var(--secondary-bg);
      color: var(--text-primary);
      padding: 0.5rem;
    }

    .refresh-button {
      display: inline-flex;
      align-items: center;
      gap: 0.5rem;
      padding: 0.5rem 1rem;
      background: var(--secondary-bg);
      border: 1px solid var(--border-color);
      border-radius: 6px;
      color: var(--text-secondary);
      font-size: 0.9rem;
      cursor: pointer;
      transition: all 0.3s ease;
      margin-bottom: 0.5rem;
    }

    .refresh-button:hover {
      border-color: var(--accent-green);
      color: var(--accent-green);
    }

    .refresh-button:disabled {
      opacity: 0.6;
      cursor: not-allowed;
    }

    .btn {
      padding: 1rem 2rem;
      border: none;
      border-radius: 8px;
      font-size: 1rem;
      font-weight: 600;
      cursor: pointer;
      transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
      position: relative;
      overflow: hidden;
      text-transform: uppercase;
      letter-spacing: 0.05em;
    }

    .btn::before {
      content: '';
      position: absolute;
      top: 0;
      left: -100%;
      width: 100%;
      height: 100%;
      background: linear-gradient(90deg, transparent, rgba(255, 255, 255, 0.2), transparent);
      transition: left 0.5s;
    }

    .btn:hover::before {
      left: 100%;
    }

    .btn-primary {
      background: var(--gradient-accent);
      color: var(--primary-bg);
    }

    .btn-primary:hover {
      transform: translateY(-2px);
      box-shadow: 0 8px 25px rgba(0, 255, 136, 0.3);
    }

    .btn-secondary {
      background: var(--secondary-bg);
      color: var(--text-primary);
      border: 2px solid var(--border-color);
    }

    .btn-secondary:hover {
      border-color: var(--accent-green);
      background: var(--card-bg);
    }

    .btn:disabled {
      opacity: 0.6;
      cursor: not-allowed;
      transform: none !important;
    }

    .btn-group {
      display: flex;
      gap: 1rem;
      flex-wrap: wrap;
    }

    .toggle-group {
      display: flex;
      gap: 1rem;
      margin-top: 1rem;
      flex-wrap: wrap;
    }

    .toggle {
      display: flex;
      align-items: center;
      gap: 0.75rem;
      padding: 0.75rem;
      background: var(--secondary-bg);
      border-radius: 8px;
      border: 1px solid var(--border-color);
      flex: 1;
      min-width: 200px;
    }

    .toggle-switch {
      width: 50px;
      height: 24px;
      background: var(--border-color);
      border-radius: 12px;
      position: relative;
      cursor: pointer;
      transition: background 0.3s ease;
    }

    .toggle-switch.active {
      background: var(--accent-green);
    }

    .toggle-slider {
      width: 20px;
      height: 20px;
      background: white;
      border-radius: 50%;
      position: absolute;
      top: 2px;
      left: 2px;
      transition: transform 0.3s ease;
      box-shadow: 0 2px 4px rgba(0,0,0,0.2);
    }

    .toggle-switch.active .toggle-slider {
      transform: translateX(26px);
    }

    .message {
      padding: 1rem;
      border-radius: 8px;
      margin-top: 1rem;
      border-left: 4px solid;
      animation: slideIn 0.3s ease;
      position: relative;
    }

    @keyframes slideIn {
      from { opacity: 0; transform: translateY(-10px); }
      to { opacity: 1; transform: translateY(0); }
    }

    .message.success {
      background: rgba(0, 255, 136, 0.1);
      border-color: var(--accent-green);
      color: var(--accent-green);
    }

    .message.error {
      background: rgba(255, 68, 68, 0.1);
      border-color: var(--accent-red);
      color: var(--accent-red);
    }

    .message.info {
      background: rgba(0, 153, 255, 0.1);
      border-color: var(--accent-blue);
      color: var(--accent-blue);
    }

    .message.warning {
      background: rgba(255, 149, 0, 0.1);
      border-color: var(--accent-orange);
      color: var(--accent-orange);
    }

    .loading {
      display: inline-block;
      width: 20px;
      height: 20px;
      border: 3px solid var(--border-color);
      border-top: 3px solid var(--accent-green);
      border-radius: 50%;
      animation: spin 1s linear infinite;
    }

    @keyframes spin {
      0% { transform: rotate(0deg); }
      100% { transform: rotate(360deg); }
    }

    .data-row {
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding: 0.5rem 0;
      border-bottom: 1px solid var(--border-color);
    }

    .data-row:last-child {
      border-bottom: none;
    }

    .data-label {
      color: var(--text-secondary);
      font-size: 0.9rem;
    }

    .data-value {
      color: var(--text-primary);
      font-weight: 600;
    }

    .signal-strength {
      display: flex;
      gap: 2px;
      align-items: flex-end;
    }

    .signal-bar {
      width: 4px;
      height: 8px;
      background: var(--border-color);
      border-radius: 1px;
    }

    .signal-bar.active {
      background: var(--accent-green);
    }

    .signal-bar:nth-child(2) { height: 12px; }
    .signal-bar:nth-child(3) { height: 16px; }
    .signal-bar:nth-child(4) { height: 20px; }

    @media (max-width: 768px) {
      .container {
        padding: 1rem;
      }

      .header h1 {
        font-size: 2rem;
      }

      .dashboard-grid {
        grid-template-columns: 1fr;
        gap: 1rem;
      }

      .card {
        padding: 1.5rem;
      }

      .btn-group {
        flex-direction: column;
      }

      .btn {
        width: 100%;
      }

      .sensor-grid {
        gap: 1rem;
      }

      .gauge-container {
        width: 150px;
        height: 150px;
      }

      .system-status {
        flex-direction: column;
      }

      .temperature-card {
        grid-column: span 1;
      }
    }
  </style>
</head>
<body>
  <div class="container">
    <div class="header">
      <h1>Greenhouse Control</h1>
      <p>IoT Monitoring & Automation System</p>
      <div class="system-status" id="systemStatus">
        <div class="status-chip connecting">
          <div class="loading"></div>
          Initializing...
        </div>
      </div>
    </div>

    <div class="dashboard-grid">
      <div class="card temperature-card">
        <h2 class="card-title">
          <div class="card-icon"></div>
          Environmental Sensors
        </h2>
        <div class="temperature-grid">
          <div class="gauge-container">
            <div class="gauge">
              <div class="gauge-inner">
                <div class="gauge-value" id="temperatureValue">--°C</div>
                <div class="gauge-label">Temperature</div>
                <div class="gauge-trend" id="tempTrend">
                  <span>--</span>
                </div>
              </div>
            </div>
          </div>
          <div class="gauge-container">
            <div class="gauge">
              <div class="gauge-inner">
                <div class="gauge-value" id="humidityValue">--%</div>
                <div class="gauge-label">Humidity</div>
                <div class="gauge-trend" id="humidityTrend">
                  <span>--</span>
                </div>
              </div>
            </div>
          </div>
          <div class="gauge-container">
            <div class="gauge">
              <div class="gauge-inner">
                <div class="gauge-value" id="lightValue">-- lx</div>
                <div class="gauge-label">Light Level</div>
                <div class="gauge-trend" id="lightTrend">
                  <span>--</span>
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>

      <div class="card">
        <h2 class="card-title">
          <div class="card-icon"></div>
          Network Status
        </h2>
        <div class="status-display" id="wifiStatusDisplay">
          <div class="status-item">
            <span class="status-label">Checking connection...</span>
            <span class="status-value"><div class="loading"></div></span>
          </div>
        </div>
      </div>

      <div class="card">
        <h2 class="card-title">
          <div class="card-icon"></div>
          WiFi Configuration
        </h2>
        <div class="form-group">
          <label class="form-label" for="wifiNetworkSelect">Select WiFi Network</label>
          <button class="refresh-button" onclick="scanWiFiNetworks()" id="scanBtn">
             Scan Networks
          </button>
          <select id="wifiNetworkSelect" class="form-select">
            <option value="">Click "Scan Networks" to find available networks...</option>
          </select>
        </div>
        <div class="form-group">
          <label class="form-label" for="PW">Password</label>
          <input type="password" id="PW" class="form-input" placeholder="Enter WiFi password (leave empty for open networks)" />
        </div>
        <div class="btn-group">
          <button class="btn btn-primary" onclick="connectToWiFi()" id="connectBtn">
            Connect Network
          </button>
          <button class="btn btn-secondary" onclick="disconnectWiFi()" id="disconnectBtn">
            Disconnect
          </button>
        </div>
        <div id="connectionMessage"></div>
      </div>

      <div class="card">
        <h2 class="card-title">
          <div class="card-icon"></div>
          Automation Thresholds
        </h2>
        <div class="form-group">
          <label class="form-label" for="tempMin">Min Temperature (°C)</label>
          <input type="number" id="tempMin" class="form-input" placeholder="e.g. 18" min="0" max="50" />
        </div>
        <div class="form-group">
          <label class="form-label" for="tempMax">Max Temperature (°C)</label>
          <input type="number" id="tempMax" class="form-input" placeholder="e.g. 28" min="0" max="50" />
        </div>
        <div class="form-group">
          <label class="form-label" for="humidityThreshold">Humidity Threshold (%)</label>
          <input type="number" id="humidityThreshold" class="form-input" placeholder="e.g. 70" min="0" max="100" />
        </div>
        <div class="form-group">
          <label class="form-label" for="lightThreshold">Light Level Threshold (lx)</label>
          <input type="number" id="lightThreshold" class="form-input" placeholder="e.g. 1000" min="0" max="10000" />
        </div>
        <button class="btn btn-primary" onclick="saveThresholds()">Save Thresholds</button>
        <div id="thresholdMessage"></div>
      </div>

      <div class="card">
        <h2 class="card-title">
          <div class="card-icon"></div>
          Automation Control
        </h2>
        <div class="toggle-group">
          <div class="toggle">
            <div class="toggle-switch" id="tempToggle" onclick="toggleAutoTemperature()">
              <div class="toggle-slider"></div>
            </div>
            <span>Auto Temperature Control</span>
          </div>
          <div class="toggle">
            <div class="toggle-switch" id="humidityToggle" onclick="toggleAutoHumidity()">
              <div class="toggle-slider"></div>
            </div>
            <span>Auto Humidity Control</span>
          </div>
        </div>
        <div class="toggle-group">
          <div class="toggle">
            <div class="toggle-switch" id="lightToggle" onclick="toggleAutoLight()">
              <div class="toggle-slider"></div>
            </div>
            <span>Auto Light Control</span>
          </div>
          <div class="toggle">
            <div class="toggle-switch" id="irrigationToggle" onclick="toggleAutoIrrigation()">
              <div class="toggle-slider"></div>
            </div>
            <span>Auto Irrigation System</span>
          </div>
        </div>
        <div id="controlMessage"></div>
      </div>

      <div class="card">
        <h2 class="card-title">
          <div class="card-icon"></div>
          System Information
        </h2>
        <div class="status-display" id="systemInfo">
          <div class="data-row">
            <span class="data-label">System Uptime</span>
            <span class="data-value" id="uptime">--</span>
          </div>
          <div class="data-row">
            <span class="data-label">Memory Usage</span>
            <span class="data-value" id="memory">--</span>
          </div>
          <div class="data-row">
            <span class="data-label">Last Update</span>
            <span class="data-value" id="lastUpdate">--</span>
          </div>
          <div class="data-row">
            <span class="data-label">Available Networks</span>
            <span class="data-value" id="networkCount">--</span>
          </div>
        </div>
      </div>
    </div>
  </div>

  <script>
    let systemState = {
      sensors: {
        temperature: { current: null, previous: null, trend: 'stable' },
        humidity: { current: null, previous: null, trend: 'stable' },
        light: { current: null, previous: null, trend: 'stable' }
      },
      automation: {
        temperature: false,
        humidity: false,
        light: false,
        irrigation: false
      },
      thresholds: {
        tempMin: 18,
        tempMax: 28,
        humidity: 70,
        light: 1000
      },
      connection: {
        status: 'connecting',
        lastUpdate: null,
        retryCount: 0
      },
      system: {
        uptime: 0,
        dataPoints: 0,
        startTime: Date.now()
      },
      availableNetworks: [],
      scanInProgress: false
    };

    let scanRetryCount = 0;
    const maxScanRetries = 3;
    const scanRetryDelay = 5000;

    function autoRetryScan() {
      if (scanRetryCount < maxScanRetries && systemState.availableNetworks.length === 0 && !systemState.scanInProgress) {
        scanRetryCount++;
        console.log(`Auto-retrying scan (attempt ${scanRetryCount}/${maxScanRetries})`);
        
        showMessage(`Retrying network scan (${scanRetryCount}/${maxScanRetries})...`, "warning", "connectionMessage");
        
        setTimeout(() => {
          if (!systemState.scanInProgress) {
            scanWiFiNetworks();
          }
        }, scanRetryDelay);
      } else if (scanRetryCount >= maxScanRetries) {
        console.log('Max scan retries reached. Stopping auto-retry.');
        showMessage("Unable to scan networks after multiple attempts. Please try manual scan.", "error", "connectionMessage");
        
        setTimeout(() => {
          scanRetryCount = 0;
          console.log('Scan retry count reset. Auto-retry available again.');
        }, 30000);
      }
    }

    function resetScanRetryCount() {
      if (scanRetryCount > 0) {
        console.log(`Scan successful! Reset retry count from ${scanRetryCount} to 0`);
        scanRetryCount = 0;
      }
    }

    document.addEventListener('DOMContentLoaded', function() {
      console.log('Dashboard initializing...');
      updateSystemStatus('connecting', 'Initializing system...');
      startSensorSimulation();
      loadStoredSettings();
      
      setInterval(updateSensors, 3000);
      setInterval(updateWiFiStatus, 5000);
      setInterval(updateSystemInfo, 1000);
      
      updateWiFiStatus();
      updateSystemInfo();
      
      setTimeout(() => {
        scanWiFiNetworks();
        setTimeout(() => {
          autoRetryScan();
        }, 10000);
      }, 3000);
    });

    function scanWiFiNetworks() {
      const scanBtn = document.getElementById('scanBtn');
      const select = document.getElementById('wifiNetworkSelect');
      
      if (systemState.scanInProgress) {
        console.log('Scan already in progress');
        return;
      }
      
      systemState.scanInProgress = true;
      scanBtn.disabled = true;
      scanBtn.innerHTML = '<div class="loading"></div> Starting Scan...';
      
      select.innerHTML = '<option value="">Starting network scan...</option>';
      showMessage("Starting WiFi network scan...", "info", "connectionMessage");
      
      console.log('Starting async network scan...');
      
      fetch('/api/wifi/scan')
        .then(response => {
          console.log('Scan start response status:', response.status);
          if (response.status === 202) {
            scanBtn.innerHTML = '<div class="loading"></div> Scanning...';
            select.innerHTML = '<option value="">Scanning for networks...</option>';
            showMessage("Scanning in progress, please wait...", "info", "connectionMessage");
            
            pollScanResults();
          } else if (response.status === 429) {
            throw new Error('Please wait before scanning again');
          } else if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
          } else {
            return response.json();
          }
        })
        .then(data => {
          if (data) {
            processScanResults(data);
          }
        })
        .catch(error => {
          console.error('Error starting WiFi scan:', error);
          handleScanError(error.message);
        });
    }

    function handleScanError(errorMessage) {
      systemState.scanInProgress = false;
      const scanBtn = document.getElementById('scanBtn');
      const select = document.getElementById('wifiNetworkSelect');
      
      scanBtn.disabled = false;
      scanBtn.innerHTML = ' Scan Networks';
      
      select.innerHTML = '<option value="">Error starting scan - Click to retry...</option>';
      showMessage("Scan error: " + errorMessage, "error", "connectionMessage");
      
      setTimeout(() => {
        autoRetryScan();
      }, 3000);
    }

    function pollScanResults() {
      const maxAttempts = 25;
      let attempts = 0;
      
      const checkResults = () => {
        attempts++;
        console.log(`Checking scan results, attempt ${attempts}/${maxAttempts}`);
        
        fetch('/api/wifi/scan/results')
          .then(response => {
            if (response.status === 202) {
              if (attempts < maxAttempts) {
                setTimeout(checkResults, 1000);
              } else {
                throw new Error('Scan timeout - taking too long (watchdog protection)');
              }
            } else if (response.ok) {
              return response.json();
            } else {
              throw new Error(`HTTP error! status: ${response.status}`);
            }
          })
          .then(data => {
            if (data) {
              processScanResults(data);
            }
          })
          .catch(error => {
            console.error('Error getting scan results:', error);
            handleScanError(error.message);
          });
      };
      
      setTimeout(checkResults, 2000);
    }
    
    function processScanResults(data) {
      const scanBtn = document.getElementById('scanBtn');
      const select = document.getElementById('wifiNetworkSelect');
      
      systemState.scanInProgress = false;
      scanBtn.disabled = false;
      scanBtn.innerHTML = ' Scan Networks';
      
      console.log('Processing scan results:', data);
      
      if (data.status === 'success' && data.networks && Array.isArray(data.networks)) {
        systemState.availableNetworks = data.networks;
        updateNetworkDropdown();
        
        const networkCount = data.networks.length;
        document.getElementById('networkCount').textContent = networkCount;
        showMessage(`Found ${networkCount} network(s)`, "success", "connectionMessage");
        console.log(`Successfully found ${networkCount} networks`);
        
        resetScanRetryCount();
        
      } else if (data.status === 'success' && data.count === 0) {
        console.log('No networks found');
        systemState.availableNetworks = [];
        updateNetworkDropdown();
        showMessage("No networks found in range", "warning", "connectionMessage");
        
        setTimeout(() => {
          autoRetryScan();
        }, 3000);
        
      } else if (data.status === 'error') {
        console.log('Scan failed with error:', data);
        systemState.availableNetworks = [];
        updateNetworkDropdown();
        const errorMsg = data.message || "Scan failed - possible watchdog timeout";
        showMessage(errorMsg, "error", "connectionMessage");
        
        setTimeout(() => {
          autoRetryScan();
        }, 3000);
        
      } else {
        console.log('Invalid scan response:', data);
        systemState.availableNetworks = [];
        updateNetworkDropdown();
        showMessage("Invalid scan response received", "error", "connectionMessage");
        
        setTimeout(() => {
          autoRetryScan();
        }, 3000);
      }
    }

    function updateNetworkDropdown() {
      const select = document.getElementById('wifiNetworkSelect');
      
      console.log('Updating network dropdown with', systemState.availableNetworks.length, 'networks');
      
      select.innerHTML = '';
      
      const defaultOption = document.createElement('option');
      defaultOption.value = '';
      defaultOption.textContent = 'Select a network...';
      select.appendChild(defaultOption);
      
      if (systemState.availableNetworks.length === 0) {
        const noNetworkOption = document.createElement('option');
        noNetworkOption.value = '';
        noNetworkOption.textContent = 'No networks found - Click scan to refresh';
        noNetworkOption.disabled = true;
        select.appendChild(noNetworkOption);
        return;
      }
      
      const sortedNetworks = [...systemState.availableNetworks].sort((a, b) => b.rssi - a.rssi);
      
      sortedNetworks.forEach((network, index) => {
        if (network.ssid && network.ssid.trim() !== '') {
          const option = document.createElement('option');
          option.value = network.ssid;
          
          const signalStrength = getSignalStrengthText(network.rssi);
          const securityIcon = network.encryption === 'Open' ? '' : '';
          
      

        const ssidWidth = 20;
        const rssiWidth = 5;

        const ssidPadded = network.ssid.padEnd(ssidWidth, ' ');
        const rssiPadded = String(signalStrength).padStart(rssiWidth, ' ');

        option.textContent = `${ssidPadded} ${securityIcon} (${rssiPadded})`;
          

          option.dataset.rssi = network.rssi;
          option.dataset.encryption = network.encryption;
          option.dataset.encrypted = network.encrypted;
          
          select.appendChild(option);
          
          console.log(`Added network ${index + 1}: ${network.ssid} (${signalStrength}, ${network.encryption})`);
        }
      });
      
      console.log(`Dropdown updated with ${select.options.length - 1} networks`);
    }

    function getSignalStrengthText(rssi) {
      if (rssi >= -50) return 'Excellent';
      if (rssi >= -60) return 'Very Good';
      if (rssi >= -70) return 'Good';
      if (rssi >= -80) return 'Fair';
      return 'Weak';
    }

    function connectToWiFi() {
      const selectedNetwork = document.getElementById("wifiNetworkSelect").value.trim();
      const password = document.getElementById("PW").value.trim();
      const connectBtn = document.getElementById("connectBtn");
      
      if (!selectedNetwork) {
        showMessage("Please select a WiFi network from the dropdown", "error", "connectionMessage");
        return;
      }
      
      console.log('Attempting to connect to:', selectedNetwork);
      
      const selectedOption = document.getElementById("wifiNetworkSelect").selectedOptions[0];
      const isOpenNetwork = selectedOption && selectedOption.dataset.encrypted === 'false';
      
      console.log('Network is open:', isOpenNetwork);
      
      if (!isOpenNetwork && !password) {
        showMessage("Please enter the WiFi password", "error", "connectionMessage");
        return;
      }
      
      connectBtn.disabled = true;
      connectBtn.innerHTML = '<div class="loading"></div> Connecting...';
      showMessage(`Connecting to ${selectedNetwork}...`, "info", "connectionMessage");
      updateSystemStatus('connecting', `Connecting to ${selectedNetwork}...`);
      
      const data = { 
        ssid: selectedNetwork, 
        password: isOpenNetwork ? "" : password 
      };
      
      console.log('Sending connection request:', { ssid: selectedNetwork, password: isOpenNetwork ? "[OPEN]" : "[HIDDEN]" });
      
      fetch('/api/wifi/connect', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(data)
      })
      .then(response => {
        console.log('Connection response status:', response.status);
        if (!response.ok) {
          throw new Error(`HTTP error! status: ${response.status}`);
        }
        return response.json();
      })
      .then(result => {
        console.log('Connection result:', result);
        showMessage(result.message, "info", "connectionMessage");
        
        setTimeout(() => {
          updateWiFiStatus();
          connectBtn.disabled = false;
          connectBtn.textContent = "Connect Network";
        }, 15000);
      })
      .catch(error => {
        console.error('Error connecting to WiFi:', error);
        showMessage("Error connecting to WiFi: " + error.message, "error", "connectionMessage");
        connectBtn.disabled = false;
        connectBtn.textContent = "Connect Network";
        updateSystemStatus('offline', 'Connection failed');
      });
    }

    function disconnectWiFi() {
      const disconnectBtn = document.getElementById("disconnectBtn");
      
      disconnectBtn.disabled = true;
      disconnectBtn.innerHTML = '<div class="loading"></div> Disconnecting...';
      updateSystemStatus('connecting', 'Disconnecting...');
      
      fetch('/api/wifi/disconnect', { method: 'POST' })
        .then(response => {
          if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
          }
          return response.json();
        })
        .then(result => {
          console.log('Disconnect result:', result);
          showMessage(result.message, "info", "connectionMessage");
          updateWiFiStatus();
          disconnectBtn.disabled = false;
          disconnectBtn.textContent = "Disconnect";
        })
        .catch(error => {
          console.error('Error disconnecting from WiFi:', error);
          showMessage("Error disconnecting from WiFi: " + error.message, "error", "connectionMessage");
          disconnectBtn.disabled = false;
          disconnectBtn.textContent = "Disconnect";
        });
    }

    function startSensorSimulation() {
      systemState.sensors.temperature.current = 22 + Math.random() * 6;
      systemState.sensors.humidity.current = 60 + Math.random() * 20;
      systemState.sensors.light.current = 500 + Math.random() * 1000;
      
      updateSensorDisplays();
    }

    function updateSensors() {
      const temp = systemState.sensors.temperature;
      const humidity = systemState.sensors.humidity;
      const light = systemState.sensors.light;
      
      temp.previous = temp.current;
      humidity.previous = humidity.current;
      light.previous = light.current;
      
      temp.current = Math.max(15, Math.min(35, temp.current + (Math.random() - 0.5) * 2));
      humidity.current = Math.max(30, Math.min(90, humidity.current + (Math.random() - 0.5) * 5));
      light.current = Math.max(0, Math.min(2000, light.current + (Math.random() - 0.5) * 200));
      
      temp.trend = calculateTrend(temp.previous, temp.current);
      humidity.trend = calculateTrend(humidity.previous, humidity.current);
      light.trend = calculateTrend(light.previous, light.current);
      
      systemState.system.dataPoints++;
      systemState.connection.lastUpdate = new Date();
      
      updateSensorDisplays();
      checkThresholds();
    }

    function calculateTrend(previous, current) {
      if (previous === null) return 'stable';
      const diff = current - previous;
      if (Math.abs(diff) < 0.5) return 'stable';
      return diff > 0 ? 'rising' : 'falling';
    }

    function updateSensorDisplays() {
      const temp = systemState.sensors.temperature;
      const humidity = systemState.sensors.humidity;
      const light = systemState.sensors.light;
      
      document.getElementById("temperatureValue").textContent = `${temp.current.toFixed(1)}°C`;
      document.getElementById("humidityValue").textContent = `${humidity.current.toFixed(0)}%`;
      document.getElementById("lightValue").textContent = `${light.current.toFixed(0)} lx`;
      
      document.getElementById("tempTrend").innerHTML = getTrendIcon(temp.trend);
      document.getElementById("humidityTrend").innerHTML = getTrendIcon(humidity.trend);
      document.getElementById("lightTrend").innerHTML = getTrendIcon(light.trend);
      
      updateSystemStatus('online', 'All systems operational');
    }

    function getTrendIcon(trend) {
      switch(trend) {
        case 'rising': return '<span style="color: var(--accent-orange);">↗ Rising</span>';
        case 'falling': return '<span style="color: var(--accent-blue);">↘ Falling</span>';
        default: return '<span style="color: var(--text-secondary);">→ Stable</span>';
      }
    }

    function checkThresholds() {
      const alerts = [];
      const temp = systemState.sensors.temperature.current;
      const humidity = systemState.sensors.humidity.current;
      const light = systemState.sensors.light.current;
      
      if (temp < systemState.thresholds.tempMin || temp > systemState.thresholds.tempMax) {
        alerts.push(`Temperature ${temp.toFixed(1)}°C is outside optimal range`);
      }
      
      if (humidity > systemState.thresholds.humidity) {
        alerts.push(`Humidity ${humidity.toFixed(0)}% exceeds threshold`);
      }
      
      if (light < systemState.thresholds.light) {
        alerts.push(`Light level ${light.toFixed(0)} lx below threshold`);
      }
      
      if (alerts.length > 0 && systemState.automation.temperature) {
        console.log('Auto-adjusting climate control...');
      }
    }

    function updateSystemStatus(status, message) {
      const statusElement = document.getElementById('systemStatus');
      let statusClass = status;
      let icon = '';
      
      switch(status) {
        case 'online':
          icon = '🟢';
          break;
        case 'offline':
          icon = '🔴';
          break;
        case 'connecting':
          icon = '🟡';
          break;
        default:
          icon = '⚪';
      }
      
      statusElement.innerHTML = `
        <div class="status-chip ${statusClass}">
          ${status === 'connecting' ? '<div class="loading"></div>' : icon}
          ${message}
        </div>
      `;
    }

    function updateSystemInfo() {
      const now = Date.now();
      const uptime = Math.floor((now - systemState.system.startTime) / 1000);
      const hours = Math.floor(uptime / 3600);
      const minutes = Math.floor((uptime % 3600) / 60);
      const seconds = uptime % 60;
      
      document.getElementById('uptime').textContent = `${hours}h ${minutes}m ${seconds}s`;
      document.getElementById('memory').textContent = '78% (simulated)';
      document.getElementById('lastUpdate').textContent = 
        systemState.connection.lastUpdate ? systemState.connection.lastUpdate.toLocaleTimeString() : '--';
      
      if (document.getElementById('networkCount').textContent === '--') {
        document.getElementById('networkCount').textContent = systemState.availableNetworks.length;
      }
    }

    function updateWiFiStatus() {
      fetch('/api/wifi/status')
        .then(response => {
          if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
          }
          return response.json();
        })
        .then(data => {
          console.log('WiFi status:', data);
          const statusDisplay = document.getElementById('wifiStatusDisplay');
          let statusHTML = '';
          
          statusHTML += `
            <div class="status-item">
              <span class="status-label">Access Point</span>
              <span class="status-value">
                <div class="status-indicator status-connected"></div>
                ${data.ap.ssid} (${data.ap.connected_clients} devices)
              </span>
            </div>
            <div class="status-item">
              <span class="status-label">AP IP Address</span>
              <span class="status-value">${data.ap.ip}</span>
            </div>
          `;
          
          if (data.sta.connected) {
            const signalBars = getSignalBars(data.sta.rssi);
            statusHTML += `
              <div class="status-item">
                <span class="status-label">Internet Connection</span>
                <span class="status-value">
                  <div class="status-indicator status-connected"></div>
                  Connected to ${data.sta.ssid}
                </span>
              </div>
              <div class="status-item">
                <span class="status-label">IP Address</span>
                <span class="status-value">${data.sta.ip}</span>
              </div>
              <div class="status-item">
                <span class="status-label">Signal Strength</span>
                <span class="status-value">
                  ${signalBars}
                  ${data.sta.rssi} dBm
                </span>
              </div>
            `;
          } else {
            statusHTML += `
              <div class="status-item">
                <span class="status-label">Internet Connection</span>
                <span class="status-value">
                  <div class="status-indicator status-disconnected"></div>
                  Not connected
                </span>
              </div>
            `;
          }
          
          statusDisplay.innerHTML = statusHTML;
          systemState.connection.retryCount = 0;
        })
        .catch(error => {
          console.error('Error fetching WiFi status:', error);
          systemState.connection.retryCount++;
          
          if (systemState.connection.retryCount > 3) {
            updateSystemStatus('offline', 'Connection lost');
          }
          
          document.getElementById('wifiStatusDisplay').innerHTML = `
            <div class="status-item">
              <span class="status-label">Status</span>
              <span class="status-value">
                <div class="status-indicator status-disconnected"></div>
                Error fetching status (Retry ${systemState.connection.retryCount})
              </span>
            </div>
          `;
        });
    }

    function getSignalBars(rssi) {
      const strength = Math.max(0, Math.min(4, Math.floor((rssi + 100) / 12.5)));
      let bars = '<div class="signal-strength">';
      for (let i = 1; i <= 4; i++) {
        bars += `<div class="signal-bar ${i <= strength ? 'active' : ''}"></div>`;
      }
      bars += '</div>';
      return bars;
    }

    function toggleAutoTemperature() {
      systemState.automation.temperature = !systemState.automation.temperature;
      const toggle = document.getElementById('tempToggle');
      toggle.classList.toggle('active', systemState.automation.temperature);
      showMessage(`Auto Temperature Control: ${systemState.automation.temperature ? 'ON' : 'OFF'}`, 'success', 'controlMessage');
      saveSettings();
    }

    function toggleAutoHumidity() {
      systemState.automation.humidity = !systemState.automation.humidity;
      const toggle = document.getElementById('humidityToggle');
      toggle.classList.toggle('active', systemState.automation.humidity);
      showMessage(`Auto Humidity Control: ${systemState.automation.humidity ? 'ON' : 'OFF'}`, 'success', 'controlMessage');
      saveSettings();
    }

    function toggleAutoLight() {
      systemState.automation.light = !systemState.automation.light;
      const toggle = document.getElementById('lightToggle');
      toggle.classList.toggle('active', systemState.automation.light);
      showMessage(`Auto Light Control: ${systemState.automation.light ? 'ON' : 'OFF'}`, 'success', 'controlMessage');
      saveSettings();
    }

    function toggleAutoIrrigation() {
      systemState.automation.irrigation = !systemState.automation.irrigation;
      const toggle = document.getElementById('irrigationToggle');
      toggle.classList.toggle('active', systemState.automation.irrigation);
      showMessage(`Auto Irrigation System: ${systemState.automation.irrigation ? 'ON' : 'OFF'}`, 'success', 'controlMessage');
      saveSettings();
    }

    function saveThresholds() {
      const tempMin = document.getElementById('tempMin').value;
      const tempMax = document.getElementById('tempMax').value;
      const humidity = document.getElementById('humidityThreshold').value;
      const light = document.getElementById('lightThreshold').value;
      
      let updated = false;
      
      if (tempMin && !isNaN(tempMin)) {
        systemState.thresholds.tempMin = parseFloat(tempMin);
        updated = true;
      }
      
      if (tempMax && !isNaN(tempMax)) {
        systemState.thresholds.tempMax = parseFloat(tempMax);
        updated = true;
      }
      
      if (humidity && !isNaN(humidity)) {
        systemState.thresholds.humidity = parseFloat(humidity);
        updated = true;
      }
      
      if (light && !isNaN(light)) {
        systemState.thresholds.light = parseFloat(light);
        updated = true;
      }
      
      if (updated) {
        showMessage('Thresholds saved successfully!', 'success', 'thresholdMessage');
        saveSettings();
      } else {
        showMessage('Please enter at least one threshold value', 'error', 'thresholdMessage');
      }
    }

    function saveSettings() {
      const settings = {
        automation: systemState.automation,
        thresholds: systemState.thresholds
      };
      
      window.greenhouseSettings = settings;
      console.log('Settings saved:', settings);
    }

    function loadStoredSettings() {
      const stored = window.greenhouseSettings;
      if (stored) {
        systemState.automation = { ...systemState.automation, ...stored.automation };
        systemState.thresholds = { ...systemState.thresholds, ...stored.thresholds };
        
        Object.keys(systemState.automation).forEach(key => {
          const toggle = document.getElementById(key + 'Toggle');
          if (toggle) {
            toggle.classList.toggle('active', systemState.automation[key]);
          }
        });
        
        Object.keys(systemState.thresholds).forEach(key => {
          const input = document.getElementById(key === 'tempMin' ? 'tempMin' : 
                                                 key === 'tempMax' ? 'tempMax' :
                                                 key === 'humidity' ? 'humidityThreshold' : 
                                                 key === 'light' ? 'lightThreshold' : null);
          if (input) {
            input.value = systemState.thresholds[key];
          }
        });
        console.log('Stored settings loaded:', stored);
      }
    }

    function showMessage(message, type, containerId) {
      const container = document.getElementById(containerId);
      if (!container) return;
      
      console.log(`Message [${type}]:`, message);
      container.innerHTML = `<div class="message ${type}">${message}</div>`;
      
      setTimeout(() => {
        container.innerHTML = "";
      }, 5000);
    }

    document.addEventListener('keydown', function(e) {
      if (e.ctrlKey && e.key === 'r') {
        e.preventDefault();
        location.reload();
      }
      
      if (e.key === 'Escape') {
        ['connectionMessage', 'thresholdMessage', 'controlMessage'].forEach(id => {
          const el = document.getElementById(id);
          if (el) el.innerHTML = '';
        });
      }
      
      if (e.ctrlKey && e.key === 's') {
        e.preventDefault();
        scanWiFiNetworks();
      }
    });

    window.addEventListener('error', function(e) {
      console.error('Global error:', e.error);
      updateSystemStatus('offline', 'System error detected');
    });

    document.addEventListener('visibilitychange', function() {
      if (document.visibilityState === 'visible') {
        console.log('Tab visible, resuming updates...');
        updateWiFiStatus();
        updateSensors();
      }
    });

    setInterval(() => {
      if (systemState.availableNetworks.length === 0 && 
          !systemState.scanInProgress && 
          scanRetryCount < maxScanRetries) {
        console.log('Auto-scanning for networks (no networks available)');
        scanWiFiNetworks();
      }
    }, 45000);
  </script>
</body>
</html>)rawliteral";

// Get the length of the HTML content for Content-Length header
const size_t index_html_len = sizeof(index_html) - 1;

#endif // INDEX_HTML_H