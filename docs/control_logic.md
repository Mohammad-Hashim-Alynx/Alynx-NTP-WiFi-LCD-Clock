# Control Logic — Alynx NTP WiFi LCD Clock

## 1. Control Philosophy
The control logic is designed around **state-driven, non-blocking execution** to ensure reliable operation under variable network and power conditions.  
The system prioritizes deterministic behavior, fault tolerance, and minimal user intervention.

---

## 2. Main Control Loop Architecture

The firmware operates within a single main loop that coordinates independent subsystems without blocking execution.

### Key Characteristics
- Non-blocking timing using `millis()`
- Event-driven state transitions
- Subsystem isolation for display, WiFi, and configuration handling

---

## 3. Operating Modes

### 3.1 Normal Operation Mode
Activated when:
- Valid WiFi credentials exist
- Configuration mode is not forced

**Functions:**
- Display current time and date
- Monitor WiFi connectivity
- Handle automatic reconnection
- Periodically synchronize time via NTP

---

### 3.2 Configuration Mode
Activated when:
- No stored WiFi credentials exist
- Configuration button is held during boot
- User manually invokes config mode

**Functions:**
- ESP32 enters Access Point mode
- Embedded web server starts
- LCD displays configuration instructions
- User updates WiFi, timezone, and format settings

Exit conditions:
- Manual exit via web interface
- Button-triggered exit sequence

---

## 4. WiFi Control Logic

### 4.1 Connection Management
- WiFi auto-reconnect enabled at system level
- Background reconnection attempts without blocking UI or time display

### 4.2 Manual Intervention
- Short button press triggers manual reconnection attempt
- Prevents unnecessary device restarts during transient failures

---

## 5. Time Synchronization Logic

### 5.1 Synchronization Conditions
- Initial sync after successful WiFi connection
- Periodic sync every 1 hour
- Skipped if WiFi is unavailable

### 5.2 Time Fallback
- Cached system time used during network loss
- Prevents display freeze or invalid timestamps

---

## 6. Button Control Logic

| Button Action | Function |
|--------------|---------|
| Short press | Manual WiFi reconnect |
| Hold 3 sec | Enter configuration mode |
| Hold 5 sec | Exit configuration mode |

Debounce logic prevents false triggers due to electrical noise.

---

## 7. Display Update Logic

### Optimization Strategy
- Time digits updated only when values change
- Blinking colon updated once per second
- Full screen redraw avoided to reduce flicker

---

## 8. Safety & Fail-Safe Behavior

- Configuration mode automatically entered if credentials are missing
- Network failures do not halt timekeeping
- Blocking delays avoided in control paths
- Mode transitions guarded by explicit flags

---

## 9. Design Intent
The control logic is structured to:
- Survive unstable power and network conditions
- Operate unattended for extended durations
- Remain configurable without firmware updates
- Maintain predictable behavior under fault conditions

---
