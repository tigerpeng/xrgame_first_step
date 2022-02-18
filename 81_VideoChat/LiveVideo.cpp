#include "LiveVideo.h"

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
