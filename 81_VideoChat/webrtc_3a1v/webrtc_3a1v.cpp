#include "webrtc_3a1v.h"

#ifndef MIN
#define  MIN(A, B)        ((A) < (B) ? (A) : (B))
#endif

#ifndef MAX
#define  MAX(A, B)        ((A) > (B) ? (A) : (B))
#endif


webrtc_3a1v::webrtc_3a1v(uint32_t sampleRate,uint32_t channels, enum nsLevel level)
:sampleRate_(sampleRate)
,channels_(channels)
,level_(level)
,frames_(1024)
,NsHandles_(nullptr)
,vadInst_(nullptr)
,aecmInst_(nullptr)
,agcInst_(nullptr)
{
    
}
webrtc_3a1v::~webrtc_3a1v()
{
    if(vadInst_!=nullptr)
        WebRtcVad_Free(vadInst_);
    if(agcInst_!=nullptr)
        WebRtcAgc_Free(agcInst_);
    
    if(aecmInst_!=nullptr)
    WebRtcAecm_Free(aecmInst_);
    
    if(NsHandles_!=nullptr)
    {
        //释放
        for (size_t i = 0; i < channels_; i++)
        {
            if (NsHandles_[i])
            {
                WebRtcNs_Free(NsHandles_[i]);
            }
        }
        free(NsHandles_);
    }

}

int webrtc_3a1v::nsProcess(int16_t* buffer,uint64_t samplesCount)
{
    if (buffer == nullptr) return -1;
    if (samplesCount == 0) return -1;
    size_t samples = MIN(160, sampleRate_ / 100);
    if (samples == 0) return -1;
    
    uint32_t num_bands = 1;
    int16_t *input = buffer;
    size_t frames = (samplesCount / (samples * channels_));
    int16_t *frameBuffer = (int16_t *) malloc(sizeof(*frameBuffer) * channels_ * samples);
    
    
    //分配
    if(NsHandles_==NULL)
    {
        NsHandles_ = (NsHandle **) malloc(channels_ * sizeof(NsHandle *));
        if (NsHandles_ == NULL || frameBuffer == NULL)
        {
            if (NsHandles_)
                free(NsHandles_);
            if (frameBuffer)
                free(frameBuffer);
            fprintf(stderr, "malloc error.\n");
            return -1;
        }
        for (size_t i = 0; i < channels_; i++)
        {
            NsHandles_[i] = WebRtcNs_Create(); //
            if (NsHandles_[i] != NULL)
            {
                int status = WebRtcNs_Init(NsHandles_[i], sampleRate_);
                if (status != 0)
                {
                    fprintf(stderr, "WebRtcNs_Init fail\n");
                    WebRtcNs_Free(NsHandles_[i]);
                    NsHandles_[i] = NULL;
                }
                else
                {
                    status = WebRtcNs_set_policy(NsHandles_[i], level_);
                    if (status != 0)
                    {
                        fprintf(stderr, "WebRtcNs_set_policy fail\n");
                        WebRtcNs_Free(NsHandles_[i]);
                        NsHandles_[i] = NULL;
                    }
                }
            }
            if (NsHandles_[i] == NULL)
            {
                for (size_t x = 0; x < i; x++)
                {
                    if (NsHandles_[x])
                    {
                        WebRtcNs_Free(NsHandles_[x]);
                    }
                }
                free(NsHandles_);
                free(frameBuffer);
                return -1;
            }
        }
    }


    //process
    for (size_t i = 0; i < frames; i++)
    {
        for (size_t c = 0; c < channels_; c++)
        {
            for (size_t k = 0; k < samples; k++)
                frameBuffer[k] = input[k * channels_ + c];

            int16_t *nsIn[1] = {frameBuffer};   //ns input[band][data]
            int16_t *nsOut[1] = {frameBuffer};  //ns output[band][data]
            WebRtcNs_Analyze(NsHandles_[c], nsIn[0]);
            WebRtcNs_Process(NsHandles_[c], (const int16_t *const *) nsIn, num_bands, nsOut);
            for (size_t k = 0; k < samples; k++)
                input[k * channels_ + c] = frameBuffer[k];
        }
        input += samples * channels_;
    }


    free(frameBuffer);
    return 1;
}

int webrtc_3a1v::vadProcess(int16_t* buffer,uint64_t samplesCount
                            ,int16_t vad_mode, int per_ms_frames)
{
    if (buffer == nullptr) return -1;
    if (samplesCount == 0) return -1;
    // kValidRates : 8000, 16000, 32000, 48000
    // 10, 20 or 30 ms frames
    per_ms_frames = MAX(MIN(30, per_ms_frames), 10);
    size_t samples = sampleRate_ * per_ms_frames / 1000;
    if (samples == 0) return -1;
    int16_t *input = buffer;
    size_t nTotal = (samplesCount / samples);
    
    //add by copyleft
    //int16_t* output = buffer;

    if(vadInst_==nullptr)
    {
        vadInst_ = WebRtcVad_Create();
        if (vadInst_ == NULL) return -1;
        
        int status = WebRtcVad_Init(vadInst_);
        if (status != 0)
        {
            printf("WebRtcVad_Init fail\n");
            WebRtcVad_Free(vadInst_);
            return -1;
        }
        status = WebRtcVad_set_mode(vadInst_, vad_mode);
        if (status != 0)
        {
            printf("WebRtcVad_set_mode fail\n");
            WebRtcVad_Free(vadInst_);
            return -1;
        }
    }
    

    uint32_t   activity=0; //add by copyleft
    //printf("Activity ： \n");
    for (size_t i = 0; i < nTotal; i++)
    {
        int keep_weight = 0;
        int nVadRet = WebRtcVad_Process(vadInst_, sampleRate_, input, samples, keep_weight);
        if (nVadRet == -1)
        {
            printf("failed in WebRtcVad_Process\n");
            WebRtcVad_Free(vadInst_);
            return -1;
        }else
        {
            if(nVadRet>0)
                ++activity;
            
            // output result
            printf(" %d \t", nVadRet);
        }
        input += samples;
    }
    //printf("\n");
    
    if(activity==0)
        printf("all frame is silence!\n");
    

    return activity;
    return 1;
}




int webrtc_3a1v::aecProcess(int16_t *far_frame, int16_t *near_frame
                            , size_t samplesCount
                            , int16_t nMode,int16_t msInSndCardBuf)
{
    if (near_frame == nullptr) return -1;
    if (far_frame == nullptr) return -1;
    if (samplesCount == 0) return -1;
    
    size_t samples = MIN(160, sampleRate_ / 100);
    if (samples == 0) return -1;
    const int maxSamples = 160;
    int16_t *near_input = near_frame;
    int16_t *far_input  = far_frame;
    size_t nTotal       = (samplesCount / samples);
    
    if(nullptr==aecmInst_){
        AecmConfig config;
        config.cngMode = AecmTrue;
        config.echoMode = nMode;// 0, 1, 2, 3 (default), 4
        
        aecmInst_ = WebRtcAecm_Create();
        if (aecmInst_ == NULL) return -1;
        int status = WebRtcAecm_Init(aecmInst_, sampleRate_);//8000 or 16000 Sample rate
        if (status != 0)
        {
            printf("WebRtcAecm_Init fail\n");
            WebRtcAecm_Free(aecmInst_);
            return -1;
        }
        status = WebRtcAecm_set_config(aecmInst_, config);
        if (status != 0)
        {
            printf("WebRtcAecm_set_config fail\n");
            WebRtcAecm_Free(aecmInst_);
            return -1;
        }
    }


    int16_t out_buffer[maxSamples];
    for (size_t i = 0; i < nTotal; i++)
    {
        if (WebRtcAecm_BufferFarend(aecmInst_, far_input, samples) != 0)
        {
            printf("WebRtcAecm_BufferFarend() failed.");
            WebRtcAecm_Free(aecmInst_);
            return -1;
        }
        int nRet = WebRtcAecm_Process(aecmInst_, near_input, NULL, out_buffer, samples, msInSndCardBuf);

        if (nRet != 0)
        {
            printf("failed in WebRtcAecm_Process\n");
            WebRtcAecm_Free(aecmInst_);
            return -1;
        }
        memcpy(near_input, out_buffer, samples * sizeof(int16_t));
        near_input += samples;
        far_input += samples;
    }

    return 1;
}

int webrtc_3a1v::agcProcess(int16_t *buffer,size_t samplesCount, int16_t agcMode)
{
    if (buffer == nullptr) return -1;
    if (samplesCount == 0) return -1;
    WebRtcAgcConfig agcConfig;
    agcConfig.compressionGaindB = 9; // default 9 dB
    agcConfig.limiterEnable = 1; // default kAgcTrue (on)
    agcConfig.targetLevelDbfs = 3; // default 3 (-3 dBOv)
    int minLevel = 0;
    int maxLevel = 255;
    size_t samples = MIN(160, sampleRate_ / 100);
    if (samples == 0) return -1;
    const int maxSamples = 320;
    int16_t *input = buffer;
    size_t nTotal = (samplesCount / samples);
    
    if(nullptr==agcInst_)
    {
        agcInst_ = WebRtcAgc_Create();
        if (agcInst_ == NULL) return -1;
        int status = WebRtcAgc_Init(agcInst_, minLevel, maxLevel, agcMode, sampleRate_);
        if (status != 0)
        {
            printf("WebRtcAgc_Init fail\n");
            WebRtcAgc_Free(agcInst_);
            return -1;
        }
        status = WebRtcAgc_set_config(agcInst_, agcConfig);
        if (status != 0)
        {
            printf("WebRtcAgc_set_config fail\n");
            WebRtcAgc_Free(agcInst_);
            return -1;
        }
    }
    
    

    size_t num_bands = 1;
    int inMicLevel, outMicLevel = -1;
    int16_t out_buffer[maxSamples];
    int16_t *out16 = out_buffer;
    uint8_t saturationWarning = 1;               //是否有溢出发生，增益放大以后的最大值超过了65536
    int16_t echo = 0;                            //增益放大是否考虑回声影响
    for (int i = 0; i < nTotal; i++)
    {
        inMicLevel = 0;
        int nAgcRet = WebRtcAgc_Process(agcInst_, (const int16_t *const *) &input, num_bands, samples,
                                        (int16_t *const *) &out16, inMicLevel, &outMicLevel, echo,
                                        &saturationWarning);

        if (nAgcRet != 0)
        {
            printf("failed in WebRtcAgc_Process\n");
            WebRtcAgc_Free(agcInst_);
            return -1;
        }
        memcpy(input, out_buffer, samples * sizeof(int16_t));
        input += samples;
    }

    const size_t remainedSamples = samplesCount - nTotal * samples;
    if (remainedSamples > 0)
    {
        if (nTotal > 0)
        {
            input = input - samples + remainedSamples;
        }

        inMicLevel = 0;
        int nAgcRet = WebRtcAgc_Process(agcInst_, (const int16_t *const *) &input, num_bands, samples,
                                        (int16_t *const *) &out16, inMicLevel, &outMicLevel, echo,
                                        &saturationWarning);

        if (nAgcRet != 0)
        {
            printf("failed in WebRtcAgc_Process during filtering the last chunk\n");
            WebRtcAgc_Free(agcInst_);
            return -1;
        }
        memcpy(&input[samples - remainedSamples], &out_buffer[samples - remainedSamples],
               remainedSamples * sizeof(int16_t));
        input += samples;
    }


    return 1;
}
int webrtc_3a1v::playback_pcm(uint8_t* pcm,int& pcm_len)
{
    if(pcm_len>0)
    {
        //缓冲远端音频数据
        int16_t*    buffer      = (int16_t*)pcm;
        uint64_t samplesCount   = pcm_len/(2*channels_);  //2=16/8
        for(size_t i=0;i<samplesCount;++i)
        {
            frames_.push_back(*buffer);
            buffer++;
        }
    }
    
    return 1;
}
/*
 WebRTC 架构中上下行音频信号处理流程如图 1，音频 3A 主要集中在上行的发送端对发送信号依次进行回声消除、降噪以及音量均衡（这里只讨论 AEC 的处理流程，如果是 AECM 的处理流程 ANS 会前置），AGC 会作为压限器作用在接收端对即将播放的音频信号进行限幅。
 */
int webrtc_3a1v::process(uint8_t* pcm,int& pcm_len)
{
    try{
        int16_t*    buffer      = (int16_t*)pcm;
        uint64_t samplesCount   = pcm_len/(2*channels_);  //2=16/8
        



//        {
//            //  Aggressiveness mode (0, 1, 2, or 3)
//            int16_t mode = 1;
//            int   per_ms = 30;
//            int ret=vadProcess(buffer, samplesCount, mode, per_ms);
//            if(ret==0)
//            {
//                pcm_len=0;//全是静音帧
//                return 1;
//            }
//        }

        
        
      nsProcess(buffer, samplesCount);
        


        
        
        //回声消除
        {
            if(frames_.size()==1024)
            {
                int16_t play_buffer[1024];

                //数据拷贝出来
                std::memcpy(play_buffer,frames_.array_one().first,sizeof(int16_t)*frames_.array_one().second);
                std::memcpy(play_buffer+frames_.array_one().second,frames_.array_two().first, sizeof(int16_t)*frames_.array_two().second);
                

                int16_t*    near_frame  =buffer;
                int16_t*    far_frame   =play_buffer;
                
                int16_t echoMode = 1;// 0, 1, 2, 3 (default), 4
                int16_t msInSndCardBuf = 40;// demo 为 40  64是我按16000hz 1024个frame 计算出来的
                aecProcess(far_frame, near_frame,samplesCount, echoMode, msInSndCardBuf);
            }
        
        }
        
        
//        //
//        {
//            //  kAgcModeAdaptiveAnalog  模拟音量调节
//            //  kAgcModeAdaptiveDigital 自适应增益
//            //  kAgcModeFixedDigital    固定增益
//            agcProcess(buffer,samplesCount,kAgcModeAdaptiveDigital);
//        }

         
    }catch(...)
    {
        
    }
 
    return 1;
}

/*
 aecm.c
 
 static __inline uint32_t __clz_uint32(uint32_t v) {
 // Never used with input 0
     assert(v > 0);
 */
