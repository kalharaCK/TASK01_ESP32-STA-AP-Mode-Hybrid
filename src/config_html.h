#ifndef CONFIG_HTML_H
#define CONFIG_HTML_H

const char config_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>ESP32 Access Point Config</title>
  <style>
    body {
      background: #0f172a;
      font-family: Arial, sans-serif;
      display: flex;
      justify-content: center;
      align-items: center;
      height: 100vh;
      margin: 0;
    }
    .card {
      background: #1e293b;
      padding: 30px;
      border-radius: 15px;
      width: 320px;
      text-align: center;
      color: #fff;
      box-shadow: 0 8px 20px rgba(0,0,0,0.3);
    }
    .card h2 {
      margin-bottom: 20px;
      font-size: 20px;
    }
    label {
      display: block;
      text-align: left;
      font-size: 14px;
      margin: 10px 0 5px;
      color: #cbd5e1;
    }
    input {
      width: 100%;
      padding: 10px;
      border: none;
      border-radius: 8px;
      margin-bottom: 15px;
      background: #334155;
      color: #fff;
    }
    input:focus {
      outline: none;
      background: #475569;
    }
    .btn {
      width: 100%;
      padding: 12px;
      border: none;
      border-radius: 8px;
      cursor: pointer;
      font-weight: bold;
      margin-top: 5px;
    }
    .btn-connect {
      background: linear-gradient(90deg, #10b981, #3b82f6);
      color: #fff;
    }
  </style>
</head>
<body>
  <div class="card">
    <h2>WiFi Configuration</h2>
    <form action="/save" method="POST">
      <label for="ssid">Access Point SSID</label>
      <input type="text" id="ssid" name="ssid" placeholder="ESP32_AP" required>

      <label for="password">Access Point Password</label>
      <input type="password" id="password" name="password" placeholder="12345678" minlength="8">

      <button type="submit" class="btn btn-connect">SAVE & REBOOT</button>
    </form>
  </div>
</body>
</html>
)rawliteral";

#endif
