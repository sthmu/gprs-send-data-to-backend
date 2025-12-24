#include <Arduino.h>
#include <SoftwareSerial.h>
#include "gsm.h"

/* ---------------- ANALOG PINS ---------------- */
// Phase order: 0 = R, 1 = Y, 2 = B
const int currentPins[3] = {A0, A2, A6};
const int voltagePins[3] = {A1, A3, A7};

/* ---------------- SAMPLING ---------------- */
const int samples = 400;
const int sampleDelayUs = 50;

/* ---------------- CALIBRATION ---------------- */
float currentZero[3] = {512, 512, 512};
float voltageZero[3] = {512, 512, 512};

/* ---------------- SENSOR CONSTANTS ---------------- */
const float currentSensitivity = 0.066;   // ACS712-30A
const float voltageSlope = 230.0 / 0.85;  // Adjust for your voltage sensor

/* ---------------- RESULTS ---------------- */
float voltageRMS[3];
float currentRMS[3];

/* =================================================
   CALIBRATION
   ================================================= */
void calibrateSensors() {
  long sumC[3] = {0, 0, 0};
  long sumV[3] = {0, 0, 0};

  for (int i = 0; i < 1000; i++) {
    for (int p = 0; p < 3; p++) {
      sumC[p] += analogRead(currentPins[p]);
      sumV[p] += analogRead(voltagePins[p]);
    }
    delay(2);
  }

  for (int p = 0; p < 3; p++) {
    currentZero[p] = sumC[p] / 1000.0;
    voltageZero[p] = sumV[p] / 1000.0;
  }
}

/* =================================================
   READ RMS FOR EACH PHASE
   ================================================= */
void readThreePhase() {
  for (int p = 0; p < 3; p++) {
    unsigned long sumV2 = 0;
    unsigned long sumC2 = 0;

    for (int i = 0; i < samples; i++) {
      float v = analogRead(voltagePins[p]) - voltageZero[p];
      float c = analogRead(currentPins[p]) - currentZero[p];

      sumV2 += v * v;
      sumC2 += c * c;

      delayMicroseconds(sampleDelayUs);
    }

    float rmsVadc = sqrt(sumV2 / samples);
    float rmsCadc = sqrt(sumC2 / samples);

    float sensorV = rmsVadc * (5.0 / 1023.0);
    float sensorC = rmsCadc * (5.0 / 1023.0);

    voltageRMS[p] = sensorV * voltageSlope;
    currentRMS[p] = sensorC / currentSensitivity;

    if (voltageRMS[p] < 5) voltageRMS[p] = 0;
    if (currentRMS[p] < 0.08) currentRMS[p] = 0;
  }
}

void sendToESP() {
  Serial.println(F("\n--- Sending Data to ESP32 ---"));

  Serial.print("<DATA>");
  Serial.print("V1:");
  Serial.print(voltageRMS[0], 1);
  Serial.print(" ");
  Serial.print("V2:");
  Serial.print(voltageRMS[1], 1);
  Serial.print(" ");
  Serial.print("V3:");
  Serial.print(voltageRMS[2], 1);
  Serial.print(" ");
  Serial.print("I1:");
  Serial.print(currentRMS[0], 2);
  Serial.print(" ");
  Serial.print("I2:");
  Serial.print(currentRMS[1], 2);
  Serial.print(" ");
  Serial.print("I3:");
  Serial.print(currentRMS[2], 2);
  Serial.println("</DATA>");

  Serial.println(F("--- Data Sent ---"));
}

SoftwareSerial gsmSerial(GSM_RX, GSM_TX);

void setup() {
  Serial.begin(9600);
  
  calibrateSensors();
  
  Serial.println(F("=== GSM 3-Phase Energy Monitor ==="));
  
  // Initialize GSM Serial
  gsmSerial.begin(9600);
  delay(3000);
  
  // Calibration Countdown: Warn user and countdown
  Serial.println(F("Prepare for calibration - remove all loads!"));
  for (int i = 3; i > 0; i--) {
    Serial.print(F("Calibration in: "));
    Serial.println(i);
    delay(1000);
  }
  
  Serial.println(F("Calibrating sensors..."));
  calibrateSensors();
  Serial.println(F("Calibration complete."));
  
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
  Serial.println(F("\n--- Reading 3-Phase Measurements ---"));
  
  // Read current and voltage measurements
  readThreePhase();
  
  // Send data to ESP32
  sendToESP();
  
  // Display readings
  for (int p = 0; p < 3; p++) {
    Serial.print(F("Phase "));
    Serial.print(p + 1);
    Serial.print(F(" - Voltage: "));
    Serial.print(voltageRMS[p], 1);
    Serial.print(F("V, Current: "));
    Serial.print(currentRMS[p], 2);
    Serial.println(F("A"));
  }
  
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
    
    success = sendHTTPPost(voltageRMS[0], currentRMS[0], 0.95, voltageRMS[1], currentRMS[1], 0.92, voltageRMS[2], currentRMS[2], 0.98);
    
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
  
  delay(5000);
}