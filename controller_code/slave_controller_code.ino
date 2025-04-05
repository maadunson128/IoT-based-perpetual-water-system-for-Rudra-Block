#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "NewPing.h"
#include "esp_sleep.h"  // For deep sleep functions

// Ultrasonic Sensor Configuration
#define TRIG_PIN 5
#define ECHO_PIN 18
#define MAX_DISTANCE 400
NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DISTANCE);

// Tank Dimensions
#define TANK_DEPTH 140
#define SURFACE_AREA 91628.57

// WiFi Configuration
#define WIFI_CHANNEL 1

// Sender Identification
#define SENDER_ID 1

// Sleep Configuration
#define uS_TO_S_FACTOR 1000000     // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP 170          // Wake up 10 seconds before the 3-minute master cycle

// RTC Memory to preserve data between deep sleep cycles
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR bool dataSent = false;

// Data Structure for Sensor Readings
struct SensorData {
    uint8_t senderID;
    float waterLevel;
    float volume;
    unsigned long timestamp;
} sensorData;

// Flag to track if data has been sent during this wake cycle
bool dataTransmitted = false;
unsigned long wakeupTime = 0;
const unsigned long MAX_AWAKE_TIME = 20000; // Max time to stay awake (20 seconds)

void OnDataRecv(const esp_now_recv_info *recv_info, const uint8_t *data, int data_len) {
    const uint8_t *mac_addr = recv_info->src_addr;
    if (data_len == 1 && data[0] == 0xAA) { // Sync trigger received
        static bool peerAdded = false;
        if (!peerAdded) {
            esp_now_peer_info_t peer;
            memset(&peer, 0, sizeof(peer));
            peer.channel = WIFI_CHANNEL;
            peer.encrypt = false;
            memcpy(peer.peer_addr, mac_addr, 6);
            if (esp_now_add_peer(&peer) != ESP_OK) {
                Serial.println("Failed to add master peer.");
            }
            peerAdded = true;
        }

        float sensor_distance = sonar.ping_cm();
        
        sensorData.waterLevel = TANK_DEPTH - sensor_distance;
        sensorData.waterLevel = constrain(sensorData.waterLevel, 0, TANK_DEPTH);
        sensorData.volume = (sensorData.waterLevel * SURFACE_AREA) / 1000.0;
        sensorData.senderID = SENDER_ID;
        sensorData.timestamp = millis();

        esp_err_t sendStatus = esp_now_send(mac_addr, (uint8_t *)&sensorData, sizeof(sensorData));
        if (sendStatus == ESP_OK) {
            Serial.println("Data sent successfully");
            dataTransmitted = true;
            dataSent = true;
        } else {
            Serial.println("Data send failed.");
        }
    }
}

// Function to enable Long Range mode
void enableLongRangeMode() {
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR);
    Serial.println("WiFi Long Range (LR) Mode Enabled");
    
    // Lower data rate for better range
    esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_LORA_250K);
}

void goToDeepSleep() {
    Serial.println("Going to deep sleep...");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    btStop();
    
    // Correct for time spent awake
    unsigned long awakeTime = (millis() - wakeupTime) / 1000; // in seconds
    long sleepTime = TIME_TO_SLEEP;
    
    if (awakeTime < TIME_TO_SLEEP) {
        sleepTime = TIME_TO_SLEEP - awakeTime;
    }
    
    Serial.print("Sleeping for ");
    Serial.print(sleepTime);
    Serial.println(" seconds");
    
    esp_sleep_enable_timer_wakeup(sleepTime * uS_TO_S_FACTOR);
    esp_deep_sleep_start();
}

void setup() {
    // Save wake up time
    wakeupTime = millis();
    
    Serial.begin(115200);
    delay(500);  // Allow serial to initialize
    
    bootCount++;
    Serial.println("Boot number: " + String(bootCount));
    Serial.println("Initializing Sender with Long Range Mode...");

    // Initialize WiFi and ESP-NOW
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    WiFi.setChannel(WIFI_CHANNEL);
    
    // Enable Long Range mode after WiFi initialization
    enableLongRangeMode();

    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        goToDeepSleep();
        return;
    }

    // Register callback for receiving sync triggers
    esp_now_register_recv_cb(OnDataRecv);
    
    Serial.println("Waiting for master request...");
}

void loop() {
    // Check if we've been awake too long without transmitting data
    if (millis() - wakeupTime > MAX_AWAKE_TIME) {
        Serial.println("Awake too long without data request, going to sleep");
        goToDeepSleep();
    }
    
    // If data was transmitted during this wake cycle, go to sleep
    if (dataTransmitted) {
        Serial.println("Data transmitted successfully, going to sleep");
        delay(1000); // Small delay to ensure transmission completes
        goToDeepSleep();
    }
    
    // Small delay to prevent too frequent checking
    delay(100);
}
