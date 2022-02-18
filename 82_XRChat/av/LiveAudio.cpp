#include "LiveAudio.h"

#include <Urho3D/Audio/Audio.h>
#include <Urho3D/Audio/AudioEvents.h>
#include <Urho3D/Audio/Sound.h>
#include <Urho3D/Audio/SoundSource.h>

#include "../Player.h"

//使用噪音抑制和静音检测技术
#include "./webrtc_3a1v/webrtc_3a1v.h"

#include "../Character.h"


std::shared_ptr<webrtc_3a1v>     ns_=nullptr;


void(*pfCallback)(void*,uint8_t*,int)=nullptr;
void pcm_fram_callback(void *userdata, Uint8 *stream, int len) {
    
    if(ns_)
        ns_->processForXR(stream,len);
        
    if(pfCallback&&len>0)
        pfCallback(userdata,stream,len);
}


live_audio::live_audio(SharedPtr<Scene> scene)
:scene_(scene)
,audioCaptureIndex_(0)
,captureDev_(0)
{

}
live_audio::~live_audio()
{
    //关闭
    captureOpen(nullptr,nullptr,0,0,0,0);
}

void live_audio::captureOpen(void(*pf)(void*,uint8_t*,int),void* custom
                         ,int index,int freq,int channels,int samples)
{    
    if((*pf))
    {
        //record it
        audioCaptureIndex_=index;
        
        //close device firstly
        if(captureDev_!=0){
            SDL_CloseAudioDevice(captureDev_);
            captureDev_=0;
        }

        //SDL_Init(SDL_INIT_AUDIO);
        //SDL_AudioDeviceID dev;
        SDL_AudioSpec want, have;
        
        SDL_zero(want);
        want.format     = AUDIO_S16SYS;
        want.freq       = freq;             // 44100;
        want.channels   = channels;         // channels_;
        want.samples    = samples;          // 1024;
        //want.callback   = (*pf);
        want.callback   =pcm_fram_callback;
        pfCallback=pf;
        
        want.userdata   = custom;
        
        if(nullptr==ns_)
            ns_=std::make_shared<webrtc_3a1v>(16000,1);
        
        //保存配置
        //freq_=freq;
        //channels_=channels;
        
        captureDev_ = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(audioCaptureIndex_, 1), 1, &want, &have, 0);
        
        if (have.format != want.format) {
            SDL_Log("We didn't get the wanted format.");
            return ;
        }
        
        if(true){//设备打开后默认处于暂停状态  需要通过下面的语句开启音频
            SDL_PauseAudioDevice(captureDev_, 0); //0取消暂停，非0表示暂停
            if (captureDev_ == 0) {
                SDL_Log("Failed to open audio: %s", SDL_GetError());
                return ;
            }
        }
        //    printf("Started at %u\n", SDL_GetTicks());
        //    SDL_Delay(5000);
    }else if(captureDev_!=0)
    {
        SDL_CloseAudioDevice(captureDev_);
        captureDev_=0;
    }
}

void live_audio::playPCM(int64_t playerIndex
                         ,uint8_t* pcm,int pcm_len
                         ,int freq,int channels)
{
    auto* node=scene_->GetChild((unsigned int)playerIndex);
    if(node)
    {
        auto* ctl=node->GetComponent<Character>();
        if(ctl)
        {
            ctl->playPCM(pcm,pcm_len,freq,channels);
        }
        
    }
}

