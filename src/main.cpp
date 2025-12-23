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

  Serial.println("=== Energy Measurement GSM Sender ===");

  // Initialize GSM module
  if (!initGSM()) {
    Serial.println("GSM initialization failed!");
    while (1); // Stop here
  }

  // Connect to GPRS
  if (!connectGPRS()) {
    Serial.println("GPRS connection failed!");
    while (1); // Stop here
  }

  Serial.println("System ready!");
}

void loop() {
  // Read energy measurements
  float voltage = readVoltageRMS();
  float current = readCurrentRMS();
  float pf = calculatePowerFactor();

  // Display readings
  Serial.print("Voltage: ");
  Serial.print(voltage, 1);
  Serial.println("V");

  Serial.print("Current: ");
  Serial.print(current, 1);
  Serial.println("A");

  Serial.print("Power Factor: ");
  Serial.println(pf, 1);

  // Send data via GSM
  if (sendEnergyData(voltage, current, pf)) {
    Serial.println("✓ Data sent successfully!");
  } else {
    Serial.println("✗ Failed to send data");
  }

  // Wait 30 seconds before next reading
  Serial.println("Waiting 30 seconds...");
  delay(30000);
}