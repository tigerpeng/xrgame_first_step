#pragma once

#include "../Urho3DAll.h"
#include <SDL/SDL.h>
#include <thread>
#include <mutex>
#include <map>
#include <unordered_map>
#include <set>
#include <vector>

//http://dranger.com/ffmpeg/tutorial01.html
class URHO3D_API LiveVideo : public Component
{
	URHO3D_OBJECT(LiveVideo, Component);

    enum YUVP420PlaneType
    {
        YUV_PLANE_Y,
        YUV_PLANE_U,
        YUV_PLANE_V,
        YUV_PLANE_MAX_SIZE
    };

    typedef struct {
        int   y_width;      /**< Width of the Y' luminance plane */
        int   y_height;     /**< Height of the luminance plane */
        int   y_stride;     /**< Offset in bytes between successive rows */
        
        int   uv_width;     /**< Width of the Cb and Cr chroma planes */
        int   uv_height;    /**< Height of the chroma planes */
        int   uv_stride;    /**< Offset between successive chroma rows */
        unsigned char *y;   /**< Pointer to start of luminance data */
        unsigned char *u;   /**< Pointer to start of Cb data */
        unsigned char *v;   /**< Pointer to start of Cr data */
        
    } yuv_buffer;
    
    std::function <void(SharedPtr<Material>)>  call_back_;
public:
    LiveVideo(Context* context);
	virtual ~LiveVideo();
	static void RegisterObject(Context* context);

    void OnVideoFrame(unsigned char* plane[3],int y_info[3],int uv_info[3]);

    void SetMaterailReady(std::function <void(SharedPtr<Material>)>  callback)
    {
        //准备视频回放
        ResourceCache* cache = GetSubsystem<ResourceCache>();
        outputMaterial=cache->GetResource<Material>("Materials/TVmaterialGPUYUV.xml");
        call_back_=callback;
    }
     

private:
	void HandleUpdate(StringHash eventType, VariantMap& eventData);
	bool InitTexture();
    void UpdatePlaneTextures();
    
private:
    yuv_buffer              m_YUVFrame;
    std::mutex              mutex_;
    
    SharedPtr<Material>     outputMaterial;
    SharedPtr<Texture2D>    outputTexture[YUV_PLANE_MAX_SIZE];
    int64_t                 videoFrameNO_;
    bool                    frameDirty_;
    
    
    
    unsigned char* framePlanarDataY_;
    unsigned char* framePlanarDataU_;
    unsigned char* framePlanarDataV_;
};

//class webrtc_3a1v;
class live_audio : public std::enable_shared_from_this<live_audio>
{
public:
    live_audio(bool& speaker);
    ~live_audio();
    
    void startCaptureForOpus(int index=0,int freq=16000,int channels=1,int samples=1600);
    
    void processPCM(Uint8 *stream, int len);
        
    //void startCapture(SharedPtr<Scene> scene,Network* net);
public:
//    void captureOpen(void(*pf)(void*,uint8_t*,int),void* custom
//                     ,int index=0,int freq=16000,int channels=1,int samples=1024) ;
    

    
    void setScene(SharedPtr<Scene> scene,std::shared_ptr<IXRGroup> xr){
        scene_=scene;
        xrGroup_=xr;
    }
    
    
    
    
    PODVector<int> GetChangeSounder(const std::set<int>& newSounder);
    
    int GetSubcribeSounderCount(){return sounders_.size();}
    
    
    //int on_encoded_frame(int64_t uid,uint8_t* buf,int len);
    
private:
    void playPCM(int64_t playerIndex,uint8_t* pcm,int pcm_len);
    //void playPCM(int64_t playerIndex,uint8_t* pcm,int pcm_len,int freq,int channels) ;


private:
    const bool&                                     bSpeaker_;
    SharedPtr<Scene>                                scene_;
    //for capture
    int                                             audioCaptureIndex_;
    SDL_AudioDeviceID                               captureDev_;
    
    
    //分组的语音视频权限管理
    std::shared_ptr<IXRGroup>                       xrGroup_;
    audioConfig                                     conf_;

    
    std::map<int,SharedPtr<BufferedSoundStream>>    steamBuffers_;
    
    std::set<int>           sounders_;
};


#include "LiveVideo.inl"
