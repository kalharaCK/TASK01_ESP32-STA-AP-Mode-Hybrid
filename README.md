# ESP32 Greenhouse Control System

A comprehensive IoT monitoring and automation system for greenhouse management, featuring WiFi configuration, environmental monitoring, and remote control capabilities.

## Development Status

### Completed
- ESP32 Hybrid operation
- Web dashboard with UI
- Dynamic WiFi configuration
- Real-time status monitoring
- Responsive design for mobile/desktop
- Simulated sensor data for testing

### In Progress
- **Sensor Integration**
  - DHT22 temperature/humidity sensor
  - Light intensity measurement
- **Actuator Control**
  - Ventilation fan control
  - LED grow light control
  - Heater/cooler control
- **Automation System**
  - Threshold-based automation

## Network Configuration

### Access Point (AP) Mode
- **SSID**: `ESP32-AccessPoint`
- **Password**: `12345678`
- **IP Address**: `192.168.4.1`
- **Purpose**: Always available for configuration
![SCREENSHOT](https://github.com/kalharaCK/TASK01_ESP32-STA-AP-Mode-Hybrid/blob/main/screenshot.jpg)


### Station (STA) Mode
- **Connects to**: Your home WiFi network
- **IP Address**: Assigned by your router (DHCP)
- **Purpose**: Internet connectivity and remote access

