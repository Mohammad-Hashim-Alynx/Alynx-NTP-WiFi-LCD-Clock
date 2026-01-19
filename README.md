# Alynx-NTP-WiFi-LCD-Clock
Resilient ESP32-based WiFi clock with NTP time sync, LCD big-digit display, web configuration portal, and robust auto-reconnection logic for power and network recovery.
# Alynx NTP WiFi LCD Clock (ESP32)

## Overview
The **Alynx NTP WiFi LCD Clock** is a resilient ESP32-based time display system designed for continuous operation in environments with **power interruptions and unstable WiFi connectivity**.  
It automatically synchronizes time via **NTP**, provides **web-based configuration**, and incorporates **robust boot, reconnection, and recovery logic** to ensure unattended operation.

This project emphasizes **system reliability, fault tolerance, and real-world usability**, rather than basic IoT demonstrations.

---

## Problem Statement
Conventional WiFi clocks and IoT time displays often fail due to:
- Power outages and improper restart handling  
- WiFi router boot delays after power restoration  
- Manual reconfiguration requirements  
- Poor reconnection and recovery logic  

This project addresses these issues through **state-driven boot sequencing**, **persistent configuration storage**, and **multi-layer recovery mechanisms**.

---

## Key Features
- NTP time synchronization with configurable timezone support  
- Large 20×4 I²C LCD with custom big-digit rendering  
- Automatic WiFi reconnection with adaptive retry logic  
- Web-based configuration portal (WiFi, timezone, time format)  
- Non-volatile storage using ESP32 Preferences (NVS)  
- Manual recovery controls via physical button  
- Fail-safe boot sequencing with staged retry states  
- 12-hour / 24-hour time format selection  

---

## System Architecture
**Controller:** ESP32  
**Display:** 20×4 I²C LCD  
**Networking:** WiFi (Station + Access Point for configuration)  
**Storage:** ESP32 Preferences (Flash-based NVS)

**High-Level Flow:**
