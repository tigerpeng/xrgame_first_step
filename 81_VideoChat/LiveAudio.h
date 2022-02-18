#pragma once
#include "Urho3DAll.h"

#include <memory>
#include <thread>
#include <mutex>
#include <SDL/SDL.h>

#include <facepower/api_facepower.h>


class live_audio : public ILiveAudio
                 , public std::enable_shared_from_this<live_audio>
{
public:
    live_audio(Context* context);
    ~live_audio();
    
public:
    void captureOpen(void(*pf)(void*,uint8_t*,int),void* custom
                     ,int index=0,int freq=1600,int channels=1,int samples=1024) override;
    
    void playPCM(int64_t uid,uint8_t* pcm,int pcm_len,int freq,int channels) override;
    
private:
    Context* context_;
    //for play back
    SharedPtr<Node>                             node_;
    SharedPtr<BufferedSoundStream>              soundStream_;
    
    
    int                 audioCaptureIndex_;
    SDL_AudioDeviceID   captureDev_;
};
