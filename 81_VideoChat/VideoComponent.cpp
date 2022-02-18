#include "VideoComponent.h"

VideoComponent::VideoComponent(Context* context) :
	Component(context),
	frameWidth_(0),
	frameHeight_(0),
	outputMaterial(nullptr),
	framePlanarDataY_(0),
	framePlanarDataU_(0),
	framePlanarDataV_(0)
    ,firstvframe_(false)
    ,inintFrame_(false)
    ,dirty_(false)
{
	SubscribeToEvent(E_SCENEPOSTUPDATE, URHO3D_HANDLER(VideoComponent, HandleUpdate));
}

VideoComponent::~VideoComponent()
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
void VideoComponent::RegisterObject(Context* context)
{
	context->RegisterFactory<VideoComponent>();
}

bool VideoComponent::SetOutputModel(StaticModel* sm)
{
    bool ret = false;
    if (sm)
    {
        // Set model surface
        outputModel = sm;
        outputMaterial = outputModel->GetMaterial(0);
        
        int a=0;a++;

        
        
//        // Create textures & images
//        InitTexture();
//        //InitCopyBuffer();
//        ScaleModelAccordingVideoRatio();
    }

    return ret;
}

void VideoComponent::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    if(firstvframe_){
        if(!inintFrame_)
        {
            // Create textures & images
            InitTexture();
            //InitCopyBuffer();
            //ScaleModelAccordingVideoRatio();
            inintFrame_=true;
        }
    }

    if(dirty_){
        UpdatePlaneTextures();
        dirty_=false;
    }
    return ;
//    using namespace Update;
//    float timeStep = eventData[P_TIMESTEP].GetFloat();
//    //Time* time = GetSubsystem<Time>();
//
//    unsigned frame = Advance(timeStep);
//    if (!isStopped_ && prevFrame_ != frame)
//    {
//        UpdatePlaneTextures();
//    }
//
//    prevFrame_ = frame;
}


//yuv420p
void VideoComponent::OnVideoFrame(unsigned char* plane[3],int y_info[3],int uv_info[3])
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        m_YUVFrame.y=plane[0];
        m_YUVFrame.u=plane[1];
        m_YUVFrame.v=plane[2];
        
        m_YUVFrame.y_width  =y_info[0];
        m_YUVFrame.y_height =y_info[1];
        m_YUVFrame.y_stride =y_info[2];
        
        m_YUVFrame.uv_width =uv_info[0];
        m_YUVFrame.uv_height=uv_info[1];
        m_YUVFrame.uv_stride=uv_info[2];
    }

    if(!firstvframe_)
        firstvframe_=true;
    
    dirty_=true;
}
bool VideoComponent::InitTexture()
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
			framePlanarDataY_ = new unsigned char[texWidth * texHeight];
			break;
		
		case YUV_PLANE_U:
			texWidth = m_YUVFrame.uv_width;
			texHeight = m_YUVFrame.uv_height;
			framePlanarDataU_ = new unsigned char[texWidth * texHeight];
			break;

		case YUV_PLANE_V:
			texWidth = m_YUVFrame.uv_width;
			texHeight = m_YUVFrame.uv_height;
			framePlanarDataV_ = new unsigned char[texWidth * texHeight];
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
	
	outputModel->SetMaterial(0, outputMaterial);

    //for test
    if(cube1_)
        cube1_->SetMaterial(0, outputMaterial);
    
	return ret;
}

void VideoComponent::UpdatePlaneTextures()
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    if(!firstvframe_)
        return ;
	Graphics* graphics = GetSubsystem<Graphics>();

//    // Convert non-planar YUV-frame into separated planar raw-textures
//    for (int y = 0; y < m_YUVFrame.uv_height; ++y)
//    {
//        for (int x = 0; x < m_YUVFrame.uv_width; ++x)
//        {
//
//            const int offsetUV = x + y * m_YUVFrame.uv_width;
//
//            framePlanarDataU_[offsetUV] = m_YUVFrame.u[x + y * m_YUVFrame.uv_stride];
//            framePlanarDataV_[offsetUV] = m_YUVFrame.v[x + y * m_YUVFrame.uv_stride];
//
//            // 2x2 more work for Y
//
//            const int offsetLUTile = x + y * m_YUVFrame.y_width;
//            framePlanarDataY_[offsetLUTile] = m_YUVFrame.y[x + y * m_YUVFrame.y_stride];
//
//            const int offsetRUTile = m_YUVFrame.uv_width + x + y * m_YUVFrame.y_width;
//            framePlanarDataY_[offsetRUTile] = m_YUVFrame.y[m_YUVFrame.uv_width + x + y * m_YUVFrame.y_stride];
//
//            const int offsetLBTile = x + (y + m_YUVFrame.uv_height) * m_YUVFrame.y_width;
//            framePlanarDataY_[offsetLBTile] = m_YUVFrame.y[x + ((y + m_YUVFrame.uv_height) * m_YUVFrame.y_stride)];
//
//            const int offsetRBTile = (m_YUVFrame.uv_width + x) + (y + m_YUVFrame.uv_height) * m_YUVFrame.y_width;
//            framePlanarDataY_[offsetRBTile] = m_YUVFrame.y[(m_YUVFrame.uv_width + x) + ((y + m_YUVFrame.uv_height) * m_YUVFrame.y_stride)];
//
//        }
//    }
    
    framePlanarDataY_=m_YUVFrame.y;
    framePlanarDataU_=m_YUVFrame.u;
    framePlanarDataV_=m_YUVFrame.v;

	// Fill textures with new data
	for (int i = 0; i < YUV_PLANE_MAX_SIZE; ++i)
	{
		switch (i)
		{
        case YUVP420PlaneType::YUV_PLANE_Y:
            outputTexture[i]->SetSize(m_YUVFrame.y_width, m_YUVFrame.y_height, Graphics::GetLuminanceFormat(), TEXTURE_DYNAMIC);
            outputTexture[i]->SetData(0, 0, 0, m_YUVFrame.y_width, m_YUVFrame.y_height, (const void*)framePlanarDataY_);
            break;

        case YUVP420PlaneType::YUV_PLANE_U:
            outputTexture[i]->SetSize(m_YUVFrame.uv_width, m_YUVFrame.uv_height, Graphics::GetLuminanceFormat(), TEXTURE_DYNAMIC);
            outputTexture[i]->SetData(0, 0, 0, m_YUVFrame.uv_width, m_YUVFrame.uv_height, (const void*)framePlanarDataU_);
            break;

        case YUVP420PlaneType::YUV_PLANE_V:
            outputTexture[i]->SetSize(m_YUVFrame.uv_width, m_YUVFrame.uv_height, Graphics::GetLuminanceFormat(), TEXTURE_DYNAMIC);
            outputTexture[i]->SetData(0, 0, 0, m_YUVFrame.uv_width, m_YUVFrame.uv_height, (const void*)framePlanarDataV_);
            break;
		}
	}
}

void VideoComponent::ScaleModelAccordingVideoRatio()
{
	if (outputModel)
	{
		Node* node = outputModel->GetNode();
		float ratioW = (float)frameWidth_ / (float)frameHeight_;
		float ratioH = (float)frameHeight_ / (float)frameWidth_;

		Vector3 originalScale = node->GetScale();
		node->SetScale(Vector3(originalScale.x_, originalScale.x_ * ratioH, originalScale.z_ * ratioH));
	}
}
