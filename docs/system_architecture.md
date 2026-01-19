# System Architecture — Alynx NTP WiFi LCD Clock

## 1. Overview
The Alynx NTP WiFi LCD Clock is an ESP32-based, network-synchronized time display system designed for reliable, unattended operation in residential and utility environments.  
The system prioritizes power-recovery resilience, autonomous WiFi reconnection, and field-configurable operation without requiring firmware reflashing.

---

## 2. Hardware Architecture

### 2.1 Core Controller
- **Microcontroller:** ESP32 (WROOM series)
- Handles WiFi connectivity, NTP time synchronization, LCD control, and embedded web services.

### 2.2 Display Subsystem
- **Display:** 20×4 I²C LCD
- **Interface:** I²C bus
- Custom large-digit rendering implemented using LCD CGRAM for enhanced readability.

### 2.3 User Input
- **Configuration Button (GPIO 4):**
  - Short press: Manual WiFi reconnect
  - Long press (3s): Enter configuration mode
  - Long press (5s): Exit configuration mode

### 2.4 Non-Volatile Storage
- **ESP32 Preferences (NVS):**
  - Stores WiFi credentials
  - Timezone offset
  - Time format (12/24-hour)

---

## 3. Software Architecture

### 3.1 High-Level Architecture
The firmware is structured as modular subsystems coordinated through a central state-driven loop.

**Major subsystems:**
- WiFi & Network Manager
- NTP Time Synchronization
- LCD Display Engine
- Web Configuration Server
- Boot & Recovery State Machine
- Persistent Settings Manager

---

## 4. Boot & Recovery Architecture

### 4.1 Boot State Machine
The system uses a timed boot-state machine to ensure reliable startup after power loss.

**Boot states include:**
- Initial boot
- Progressive WiFi retry phases
- Quick retry window
- Normal operation mode

This prevents boot-lock during router startup delays.

### 4.2 Power Loss Recovery
- WiFi auto-reconnect enabled
- Persistent credentials stored in NVS
- Time re-synchronized automatically after reconnection

---

## 5. Network Architecture

### 5.1 WiFi Operating Modes
- **Station Mode:** Normal operation
- **Access Point Mode:** Configuration mode (fallback or user-invoked)

### 5.2 Embedded Web Server
- Runs on port 80
- Provides:
  - Secure configuration interface
  - WiFi credential management
  - Timezone and format selection
  - Controlled exit from configuration mode

---

## 6. Time Synchronization Architecture

### 6.1 Time Source
- NTP servers:
  - `pool.ntp.org`
  - `time.nist.gov`

### 6.2 Time Handling
- Local time calculated using user-defined timezone offset
- Hourly re-synchronization when network is available
- Cached time maintained during temporary network loss

---

## 7. Display Architecture

### 7.1 Large Digit Rendering
- Uses LCD CGRAM custom characters
- Dynamically updates only changed segments to minimize flicker

### 7.2 Status Feedback
- Displays:
  - Time (large format)
  - Date
  - WiFi connection status
  - Boot and recovery indicators

---

## 8. Safety & Reliability Considerations

- Debounced physical input handling
- Controlled mode transitions
- Non-blocking reconnection logic
- Fail-safe configuration entry when credentials are missing

---

## 9. Design Intent
This architecture is designed for:
- Continuous operation without supervision
- Graceful recovery from power and network failures
- Field deployment without developer tools
- Clear separation between control logic and user interface

---

## 10. Future Architecture Extensions
- OTA firmware updates
- SNTP redundancy sources
- External RTC fallback
- MQTT-based monitoring

---
