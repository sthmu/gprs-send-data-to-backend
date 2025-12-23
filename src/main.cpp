#include <Arduino.h>
#include "gsm.h"

// Energy measurement simulation
float readVoltageRMS() {
  // Simulate voltage reading around 230V
  return 228.0 + (random(0, 41) / 10.0); // 228.0 to 232.0V
}

float readCurrentRMS() {
  // Simulate current reading 0.5A to 10.0A
  return 0.5 + (random(0, 96) / 10.0); // 0.5 to 10.0A
}

float calculatePowerFactor() {
  // Fixed power factor for this demo
  return 1.0;
}

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println(F("=== Energy Measurement GSM Sender ==="));
  Serial.println(F("Initializing..."));

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

  // Read energy measurements
  float voltage = readVoltageRMS();
  float current = readCurrentRMS();
  float pf = calculatePowerFactor();

  // Display readings
  Serial.print(F("Voltage: "));
  Serial.print(voltage, 1);
  Serial.println(F("V"));

  Serial.print(F("Current: "));
  Serial.print(current, 1);
  Serial.println(F("A"));

  Serial.print(F("Power Factor: "));
  Serial.println(pf, 1);

  int attempts = 0;
  bool success = false;

  // Retry logic
  while (attempts < MAX_RETRIES && !success) {
    attempts++;
    Serial.print(F("Attempt "));
    Serial.print(attempts);
    Serial.print(F(" of "));
    Serial.println(MAX_RETRIES);

    success = sendEnergyData(voltage, current, pf);

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

  // Wait 30 seconds before next reading
  Serial.println(F("\nWaiting 30 seconds before next request..."));
  delay(30000);
}