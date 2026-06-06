#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "secrets.h"

// Timing variables for heartbeat and event tracking
unsigned long lastEventTime = 0;
const long heartbeatInterval = 120000; 

// Buzzer state tracking
unsigned long buzzerStartTime = 0;
bool buzzerIsActive = false;

// IR sensor configuration
const int IR_PIN = 14;
const int buzzerPin = 4;
volatile bool detected = false;

// WiFi and Supabase configuration
const char* ssid = "Kweh";
const char* password = "Aaaa1111";
const char* supabaseUrl = "mpghpytgetjbjrfgbaoz.supabase.co";
const char* apiKey = SUPABASE_KEY;
void IRAM_ATTR isr() {
  detected = true;
}

void updateSupabase(String eventType) {
  WiFiClientSecure client;
  client.setInsecure();

  if (client.connect(supabaseUrl, 443)) {
    String jsonString = "{\"event_type\": \"" + eventType + "\"}";
    
    client.print("POST /rest/v1/MouseDetector HTTP/1.1\r\n");
    client.print("Host: " + String(supabaseUrl) + "\r\n");
    client.print("apikey: " + String(apiKey) + "\r\n");
    client.print("Authorization: Bearer " + String(apiKey) + "\r\n");
    client.print("Content-Type: application/json\r\n");
    client.print("Prefer: return=minimal\r\n");
    client.print("Content-Length: " + String(jsonString.length()) + "\r\n");
    client.print("Connection: close\r\n\r\n");
    client.print(jsonString);
    
    // Read response completely
    unsigned long timeout = millis();
    while (client.connected() && millis() - timeout < 3000) {
      if (client.available()) {
        client.readString(); // Read the whole response to close socket properly
      }
    }
    client.stop();
    lastEventTime = millis(); // Reset heartbeat timer on success
    Serial.println("Sent: " + eventType);
  } else {
    Serial.println("Connection failed!");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(buzzerPin, OUTPUT);
  pinMode(IR_PIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(IR_PIN), isr, RISING);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
  
  updateSupabase("System Boot");
}

void playBuzz() {
  Serial.println("Starting buzz...");
  tone(buzzerPin, 500);
  buzzerStartTime = millis(); // Record the start time
  buzzerIsActive = true;      // Flag that the buzzer is currently on
}

void loop() {
  if (detected) {
    detected = false;
    updateSupabase("movement");
    playBuzz();
  }
  
  if (buzzerIsActive && (millis() - buzzerStartTime >= 500)) {
    noTone(buzzerPin);
    buzzerIsActive = false; // Reset the flag
    Serial.println("Buzzer stopped.");
  }

  if ((millis() - lastEventTime) > heartbeatInterval) {
    updateSupabase("heartbeat");
  }

  delay(10); 
}