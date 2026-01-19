# Boot & Recovery Flow — Alynx NTP WiFi LCD Clock

## 1. Purpose
This document describes the system's boot, recovery, and fault-handling behavior following power restoration or network failure.

The design addresses real-world issues such as delayed router startup, intermittent WiFi availability, and unattended deployment.

---

## 2. Boot Sequence Overview

Upon power restoration, the system executes a structured boot sequence governed by a timed state machine.

### Boot Timeline
| Time Window | Behavior |
|------------|---------|
| 0–10 sec | Initial boot and hardware initialization |
| 10–30 sec | Progressive WiFi connection attempts |
| 30–60 sec | Extended retry phase |
| 1–2 min | Quick retry mode |
| >2 min | Normal operation mode |

---

## 3. Boot State Machine

### Boot States
- `BOOT_START`
- `BOOT_WIFI_ATTEMPT_1`
- `BOOT_WIFI_ATTEMPT_2`
- `BOOT_WIFI_ATTEMPT_3`
- `BOOT_QUICK_RETRY`
- `BOOT_NORMAL`

Each state defines:
- Retry interval
- Display feedback
- Network handling behavior

---

## 4. Power Recovery Logic

### Key Behaviors
- WiFi credentials loaded from non-volatile storage
- Automatic reconnection without user input
- Time synchronization initiated upon reconnection
- System remains operational even if WiFi is unavailable

---

## 5. Network Failure Recovery

### Detection
- Continuous monitoring of WiFi connection state

### Response
- Immediate state update on disconnect
- Scheduled background reconnection attempts
- Display feedback provided during retry phases

---

## 6. Router Startup Accommodation

The system intentionally delays aggressive retries during early boot to accommodate:
- Router boot latency
- ISP reconnection delays

This avoids rapid failure loops and unnecessary restarts.

---

## 7. Configuration Recovery

If:
- WiFi credentials are missing
- User forces configuration mode

Then:
- Device enters AP mode
- Configuration web server is launched
- LCD provides connection instructions

---

## 8. Manual Recovery Controls

### Button-Based Recovery
- Manual WiFi reconnect without reboot
- Forced entry into configuration mode
- Safe exit from configuration mode

---

## 9. Reliability Design Summary

This recovery architecture ensures:
- No permanent lockout due to network failure
- Graceful behavior after power outages
- Continuous display operation
- Minimal user intervention in field deployments

---

## 10. Design Intent
The boot and recovery flow is engineered for:
- Long-term unattended operation
- Residential and utility environments
- Power and network instability
- Maintenance-free deployment

---
