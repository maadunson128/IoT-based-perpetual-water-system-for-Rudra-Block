# Project Title: IoT-Based Perpetual Water System for Rudra Block

## Project Site Location:
**Rudra Block, Government College of Technology (GCT)**  
[Google Maps Location](#) *(Insert actual link here)*  

## Tools & Software Used:
- **Hardware:** ESP32, BharatPi ESP32 (Master), Waterproof Ultrasonic Sensors  
- **Communication Protocol:** ESP-NOW, MQTT  
- **Cloud Services:** HiveMQ, Railway.app, InfluxDB  
- **Visualization:** Grafana  
- **Programming Languages:** Python, Arduino (C++)  
- **Alert System:** Telegram Bot  

---

## Objective of the Project:
- To monitor water levels in two overhead tanks at Rudra Block.  
- Estimate filling time and track water consumption over time.  
- Detect continuous abnormal usage patterns to identify possible leaks.  
- Send real-time alerts via Telegram when a leak or low water level is detected.  

---

## Site Overview:
This section provides an overview of the project location, tank placement, and distances between components.  

### **Images & Videos:**
- **Distance Between Two Tanks:** *(Include the distance diagram image here)*  
- **Site View Video:** *(Embed the site view video or provide a link)*  

---

## Tasks Overview:
The project is divided into three main tasks:

1. **Task 1 - Data Collection & Communication**  
   - ESP32 modules collect water level data from two tanks using ultrasonic sensors.  
   - Data is transmitted via ESP-NOW to the master controller (BharatPi ESP32).  

2. **Task 2 - Data Transmission to Cloud**  
   - The master controller sends sensor data via MQTT to a HiveMQ broker.  
   - A Python script running on Railway.app subscribes to MQTT topics and stores the data in InfluxDB.  

3. **Task 3 - Data Visualization & Alerts**  
   - Grafana dashboards display real-time water levels and trends.  
   - Alerts are triggered via a Telegram bot for leaks and low water levels.  

---

## Each Task Explained Clearly:

### **Task 1 - Data Collection & Communication**
- **ESP32 Devices & Sensors:**  
  - Two ultrasonic sensors measure water levels.  
  - Slave ESP32 collects data and sends it to the Master ESP32 via ESP-NOW.  
- **Communication Setup:**  
  - Short-range ESP-NOW wireless communication used between ESP32 devices.  

**Image:** *(Include the flowchart or circuit diagram here)*  

---

### **Task 2 - Data Transmission to Cloud**
- **MQTT Protocol:**  
  - Data is published from BharatPi ESP32 to HiveMQ cloud broker.  
- **Python Server on Railway.app:**  
  - Subscribes to MQTT topics and writes data into InfluxDB.  
- **InfluxDB Database:**  
  - Stores timestamped water level and consumption data.  

**Images:**  
- *(Include MQTT protocol explanation image)*  
- *(Include HiveMQ cloud cluster screenshot)*  
- *(Include Railway.app running Python script image)*  
- *(Include InfluxDB database screenshot)*  

---

### **Task 3 - Data Visualization & Alerts**
- **Grafana Dashboard Panels:**  
  - Panel 1: Live water level monitoring.  
  - Panel 2: 30-day water consumption trends.  
  - Panel 3: Leakage detection & low-level alerts.  
- **Alert System:**  
  - Telegram bot sends alerts when leaks or low water levels are detected.  

**Images:** *(Include screenshots of each Grafana dashboard panel)*  

---

## Final Output of the Project:
A complete working system that monitors, visualizes, and alerts users about water consumption and leakages in real-time.

**Final Project Video:** *(Embed the final project demonstration video here)*  

 
