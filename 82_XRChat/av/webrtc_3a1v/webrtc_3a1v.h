#pragma once
#include <memory>
#include <boost/circular_buffer.hpp>

extern "C"
{
#include "vad.h"
#include "noise_suppression.h"
#include "aecm.h"
#include "agc.h"
//ref https://github.com/YangangCao/WebRTC-3A1V
}





// 4 types, the second is the best
enum nsLevel
{
    kLow,
    kModerate,
    kHigh,
    kVeryHigh
};

class webrtc_3a1v : public std::enable_shared_from_this<webrtc_3a1v>
{
public:
    webrtc_3a1v(uint32_t sampleRate=16000,uint32_t channels=1,nsLevel level=nsLevel::kModerate);
    ~webrtc_3a1v();
    
    int playback_pcm(uint8_t* pcm,int& pcm_len);
    
    int process(uint8_t* pcm,int& pcm_len);
    
    int processForXR(uint8_t* pcm,int& pcm_len);
private:
    int vadProcess(int16_t *buffer,uint64_t samplesCount
                   ,int16_t vad_mode, int per_ms_frames);
    int nsProcess(int16_t *buffer, uint64_t samplesCount);
    int agcProcess(int16_t *buffer, size_t samplesCount, int16_t agcMode);
    int aecProcess(int16_t *far_frame, int16_t *near_frame
                   ,size_t samplesCount
                   ,int16_t nMode,int16_t msInSndCardBuf);
private:
    uint32_t    sampleRate_;
    uint32_t    channels_;
    nsLevel     level_;
    
    boost::circular_buffer<int16_t>    frames_;
    
    NsHandle**                          NsHandles_;
    VadInst*                            vadInst_;
    void*                               aecmInst_;
    void*                               agcInst_;
};
