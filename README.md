# Arduino GSM HTTP POST Project

A comprehensive Arduino project that implements GSM HTTP POST requests using SIM800L/SIM900 modules for IoT applications.

## 📋 Features

### Core Functionality
- **GSM Module Integration**: Full SoftwareSerial communication with SIM800L/SIM900
- **Network Registration**: Automatic cellular network detection and registration
- **GPRS Connection**: Mobile data connection with configurable APN settings
- **HTTP POST Requests**: JSON payload transmission to REST APIs
- **Response Validation**: Server response parsing and status checking

### Advanced Features
- **Comprehensive Debugging**: Real-time AT command/response logging
- **Error Handling**: Detailed error messages and retry logic (3 attempts)
- **Signal Monitoring**: Network signal quality tracking
- **Operator Detection**: Automatic carrier identification
- **Connection Management**: Proper GPRS context handling

## 🔧 Hardware Requirements

### Components
- Arduino Uno (or compatible board)
- SIM800L or SIM900 GSM module
- SIM card with active data plan
- Antenna for GSM module
- Power supply (2A minimum for GSM module)

### Wiring Connections
```
GSM Module    →    Arduino
TX (Pin 10)   →    Digital Pin 10
RX (Pin 11)   →    Digital Pin 11
GND           →    GND
VCC           →    External 5V (2A supply)
```

## ⚙️ Configuration

### APN Settings
Update these in `src/main.cpp`:
```cpp
#define APN "your_apn_here"        // e.g., "ppwap", "internet", "airtelgprs.com"
#define APN_USER ""                // Usually empty
#define APN_PASS ""                // Usually empty
```

### API Configuration
```cpp
#define API_URL "your-api-domain.com"
#define API_PATH "/api/endpoint"
#define API_PORT "80"
```

### Common APN Settings by Carrier
- **Dialog (Sri Lanka)**: `ppwap`
- **AT&T (USA)**: `phone`
- **T-Mobile (USA)**: `fast.t-mobile.com`
- **Vodafone**: `internet`
- **Airtel**: `airtelgprs.com`

## 📡 API Integration

### Request Format
```json
{
  "sensor": "temperature",
  "value": 25.5,
  "device": "GSM_001"
}
```

### Expected Response
```json
{
  "status": "SUCCESS",
  "serverTime": "2025-12-17T10:30:00.000Z",
  "totalLogs": 5
}
```

## 🚀 Usage

### 1. Hardware Setup
1. Connect GSM module to Arduino as shown in wiring diagram
2. Insert SIM card with active data plan
3. Connect antenna to GSM module
4. Power GSM module with adequate 5V/2A supply

### 2. Software Setup
1. Install PlatformIO extension in VS Code
2. Open project folder
3. Update APN settings in `src/main.cpp`
4. Update API URL and path
5. Build and upload to Arduino

### 3. Monitoring
1. Open Serial Monitor at 9600 baud
2. Power on Arduino
3. Watch initialization sequence:
   - GSM module detection
   - Network registration
   - GPRS connection
   - HTTP POST attempts

## 📊 Serial Output

### Successful Operation
```
=== GSM HTTP POST Test ===
Initializing...
✓ GSM module responding
Signal quality: +CSQ: 27,0 OK
✓ SIM card ready
✓ Registered on network
Operator: +COPS: 0,0,"Dialog" OK
✓ GPRS connected
IP Address: +SAPBR: 1,1,"10.xxx.xxx.xxx"
✓ HTTP POST successful!
```

### Error Indicators
- `✗ ERROR: GSM module not responding` - Check wiring/power
- `✗ ERROR: SIM card not ready` - Check SIM card
- `✗ ERROR: Failed to register on network` - Check signal/antenna
- `✗ ERROR: Failed to open GPRS context` - Check APN settings
- `HTTP 301` - Server redirect (HTTPS enforcement)

## 🔍 Troubleshooting

### Common Issues

**1. GSM Module Not Responding**
- Check power supply (needs 2A minimum)
- Verify wiring connections
- Try different baud rate (9600/115200)

**2. SIM Card Issues**
- Ensure SIM has active data plan
- Check PIN requirements
- Try different SIM card

**3. Network Registration Failed**
- Check antenna connection
- Move to better signal area
- Verify SIM card compatibility

**4. GPRS Connection Failed**
- Verify APN settings for your carrier
- Check data plan activation
- Try different APN configurations

**5. HTTP 301 Redirect**
- Server enforces HTTPS - deploy on HTTP-friendly platform
- Use services like Railway, Render, or VPS

### Debug Commands
The code provides detailed debug output for every AT command. Look for:
- `[DEBUG] Sending: <command>`
- `[DEBUG] <Response>: <result>`
- HTTP status codes and responses

## 📁 Project Structure

```
Energo/
├── .vscode/           # VS Code settings
├── include/           # Header files
├── lib/              # Libraries
├── src/
│   └── main.cpp      # Main Arduino code
├── test/             # Test files
├── platformio.ini    # PlatformIO configuration
└── README.md         # This file
```

## 🛠️ Development

### Building
```bash
platformio run
```

### Uploading
```bash
platformio run --target upload --upload-port COM37
```

### Monitoring
```bash
platformio device monitor --port COM37 --baud 9600
```

## 📝 License

This project is open source. Feel free to modify and distribute.

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## 📞 Support

For issues and questions:
1. Check the troubleshooting section
2. Review serial debug output
3. Verify hardware connections
4. Test with different SIM cards/APNs

---

**Note**: This project requires a GSM module with active cellular data plan. Ensure compliance with local regulations for IoT deployments.</content>
<parameter name="filePath">README.md