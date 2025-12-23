#ifndef GSM_MODULE_H
#define GSM_MODULE_H

#include <Arduino.h>
#include <SoftwareSerial.h>

// GSM Module pins (adjust according to your wiring)
#define GSM_RX 6  // Connect to GSM TX
#define GSM_TX 7  // Connect to GSM RX

// APN Configuration (Update with your network provider's APN)
#define APN "ppwap"           // Example: "internet", "airtelgprs.com", "www"
#define APN_USER ""              // Usually empty for most providers
#define APN_PASS ""              // Usually empty for most providers

// API Configuration
#define API_URL "energo.azurewebsites.net"  // Use domain name directly
#define API_PATH "/api/energy-measurement"
#define API_PORT "80"

// Retry configuration
#define MAX_RETRIES 3
#define RETRY_DELAY 5000  // 5 seconds between retries

// Function declarations
bool initGSM();
bool connectGPRS();
bool sendEnergyData(float v_rms, float i_rms, float pf);
bool checkResponse(String response);
String readGSMResponse(unsigned long timeout = 5000);
void sendATCommand(String command, unsigned long timeout = 1000);
bool waitForResponse(String expected, unsigned long timeout = 10000);

#endif