#include <Arduino.h>
#include <SoftwareSerial.h>
#include "gsm.h"

SoftwareSerial gsmSerial(GSM_RX, GSM_TX);

void setup() {
  Serial.begin(9600);
  while (!Serial);
  
  Serial.println(F("=== GSM HTTP POST Test ==="));
  Serial.println(F("Initializing..."));
  
  // Initialize GSM Serial
  gsmSerial.begin(9600);
  delay(3000);
  
  // Initialize GSM module
  if (!initGSM()) {
    Serial.println(F("Failed to initialize GSM module!"));
    Serial.println(F("Check wiring and power supply."));
    return;
  }
  
  // Connect to GPRS
  if (!connectGPRS()) {
    Serial.println(F("Failed to connect to GPRS!"));
    Serial.println(F("Check APN settings and SIM card."));
    return;
  }
  
  Serial.println(F("Setup complete!"));
  Serial.println(F("Ready to send HTTP POST requests."));
}

void loop() {
  Serial.println(F("\n--- Attempting HTTP POST ---"));
  
  int attempts = 0;
  bool success = false;
  
  // Retry logic
  while (attempts < MAX_RETRIES && !success) {
    attempts++;
    Serial.print(F("Attempt "));
    Serial.print(attempts);
    Serial.print(F(" of "));
    Serial.println(MAX_RETRIES);
    
    success = sendHTTPPost();
    
    if (!success && attempts < MAX_RETRIES) {
      Serial.print(F("Retrying in "));
      Serial.print(RETRY_DELAY / 1000);
      Serial.println(F(" seconds..."));
      delay(RETRY_DELAY);
    }
  }
  
  if (success) {
    Serial.println(F("\n✓ HTTP POST successful!"));
  } else {
    Serial.println(F("\n✗ HTTP POST failed after all retries."));
  }
  
  // Wait 30 seconds before next request
  Serial.println(F("\nWaiting 30 seconds before next request..."));
  delay(30000);
}