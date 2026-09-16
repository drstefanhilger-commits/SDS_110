/*
 * SDS_Structs.hpp
 *
 * SDS Data Model Description
 * --------------------------
 * This header defines all shared enums and data structures used across the
 * SDS (Sensor‑DSP‑System). These structures form the backbone of the
 * real‑time DSP pipeline, connecting:
 *
 *   - MicTask (acquisition / simulation)
 *   - SRPTask (localization)
 *   - DSPTask (FFT / filtering)
 *   - LCDTask (visualization)
 *   - SystemManager (state aggregation)
 *
 * Created on: Jul 28, 2026
 * Author: Stefan (310004)
 */

#pragma once
#include <cstdint>
#include <vector>
#include "SDS_PARAMS.hpp"

// ======================================================
//  SDS DATA MODEL – Shared Event Types
// ======================================================
enum class SDS_DataEventType : uint8_t {
    SRP_UPDATE,
    DEBUG_UPDATE,
    MICBLOCK_UPDATE
};

// ======================================================
//  SDS DATA MODEL – Mode Event Types
// ======================================================
enum class SDS_ModeEventType : uint32_t {
    DETECT    = 1,
    CALIBRATE = 2,
    READ      = 3,
    ERROR     = 99
};

// ======================================================
//  SDS DATA MODEL – Shared Structures
// ======================================================

struct Vec3 { float x; float y; float z; };

struct SDSEvent {
    float    azimuth_deg;
    float    distance_m;
    float    confidence;
    uint32_t timestamp_ms;
};

struct SRPFrame {
    const float* srp_grid;
    int          ntheta;
    float        dtheta_deg;
    float        azimuth_deg;
    uint32_t     timestamp_ms;
    bool         valid;
    float        peak_value;
};

struct DistanceResult {
    float    distance_m;
    float    confidence;
    float    azimuth_deg;
    uint32_t timestamp_ms;
    bool     valid;
};

struct TrackState {
    float    azimuth_deg;
    float    distance_m;
    float    confidence;
    uint32_t timestamp_ms;
    bool     valid;
};

struct SystemState {
    float angle_deg;
    float distance_m;
    float confidence;
    bool  detected;
    uint8_t activeMask;
    uint8_t activeCount;
};

struct DetectionState {
    bool  detected;
    float confidence;
};

struct SDS_DataEvent {
    SDS_DataEventType type;
    float              processTime;
};

#pragma pack(push, 1)
struct SDS_UnixTimeSync {
    uint8_t magic[4]	= {0xDE, 0xAD, 0xBE, 0xEF};
    uint8_t id      	=  0x01;
    uint8_t size[3] 	= {0x00, 0x00, 0x0C};
    uint8_t time[4] 	= {0x00, 0x00, 0x00, 0x00};		// Sync Time
    uint8_t crc[4]  	= {0x00, 0x00, 0x00, 0x00};		// ToDo activate crc
};
#pragma pack(pop)

#pragma pack(push, 1)
struct SDS_ModeChange {
    uint8_t magic[4]	= {0xDE, 0xAD, 0xBE, 0xEF};
    uint8_t id      	=  0x02;
    uint8_t size[3] 	= {0x00, 0x00, 0x0C};
    uint8_t mode[4] 	= {0x00, 0x00, 0x00, 0x01};		// {1 = Detect, 2 Read, 3 Calibrate}
    uint8_t crc[4]  	= {0x00, 0x00, 0x00, 0x00};		// ToDo activate crc
};
#pragma pack(pop)

#pragma pack(push, 1)
struct SDS_ModeSimulation {
    uint8_t magic[4]	= {0xDE, 0xAD, 0xBE, 0xEF};
    uint8_t id      	=  0x03;
    uint8_t size[3] 	= {0x00, 0x00, 0x0C};
    uint8_t sim[4]  	= {0x00, 0x00, 0x00, 0x01};		// { 0 = Real, 1 = Simulated}
    uint8_t crc[4]  	= {0x00, 0x00, 0x00, 0x00};		// ToDo activate crc
};
#pragma pack(pop)


// ======================================================
//  SDS DATA MODEL – USB READ(3) Message Structures
// ======================================================

// Message type for USB frames
enum class SDS_MsgType : uint8_t {
    READ_MICS = 3
};

// ======================================================
//  SDS DATA MODEL – USB DETECT / READ Messages (magic + len_id)
// ======================================================

#pragma pack(push, 1)
struct SDS_MsgDetect {
    uint32_t magic     = 0xDEADBEEF;
    uint32_t len_id    = ((sizeof(SDS_MsgDetect) & 0x00FFFFFF) |
                          ((uint32_t)0x01 << 24));
    uint32_t timestamp = 0;   // TimeStamp

    uint32_t mic;
    float    azi;
    float    distance;
    float    conf;
    uint32_t crc32;
};
#pragma pack(pop)

static_assert(sizeof(SDS_MsgDetect) == 32,"SDS_MsgDetect must be 32 bytes (wire format)");

#define SDS_MSG_BUFFER_SIZE 128
#pragma pack(push, 1)
struct SDS_MsgRead {
    uint32_t magic     = 0xDEADBEEF;
    uint32_t len_id    = ((sizeof(SDS_MsgRead) & 0x00FFFFFF) |
                          ((uint32_t)0x02 << 24));
    uint32_t timestamp = 0;   // TimeStamp

    uint16_t micNr     = 1;
    uint16_t frameNr   = 0;
    uint32_t data[SDS_MSG_BUFFER_SIZE];
    uint32_t crc32;
};
#pragma pack(pop)

static_assert(sizeof(SDS_MsgRead) == 532,"SDS_MsgRead must be 532 bytes (wire format)");

// ======================================================
//  SDS DATA MODEL – USB Mic Frame Message (legacy float payload)
// ======================================================

// Payload: 8× microphone block
struct SDS_MicPayload {
    float micData[SDS_NUM_MICS][SDS_FRAME_LEN];
};

// ======================================================
//  SDS DATA MODEL – Generic Message (legacy)
// ======================================================

union MessageData {
    uint8_t  b[128];
    uint16_t h[64];
    uint32_t w[32];
};

#pragma pack(push, 1)
struct Message {
    uint32_t magic     = 0xDEADBEEF;
    uint32_t len_id    = ((sizeof(SDS_MsgRead) & 0x00FFFFFF) |
                          ((uint32_t)0x03 << 24));
    uint32_t timestamp = 0;   // TimeStamp

    MessageData data;
    uint32_t crc32  = 0;
};
#pragma pack(pop)

// ======================================================
//  SDS DATA MODEL – Error Message Type
// ======================================================

struct SDS_ErrorMessage {
    char text[32];
};
