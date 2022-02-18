#pragma once

#include "../Urho3DAll.h"
#include <thread>
#include <mutex>



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
