#ifndef FETCH_RECORD_H
#define FETCH_RECORD_H

#include <dt8824client.h>
#include <memory>
#include <array>

//Channel type/mask bits
typedef enum {
    CHAN_AIN1  = 0x01,
    CHAN_AIN2  = 0x02,
    CHAN_AIN3  = 0x04,
    CHAN_AIN4  = 0x08
} CHAN_MASK;

//Time stamp
struct timespec_t
{
    long tv_sec;
    long tv_nsec;
};

//channel values converted from raw readings into engineering units
struct ChannelValue
{
    CHAN_MASK chanType;
    std::vector<float> values;
};

class FetchRecord
{
public:

public:
    FetchRecord();
    ~FetchRecord();

public:
    bool convertRawToValues(std::unique_ptr<uint8_t[]> pRawRecord,
                            int lenBuff,
                            CHAN_MASK chanMask,
                            GAIN gains[4]);

    ///Return the number of scans in the record
    uint32_t getScanCount()  const {return m_scanCount;}

    ///Return the index of the first scan in the record
    uint32_t getScanIndex()  const {return m_scanIndex;}

    ///Return the the number of channels in each scan of the record
    uint32_t getChannelCount()  const {return m_nNumChan;}

    ///Return the time stamp of the first scan in the record
    timespec_t getTimestamp() const {return m_ts;}

private :
    uint32_t    m_scanCount; //Integral number of scans in this record
    uint32_t    m_scanIndex; //Scan number of the first scan in the record
    int         m_nNumChan;  //Number of channels in the record
    timespec_t  m_ts;        //Time stamp of the first scan in the record
    std::array<ChannelValue,8> m_ChanValue;
};

#endif // FETCH_RECORD_H
