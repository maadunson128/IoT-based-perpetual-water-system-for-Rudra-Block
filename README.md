# Project Title: IoT-Based Perpetual Water System for Rudra Block

## Project Site Location:
**Rudra Block, Government College of Technology (GCT)**  
[Google Maps Location](#) *(https://maps.app.goo.gl/fLT6oLrJnmRZ2eJK9)*  

## Project Flow Diagram

 <img width="2900" alt="Image" src="https://github.com/user-attachments/assets/f2c2f1d6-4290-48ce-9dd2-b38e0fa19043" />

## Tools & Software Used:
- **Hardware:** ESP32(Slave), BharatPi ESP32 with Simcom A7672s (Master), Waterproof Ultrasonic Sensors, Li-ion batteries  
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
   
   In the top of Rudra Block, there are two overhead tanks each placed on opposite ends of the block with a distance between of 50m.
   Each tank has two compartments → One for drinking water supply, other for restroom usage supply and other usages.Both the tank and the compartment dimensions are same.
   Sensors only for the restroom water supply compartments was placed of both the tanks due to limited funding for our project.

### **Images & Videos:**
- **Distance Between Two Tanks:**
  ![Image](https://github.com/user-attachments/assets/4ce4b185-853e-4aaa-a798-6baeac9dfb81)
  
- **Site View Video:**
  
   <video src="https://github.com/user-attachments/assets/d28bab54-3d2b-4f83-ab3e-565f98148dd7" controls>
</video>


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

## Individual Task Explanation:

### **Task 1 - Data Collection & Communication**
   Each ultrasonic sensors connected with ESP32’s on each tank to sense the data current water level and water volume (derived using water level data and surface area         which is calculated by dimensions of the tank) was placed on the top of the tanks.
   
   Now, for communication between two ESP32’s, we chose ESP NOW which is very reliable over 100m practically if there is no interference like metal, and concrete walls        between them. Placement of the sensors on the top of the Tanks so this condition is ideal for our ESP NOW communication was done.
   
   Next the code for both the controllers ESP32 and another ESP32 which is BharatPI ESP32 with Simcom module was written.
   
   Here, both the controllers connected with sensors will be connected into a network by ESP NOW where ESP32 act as slave and BharatPi will act as Master.
   The master will send a signal to the slave ESP32 via ESP NOW. After receiving the signal,  ESP32 will send trigger signal to the sensor to get the water level data from    Tank 1 in cm. After the that, slave ESP32 will send the data (water level, water volume) to the BharatPi ESP32 via ESP NOW. After getting the data, the BharatPI will       get sensor data from its sensor in Tank 2 and combine all the data (water level and water volume for both tanks) ready to send for the cloud.
    
---

### **Task 2 - Data Transmission to Cloud**

   After getting the data, the data has to be sent to the cloud database. We faced difficulties for transmission of data from master ESP32 directly to the database. So, we    used MQTT protocol to send the data to Hive MQ private cloud cluster (MQTT broker) via under 5 topics. Topic 1 → water level of tank 1 in cm, Topic 2 → water volume of     tank 1, Topic 3 → water level of tank 2 in cm, Topic 4 → water volume of tank 2 , Topic 5 → current time. These data will sent to the Hive MQ. Then we written a python     that will subscibes to this Hive MQ broker and retrives the data whenever any data is published to broker. After getting all the data from all the topics, the python       code will write the data to InfluxDB as a single row. The python code is deployed in Railway.app website to run as a server 24/7 in the cloud to get the data from the      Broker.

Note: The code for controllers will get the data for every 3 mins from the tank and send the data to broker. The code for the both controllers and server are attached in this repository.

- **MQTT Protocol:**  
  - Data is published from BharatPi ESP32 to HiveMQ cloud broker.  

    ![Image](https://github.com/user-attachments/assets/4e3d8971-7248-4780-8c17-a19e7fec5acd)

- **Python Server on Railway.app:**  
  - Subscribes to MQTT topics and writes data into InfluxDB.  
    ![Image](https://github.com/user-attachments/assets/675ae2d5-047f-434e-a5de-f20f33e29f13)

- **InfluxDB Database:**  
  - Stores timestamped water level and consumption data. 

    ![Image](https://github.com/user-attachments/assets/3c73f1b5-defe-4a4c-a4b3-0722276302bd) 

---

### **Task 3 - Data Visualization & Alerts**
- **Grafana Dashboard Panels:**  
  - Panel 1: Live water level monitoring.
  
     water level for tank 1(left side of Rudra Block), water level for tank 2 (left side of Rudra Block), 
     Tank 1 motor has to be ON/OFF, Tank 2 motor ON/OFF, Tank 1 filling time with expected filling timestamp, Tank 2 filling time with expected filling timestamp
    
    ![Image](https://github.com/user-attachments/assets/c19f4f5a-684a-476e-b37d-a00bd59fd6cb)
  
  - Panel 2: 30-day water consumption trends.

    Water consumption for past 30 days, Maximum , minimum, average conumsption amount, max, min conumed days
    
    ![Image](https://github.com/user-attachments/assets/8121d071-0879-4852-b6da-ea5448bd8e8a)

  - Panel 3: Leakage detection & low-level alerts.
    
    Leakage indicators for both tank 1 and tank 2
    
     ![Image](https://github.com/user-attachments/assets/2129e046-50a9-48d0-a5c1-bf6b61e32a4e)
    
- **Alert System:**  
  - Telegram bot sends alerts when leaks or low, high water levels are detected.
  

    ![Image](https://github.com/user-attachments/assets/c7bce7fb-d14a-4ad6-97dc-fd82570093b4)

---
### Leakage and Alert System with Linear regression:

   This Flux query analyzes tank level data to detect leaks by performing linear regression on the most recent 20 readings from the past readings for every 3 mins ( This      synchronizes with sensor data for every 3mins). It first retrieves tank level measurements, then calculates statistical components (sums, counts, and products) needed      for linear regression analysis. The query then computes the slope, y-intercept, and R-squared value of the tank level trend line. A leak is identified when two             conditions are met simultaneously: the R-squared value exceeds 0.95 (indicating a strong linear correlation) and the slope is less than -0.1 (showing a consistent          downward trend in fluid level). When the two conditionsWe placed each ultrasonic sensors connected with ESP32’s on each tank to sense the data current water level and water volume (derived using water level data and surface area which is calculated by dimensions of the tank).

Now, for communication between two ESP32’s, we chose ESP NOW which is very reliable over 100m practically if there is no interference like metal, and concrete walls between them. We are going to place the sensors on the top of the Tanks so this condition is ideal for our ESP NOW communication.

Next we written code for both the controllers ESP32 and another ESP32 which is BharatPI ESP32 with Simcom module.

Here, both the controllers connected with sensors will be connected into a network by ESP NOW where ESP32 act as slave and BharatPi will act as Master.
The master will send a signal to the slave ESP32 via ESP NOW. After receiving the signal,  ESP32 will send trigger signal to the sensor to get the water level data from Tank 1 in cm. After the that, slave ESP32 will send the data (water level, water volume) to the BharatPi ESP32 via ESP NOW. After getting the data, the BharatPI will get sensor data from its sensor in Tank 2 and combine all the data (water level and water volume for both tanks) ready to send for the cloud. are met, then the indicator will be marked as 1 for each tanks. Then Alert will be sent to Telegram Chats.

### Low level Alert System:
   Whenever the tanks level goes below the 40cm, then it will send the alerts to Telegram chat via the bot integrated with Grafana Alerts System.
   
### High Level Alert System:
   Telegram bot also sends alerts when the tank water levels are above 120cm while refilling, informing the operator to indicate the tanks are fulled and overcoming water  wastage.



### Implementation Images in Rudra Block Overhead Tanks

**Left Side Tank project Setup**
![Image](https://github.com/user-attachments/assets/8c586649-d2c7-4dca-9e79-efdafdfdb3ce)


**Right Side Tank project Setup**
![Image](https://github.com/user-attachments/assets/7311c680-bcf6-432d-96e7-7cc87d00de81)

**Project Realtime Output Video:**
  
   <video src="https://github.com/user-attachments/assets/8048cf95-15ed-4b13-a5e7-295537d7b784" controls>
</video>




**Note**: The li-ion batteries setup will be charged for every 10 days for the ESP32, and every 3 days for the master BharatPI ESP32 controller. The dashboard will be set to refresh for 3 mins where all the Flux Queries will run for every 3 mins and update all the visualisations and make it a real time dashboard.
Data collected for some dashboard Visualisations: Tank depth -> 140cm, Time taken to fill the entire tank for the particular compartment -> 45 mins

 
