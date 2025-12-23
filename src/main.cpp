#include <Arduino.h>
#include <SoftwareSerial.h>

// GSM Module pins (adjust according to your wiring)
#define GSM_RX 9  // Connect to GSM TX
#define GSM_TX 10  // Connect to GSM RX

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

SoftwareSerial gsmSerial(GSM_RX, GSM_TX);

// Function declarations
bool initGSM();
bool connectGPRS();
bool sendHTTPPost();
bool checkResponse(String response);
String readGSMResponse(unsigned long timeout = 5000);
void sendATCommand(String command, unsigned long timeout = 1000);
bool waitForResponse(String expected, unsigned long timeout = 10000);

void setup() {
  // Initialize Serial Monitor
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

bool initGSM() {
  Serial.println(F("Initializing GSM module..."));
  
  // Test AT command
  gsmSerial.println(F("AT"));
  if (!waitForResponse("OK", 2000)) {
    Serial.println(F("✗ ERROR: GSM module not responding"));
    return false;
  }
  Serial.println(F("✓ GSM module responding"));
  
  // Enable verbose error messages
  gsmSerial.println(F("AT+CMEE=2"));
  waitForResponse("OK", 1000);
  
  // Disable echo
  gsmSerial.println(F("ATE0"));
  waitForResponse("OK", 1000);
  
  // Check signal quality
  gsmSerial.println(F("AT+CSQ"));
  String response = readGSMResponse(2000);
  Serial.print(F("Signal quality: "));
  Serial.println(response);
  
  // Check SIM card
  gsmSerial.println(F("AT+CPIN?"));
  String pinResponse = readGSMResponse(2000);
  if (pinResponse.indexOf("READY") == -1) {
    Serial.println(F("✗ ERROR: SIM card not ready!"));
    Serial.print(F("Response: "));
    Serial.println(pinResponse);
    return false;
  }
  Serial.println(F("✓ SIM card ready"));
  
  // Check network registration status
  Serial.println(F("Checking network registration..."));
  int regAttempts = 0;
  bool registered = false;
  
  while (regAttempts < 10 && !registered) {
    gsmSerial.println(F("AT+CREG?"));
    String regResponse = readGSMResponse(2000);
    Serial.print(F("CREG Response: "));
    Serial.println(regResponse);
    
    // Check for registration status
    // +CREG: 0,1 = registered, home network
    // +CREG: 0,5 = registered, roaming
    if (regResponse.indexOf(",1") != -1 || regResponse.indexOf(",5") != -1) {
      registered = true;
      Serial.println(F("✓ Registered on network"));
      
      // Get operator name
      gsmSerial.println(F("AT+COPS?"));
      String opsResponse = readGSMResponse(2000);
      Serial.print(F("Operator: "));
      Serial.println(opsResponse);
    } else {
      regAttempts++;
      Serial.print(F("Not registered yet, attempt "));
      Serial.print(regAttempts);
      Serial.println(F("/10"));
      delay(2000);
    }
  }
  
  if (!registered) {
    Serial.println(F("✗ ERROR: Failed to register on network!"));
    Serial.println(F("Check SIM card, antenna, and signal strength"));
    return false;
  }
  
  return true;
}

bool connectGPRS() {
  Serial.println(F("Connecting to GPRS..."));
  
  // Verify network registration one more time
  gsmSerial.println(F("AT+CREG?"));
  String regCheck = readGSMResponse(2000);
  Serial.print(F("Network status: "));
  Serial.println(regCheck);
  
  if (regCheck.indexOf(",1") == -1 && regCheck.indexOf(",5") == -1) {
    Serial.println(F("✗ WARNING: Not registered on network!"));
  }
  
  // Close any existing GPRS context first
  Serial.println(F("[DEBUG] Closing any existing GPRS context..."));
  gsmSerial.println(F("AT+SAPBR=0,1"));
  delay(2000);
  String closeResp = readGSMResponse(2000);
  Serial.print(F("[DEBUG] Close Response: "));
  Serial.println(closeResp);
  
  // Attach to GPRS service
  Serial.println(F("[DEBUG] Sending: AT+CGATT=1"));
  gsmSerial.println(F("AT+CGATT=1"));
  String attachResponse = readGSMResponse(10000);
  Serial.print(F("[DEBUG] CGATT Response: "));
  Serial.println(attachResponse);
  if (attachResponse.indexOf("OK") == -1) {
    Serial.println(F("✗ ERROR: Failed to attach to GPRS"));
    Serial.print(F("Response: "));
    Serial.println(attachResponse);
    return false;
  }
  Serial.println(F("✓ GPRS attached"));
  
  // Set connection type to GPRS
  Serial.println(F("[DEBUG] Sending: AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\""));
  gsmSerial.println(F("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\""));
  String contypeResp = readGSMResponse(2000);
  Serial.print(F("[DEBUG] CONTYPE Response: "));
  Serial.println(contypeResp);
  
  // Set APN
  String apnCommand = "AT+SAPBR=3,1,\"APN\",\"" + String(APN) + "\"";
  Serial.print(F("[DEBUG] Sending: "));
  Serial.println(apnCommand);
  gsmSerial.println(apnCommand);
  String apnResp = readGSMResponse(2000);
  Serial.print(F("[DEBUG] APN Response: "));
  Serial.println(apnResp);
  
  // Set APN user if needed
  if (strlen(APN_USER) > 0) {
    String userCommand = "AT+SAPBR=3,1,\"USER\",\"" + String(APN_USER) + "\"";
    Serial.print(F("[DEBUG] Sending: "));
    Serial.println(userCommand);
    gsmSerial.println(userCommand);
    String userResp = readGSMResponse(2000);
    Serial.print(F("[DEBUG] USER Response: "));
    Serial.println(userResp);
  }
  
  // Set APN password if needed
  if (strlen(APN_PASS) > 0) {
    String passCommand = "AT+SAPBR=3,1,\"PWD\",\"" + String(APN_PASS) + "\"";
    Serial.print(F("[DEBUG] Sending: "));
    Serial.println(passCommand);
    gsmSerial.println(passCommand);
    String passResp = readGSMResponse(2000);
    Serial.print(F("[DEBUG] PWD Response: "));
    Serial.println(passResp);
  }
  
  // Open GPRS context
  Serial.println(F("[DEBUG] Sending: AT+SAPBR=1,1"));
  gsmSerial.println(F("AT+SAPBR=1,1"));
  String openResponse = readGSMResponse(30000);
  Serial.print(F("[DEBUG] SAPBR Open Response: "));
  Serial.println(openResponse);
  
  // Check if already connected (error code 1 means already connected)
  if (openResponse.indexOf("OK") == -1 && openResponse.indexOf("ERROR") != -1) {
    Serial.println(F("⚠ GPRS context may already be open, checking status..."));
    
    // Query current status
    gsmSerial.println(F("AT+SAPBR=2,1"));
    String statusResp = readGSMResponse(2000);
    Serial.print(F("[DEBUG] Status check: "));
    Serial.println(statusResp);
    
    // If we have an IP, we're already connected
    if (statusResp.indexOf("SAPBR: 1,1,") != -1) {
      Serial.println(F("✓ GPRS context already active"));
    } else {
      Serial.println(F("✗ ERROR: Failed to open GPRS context"));
      Serial.print(F("Response: "));
      Serial.println(openResponse);
      return false;
    }
  } else if (openResponse.indexOf("OK") != -1) {
    Serial.println(F("✓ GPRS context opened"));
  } else {
    Serial.println(F("✗ ERROR: Unexpected GPRS response"));
    return false;
  }
  
  // Get IP address
  Serial.println(F("[DEBUG] Sending: AT+SAPBR=2,1"));
  gsmSerial.println(F("AT+SAPBR=2,1"));
  String response = readGSMResponse(2000);
  Serial.print(F("[DEBUG] IP Query Response: "));
  Serial.println(response);
  Serial.print(F("IP Address: "));
  Serial.println(response);
  
  Serial.println(F("✓ GPRS connected"));
  return true;
}

bool sendHTTPPost() {
  Serial.println(F("Sending HTTP POST request..."));
  
  // Terminate any existing HTTP session first
  Serial.println(F("[DEBUG] Sending: AT+HTTPTERM"));
  gsmSerial.println(F("AT+HTTPTERM"));
  delay(500);
  String termResp = readGSMResponse(1000);
  Serial.print(F("[DEBUG] HTTPTERM Response: "));
  Serial.println(termResp);
  
  // Initialize HTTP service
  Serial.println(F("[DEBUG] Sending: AT+HTTPINIT"));
  gsmSerial.println(F("AT+HTTPINIT"));
  String initResp = readGSMResponse(2000);
  Serial.print(F("HTTPINIT: "));
  Serial.println(initResp);
  if (initResp.indexOf("OK") == -1) {
    Serial.println(F("✗ ERROR: HTTP init failed"));
    return false;
  }
  Serial.println(F("✓ HTTP initialized"));
  
  // Set HTTP parameters
  Serial.println(F("[DEBUG] Sending: AT+HTTPPARA=\"CID\",1"));
  gsmSerial.println(F("AT+HTTPPARA=\"CID\",1"));
  String cidResp = readGSMResponse(1000);
  Serial.print(F("[DEBUG] CID Response: "));
  Serial.println(cidResp);
  if (cidResp.indexOf("ERROR") != -1) {
    Serial.println(F("✗ WARNING: CID parameter failed"));
  }
  
  // Generate random energy measurement data
  // v_rms: Voltage RMS around 230V (228-232V range)
  float v_rms = 228.0 + (random(0, 41) / 10.0);  // 228.0 to 232.0 in 0.1V steps
  
  // i_rms: Current RMS (0.5A to 10.0A)
  float i_rms = 0.5 + (random(0, 96) / 10.0);  // 0.5 to 10.0 in 0.1A steps
  
  // pf: Power factor (fixed at 1.0)
  float pf = 1.0;
  
  Serial.print(F("Data: v_rms="));
  Serial.print(v_rms, 1);
  Serial.print(F(", i_rms="));
  Serial.print(i_rms, 1);
  Serial.print(F(", pf="));
  Serial.println(pf, 1);
  
  // Build URL with query parameters (workaround for SIM900 Content-Type limitation)
  String queryParams = "?v_rms=" + String(v_rms, 1) + "&i_rms=" + String(i_rms, 1) + "&pf=" + String(pf, 1);
  String fullUrl = "http://" + String(API_URL) + String(API_PATH) + queryParams;
  
  Serial.println(F("[INFO] Using query parameters (SIM900 workaround)"));
  Serial.print(F("[INFO] Full URL: "));
  Serial.println(fullUrl);
  
  // Set URL with query parameters
  String urlCommand = "AT+HTTPPARA=\"URL\",\"" + fullUrl + "\"";
  Serial.print(F("[DEBUG] Sending: "));
  Serial.println(urlCommand);
  gsmSerial.println(urlCommand);
  String urlResp = readGSMResponse(2000);
  Serial.print(F("[DEBUG] URL Response: "));
  Serial.println(urlResp);
  if (urlResp.indexOf("OK") == -1) {
    Serial.println(F("✗ ERROR: URL set failed"));
    gsmSerial.println(F("AT+HTTPTERM"));
    return false;
  }
  Serial.println(F("✓ URL set with query parameters"));
  
  // Execute HTTP POST
  Serial.println(F("[DEBUG] Sending: AT+HTTPACTION=1"));
  Serial.println(F("Executing POST request..."));
  gsmSerial.println(F("AT+HTTPACTION=1"));  // 1 = POST
  
  // First wait for OK response
  String okResponse = readGSMResponse(2000);
  Serial.print(F("[DEBUG] AT+HTTPACTION OK: "));
  Serial.println(okResponse);
  
  // Then wait for the actual +HTTPACTION unsolicited response
  Serial.println(F("[DEBUG] Waiting for +HTTPACTION response..."));
  unsigned long startWait = millis();
  String actionResponse = "";
  
  while (millis() - startWait < 30000) {
    if (gsmSerial.available()) {
      char c = gsmSerial.read();
      actionResponse += c;
      Serial.print(c);  // Show characters as they arrive
    }
    
    // Check if we got the complete +HTTPACTION response
    if (actionResponse.indexOf("+HTTPACTION") != -1 && actionResponse.indexOf("\n") > actionResponse.indexOf("+HTTPACTION")) {
      break;
    }
  }
  
  Serial.println();
  Serial.print(F("[DEBUG] HTTP Action Response: "));
  Serial.println(actionResponse);
  
  // Parse HTTP status code
  // Response format: +HTTPACTION: 1,<status_code>,<data_len>
  if (actionResponse.indexOf("+HTTPACTION") != -1) {
    int firstComma = actionResponse.indexOf(',');
    int secondComma = actionResponse.indexOf(',', firstComma + 1);
    
    if (firstComma != -1 && secondComma != -1) {
      String statusCode = actionResponse.substring(firstComma + 1, secondComma);
      String dataLen = actionResponse.substring(secondComma + 1);
      statusCode.trim();
      dataLen.trim();
      
      Serial.print(F("[DEBUG] HTTP Status Code: "));
      Serial.println(statusCode);
      Serial.print(F("[DEBUG] Response Data Length: "));
      Serial.print(dataLen);
      Serial.println(F(" bytes"));
      
      int code = statusCode.toInt();
      if (code == 200) {
        Serial.println(F("✓ HTTP 200 OK - Success!"));
      } else if (code == 0) {
        Serial.println(F("✗ ERROR: HTTP request failed (timeout or connection error)"));
        gsmSerial.println(F("AT+HTTPTERM"));
        return false;
      } else if (code >= 400 && code < 500) {
        Serial.print(F("✗ HTTP Client Error: "));
        Serial.println(code);
        if (code == 404) Serial.println(F("  → Endpoint not found"));
        else if (code == 400) Serial.println(F("  → Bad request - check JSON format"));
      } else if (code >= 500) {
        Serial.print(F("✗ HTTP Server Error: "));
        Serial.println(code);
        Serial.println(F("  → Server failed to process request"));
      } else {
        Serial.print(F("⚠ HTTP Code: "));
        Serial.println(code);
      }
    }
  } else {
    Serial.println(F("✗ ERROR: No HTTP response received"));
    gsmSerial.println(F("AT+HTTPTERM"));
    return false;
  }
  
  // Read HTTP response
  Serial.println(F("[DEBUG] Reading server response..."));
  delay(1000);
  Serial.println(F("[DEBUG] Sending: AT+HTTPREAD"));
  gsmSerial.println(F("AT+HTTPREAD"));
  String httpResponse = readGSMResponse(10000);
  
  Serial.println(F("\n========== SERVER RESPONSE START =========="));
  Serial.println(httpResponse);
  Serial.println(F("========== SERVER RESPONSE END ============\n"));
  
  // Try to extract just the JSON/text content
  if (httpResponse.indexOf("+HTTPREAD:") != -1) {
    int contentStart = httpResponse.indexOf('\n', httpResponse.indexOf("+HTTPREAD:"));
    if (contentStart != -1) {
      String content = httpResponse.substring(contentStart + 1);
      content.trim();
      Serial.println(F("[DEBUG] Extracted Response Body:"));
      Serial.println(content);
    }
  }
  
  // Terminate HTTP service
  gsmSerial.println(F("AT+HTTPTERM"));
  waitForResponse("OK", 2000);
  
  // Check if response contains SUCCESS
  bool success = checkResponse(httpResponse);
  return success;
}

bool checkResponse(String response) {
  // Check for "OK" in the response
  if (response.indexOf("OK") != -1) {
    Serial.println(F("✓ Server returned OK status"));
    return true;
  } else {
    Serial.println(F("✗ Response does not contain OK status"));
    return false;
  }
}

String readGSMResponse(unsigned long timeout) {
  String response = "";
  unsigned long startTime = millis();
  
  while (millis() - startTime < timeout) {
    while (gsmSerial.available()) {
      char c = gsmSerial.read();
      response += c;
    }
    
    // Check if we have a complete response
    if (response.length() > 0 && 
        (response.indexOf("OK") != -1 || 
         response.indexOf("ERROR") != -1 ||
         response.indexOf("DOWNLOAD") != -1)) {
      break;
    }
  }
  
  return response;
}

bool waitForResponse(String expected, unsigned long timeout) {
  String response = readGSMResponse(timeout);
  return (response.indexOf(expected) != -1);
}

void sendATCommand(String command, unsigned long timeout) {
  gsmSerial.println(command);
  String response = readGSMResponse(timeout);
  Serial.print(F("CMD: "));
  Serial.print(command);
  Serial.print(F(" -> "));
  Serial.println(response);
}