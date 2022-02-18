#include "LiveAudio.h"

#include <Urho3D/Audio/Audio.h>
#include <Urho3D/Audio/AudioEvents.h>
#include <Urho3D/Audio/Sound.h>
#include <Urho3D/Audio/SoundSource.h>



live_audio::live_audio(Context* context)
:context_(context)
,audioCaptureIndex_(0)
{
    
}
live_audio::~live_audio()
{
    
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
        want.callback   = (*pf);
        //want.callback   =callback;
        want.userdata   = custom;
        
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

void live_audio::playPCM(int64_t uid
                         ,uint8_t* pcm,int pcm_len
                         ,int freq,int channels)
{
    if(soundStream_==nullptr)
    {
        //语音回放
        // Sound source needs a node so that it is considered enabled
        node_ = new Node(context_);
        auto* source = node_->CreateComponent<SoundSource>();
        
        soundStream_ = new BufferedSoundStream();
        // Set format: 44100 Hz, sixteen bit, mono
        soundStream_->SetFormat(freq, true, (channels==2));
        // Start playback. We don't have data in the stream yet, but the SoundSource will wait until there is data,
        // as the stream is by default in the "don't stop at end" mode
        
        source->Play(soundStream_);
    }
    
    if(pcm_len>0)
    {
        /*
             static std::fstream        pcmDump_;
             if(!pcmDump_.is_open())
                 pcmDump_.open("/Users/leeco/Music/dump_pcm.dat", std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
             pcmDump_.write((char*)pcm,len);
        */
            
            
            //保证语音的实时性 语音的滋滋声还不清楚
            //Return length of buffered (unplayed) sound data in seconds.
            float buffLen=soundStream_->GetBufferLength();
        //    if(buffLen>=2)
        //        soundStream_->Clear();
        
        
        soundStream_->AddData(pcm, pcm_len);
    }
}

