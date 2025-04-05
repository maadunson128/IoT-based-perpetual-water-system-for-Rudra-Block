/*************************************************************************
   PROJECT: Water Tank Monitoring System with MQTT Cloud Integration
   AUTHOR: Modified for maadunson128
   DATE: 2025-04-05
   
   FUNC: Monitors water levels in two tanks with deep sleep for power saving
*************************************************************************/

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "NewPing.h"
#include "esp_sleep.h"  // For deep sleep functions

// GSM and MQTT Includes
#define TINY_GSM_MODEM_SIM7600
#define TINY_GSM_RX_BUFFER 1024
#include <TinyGsmClient.h>
#include "SSLClient.h"
#include <PubSubClient.h>

// Tank2 Sensor Configuration
#define TANK2_TRIG 23
#define TANK2_ECHO 22
#define TANK2_DEPTH 140
#define TANK2_SURF_AREA 91628.57
NewPing tank2Sonar(TANK2_TRIG, TANK2_ECHO, 400);

// GSM Module Configuration
#define SerialAT Serial1
#define SerialMon Serial
#define UART_BAUD   115200
#define PIN_DTR     25
#define PIN_TX      17
#define PIN_RX      16
#define PWR_PIN     32
#define LED_PIN     2

// WiFi Configuration
#define WIFI_CHANNEL 1

// Sleep Configuration
#define uS_TO_S_FACTOR 1000000     // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP 180          // Sleep for 3 minutes (180 seconds)

// RTC Memory to preserve data between deep sleep cycles
RTC_DATA_ATTR int bootCount = 0;

// Sender1 (Tank1) MAC Address
const uint8_t sender1Mac[] = {0xA8, 0x42, 0xE3, 0xA8, 0x6C, 0x5C};

// APN Configuration
const char apn[] = "airtelgprs.com";

// MQTT Broker Configuration
const char* mqtt_server = "YOUR-HIVEMQ-URL";
const int mqtt_port = 8883;
const char* mqtt_username = "YOUR-MQTT-USERNAME";
const char* mqtt_password = "YOUR-MQTT-PASSWORD";
const char* clientID = "ESP32_TankMonitor";
const char* lwt_topic = "tank/status";
const char* lwt_message = "Offline";
const int qos = 1;
const bool lwt_retain = true;

// MQTT Topics for tank data
const char* topic1 = "tank/topic1"; // Tank 1 level in cm
const char* topic2 = "tank/topic2"; // Tank 1 volume in liters
const char* topic3 = "tank/topic3"; // Tank 2 level in cm
const char* topic4 = "tank/topic4"; // Tank 2 volume in liters
const char* topic5 = "tank/topic5"; // Current timestamp

// Data Structures
struct SensorData {
    uint8_t senderID;
    float waterLevel;
    float volume;
    unsigned long timestamp;
} tank1Data;

float tank2Level = 0.0;
float tank2Volume = 0.0;
unsigned long tank2Timestamp = 0;

// Workflow tracking
bool tank1DataReceived = false;
bool tank2DataCollected = false;
bool dataSentToMQTT = false;
unsigned long wakeupTime = 0;
const unsigned long MAX_AWAKE_TIME = 60000; // Max time to stay awake (60 seconds)

// GSM and MQTT objects
TinyGsm modem(SerialAT);
TinyGsmClient gsm(modem);
SSLClient secure_client(&gsm);
PubSubClient mqttClient(secure_client);

//CA Certificate for HiveMQ Private Cluster
const char root_ca[] PROGMEM =
"-----BEGIN CERTIFICATE-----\n"
"MIIFBjCCAu6gAwIBAgIRAIp9PhPWLzDvI4a9KQdrNPgwDQYJKoZIhvcNAQELBQAw\n"
"TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n"
"cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMjQwMzEzMDAwMDAw\n"
"WhcNMjcwMzEyMjM1OTU5WjAzMQswCQYDVQQGEwJVUzEWMBQGA1UEChMNTGV0J3Mg\n"
"RW5jcnlwdDEMMAoGA1UEAxMDUjExMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIB\n"
"CgKCAQEAuoe8XBsAOcvKCs3UZxD5ATylTqVhyybKUvsVAbe5KPUoHu0nsyQYOWcJ\n"
"DAjs4DqwO3cOvfPlOVRBDE6uQdaZdN5R2+97/1i9qLcT9t4x1fJyyXJqC4N0lZxG\n"
"AGQUmfOx2SLZzaiSqhwmej/+71gFewiVgdtxD4774zEJuwm+UE1fj5F2PVqdnoPy\n"
"6cRms+EGZkNIGIBloDcYmpuEMpexsr3E+BUAnSeI++JjF5ZsmydnS8TbKF5pwnnw\n"
"SVzgJFDhxLyhBax7QG0AtMJBP6dYuC/FXJuluwme8f7rsIU5/agK70XEeOtlKsLP\n"
"Xzze41xNG/cLJyuqC0J3U095ah2H2QIDAQABo4H4MIH1MA4GA1UdDwEB/wQEAwIB\n"
"hjAdBgNVHSUEFjAUBggrBgEFBQcDAgYIKwYBBQUHAwEwEgYDVR0TAQH/BAgwBgEB\n"
"/wIBADAdBgNVHQ4EFgQUxc9GpOr0w8B6bJXELbBeki8m47kwHwYDVR0jBBgwFoAU\n"
"ebRZ5nu25eQBc4AIiMgaWPbpm24wMgYIKwYBBQUHAQEEJjAkMCIGCCsGAQUFBzAC\n"
"hhZodHRwOi8veDEuaS5sZW5jci5vcmcvMBMGA1UdIAQMMAowCAYGZ4EMAQIBMCcG\n"
"A1UdHwQgMB4wHKAaoBiGFmh0dHA6Ly94MS5jLmxlbmNyLm9yZy8wDQYJKoZIhvcN\n"
"AQELBQADggIBAE7iiV0KAxyQOND1H/lxXPjDj7I3iHpvsCUf7b632IYGjukJhM1y\n"
"v4Hz/MrPU0jtvfZpQtSlET41yBOykh0FX+ou1Nj4ScOt9ZmWnO8m2OG0JAtIIE38\n"
"01S0qcYhyOE2G/93ZCkXufBL713qzXnQv5C/viOykNpKqUgxdKlEC+Hi9i2DcaR1\n"
"e9KUwQUZRhy5j/PEdEglKg3l9dtD4tuTm7kZtB8v32oOjzHTYw+7KdzdZiw/sBtn\n"
"UfhBPORNuay4pJxmY/WrhSMdzFO2q3Gu3MUBcdo27goYKjL9CTF8j/Zz55yctUoV\n"
"aneCWs/ajUX+HypkBTA+c8LGDLnWO2NKq0YD/pnARkAnYGPfUDoHR9gVSp/qRx+Z\n"
"WghiDLZsMwhN1zjtSC0uBWiugF3vTNzYIEFfaPG7Ws3jDrAMMYebQ95JQ+HIBD/R\n"
"PBuHRTBpqKlyDnkSHDHYPiNX3adPoPAcgdF3H2/W0rmoswMWgTlLn1Wu0mrks7/q\n"
"pdWfS6PJ1jty80r2VKsM/Dj3YIDfbjXKdaFU5C+8bhfJGqU3taKauuz0wHVGT3eo\n"
"6FlWkWYtbt4pgdamlwVeZEW+LM7qZEJEsMNPrfC03APKmZsJgpWCDWOKZvkZcvjV\n"
"uYkQ4omYCTX5ohy+knMjdOmdH9c7SpqEWBDC86fiNex+O0XOMEZSa8DA\n"
"-----END CERTIFICATE-----\n";

// Function declarations
void modemPowerOn();
void modemPowerOff();
void modemRestart();
void enableLongRangeMode();
void goToDeepSleep();
String getNetworkTime();
void publishTankData();
void mqttCallback(char* topic, byte* payload, unsigned int length);

void OnDataRecv(const esp_now_recv_info *recv_info, const uint8_t *data, int data_len) {
    if (data_len == sizeof(SensorData)) {
        memcpy(&tank1Data, data, sizeof(SensorData));
        Serial.print("Received Tank1 Data (ID ");
        Serial.print(tank1Data.senderID);
        Serial.print("): Level ");
        Serial.print(tank1Data.waterLevel, 1);
        Serial.print("cm, Volume ");
        Serial.print(tank1Data.volume, 1);
        Serial.println("L");
        tank1DataReceived = true;
    }
}

// Function to enable Long Range mode
void enableLongRangeMode() {
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR);
    Serial.println("WiFi Long Range (LR) Mode Enabled");
    esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_LORA_250K);
}

void setup() {
    // Save wake up time
    wakeupTime = millis();
    
    // Initialize Serial
    SerialMon.begin(115200);
    delay(500);  // Allow serial to initialize
    
    bootCount++;
    Serial.println("======================================================");
    Serial.println("  Tank Monitoring System with Deep Sleep");
    Serial.println("  Boot number: " + String(bootCount));
    Serial.println("  Current Date/Time: 2025-04-05 06:22:09");
    Serial.println("  User: maadunson128");
    Serial.println("======================================================");
    
    // Initialize LED
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    // Setup for ESP-NOW communication
    WiFi.mode(WIFI_STA);
    
    // Enable Long Range mode
    enableLongRangeMode();
    
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        goToDeepSleep();
        return;
    }
    esp_now_register_recv_cb(OnDataRecv);
    
    // Add Tank1 as peer for ESP-NOW
    esp_now_peer_info_t peer;
    memset(&peer, 0, sizeof(peer));
    peer.channel = WIFI_CHANNEL;
    peer.encrypt = false;
    memcpy(peer.peer_addr, sender1Mac, 6);
    if (esp_now_add_peer(&peer) != ESP_OK) {
        Serial.println("Failed to add peer.");
    }
    
    // Initialize Tank2 Sensor
    pinMode(TANK2_TRIG, OUTPUT);
    pinMode(TANK2_ECHO, INPUT);
    
    // Initialize GSM module
    modemPowerOn();
    SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);
    secure_client.setCACert(root_ca);
    
    Serial.println("Initializing Modem...");
    
    // Step 1: Request data from Tank1
    Serial.println("Requesting data from Tank1...");
    const uint8_t syncTrigger = 0xAA;
    esp_now_send(sender1Mac, (uint8_t *)&syncTrigger, 1);
    
    // Step 2: Read Tank2 data
    float distance = tank2Sonar.ping_cm();
    if (distance > 0 && distance <= 400) {
        tank2Level = TANK2_DEPTH - distance;
        tank2Level = constrain(tank2Level, 0, TANK2_DEPTH);
        tank2Volume = (tank2Level * TANK2_SURF_AREA) / 1000.0;
        tank2Timestamp = millis();
        
        Serial.print("Tank2: ");
        Serial.print(tank2Level, 1);
        Serial.print("cm, ");
        Serial.print(tank2Volume, 1);
        Serial.println("L");
        tank2DataCollected = true;
    } else {
        Serial.println("Error reading Tank2 sensor");
    }
    
    // Initialize the modem
    if (!modem.init()) {
        Serial.println("Failed to initialize modem, restarting...");
        modemRestart();
        delay(2000);
        
        if (!modem.init()) {
            Serial.println("Modem initialization failed");
            goToDeepSleep();
            return;
        }
    }
    
    String modemName = modem.getModemModel();
    Serial.println("Modem Name: " + modemName);
    
    // Connect to cellular network
    Serial.println("Connecting to cellular network...");
    uint8_t network = 38; // Auto mode
    modem.setNetworkMode(network);
    delay(3000);
    
    bool isConnected = false;
    int tryCount = 30; // Reduced from 60 to save battery
    while (tryCount-- && !isConnected) {
        String networkOperator = modem.getOperator();
        Serial.println("Operator: " + networkOperator);
        
        int16_t signal = modem.getSignalQuality();
        Serial.print("Signal: ");
        Serial.println(signal);
        
        isConnected = modem.isNetworkConnected();
        Serial.println(isConnected ? "CONNECTED" : "NOT CONNECTED YET");
        
        if (isConnected) break;
        
        delay(1000);
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    }
    
    if (!isConnected) {
        Serial.println("Failed to connect to cellular network");
        goToDeepSleep();
        return;
    }
    
    digitalWrite(LED_PIN, HIGH);
    Serial.println("Connected to cellular network");
    
    // Connect to GPRS
    Serial.print("Connecting to APN: ");
    Serial.println(apn);
    if (!modem.gprsConnect(apn, "", "")) {
        Serial.println("APN connection failed");
        goToDeepSleep();
        return;
    }
    Serial.println("APN connected successfully");
    
    // Initialize MQTT client
    mqttClient.setServer(mqtt_server, mqtt_port);
    mqttClient.setCallback(mqttCallback);
    
    // Connect to MQTT
    if (!mqttClient.connected()) {
        Serial.print("Connecting to MQTT...");
        if (mqttClient.connect(clientID, mqtt_username, mqtt_password, lwt_topic, qos, lwt_retain, lwt_message)) {
            Serial.println("connected");
            mqttClient.publish(lwt_topic, "Online", true);
        } else {
            Serial.print("failed, rc=");
            Serial.println(mqttClient.state());
            goToDeepSleep();
            return;
        }
    }
}

void loop() {
    // Process any incoming MQTT messages
    if (mqttClient.connected()) {
        mqttClient.loop();
    }
    
    // Publish data once both sensors have been read
    if (tank1DataReceived && tank2DataCollected && !dataSentToMQTT) {
        publishTankData();
        dataSentToMQTT = true;
        
        // After successful MQTT publish, properly disconnect and go to sleep
        Serial.println("Data collection and transmission completed");
        
        // Publish offline status (optional)
        if (mqttClient.connected()) {
            mqttClient.publish(lwt_topic, "Going to sleep", true);
            mqttClient.disconnect();
        }
        
        // Disconnect from network and go to sleep
        modem.gprsDisconnect();
        delay(1000);
        modemPowerOff();
        delay(1000);
        
        goToDeepSleep();
    }
    
    // Safety timeout - go to sleep if we've been awake too long
    if (millis() - wakeupTime > MAX_AWAKE_TIME) {
        Serial.println("Safety timeout triggered - going to sleep");
        goToDeepSleep();
    }
    
    // Small delay to prevent too frequent loop execution
    delay(100);
}

// Function to get network time from GSM module
String getNetworkTime() {
    // Try to get time from network
    String timeStr = "";
    modem.sendAT("+CCLK?");
    if (modem.waitResponse(1000, timeStr) == 1) {
        // Format: +CCLK: "yy/MM/dd,hh:mm:ss±zz"
        if (timeStr.indexOf("+CCLK:") >= 0) {
            // Extract time string between quotes
            int firstQuote = timeStr.indexOf("\"");
            int lastQuote = timeStr.lastIndexOf("\"");
            if (firstQuote >= 0 && lastQuote > firstQuote) {
                String networkTime = timeStr.substring(firstQuote + 1, lastQuote);
                
                // Parse time format
                if (networkTime.length() >= 17) {
                    String year = "20" + networkTime.substring(0, 2);
                    String month = networkTime.substring(3, 5);
                    String day = networkTime.substring(6, 8);
                    String hour = networkTime.substring(9, 11);
                    String minute = networkTime.substring(12, 14);
                    String second = networkTime.substring(15, 17);
                    
                    return year + "-" + month + "-" + day + " " + hour + ":" + minute + ":" + second;
                }
            }
        }
    }
    
    // Return hardcoded timestamp if network time fails
    return "2025-04-05 06:22:09"; // Current date/time from user input
}

// Publish tank data to MQTT topics
void publishTankData() {
    if (!mqttClient.connected()) return;
    
    // Prepare data as strings
    char tank1LevelStr[10];
    char tank1VolumeStr[10];
    char tank2LevelStr[10];
    char tank2VolumeStr[10];
    
    // Convert float values to strings with 1 decimal place
    dtostrf(tank1Data.waterLevel, 5, 1, tank1LevelStr);
    dtostrf(tank1Data.volume, 7, 1, tank1VolumeStr);
    dtostrf(tank2Level, 5, 1, tank2LevelStr);
    dtostrf(tank2Volume, 7, 1, tank2VolumeStr);
    
    // Get current timestamp
    String timestamp = getNetworkTime();
    
    // Publish to topics
    Serial.println("Publishing data to MQTT topics...");
    bool success = true;
    
    if (!mqttClient.publish(topic1, tank1LevelStr)) success = false;
    if (!mqttClient.publish(topic2, tank1VolumeStr)) success = false;
    if (!mqttClient.publish(topic3, tank2LevelStr)) success = false;
    if (!mqttClient.publish(topic4, tank2VolumeStr)) success = false;
    if (!mqttClient.publish(topic5, timestamp.c_str())) success = false;
    
    if (success) {
        Serial.println("All data published successfully");
        digitalWrite(LED_PIN, LOW);
        delay(100);
        digitalWrite(LED_PIN, HIGH);
    } else {
        Serial.println("Failed to publish some data");
    }
}

// MQTT callback function
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    
    String message = "";
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.println(message);
}

// GSM modem power functions
void modemPowerOn() {
    pinMode(PWR_PIN, OUTPUT);
    digitalWrite(PWR_PIN, LOW);
    delay(1000);
    digitalWrite(PWR_PIN, HIGH);
}

void modemPowerOff() {
    pinMode(PWR_PIN, OUTPUT);
    digitalWrite(PWR_PIN, LOW);
    delay(1500);
    digitalWrite(PWR_PIN, HIGH);
}

void modemRestart() {
    modemPowerOff();
    delay(1000);
    modemPowerOn();
}

void goToDeepSleep() {
    Serial.println("Going to deep sleep for 3 minutes (180 seconds)...");
    
    // Disconnect WiFi
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    btStop();
    
    // Calculate the actual time to sleep
    unsigned long awakeTime = (millis() - wakeupTime) / 1000; // in seconds
    long sleepTime = TIME_TO_SLEEP;
    
    if (awakeTime < TIME_TO_SLEEP) {
        sleepTime = TIME_TO_SLEEP - awakeTime;
    }
    
    Serial.print("Sleeping for ");
    Serial.print(sleepTime);
    Serial.println(" seconds");
    
    // Enable deep sleep wake timer
    esp_sleep_enable_timer_wakeup(sleepTime * uS_TO_S_FACTOR);
    
    // Go to deep sleep
    Serial.println("Entering deep sleep...");
    esp_deep_sleep_start();
}
