# SerialJSON Usage Guide

## Overview

The `SerialJSON` class is a new SerialIO implementation that converts CRSF (CrossFire) telemetry frames into JSON format before sending them to the serial output port. This makes it easy to process ExpressLRS telemetry data with JSON parsers in ground station software, logging systems, or custom applications.

## Features

- **Complete CRSF Frame Support**: Handles all major CRSF frame types including:
  - RC Channels (packed and raw)
  - Link Statistics (RSSI, SNR, Link Quality)
  - GPS Data (position, speed, heading, altitude)
  - Battery Information (voltage, current, capacity, percentage)
  - Attitude Data (pitch, roll, yaw)
  - Barometer/Vario (altitude, vertical speed)
  - Flight Mode
  - Airspeed
  - RPM Data (up to 19 motors)
  - Temperature Sensors (up to 20 sensors)
  - Battery Cells (up to 29 cells)
  - MSP Frames

- **Real-time Conversion**: Incoming CRSF frames are parsed and immediately converted to JSON
- **Timestamp Support**: Each JSON message includes a millisecond timestamp
- **Error Handling**: Invalid frames are handled gracefully
- **Compact Output**: JSON is minimized for efficient transmission

## JSON Output Examples

### RC Channels
```json
{"type":"rc_channels","timestamp":12345678,"channels":[172,992,1811,500,750,1000,1250,1500,172,500,750,1000,1250,1500,1750,1811],"frame_missed":0}
```

### Link Statistics  
```json
{"type":"link_statistics","timestamp":12345678,"uplink_rssi_1":85,"uplink_rssi_2":88,"uplink_quality":95,"uplink_snr":12,"active_antenna":1,"rf_mode":2,"uplink_power":4,"downlink_rssi":82,"downlink_quality":92,"downlink_snr":10}
```

### GPS Data
```json
{"type":"gps","timestamp":12345678,"latitude_deg":37.421998,"longitude_deg":-122.084000,"groundspeed_kmh":15.5,"heading_deg":245.67,"altitude_m":125,"satellites":12}
```

### Battery Information
```json
{"type":"battery","timestamp":12345678,"voltage_mv":16800,"current_ma":2500,"capacity_mah":5000,"remaining_percent":75}
```

### Attitude Data
```json
{"type":"attitude","timestamp":12345678,"pitch_rad":0.0523,"roll_rad":-0.0349,"yaw_rad":1.5708}
```

## Configuration

To use SerialJSON instead of the default CRSF output:

1. **Include the header** in your receiver configuration:
```cpp
#include "SerialJSON.h"
```

2. **Replace SerialCRSF with SerialJSON** in the serial protocol initialization:
```cpp
// Instead of:
// serialIO = new SerialCRSF(Serial, Serial);

// Use:
serialIO = new SerialJSON(Serial, Serial);
```

3. **Build and flash** your receiver with the new configuration.

## Integration Examples

### Python Ground Station
```python
import serial
import json

# Open serial connection to receiver
ser = serial.Serial('/dev/ttyUSB0', 420000)

while True:
    line = ser.readline().decode('utf-8').strip()
    try:
        data = json.loads(line)
        
        if data['type'] == 'gps':
            print(f"GPS: {data['latitude_deg']}, {data['longitude_deg']}")
        elif data['type'] == 'battery':
            print(f"Battery: {data['voltage_mv']}mV, {data['remaining_percent']}%")
        elif data['type'] == 'link_statistics':
            print(f"Link Quality: {data['uplink_quality']}%")
            
    except json.JSONDecodeError:
        pass  # Skip invalid JSON
```

### JavaScript/Node.js Logger
```javascript
const SerialPort = require('serialport');
const fs = require('fs');

const port = new SerialPort('/dev/ttyUSB0', { baudRate: 420000 });
const logFile = fs.createWriteStream('telemetry.log');

port.on('data', (data) => {
    const lines = data.toString().split('\n');
    
    lines.forEach(line => {
        if (line.trim()) {
            try {
                const telemetry = JSON.parse(line);
                logFile.write(JSON.stringify(telemetry) + '\n');
                
                // Real-time processing
                if (telemetry.type === 'rc_channels') {
                    console.log('Throttle:', telemetry.channels[2]);
                }
            } catch (e) {
                // Skip invalid JSON
            }
        }
    });
});
```

## Performance Considerations

- **Bandwidth**: JSON output is larger than binary CRSF, approximately 3-5x increase
- **Processing**: JSON parsing adds minimal CPU overhead on modern systems  
- **Buffering**: The implementation uses a 512-byte output buffer to minimize serial interruptions
- **Real-time**: Conversion happens in the receive interrupt, ensuring minimal latency

## Frame Type Coverage

| CRSF Frame Type | JSON Type | Status | Notes |
|----------------|-----------|---------|-------|
| RC_CHANNELS_PACKED | rc_channels_packed | ✅ | All 16 channels |
| LINK_STATISTICS | link_statistics | ✅ | RSSI, SNR, LQ |
| BATTERY_SENSOR | battery | ✅ | Voltage, current, capacity |
| GPS | gps | ✅ | Position, speed, heading |
| BARO_ALTITUDE | barometer | ✅ | Altitude, vertical speed |
| ATTITUDE | attitude | ✅ | Pitch, roll, yaw |
| FLIGHT_MODE | flight_mode | ✅ | Mode string |
| VARIO | vario | ✅ | Vertical speed |
| AIRSPEED | airspeed | ✅ | Speed in km/h |
| RPM | rpm | ✅ | Multi-motor support |
| TEMP | temperature | ✅ | Multiple sensors |
| CELLS | cells | ✅ | Individual cell voltages |
| MSP_* | msp_frame | ✅ | Raw MSP data |
| Unknown | unknown | ✅ | Raw payload dump |

## Troubleshooting

### No JSON Output
- Verify SerialJSON is being used instead of SerialCRSF
- Check serial baud rate matches your ground station
- Ensure receiver is receiving telemetry from flight controller

### Malformed JSON
- Check for buffer overflows in high-traffic scenarios
- Verify ground station can handle high-frequency JSON messages
- Consider increasing serial buffer sizes if needed

### Missing Frame Types
- Unknown frame types are output as "unknown" with raw payload
- Add custom handlers in `convertCRSFToJSON()` for new frame types
- Check CRSF protocol documentation for frame structure

## License

This implementation follows the same license as ExpressLRS.