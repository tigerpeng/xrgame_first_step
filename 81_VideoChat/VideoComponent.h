#include "Urho3DAll.h"
#include <thread>
#include <mutex>
//#include <ogg/ogg.h>
//#include <theora/theora.h>
//#include <theora/theoradec.h>
//#pragma comment( lib, "libogg_static.lib" )
//#pragma comment( lib, "libtheora_static.lib" )

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

//http://dranger.com/ffmpeg/tutorial01.html
class URHO3D_API VideoComponent : public Component
{
	URHO3D_OBJECT(VideoComponent, Component);
public:
	SharedPtr<StaticModel>  outputModel;
	SharedPtr<Material>     outputMaterial;
	SharedPtr<Texture2D>    outputTexture[YUV_PLANE_MAX_SIZE];

	unsigned prevTime_, prevFrame_;
	unsigned char* framePlanarDataY_;
	unsigned char* framePlanarDataU_;
	unsigned char* framePlanarDataV_;

	VideoComponent(Context* context);
	virtual ~VideoComponent();
	static void RegisterObject(Context* context);


	bool SetOutputModel(StaticModel* sm);
	void ScaleModelAccordingVideoRatio();

//    int GetFrameWidth(void) const { return frameWidth_; };
//    int GetFrameHeight(void) const { return frameHeight_; };
//    float GetFramesPerSecond(void) const { return framesPerSecond_; };
	//void GetFrameRGB(char* outFrame, int pitch);
	//void GetFrameYUV444(char* outFrame, int pitch);
	void UpdatePlaneTextures();
    void OnVideoFrame(unsigned char* plane[3],int y_info[3],int uv_info[3]);
    
    void SetVideoBox(SharedPtr<Material> material)
    {
        material_=material;
    }
    
    bool SetTestModel_1(StaticModel* smTest)
    {cube1_=smTest;}
private:
	void HandleUpdate(StringHash eventType, VariantMap& eventData);
	bool InitTexture();


	float       framesPerSecond_;
	unsigned    frameWidth_;
	unsigned    frameHeight_;

	yuv_buffer			m_YUVFrame;
    std::mutex          mutex_;
    bool                firstvframe_;
    bool                inintFrame_;
    bool                dirty_;
    
    
    SharedPtr<Material> material_;
    
    
    //for test
    SharedPtr<StaticModel>  cube1_;
};
