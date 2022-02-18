//#include "LiveVideo.h"
#include <Urho3D/Audio/Audio.h>
#include <Urho3D/Audio/AudioEvents.h>
#include <Urho3D/Audio/Sound.h>
#include <Urho3D/Audio/SoundSource.h>

#include <fstream>


#define FRAME_SIZE 480
#define MAX_FRAME_SIZE (100*FRAME_SIZE)
#define MAX_PACKET_SIZE (1500)
#define MAX_CHANNELS 2

uint32_t    _sampleRate =8000;  //48000 16000
uint16_t    _channels   =1;     //2
uint32_t    _samples=_sampleRate/1000 *100; //10ms

/*
#define CODEC_NAME "OPUS"
struct BreezeAudio
{
    BreezeAudio()
    {
        int len=::sprintf(codecName, "%s",CODEC_NAME);
        codecName[len]=0;
        
        sampleRate=8000;
        channels=1;
    }
    char        codecName[20];
    uint32_t    sampleRate;
    uint16_t    channels;
};
*/



void(*pfCallback)(void*,uint8_t*,int)=nullptr;
void pcm_fram_callback(void *userdata, Uint8 *stream, int len) {
    if(userdata)
    {
        live_audio* ptr=(live_audio*)userdata;
        ptr->processPCM(stream,len);
    }
}

live_audio::live_audio(bool& speaker)
:bSpeaker_(speaker)
,audioCaptureIndex_(0)
,captureDev_(0)
//,enc_(nullptr)
//,dec_(nullptr)
{

}
live_audio::~live_audio()
{
    //关闭采集
    startCaptureForOpus(-1);
//    if(enc_)
//        opus_encoder_destroy(enc_);
//    if(dec_)
//        opus_decoder_destroy(dec_);
        
    //关闭 ---- del
    //captureOpen(nullptr,nullptr,0,0,0,0);
}

void live_audio::startCaptureForOpus(int index,int freq,int channels,int samples)
{
   
    freq        =_sampleRate;
    channels    =_channels;
    samples     =_samples;
    
    conf_.sampleRate=_sampleRate;
    conf_.channels=_channels;
    conf_.samples=_samples;
    
    if(index>=0)
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
        want.samples    = samples;          // 样本数
                        //采集的缓冲区字节数=sizeof(format)*channels*samples
        
        //want.callback   = (*pf);
        want.callback   =pcm_fram_callback;
        //pfCallback=pf;
        want.userdata   = this;//custom;
        

        //保存配置
        //freq_=freq;
        //channels_=channels;
        auto fn_callback=
        std::bind(&live_audio::playPCM,shared_from_this(),
                  std::placeholders::_1,std::placeholders::_2,std::placeholders::_3);
        
        xrGroup_->configAudio(conf_, fn_callback);
        
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

void live_audio::processPCM(Uint8* pcm_bytes, int len)
{
    if(bSpeaker_)
        xrGroup_->caputurePCM(pcm_bytes, len);
}

/*
 /Applications/VLC.app/Contents/MacOS/VLC --demux=rawaud --rawaud-channels 1 --rawaud-samplerate 8000 /Users/leeco/Music/pcm_dump.dat
 */


//void live_audio::captureOpen(void(*pf)(void*,uint8_t*,int),void* custom
//                         ,int index,int freq,int channels,int samples)
//{
//    if((*pf))
//    {
//        //record it
//        audioCaptureIndex_=index;
//
//        //close device firstly
//        if(captureDev_!=0){
//            SDL_CloseAudioDevice(captureDev_);
//            captureDev_=0;
//        }
//
//        //SDL_Init(SDL_INIT_AUDIO);
//        //SDL_AudioDeviceID dev;
//        SDL_AudioSpec want, have;
//
//        SDL_zero(want);
//        want.format     = AUDIO_S16SYS;
//        want.freq       = freq;             // 44100;
//        want.channels   = channels;         // channels_;
//        want.samples    = samples;          // 1024; 样本数
//                                    //采集的缓冲区字节数=sizeof(format)*channels*samples
//
//        //want.callback   = (*pf);
//        want.callback   =pcm_fram_callback;
//        pfCallback=pf;
//
//        want.userdata   = custom;
//
////        if(nullptr==ns_)
////            ns_=std::make_shared<webrtc_3a1v>(16000,1);
//
//        //保存配置
//        //freq_=freq;
//        //channels_=channels;
//
//        captureDev_ = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(audioCaptureIndex_, 1), 1, &want, &have, 0);
//
//        if (have.format != want.format) {
//            SDL_Log("We didn't get the wanted format.");
//            return ;
//        }
//
//        if(true){//设备打开后默认处于暂停状态  需要通过下面的语句开启音频
//            SDL_PauseAudioDevice(captureDev_, 0); //0取消暂停，非0表示暂停
//            if (captureDev_ == 0) {
//                SDL_Log("Failed to open audio: %s", SDL_GetError());
//                return ;
//            }
//        }
//        //    printf("Started at %u\n", SDL_GetTicks());
//        //    SDL_Delay(5000);
//    }else if(captureDev_!=0)
//    {
//        SDL_CloseAudioDevice(captureDev_);
//        captureDev_=0;
//    }
//}


//void live_audio::startCapture(SharedPtr<Scene> scene,Network* net)
//{
//    scene_=scene;
//    
//    //network_=net;
////    if(facepower_)
////    {
////        auto func_callback=std::bind(&live_audio::on_encoded_frame,shared_from_this(),std::placeholders::_1,std::placeholders::_2,std::placeholders::_3);
////
////        facepower_->startAudioCapture(func_callback);
////    }
//}


//int live_audio::on_encoded_frame(int64_t uid,uint8_t* buf,int len)
//{
//
////    int err;
////    //decode...
////    if(nullptr==dec_)
////    {
////        dec_= opus_decoder_create(_sampleRate, _channels, &err);
////        if(err != OPUS_OK || dec_==NULL) {
////            fprintf(stderr, "failed to create an encoder: %s\n", opus_strerror(err));
////            if (!dec_)
////            {
////                opus_decoder_destroy(dec_);
////                dec_=nullptr;
////            }
////        }
////    }
////
////
////    opus_int16 out[MAX_FRAME_SIZE * MAX_CHANNELS];
////    int frame_size = opus_decode(dec_, buf, len, out, MAX_FRAME_SIZE, 0);
////    if (frame_size < 0) {
////        fprintf(stderr, "decoder failed: %s\n", opus_strerror(frame_size));
////    }
////    int pcmSize=frame_size*sizeof(opus_int16)*_channels;
////
////
////    playPCM(uid,(uint8_t*)out,pcmSize,_sampleRate,_channels);
//    return 1;
//}

PODVector<int> live_audio::GetChangeSounder(const std::set<int>& newSounder)
{
    PODVector<int>  nodeChange;
   
    if(newSounder!=sounders_)
    {
        //增加的节点
        auto itN=newSounder.begin();
        while(itN!=newSounder.end())
        {
            if(sounders_.find(*itN)==sounders_.end())
                nodeChange.Push(*itN);
            ++itN;
        }
        
        //减少的节点
        auto itO=sounders_.begin();
        while(itO!=sounders_.end())
        {
            if(newSounder.find(*itO)==newSounder.end())
                nodeChange.Push(-*itO);
            ++itO;
        }
    }
    
    sounders_=newSounder;
    return nodeChange;
}

void live_audio::playPCM(int64_t playerIndex,uint8_t* pcm,int pcm_len)
{
    SharedPtr<BufferedSoundStream> soundStream;
    auto it=steamBuffers_.find(playerIndex);
    if(it!=steamBuffers_.end())
        soundStream=it->second;
    else if(scene_)
    {
        //语音回放
        // Sound source needs a node so that it is considered enabled
        soundStream = new BufferedSoundStream();
        // Set format: 44100 Hz, sixteen bit, mono
        soundStream->SetFormat(conf_.sampleRate, true, (conf_.channels==2));
        
        auto* node=scene_->GetNode(playerIndex);
        if(node)
        {
            SoundSource3D *snd_source_3d = node->GetComponent<SoundSource3D>();
            if(snd_source_3d)
                snd_source_3d->Play(soundStream);
        }
        steamBuffers_.insert(std::make_pair(playerIndex, soundStream));
    }
        
    if(pcm_len>0&&soundStream)
    {
        soundStream->AddData(pcm, pcm_len);
        
        
       //记录播放的pcm，用于回声消除
//       if(ns_)
//           ns_->playback_pcm(pcm, pcm_len);
        
        //float buferTime=soundStream->GetBufferLength();
    }
    
}
//void live_audio::playPCM(int64_t playerIndex
//                         ,uint8_t* pcm,int pcm_len
//                         ,int freq,int channels)
//{
//    SharedPtr<BufferedSoundStream> soundStream;
//    auto it=steamBuffers_.find(playerIndex);
//    if(it!=steamBuffers_.end())
//        soundStream=it->second;
//
//    else if(scene_)
//    {
//        //语音回放
//        // Sound source needs a node so that it is considered enabled
//        soundStream = new BufferedSoundStream();
//        // Set format: 44100 Hz, sixteen bit, mono
//        soundStream->SetFormat(freq, true, (channels==2));
//
//        auto* node=scene_->GetNode(playerIndex);
//        if(node)
//        {
//            SoundSource3D *snd_source_3d = node->GetComponent<SoundSource3D>();
//            if(snd_source_3d)
//                snd_source_3d->Play(soundStream);
//        }
//        steamBuffers_.insert(std::make_pair(playerIndex, soundStream));
//    }
//
//    if(pcm_len>0&&soundStream)
//    {
//        soundStream->AddData(pcm, pcm_len);
//
//
//       //记录播放的pcm，用于回声消除
////       if(ns_)
////           ns_->playback_pcm(pcm, pcm_len);
//
//        //float buferTime=soundStream->GetBufferLength();
//    }
//
//    //steamBuffers_->SetData
//}



LiveVideo::LiveVideo(Context* context):Component(context)
,videoFrameNO_(-1)
,frameDirty_(false)
,framePlanarDataY_(0)
,framePlanarDataU_(0)
,framePlanarDataV_(0)
{
	SubscribeToEvent(E_SCENEPOSTUPDATE, URHO3D_HANDLER(LiveVideo, HandleUpdate));
}

LiveVideo::~LiveVideo()
{
    if (framePlanarDataY_)
    {
        delete[] framePlanarDataY_;
        framePlanarDataY_ = 0;
    }

    if (framePlanarDataU_)
    {
        delete[] framePlanarDataU_;
        framePlanarDataU_ = 0;
    }

    if (framePlanarDataV_)
    {
        delete[] framePlanarDataV_;
        framePlanarDataV_ = 0;
    }
}
void LiveVideo::RegisterObject(Context* context)
{
	context->RegisterFactory<LiveVideo>();
}

void LiveVideo::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    //更新视频
    if(videoFrameNO_==0)
    {
        InitTexture();
    }else if(videoFrameNO_>0&&frameDirty_){
        UpdatePlaneTextures();
        frameDirty_=false;
    }
}

void LiveVideo::OnVideoFrame(unsigned char* plane[3],int y_info[3],int uv_info[3])
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        int y_realWidth=std::max(y_info[0],std::abs(y_info[2]));
        int uv_realWidth=std::max(uv_info[0],std::abs(uv_info[2]));
        if(framePlanarDataY_==0)
        {
            framePlanarDataY_ = new unsigned char[y_realWidth * y_info[1]];
        }
        if(framePlanarDataU_==0)
        {
            framePlanarDataU_ = new unsigned char[uv_realWidth* uv_info[1]];
        }
        if(framePlanarDataV_==0)
        {
            framePlanarDataV_ = new unsigned char[uv_realWidth * uv_info[1]];
        }
        
        
        //copy
        memcpy(framePlanarDataY_, plane[0], std::abs(y_info[2]) * y_info[1]);
        memcpy(framePlanarDataU_, plane[1], std::abs(uv_info[2]) * uv_info[1]);
        memcpy(framePlanarDataV_, plane[2], std::abs(uv_info[2]) * uv_info[1]);

        m_YUVFrame.y=framePlanarDataY_;
        m_YUVFrame.u=framePlanarDataU_;
        m_YUVFrame.v=framePlanarDataV_;
//        m_YUVFrame.y=plane[0];
//        m_YUVFrame.u=plane[1];
//        m_YUVFrame.v=plane[2];
        
        m_YUVFrame.y_width  =y_info[0];
        m_YUVFrame.y_height =y_info[1];
        m_YUVFrame.y_stride =y_info[2];
        
        m_YUVFrame.uv_width =uv_info[0];
        m_YUVFrame.uv_height=uv_info[1];
        m_YUVFrame.uv_stride=uv_info[2];
    }
    
    if(videoFrameNO_<0)
        videoFrameNO_=0;
    else if(videoFrameNO_>0)
        videoFrameNO_++;
    
    frameDirty_=true;
}

bool LiveVideo::InitTexture()
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    bool ret = false;
    
    // Try clear if using case of reassingn the movie file
    for (int i = 0; i < YUV_PLANE_MAX_SIZE; ++i)
    {
        if (outputTexture[i])
        {
            outputTexture[i]->ReleaseRef();
            outputTexture[i] = 0;
        }
    }
    
    // do this for fill m_YUVFrame with properly info about frame
    //Advance(0);
    
    // Planes textures create
    for (int i = 0; i < YUV_PLANE_MAX_SIZE; ++i)
    {
        int texWidth = 0;
        int texHeight = 0;
        
        switch (i)
        {
            case YUV_PLANE_Y:
                texWidth = m_YUVFrame.y_width;
                texHeight = m_YUVFrame.y_height;
                //framePlanarDataY_ = new unsigned char[texWidth * texHeight];
                break;
                
            case YUV_PLANE_U:
                texWidth = m_YUVFrame.uv_width;
                texHeight = m_YUVFrame.uv_height;
                //framePlanarDataU_ = new unsigned char[texWidth * texHeight];
                break;
                
            case YUV_PLANE_V:
                texWidth = m_YUVFrame.uv_width;
                texHeight = m_YUVFrame.uv_height;
                //framePlanarDataV_ = new unsigned char[texWidth * texHeight];
                break;
        }
        outputTexture[i] = SharedPtr<Texture2D>(new Texture2D(context_));
        outputTexture[i]->SetSize(texWidth, texHeight, Graphics::GetLuminanceFormat(), TEXTURE_DYNAMIC);
        outputTexture[i]->SetFilterMode(FILTER_BILINEAR);
        outputTexture[i]->SetNumLevels(1);
        outputTexture[i]->SetAddressMode(TextureCoordinate::COORD_U, TextureAddressMode::ADDRESS_MIRROR);
        outputTexture[i]->SetAddressMode(TextureCoordinate::COORD_V, TextureAddressMode::ADDRESS_MIRROR);
        
    }
    
    // assign planes textures into sepparated samplers for shader
    outputMaterial->SetTexture(TextureUnit::TU_DIFFUSE, outputTexture[YUV_PLANE_Y]);
    outputMaterial->SetTexture(TextureUnit::TU_SPECULAR, outputTexture[YUV_PLANE_U]);
    outputMaterial->SetTexture(TextureUnit::TU_NORMAL, outputTexture[YUV_PLANE_V]);
    
    
    if(call_back_)
        call_back_(outputMaterial);
    
    videoFrameNO_=1;
    return ret;
}

void LiveVideo::UpdatePlaneTextures()
{
    std::lock_guard<std::mutex> lock(mutex_);
    //Graphics* graphics = GetSubsystem<Graphics>();
    // Fill textures with new data
    for (int i = 0; i < YUV_PLANE_MAX_SIZE; ++i)
    {
        switch (i)
        {
            case YUVP420PlaneType::YUV_PLANE_Y:
                outputTexture[i]->SetSize(m_YUVFrame.y_width, m_YUVFrame.y_height, Graphics::GetLuminanceFormat(), TEXTURE_DYNAMIC);
                outputTexture[i]->SetData(0, 0, 0, m_YUVFrame.y_width, m_YUVFrame.y_height, (const void*)m_YUVFrame.y);
                break;
                
            case YUVP420PlaneType::YUV_PLANE_U:
                outputTexture[i]->SetSize(m_YUVFrame.uv_width, m_YUVFrame.uv_height, Graphics::GetLuminanceFormat(), TEXTURE_DYNAMIC);
                outputTexture[i]->SetData(0, 0, 0, m_YUVFrame.uv_width, m_YUVFrame.uv_height, (const void*)m_YUVFrame.u);
                break;
                
            case YUVP420PlaneType::YUV_PLANE_V:
                outputTexture[i]->SetSize(m_YUVFrame.uv_width, m_YUVFrame.uv_height, Graphics::GetLuminanceFormat(), TEXTURE_DYNAMIC);
                outputTexture[i]->SetData(0, 0, 0, m_YUVFrame.uv_width, m_YUVFrame.uv_height, (const void*)m_YUVFrame.v);
                break;
        }
    }
}
