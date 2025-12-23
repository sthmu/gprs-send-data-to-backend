#ifndef GSM_MODULE_H
#define GSM_MODULE_H

#include <Arduino.h>
#include <SoftwareSerial.h>

// GSM Configuration
#define GSM_RX 6  // Connect to GSM TX
#define GSM_TX 7  // Connect to GSM RX
#define APN "ppwap"

// API Configuration
#define API_URL "energo.azurewebsites.net"
#define API_PATH "/api/energy-measurement"

// Function declarations
bool initGSM();
bool connectGPRS();
bool sendEnergyData(float v_rms, float i_rms, float pf);

#endif