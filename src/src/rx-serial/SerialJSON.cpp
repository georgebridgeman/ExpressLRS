#include "SerialJSON.h"
#include "common.h"
#include "CRSF.h"
#include <stdio.h>
#include <string.h>

uint32_t SerialJSON::sendRCFrame(bool frameAvailable, bool frameMissed, uint32_t *channelData)
{
    if (!frameAvailable)
        return DURATION_IMMEDIATELY;

    // DBGVLN("got RC frame");
    // writeJSONStart();
    // appendToOutput("\"type\":\"rc_channels\",");
    // appendToOutput("\"timestamp\":");
    // char temp[32];
    // snprintf(temp, sizeof(temp), "%lu,", millis());
    // appendToOutput(temp);
    
    // appendToOutput("\"channels\":[");
    // for (int i = 0; i < 16; i++) {
    //     snprintf(temp, sizeof(temp), "%lu", channelData[i]);
    //     appendToOutput(temp);
    //     if (i < 15) appendToOutput(",");
    // }
    // appendToOutput("],");
    
    // writeJSONFieldInt("frame_missed", frameMissed ? 1 : 0, true);
    // writeJSONEnd();
    // flushOutput();
    
    return DURATION_IMMEDIATELY;
}

void SerialJSON::queueMSPFrameTransmission(uint8_t* data)
{
    // For JSON output, we'll immediately convert and send MSP frames
    // writeJSONStart();
    // appendToOutput("\"type\":\"msp_frame\",");
    // appendToOutput("\"timestamp\":");
    // char temp[32];
    // snprintf(temp, sizeof(temp), "%lu,", millis());
    // appendToOutput(temp);
    
    // const uint8_t totalBufferLen = CRSF_FRAME_SIZE(data[1]);
    // appendToOutput("\"length\":");
    // snprintf(temp, sizeof(temp), "%d,", totalBufferLen);
    // appendToOutput(temp);
    
    // appendToOutput("\"data\":[");
    // for (int i = 0; i < totalBufferLen && i < CRSF_FRAME_SIZE_MAX; i++) {
    //     snprintf(temp, sizeof(temp), "%d", data[i]);
    //     appendToOutput(temp);
    //     if (i < totalBufferLen - 1) appendToOutput(",");
    // }
    // appendToOutput("]");
    // writeJSONEnd();
    flushOutput();
}

void SerialJSON::queueLinkStatisticsPacket()
{
    // writeJSONStart();
    // appendToOutput("\"type\":\"link_statistics\",");
    // appendToOutput("\"timestamp\":");
    // char temp[32];
    // snprintf(temp, sizeof(temp), "%lu,", millis());
    // appendToOutput(temp);
    
    // writeJSONFieldInt("uplink_rssi_1", (int32_t)CRSF::LinkStatistics.uplink_RSSI_1);
    // writeJSONFieldInt("uplink_rssi_2", (int32_t)CRSF::LinkStatistics.uplink_RSSI_2);
    // writeJSONFieldInt("uplink_quality", (int32_t)CRSF::LinkStatistics.uplink_Link_quality);
    // writeJSONFieldInt("uplink_snr", (int32_t)CRSF::LinkStatistics.uplink_SNR);
    // writeJSONFieldInt("active_antenna", (int32_t)CRSF::LinkStatistics.active_antenna);
    // writeJSONFieldInt("rf_mode", (int32_t)CRSF::LinkStatistics.rf_Mode);
    // writeJSONFieldInt("uplink_power", (int32_t)CRSF::LinkStatistics.uplink_TX_Power);
    // writeJSONFieldInt("downlink_rssi", (int32_t)CRSF::LinkStatistics.downlink_RSSI_1);
    // writeJSONFieldInt("downlink_quality", (int32_t)CRSF::LinkStatistics.downlink_Link_quality);
    // writeJSONFieldInt("downlink_snr", (int32_t)CRSF::LinkStatistics.downlink_SNR, true);
    
    // writeJSONEnd();
    flushOutput();
}

void SerialJSON::processRFTelemetryPacket(const uint8_t* telemetryData, uint8_t dataLen)
{
    // Process RF telemetry data - this might be fragmented CRSF data
    // We'll attempt to parse it as individual bytes like serial input
    logDebugMessage("RF_TLM_START", dataLen, telemetryData ? telemetryData[0] : 0);
    if (dataLen > 0 && telemetryData != nullptr) {
        ParsingState parseState = WAITING_FOR_SYNC;
        uint8_t frameBuffer[CRSF_MAX_PACKET_LEN];
        uint8_t frameLength = 0;
        uint8_t framePosition = 0;

        // Process the telemetry chunk byte by byte through our CRSF parser
        // This will handle fragmented frames and reassemble them
        for (uint8_t i = 0; i < dataLen; i++) {
            uint8_t byte = telemetryData[i];
            
            // Skip null bytes (padding)
            // if (byte == 0) continue;
            
            switch (parseState) {
                case WAITING_FOR_SYNC:
                    if (byte == CRSF_SYNC_BYTE || byte == CRSF_ADDRESS_RADIO_TRANSMITTER || 
                        byte == CRSF_ADDRESS_CRSF_RECEIVER || byte == CRSF_ADDRESS_FLIGHT_CONTROLLER) {
                        frameBuffer[0] = byte;
                        framePosition = 1;
                        parseState = READING_LENGTH;
                    }
                    break;
                    
                case READING_LENGTH:
                    if (byte >= CRSF_MAX_PACKET_LEN) {
                        DBGLN("byte %u >= CRSF_MAX_PACKET_LEN", byte);
                        return;
                    } else {
                        frameLength = byte;
                        frameBuffer[1] = byte;
                        framePosition = 2;
                        parseState = READING_FRAME;
                    }
                    break;
                    
                case READING_FRAME:
                    frameBuffer[framePosition] = byte;
                    if (framePosition == frameLength + 1) { // Reached the CRC byte?
                        // Frame complete, validate CRC and convert
                        uint8_t crc = crsf_crc.calc(telemetryData + CRSF_FRAME_NOT_COUNTED_BYTES, telemetryData[CRSF_TELEMETRY_LENGTH_INDEX] - CRSF_TELEMETRY_CRC_LENGTH);
                        logDebugMessage("RF_FRAME_CHECK_FIXED", frameBuffer[2], framePosition);
                        if (crc == frameBuffer[framePosition]) {
                            logDebugMessage("RF_FRAME_VALID", frameBuffer[2], framePosition);
                            convertCRSFToJSON(frameBuffer, framePosition + 1);
                        } else {
                            logDebugMessage("RF_FRAME_CRC_BAD", crc, frameBuffer[framePosition]);
                        }
                    }
                    framePosition++;
                    break;
            }
        }
    }
}

void SerialJSON::logDebugMessage(const char* event, int value1, int value2)
{
    writeJSONStart();
    appendToOutput("\"type\":\"debug\",");
    appendToOutput("\"timestamp\":");
    char temp[32];
    snprintf(temp, sizeof(temp), "%lu,", millis());
    appendToOutput(temp);
    
    writeJSONFieldString("event", event);
    writeJSONFieldInt("value1", value1);
    writeJSONFieldInt("value2", value2, true);
    
    writeJSONEnd();
    flushOutput();
}

void SerialJSON::processBytes(uint8_t *bytes, uint16_t size)
{
    // for (uint16_t i = 0; i < size; i++) {
    //     uint8_t byte = bytes[i];
        
    //     switch (parseState) {
    //         case WAITING_FOR_SYNC:
    //             if (byte == CRSF_SYNC_BYTE || byte == CRSF_ADDRESS_RADIO_TRANSMITTER || 
    //                 byte == CRSF_ADDRESS_CRSF_RECEIVER || byte == CRSF_ADDRESS_FLIGHT_CONTROLLER) {
    //                 frameBuffer[0] = byte;
    //                 framePosition = 1;
    //                 parseState = READING_LENGTH;
    //             }
    //             break;
                
    //         case READING_LENGTH:
    //             if (byte >= CRSF_MAX_PACKET_LEN) {
    //                 resetParser();
    //             } else {
    //                 frameLength = byte;
    //                 frameBuffer[1] = byte;
    //                 framePosition = 2;
    //                 parseState = READING_FRAME;
    //             }
    //             break;
                
    //         case READING_FRAME:
    //             frameBuffer[framePosition++] = byte;
    //             if (framePosition >= frameLength + 2) {
    //                 // Frame complete, validate CRC and convert
    //                 uint8_t crc = crsf_crc.calc(&frameBuffer[2], frameLength - 1);
    //                 if (crc == frameBuffer[framePosition - 1]) {
    //                     convertCRSFToJSON(frameBuffer, framePosition);
    //                 }
    //                 resetParser();
    //             }
    //             break;
    //     }
    // }
}

void SerialJSON::convertCRSFToJSON(uint8_t *crsfData, uint16_t size)
{
    if (size < 3) return;
    
    const crsf_header_t* header = (const crsf_header_t*)crsfData;
    const uint8_t* payload = &crsfData[3];
    
    switch (header->type) {
        case CRSF_FRAMETYPE_RC_CHANNELS_PACKED:
            if (size >= sizeof(crsf_header_t) + sizeof(crsf_channels_t) + 1) {
                handleRCChannelsFrame((const crsf_channels_t*)payload);
            }
            break;
            
        case CRSF_FRAMETYPE_LINK_STATISTICS:
            if (size >= sizeof(crsf_header_t) + sizeof(crsfLinkStatistics_t) + 1) {
                handleLinkStatisticsFrame((const crsfLinkStatistics_t*)payload);
            }
            break;
            
        case CRSF_FRAMETYPE_BATTERY_SENSOR:
            if (size >= sizeof(crsf_header_t) + sizeof(crsf_sensor_battery_t) + 1) {
                handleBatteryFrame((const crsf_sensor_battery_t*)payload);
            }
            break;
            
        case CRSF_FRAMETYPE_GPS:
            if (size >= sizeof(crsf_header_t) + sizeof(crsf_sensor_gps_t) + 1) {
                handleGPSFrame((const crsf_sensor_gps_t*)payload);
            }
            break;
            
        case CRSF_FRAMETYPE_BARO_ALTITUDE:
            if (size >= sizeof(crsf_header_t) + sizeof(crsf_sensor_baro_vario_t) + 1) {
                handleBarometerFrame((const crsf_sensor_baro_vario_t*)payload);
            }
            break;
            
        case CRSF_FRAMETYPE_ATTITUDE:
            if (size >= sizeof(crsf_header_t) + sizeof(crsf_sensor_attitude_t) + 1) {
                handleAttitudeFrame((const crsf_sensor_attitude_t*)payload);
            }
            break;
            
        case CRSF_FRAMETYPE_FLIGHT_MODE:
            // C807214149522A0070
            // Flight mode is a variable-length null-terminated string, max length of 16 bytes
            if (size >= sizeof(crsf_header_t)/* + sizeof(crsf_flight_mode_t) + 1*/) {
                handleFlightModeFrame((const crsf_flight_mode_t*)payload);
            }
            break;
            
        case CRSF_FRAMETYPE_VARIO:
            if (size >= sizeof(crsf_header_t) + sizeof(crsf_sensor_vario_t) + 1) {
                handleVarioFrame((const crsf_sensor_vario_t*)payload);
            }
            break;
            
        case CRSF_FRAMETYPE_AIRSPEED:
            if (size >= sizeof(crsf_header_t) + sizeof(crsf_sensor_airspeed_t) + 1) {
                handleAirspeedFrame((const crsf_sensor_airspeed_t*)payload);
            }
            break;
            
        case CRSF_FRAMETYPE_RPM:
            if (size >= sizeof(crsf_header_t) + sizeof(crsf_sensor_rpm_t) + 1) {
                handleRPMFrame((const crsf_sensor_rpm_t*)payload);
            }
            break;
            
        case CRSF_FRAMETYPE_TEMP:
            if (size >= sizeof(crsf_header_t) + sizeof(crsf_sensor_temp_t) + 1) {
                handleTemperatureFrame((const crsf_sensor_temp_t*)payload);
            }
            break;
            
        case CRSF_FRAMETYPE_CELLS:
            if (size >= sizeof(crsf_header_t) + sizeof(crsf_sensor_cells_t) + 1) {
                handleCellsFrame((const crsf_sensor_cells_t*)payload);
            }
            break;
            
        default:
            // Handle unknown frame types by outputting raw data
            writeJSONStart();
            appendToOutput("\"type\":\"unknown\",");
            writeJSONFieldInt("frame_type", (int32_t)header->type);
            writeJSONFieldInt("address", (int32_t)header->device_addr);
            writeJSONFieldInt("frame_size", (int32_t)header->frame_size);
            
            appendToOutput("\"payload\":[");
            for (int i = 0; i < header->frame_size - 2 && i < 60; i++) {
                char temp[8];
                snprintf(temp, sizeof(temp), "%d", payload[i]);
                appendToOutput(temp);
                if (i < header->frame_size - 3) appendToOutput(",");
            }
            appendToOutput("]");
            writeJSONEnd();
            flushOutput();
            break;
    }
}

void SerialJSON::handleRCChannelsFrame(const crsf_channels_t* channels)
{
    writeJSONStart();
    appendToOutput("\"type\":\"rc_channels_packed\",");
    appendToOutput("\"timestamp\":");
    char temp[32];
    snprintf(temp, sizeof(temp), "%lu,", millis());
    appendToOutput(temp);
    
    uint32_t channelData[16] = {
        channels->ch0, channels->ch1, channels->ch2, channels->ch3,
        channels->ch4, channels->ch5, channels->ch6, channels->ch7,
        channels->ch8, channels->ch9, channels->ch10, channels->ch11,
        channels->ch12, channels->ch13, channels->ch14, channels->ch15
    };
    
    appendToOutput("\"channels\":[");
    for (int i = 0; i < 16; i++) {
        snprintf(temp, sizeof(temp), "%lu", channelData[i]);
        appendToOutput(temp);
        if (i < 15) appendToOutput(",");
    }
    appendToOutput("]");
    writeJSONEnd();
    flushOutput();
}

void SerialJSON::handleLinkStatisticsFrame(const crsfLinkStatistics_t* linkStats)
{
    writeJSONStart();
    appendToOutput("\"type\":\"link_statistics\",");
    appendToOutput("\"timestamp\":");
    char temp[32];
    snprintf(temp, sizeof(temp), "%lu,", millis());
    appendToOutput(temp);
    
    writeJSONFieldInt("uplink_rssi_1", (int32_t)linkStats->uplink_RSSI_1);
    writeJSONFieldInt("uplink_rssi_2", (int32_t)linkStats->uplink_RSSI_2);
    writeJSONFieldInt("uplink_quality", (int32_t)linkStats->uplink_Link_quality);
    writeJSONFieldInt("uplink_snr", (int32_t)linkStats->uplink_SNR);
    writeJSONFieldInt("active_antenna", (int32_t)linkStats->active_antenna);
    writeJSONFieldInt("rf_mode", (int32_t)linkStats->rf_Mode);
    writeJSONFieldInt("uplink_power", (int32_t)linkStats->uplink_TX_Power);
    writeJSONFieldInt("downlink_rssi", (int32_t)linkStats->downlink_RSSI_1);
    writeJSONFieldInt("downlink_quality", (int32_t)linkStats->downlink_Link_quality);
    writeJSONFieldInt("downlink_snr", (int32_t)linkStats->downlink_SNR, true);
    
    writeJSONEnd();
    flushOutput();
}

void SerialJSON::handleBatteryFrame(const crsf_sensor_battery_t* battery)
{
    writeJSONStart();
    appendToOutput("\"type\":\"battery\",");
    appendToOutput("\"timestamp\":");
    char temp[32];
    snprintf(temp, sizeof(temp), "%lu,", millis());
    appendToOutput(temp);
    
    writeJSONFieldInt("voltage_mv", (int32_t)(be16toh(battery->voltage)));
    writeJSONFieldInt("current_ma", (int32_t)(be16toh(battery->current)));
    // This is buggy, perhaps unnecessary, endian swapping that Claude thought was a good idea. Testing without for now.
    // writeJSONFieldInt("capacity_mah", (int32_t)((battery->capacity >> 8) | ((battery->capacity & 0xFF) << 16)));
    writeJSONFieldInt("capacity_mah", (int32_t)battery->capacity);
    writeJSONFieldInt("remaining_percent", (int32_t)battery->remaining, true);
    
    writeJSONEnd();
    flushOutput();
}

void SerialJSON::handleGPSFrame(const crsf_sensor_gps_t* gps)
{
    writeJSONStart();
    appendToOutput("\"type\":\"gps\",");
    appendToOutput("\"timestamp\":");
    char temp[64];
    snprintf(temp, sizeof(temp), "%lu,", millis());
    appendToOutput(temp);
    
    writeJSONFieldDouble("latitude_deg", (double)be32toh(gps->latitude) / 10000000.0);
    writeJSONFieldDouble("longitude_deg", (double)be32toh(gps->longitude) / 10000000.0);
    writeJSONFieldDouble("groundspeed_kmh", (double)be16toh(gps->groundspeed) / 10.0);
    writeJSONFieldDouble("heading_deg", (double)be16toh(gps->gps_heading) / 100.0);
    writeJSONFieldInt("altitude_m", (int32_t)(be16toh(gps->altitude) - 1000));
    writeJSONFieldInt("satellites", (int32_t)gps->satellites_in_use, true);
    
    writeJSONEnd();
    flushOutput();
}

void SerialJSON::handleBarometerFrame(const crsf_sensor_baro_vario_t* baro)
{
    writeJSONStart();
    appendToOutput("\"type\":\"barometer\",");
    appendToOutput("\"timestamp\":");
    char temp[32];
    snprintf(temp, sizeof(temp), "%lu,", millis());
    appendToOutput(temp);
    
    uint16_t altitude_raw = be16toh(baro->altitude);
    if (altitude_raw & 0x8000) {
        // High bit set means altitude in meters
        writeJSONFieldInt("altitude_m", (int32_t)(altitude_raw & 0x7FFF));
    } else {
        // Altitude in decimeters + 10000dm offset
        writeJSONFieldDouble("altitude_m", (double)(altitude_raw - 10000) / 10.0);
    }
    writeJSONFieldInt("vertical_speed_cms", (int32_t)(int16_t)be16toh(baro->verticalspd), true);
    
    writeJSONEnd();
    flushOutput();
}

void SerialJSON::handleAttitudeFrame(const crsf_sensor_attitude_t* attitude)
{
    writeJSONStart();
    appendToOutput("\"type\":\"attitude\",");
    appendToOutput("\"timestamp\":");
    char temp[32];
    snprintf(temp, sizeof(temp), "%lu,", millis());
    appendToOutput(temp);
    
    writeJSONFieldDouble("pitch_rad", (double)be16toh(attitude->pitch) / 10000.0);
    writeJSONFieldDouble("roll_rad", (double)be16toh(attitude->roll) / 10000.0);
    writeJSONFieldDouble("yaw_rad", (double)be16toh(attitude->yaw) / 10000.0, true);
    
    writeJSONEnd();
    flushOutput();
}

void SerialJSON::handleFlightModeFrame(const crsf_flight_mode_t* flightMode)
{
    writeJSONStart();
    appendToOutput("\"type\":\"flight_mode\",");
    appendToOutput("\"timestamp\":");
    char temp[32];
    snprintf(temp, sizeof(temp), "%lu,", millis());
    appendToOutput(temp);
    
    char modeStr[17];
    strncpy(modeStr, flightMode->flight_mode, 16);
    modeStr[16] = '\0';
    writeJSONFieldString("mode", modeStr, true);
    
    writeJSONEnd();
    flushOutput();
}

void SerialJSON::handleVarioFrame(const crsf_sensor_vario_t* vario)
{
    writeJSONStart();
    appendToOutput("\"type\":\"vario\",");
    appendToOutput("\"timestamp\":");
    char temp[32];
    snprintf(temp, sizeof(temp), "%lu,", millis());
    appendToOutput(temp);
    
    writeJSONFieldInt("vertical_speed_cms", (int32_t)(int16_t)be16toh(vario->verticalspd), true);
    
    writeJSONEnd();
    flushOutput();
}

void SerialJSON::handleAirspeedFrame(const crsf_sensor_airspeed_t* airspeed)
{
    writeJSONStart();
    appendToOutput("\"type\":\"airspeed\",");
    appendToOutput("\"timestamp\":");
    char temp[32];
    snprintf(temp, sizeof(temp), "%lu,", millis());
    appendToOutput(temp);
    
    writeJSONFieldDouble("speed_kmh", (double)be16toh(airspeed->speed) / 10.0, true);
    
    writeJSONEnd();
    flushOutput();
}

void SerialJSON::handleRPMFrame(const crsf_sensor_rpm_t* rpm)
{
    writeJSONStart();
    appendToOutput("\"type\":\"rpm\",");
    appendToOutput("\"timestamp\":");
    char temp[32];
    snprintf(temp, sizeof(temp), "%lu,", millis());
    appendToOutput(temp);
    
    writeJSONFieldInt("source_id", (int32_t)rpm->source_id);
    
    // Extract 24-bit RPM values and convert to 32-bit signed
    int32_t rpmValues[19];
    const uint8_t* rpmData = (const uint8_t*)rpm + 1; // Skip source_id
    for (int i = 0; i < 19; i++) {
        int32_t val = (rpmData[i*3] << 16) | (rpmData[i*3+1] << 8) | rpmData[i*3+2];
        // Sign extend from 24-bit to 32-bit
        if (val & 0x800000) val |= 0xFF000000;
        rpmValues[i] = val;
    }
    
    writeJSONArray("rpm_values", rpmValues, 19, true);
    
    writeJSONEnd();
    flushOutput();
}

void SerialJSON::handleTemperatureFrame(const crsf_sensor_temp_t* temp)
{
    writeJSONStart();
    appendToOutput("\"type\":\"temperature\",");
    appendToOutput("\"timestamp\":");
    char tempStr[32];
    snprintf(tempStr, sizeof(tempStr), "%lu,", millis());
    appendToOutput(tempStr);
    
    writeJSONFieldInt("source_id", (int32_t)temp->source_id);
    
    // Count valid temperatures (non-zero)
    uint8_t validCount = 0;
    for (int i = 0; i < 20; i++) {
        if (temp->temperature[i] != 0) validCount++;
    }
    
    appendToOutput("\"temperatures\":[");
    bool first = true;
    for (int i = 0; i < 20; i++) {
        if (temp->temperature[i] != 0) {
            if (!first) appendToOutput(",");
            snprintf(tempStr, sizeof(tempStr), "%.1f", (double)be16toh(temp->temperature[i]) / 10.0);
            appendToOutput(tempStr);
            first = false;
        }
    }
    appendToOutput("]");
    
    writeJSONEnd();
    flushOutput();
}

void SerialJSON::handleCellsFrame(const crsf_sensor_cells_t* cells)
{
    writeJSONStart();
    appendToOutput("\"type\":\"cells\",");
    appendToOutput("\"timestamp\":");
    char temp[32];
    snprintf(temp, sizeof(temp), "%lu,", millis());
    appendToOutput(temp);
    
    writeJSONFieldInt("source_id", (int32_t)cells->source_id);
    
    // Count valid cells (non-zero)
    uint8_t validCount = 0;
    for (int i = 0; i < 29; i++) {
        if (cells->cell[i] != 0) validCount++;
    }
    
    appendToOutput("\"cell_voltages\":[");
    bool first = true;
    for (int i = 0; i < 29; i++) {
        if (cells->cell[i] != 0) {
            if (!first) appendToOutput(",");
            snprintf(temp, sizeof(temp), "%.3f", (double)be16toh(cells->cell[i]) / 1000.0);
            appendToOutput(temp);
            first = false;
        }
    }
    appendToOutput("]");
    
    writeJSONEnd();
    flushOutput();
}

// Helper methods
void SerialJSON::writeJSONStart()
{
    outputPosition = 0;
    appendToOutput("{");
}

void SerialJSON::writeJSONEnd()
{
    appendToOutput("}\n");
}

void SerialJSON::writeJSONFieldInt(const char* name, int32_t value, bool lastField)
{
    char temp[64];
    snprintf(temp, sizeof(temp), "\"%s\":%ld", name, value);
    appendToOutput(temp);
    if (!lastField) appendToOutput(",");
}

void SerialJSON::writeJSONFieldString(const char* name, const char* value, bool lastField)
{
    char temp[128];
    snprintf(temp, sizeof(temp), "\"%s\":\"%s\"", name, value);
    appendToOutput(temp);
    if (!lastField) appendToOutput(",");
}

void SerialJSON::writeJSONFieldDouble(const char* name, double value, bool lastField)
{
    char temp[64];
    snprintf(temp, sizeof(temp), "\"%s\":%.6f", name, value);
    appendToOutput(temp);
    if (!lastField) appendToOutput(",");
}

void SerialJSON::writeJSONArray(const char* name, int32_t* values, uint8_t count, bool lastField)
{
    char temp[32];
    snprintf(temp, sizeof(temp), "\"%s\":[", name);
    appendToOutput(temp);
    
    for (uint8_t i = 0; i < count; i++) {
        snprintf(temp, sizeof(temp), "%ld", values[i]);
        appendToOutput(temp);
        if (i < count - 1) appendToOutput(",");
    }
    appendToOutput("]");
    if (!lastField) appendToOutput(",");
}

void SerialJSON::writeJSONArray(const char* name, uint16_t* values, uint8_t count, bool lastField)
{
    char temp[32];
    snprintf(temp, sizeof(temp), "\"%s\":[", name);
    appendToOutput(temp);
    
    for (uint8_t i = 0; i < count; i++) {
        snprintf(temp, sizeof(temp), "%u", values[i]);
        appendToOutput(temp);
        if (i < count - 1) appendToOutput(",");
    }
    appendToOutput("]");
    if (!lastField) appendToOutput(",");
}

void SerialJSON::flushOutput()
{
    if (outputPosition > 0) {
        _outputPort->write((const uint8_t*)outputBuffer, outputPosition);
        outputPosition = 0;
    }
}

void SerialJSON::appendToOutput(const char* str)
{
    size_t len = strlen(str);
    if (outputPosition + len >= sizeof(outputBuffer)) {
        flushOutput();
    }
    
    if (len < sizeof(outputBuffer)) {
        strcpy(&outputBuffer[outputPosition], str);
        outputPosition += len;
    }
}

void SerialJSON::appendToOutput(char c)
{
    if (outputPosition >= sizeof(outputBuffer) - 1) {
        flushOutput();
    }
    outputBuffer[outputPosition++] = c;
}