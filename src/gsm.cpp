#include "gsm.h"

SoftwareSerial gsmSerial(GSM_RX, GSM_TX);

// Send AT command and wait for response
String sendATCommand(String cmd, unsigned long timeout = 2000) {
  gsmSerial.println(cmd);
  String response = "";
  unsigned long start = millis();

  while (millis() - start < timeout) {
    while (gsmSerial.available()) {
      response += (char)gsmSerial.read();
    }
    if (response.indexOf("OK") != -1 || response.indexOf("ERROR") != -1) {
      break;
    }
  }
  return response;
}

// Initialize GSM module
bool initGSM() {
  Serial.println("Initializing GSM...");
  gsmSerial.begin(9600);

  // Test connection
  if (sendATCommand("AT").indexOf("OK") == -1) {
    Serial.println("GSM not responding!");
    return false;
  }

  // Check SIM card
  if (sendATCommand("AT+CPIN?").indexOf("READY") == -1) {
    Serial.println("SIM card not ready!");
    return false;
  }

  // Wait for network registration
  for (int i = 0; i < 10; i++) {
    String reg = sendATCommand("AT+CREG?");
    if (reg.indexOf(",1") != -1 || reg.indexOf(",5") != -1) {
      Serial.println("Network registered");
      return true;
    }
    delay(2000);
  }

  Serial.println("Network registration failed!");
  return false;
}

// Connect to GPRS
bool connectGPRS() {
  Serial.println("Connecting to GPRS...");

  // Attach to GPRS
  if (sendATCommand("AT+CGATT=1").indexOf("OK") == -1) {
    return false;
  }

  // Set APN
  String apnCmd = "AT+SAPBR=3,1,\"APN\",\"" + String(APN) + "\"";
  if (sendATCommand(apnCmd).indexOf("OK") == -1) {
    return false;
  }

  // Open GPRS context
  if (sendATCommand("AT+SAPBR=1,1").indexOf("OK") == -1) {
    return false;
  }

  Serial.println("GPRS connected");
  return true;
}

// Send energy data via HTTP POST with query parameters
bool sendEnergyData(float v_rms, float i_rms, float pf) {
  Serial.println("Sending energy data...");

  // Initialize HTTP
  if (sendATCommand("AT+HTTPINIT").indexOf("OK") == -1) {
    return false;
  }

  // Set connection ID
  sendATCommand("AT+HTTPPARA=\"CID\",1");

  // Build URL with query parameters
  String query = "?v_rms=" + String(v_rms, 1) + "&i_rms=" + String(i_rms, 1) + "&pf=" + String(pf, 1);
  String url = "http://" + String(API_URL) + String(API_PATH) + query;
  String urlCmd = "AT+HTTPPARA=\"URL\",\"" + url + "\"";

  if (sendATCommand(urlCmd).indexOf("OK") == -1) {
    sendATCommand("AT+HTTPTERM");
    return false;
  }

  // Execute POST request
  String action = sendATCommand("AT+HTTPACTION=1", 15000);

  // Check response
  if (action.indexOf("+HTTPACTION:1,200,") != -1) {
    Serial.println("Data sent successfully!");
    sendATCommand("AT+HTTPTERM");
    return true;
  } else {
    Serial.println("Failed to send data");
    sendATCommand("AT+HTTPTERM");
    return false;
  }
}