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

struct SDS_UnixTimeSync {
    uint8_t id      = 0x01;
    uint8_t size[3] = {0x00, 0x00, 0x0C};
    uint8_t time[4] = {0x00, 0x00, 0x00, 0x00};
    uint8_t crc[4]  = {0x00, 0x00, 0x00, 0x00};
};

struct SDS_ModeChange {
    uint8_t id      = 0x02;
    uint8_t size[3] = {0x00, 0x00, 0x0C};
    uint8_t mode[4] = {0x00, 0x00, 0x00, 0x01};
    uint8_t crc[4]  = {0x00, 0x00, 0x00, 0x00};
};

// ======================================================
//  SDS DATA MODEL – USB READ(3) Message Structures
// ======================================================

// Message type for USB frames
enum class SDS_MsgType : uint8_t {
    READ_MICS = 3
};

// Binary header (16 bytes)
struct SDS_MsgHeader {
    uint8_t  sof;        // 0xA5
    uint8_t  msgType;    // SDS_MsgType
    uint16_t version;    // protocol version
    uint16_t frameIndex; // running counter
    uint16_t numMics;    // always 8
    uint16_t frameLen;   // SDS_FRAME_LEN
    uint32_t payloadLen; // numMics * frameLen * sizeof(float)
};


struct SDS_MsgDetect {
    uint32_t  magic 	= 0xDEADBEEF;
    uint8_t	  id		= 1;					// DETECT_ID
    uint8_t   len[3]    = {0x00, 0x00, 0x20}; 	// 32
    uint32_t  timestamp = 0; 					// TimeStamp
    uint32_t mic;
    float    azi;
    float    distance;
    float    conf;
    uint32_t  crc32;
};


struct SDS_MsgRead {
    uint32_t  magic 	= 0xDEADBEEF;
    uint8_t	  id		= 2;					// READ_ID
    uint8_t   len[3]    = {0x00, 0x00, 0x90}; 	// 8220
    uint32_t  timestamp = 0; 					// TimeStamp
    float 	  payload[32];
    uint32_t  crc32;
};

struct SDS_MsgRead1 {
    uint32_t  magic 	= 0xDEADBEEF;
    uint8_t	  id		= 2;					// READ_ID
    uint8_t   len[3]    = {0x00, 0x04, 0x18}; 	// 4 + 1 + 3 + 4 + 4 + 4 + 1024 + 4 + 4 = 1048
    uint32_t  timestamp = 0; 					// TimeStamp
    uint32_t  frameNumber = 0;
    uint32_t  micNumber = 0;					// MicNumber
    float 	  payload[256];
    uint32_t  crc32;
};



// Payload: 8× microphone block
struct SDS_MicPayload {
    float micData[SDS_NUM_MICS][SDS_FRAME_LEN];
};

// Full USB message
struct SDS_MicFrameMsg {
    SDS_MsgHeader header;
    SDS_MicPayload payload;
    uint32_t crc32;
};
