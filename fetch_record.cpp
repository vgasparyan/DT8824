#include "fetch_record.h"


#include <arpa/inet.h>
#include <iostream>

///constants
constexpr static const int RESOLUTION  = (0xffffff + 1); //2^24
constexpr static const float VOLT_MIN_32 = -0.3125f; //Minimum voltage x32 gain
constexpr static const float VOLT_MAX_32 = 0.3125f;  //Maximum voltage x32 gain

/**
 * Convert raw 4-byte ULONG for channels AIN1-AIN4 back to volts
 *    @param [rawValue] : raw 24-bit AD value packed in the least significant 3 bytes
 *    @param [gain] associated with the value
 * 	  @return floating point volts value
 **/
static float RawToVolts(uint32_t rawValue, GAIN gain)
{
    constexpr float PERCENT_FS = (VOLT_MAX_32 - VOLT_MIN_32)/RESOLUTION;
    float volts = (rawValue * PERCENT_FS) + VOLT_MIN_32;
    switch (gain) {
        case GAIN_16:
            volts *= 2.0f;
            break;
        case GAIN_8:
            volts *= 4.0f;
            break;
        case GAIN_1:
            volts *= 32.0f;
            break;
        default:
            break;
    }
    return volts;
}

FetchRecord::FetchRecord()
    : m_scanCount(0)
    , m_scanIndex(0)
    , m_nNumChan(0)
{
}

FetchRecord::~FetchRecord()
{
}

bool FetchRecord::convertRawToValues(std::unique_ptr<uint8_t[]> pRawRecord,
                                     int lenBuff,
                                     CHAN_MASK chanMask,
                                     GAIN gains[4]
                                     )
{
    const auto SZ = sizeof(uint32_t);
    if (lenBuff < (5* SZ)) {
        return false;
    }
    uint32_t *p = (uint32_t*)pRawRecord.get();
    m_scanCount = ntohl(*p); ++p;
    m_nNumChan = ntohl(*p) / SZ; ++p;
    m_scanIndex = ntohl(*p); ++p;
    m_ts.tv_sec = ntohl(*p); ++p;
    m_ts.tv_nsec = ntohl(*p); ++p;

    lenBuff -= (5* SZ);
    if (lenBuff < (int)(m_scanCount * m_nNumChan * SZ)) {
        //buffer too small or received incorrect header
        return false;
    }

    for (int mask = (CHAN_MASK)CHAN_AIN1, chan = 0; chan < m_nNumChan; mask <<= 1) {
        if ((mask & chanMask) == 0) {
            continue;
        }
        if (m_ChanValue[chan].values.size() != m_scanCount) {
            m_ChanValue[chan].values.resize(m_scanCount);
        }
        m_ChanValue[chan].chanType = (CHAN_MASK)mask;
        ++chan;
    }

    for (auto scan = 0; scan < m_scanCount; ++scan) {
        for (auto chan = 0; chan < m_nNumChan; ++chan, ++p) {
            auto raw = ntohl(*p);
            switch (m_ChanValue[chan].chanType) {
            case CHAN_AIN1 : {
                m_ChanValue[chan].values[scan] = RawToVolts(raw, gains[0]);
            }
                break;
            case CHAN_AIN2 : {
                m_ChanValue[chan].values[scan] = RawToVolts(raw, gains[1]);
            }
                break;
            case CHAN_AIN3 : {
                m_ChanValue[chan].values[scan] = RawToVolts(raw, gains[2]);
            }
                break;
            case CHAN_AIN4 : {
                m_ChanValue[chan].values[scan] = RawToVolts(raw, gains[3]);
            }
                break;
            }
        }
    }
    return true;
}
