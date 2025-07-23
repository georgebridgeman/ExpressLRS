#pragma once

#include "SerialIO.h"
#include "crsf_protocol.h"

class SerialJSON : public SerialIO {
public:
    explicit SerialJSON(Stream &out, Stream &in) : SerialIO(&out, &in) {}
    virtual ~SerialJSON() {}

    uint32_t sendRCFrame(bool frameAvailable, bool frameMissed, uint32_t *channelData) override;
    void queueMSPFrameTransmission(uint8_t* data) override;
    void queueLinkStatisticsPacket() override;
    
    // RF telemetry capture methods
    void processRFTelemetryPacket(const uint8_t* telemetryData, uint8_t dataLen);
    
    // Debug logging method
    void logDebugMessage(const char* event, int value1, int value2);

private:
    void processBytes(uint8_t *bytes, uint16_t size) override;
    void convertCRSFToJSON(uint8_t *crsfData, uint16_t size);
    void writeJSONStart();
    void writeJSONEnd();
    void writeJSONFieldInt(const char* name, int32_t value, bool lastField = false);
    void writeJSONFieldString(const char* name, const char* value, bool lastField = false);
    void writeJSONFieldDouble(const char* name, double value, bool lastField = false);
    void writeJSONArray(const char* name, int32_t* values, uint8_t count, bool lastField = false);
    void writeJSONArray(const char* name, uint16_t* values, uint8_t count, bool lastField = false);
    
    // CRSF frame type handlers
    void handleRCChannelsFrame(const crsf_channels_t* channels);
    void handleLinkStatisticsFrame(const crsfLinkStatistics_t* linkStats);
    void handleBatteryFrame(const crsf_sensor_battery_t* battery);
    void handleGPSFrame(const crsf_sensor_gps_t* gps);
    void handleBarometerFrame(const crsf_sensor_baro_vario_t* baro);
    void handleAttitudeFrame(const crsf_sensor_attitude_t* attitude);
    void handleFlightModeFrame(const crsf_flight_mode_t* flightMode);
    void handleVarioFrame(const crsf_sensor_vario_t* vario);
    void handleAirspeedFrame(const crsf_sensor_airspeed_t* airspeed);
    void handleRPMFrame(const crsf_sensor_rpm_t* rpm);
    void handleTemperatureFrame(const crsf_sensor_temp_t* temp);
    void handleCellsFrame(const crsf_sensor_cells_t* cells);
    
    // CRSF parsing state
    enum ParsingState {
        WAITING_FOR_SYNC,
        READING_LENGTH,
        READING_FRAME
    };
    
    ParsingState parseState = WAITING_FOR_SYNC;
    uint8_t frameBuffer[CRSF_MAX_PACKET_LEN];
    uint8_t frameLength = 0;
    uint8_t framePosition = 0;
    
    // Output buffer for JSON strings (reduced size for stability)
    char outputBuffer[128];
    uint16_t outputPosition = 0;
    
    void resetParser();
    void flushOutput();
    void appendToOutput(const char* str);
    void appendToOutput(char c);
};