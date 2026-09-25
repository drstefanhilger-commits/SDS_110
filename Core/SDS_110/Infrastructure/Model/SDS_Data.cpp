/*
 * SDS_Data.cpp  (Infrastructure)
 */
#include "SDS_Data.hpp"

SDS_Data& SDS_Data::instance()
{
    static SDS_Data d;
    return d;
}

SDS_Data::SDS_Data()
{
    osMutexAttr_t mutexAttr{};
    mutexAttr.name = "SDS_DataMutex";
    mutex_ = osMutexNew(&mutexAttr);

    osMessageQueueAttr_t queueAttr{};
    queueAttr.name = "SDS_EventQueue";
    eventQueue_    = osMessageQueueNew(16, sizeof(SDS_DataEvent), &queueAttr);
    errorMsgQueue_ = osMessageQueueNew(8, sizeof(SDS_ErrorMessage), nullptr);
}

// ---------------------------------------------------------------- 120 -> Status
void SDS_Data::setAcousticState(const sds110::AcousticState& s)
{
    if (!lock()) { errorFlag = 999; return; }
    acousticState = s;
    uint32_t n = 0;
    for (uint32_t b = 0; b < sds110::NUM_BANDS; ++b)
        if (s.p[b] > sds110::THETA_SEL) ++n;
    selectedBands = n;
    detected = (n >= sds110::B_MIN);
    unlock();
}

void SDS_Data::getAcousticState(sds110::AcousticState& out) const
{
    if (!lock()) { out = sds110::AcousticState{}; return; }
    out = acousticState;
    unlock();
}

void SDS_Data::setCandidate(float az, float dist, float conf, bool valid)
{
    if (!lock()) { errorFlag = 999; return; }
    if (valid) {
        azimuthDeg = az;
        distance   = dist;
        confidence = conf;
        ++reportCount;
    }
    unlock();
}

// ---------------------------------------------------------------- Tasks / Debug
void SDS_Data::setTaskStats(TaskId t, uint32_t freeStack, float loopTime, uint32_t loopCounter)
{
    if (!lock()) { errorFlag = 999; return; }
    TaskStats& s = tasks_[static_cast<uint8_t>(t)];
    s.freeStack = freeStack; s.loopTime = loopTime; s.loopCounter = loopCounter;
    unlock();
}

TaskStats SDS_Data::getTaskStats(TaskId t) const
{
    if (!lock()) return TaskStats{};
    TaskStats s = tasks_[static_cast<uint8_t>(t)];
    unlock();
    return s;
}

void SDS_Data::setDebugValue(uint32_t i, float v)
{
    if (i >= 4) return;
    setValue(debugValue[i], v);
}

float SDS_Data::getDebugValue(uint32_t i) const
{
    return (i < 4) ? getValue(debugValue[i]) : 0.0f;
}

// ---------------------------------------------------------------- Error
void SDS_Data::pushErrorMessage(const char* msg)
{
    SDS_ErrorMessage em{};
    strncpy(em.text, msg, sizeof(em.text) - 1);
    osMessageQueuePut(errorMsgQueue_, &em, 0, 0);
}

bool SDS_Data::popErrorMessage(SDS_ErrorMessage& out)
{
    return osMessageQueueGet(errorMsgQueue_, &out, nullptr, 0) == osOK;
}

void SDS_Data::setErrorBuffer(const uint8_t* src, uint32_t len)
{
    if (len > sizeof(errorBuffer_)) len = sizeof(errorBuffer_);
    if (!lock()) { errorFlag = 998; return; }
    memcpy(errorBuffer_, src, len);
    errorLen = len;
    unlock();
}

uint32_t SDS_Data::getErrorBuffer(uint8_t* dst, uint32_t maxLen) const
{
    if (!lock()) return 0;
    uint32_t n = (errorLen < maxLen) ? errorLen : maxLen;
    memcpy(dst, errorBuffer_, n);
    unlock();
    return n;
}

// ---------------------------------------------------------------- Events
void SDS_Data::pushEvent(SDS_DataEventType type, float processTime)
{
    SDS_DataEvent evt{ type, processTime };
    osMessageQueuePut(eventQueue_, &evt, 0, 0);
}
