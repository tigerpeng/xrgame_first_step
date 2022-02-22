//
// Copyright (c) 2008-2020 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include <Urho3D/Engine/Application.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Engine/Console.h>
#include <Urho3D/UI/Cursor.h>
#include <Urho3D/Engine/DebugHud.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Engine/EngineDefs.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Input/InputEvents.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/UI/Sprite.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Core/Timer.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/IO/Log.h>


Sample::Sample(Context* context) :
    Application(context),
    yaw_(0.0f),
    pitch_(0.0f),
    touchEnabled_(false),
    useMouseMode_(MM_ABSOLUTE),
    screenJoystickIndex_(M_MAX_UNSIGNED),
    screenJoystickSettingsIndex_(M_MAX_UNSIGNED),
    paused_(false)
{
    
    facepower_=getFacePower();
    clientObjectID_=0;
    avReady_=false;
    open_camera_distn_=10;
    broadcasting_video_=false;
    broadcasting_audio_=true;
    touch_rotate_=true;
    timer_counter_=std::chrono::steady_clock::now();
    
    bMyEcho_=false;//是否订阅自己的声音？ true 用于调试
    
    #if defined _WIN32

    #elif defined  __APPLE__
        //bMyEcho_=true;              //for debug
    #elif defined __ANDROID__

    #endif
}

void Sample::Setup()
{
    // Modify engine startup parameters
    engineParameters_[EP_WINDOW_TITLE] = GetTypeName();
    engineParameters_[EP_LOG_NAME]     = GetSubsystem<FileSystem>()->GetAppPreferencesDir("urho3d", "logs") + GetTypeName() + ".log";
    engineParameters_[EP_FULL_SCREEN]  = false;
    engineParameters_[EP_HEADLESS]     = false;
    engineParameters_[EP_SOUND]        = true;

    // Construct a search path to find the resource prefix with two entries:
    // The first entry is an empty path which will be substituted with program/bin directory -- this entry is for binary when it is still in build tree
    // The second and third entries are possible relative paths from the installed program/bin directory to the asset directory -- these entries are for binary when it is in the Urho3D SDK installation location
    if (!engineParameters_.Contains(EP_RESOURCE_PREFIX_PATHS))
        engineParameters_[EP_RESOURCE_PREFIX_PATHS] = ";../share/Resources;../share/Urho3D/Resources";
}

void Sample::Start()
{
    if (GetPlatform() == "Android" || GetPlatform() == "iOS")
        // On mobile platform, enable touch by adding a screen joystick
        InitTouchInput();
    else if (GetSubsystem<Input>()->GetNumJoysticks() == 0)
        // On desktop platform, do not detect touch when we already got a joystick
        SubscribeToEvent(E_TOUCHBEGIN, URHO3D_HANDLER(Sample, HandleTouchBegin));

    // Create logo
    CreateLogo();

    // Set custom window Title & Icon
    SetWindowTitleAndIcon();

    // Create console and debug HUD
    CreateConsoleAndDebugHud();

    // Subscribe key down event
    SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(Sample, HandleKeyDown));
    // Subscribe key up event
    SubscribeToEvent(E_KEYUP, URHO3D_HANDLER(Sample, HandleKeyUp));
    // Subscribe scene update event
    SubscribeToEvent(E_SCENEUPDATE, URHO3D_HANDLER(Sample, HandleSceneUpdate));
    
}

void Sample::Stop()
{
    la_.reset();
    if(facepower_){
        facepower_->setVideoCallBack(nullptr);
        facepower_->setLiveAudio(nullptr);
        facepower_->cmd("{\"cmd\":\"appExit\"}");
    }
    
    engine_->DumpResources(true);
}

void Sample::InitTouchInput()
{
    touchEnabled_ = true;

    ResourceCache* cache = GetSubsystem<ResourceCache>();
    Input* input = GetSubsystem<Input>();
    XMLFile* layout = cache->GetResource<XMLFile>("UI/ScreenJoystick_Samples.xml");
    const String& patchString = GetScreenJoystickPatchString();
    if (!patchString.Empty())
    {
        // Patch the screen joystick layout further on demand
        SharedPtr<XMLFile> patchFile(new XMLFile(context_));
        if (patchFile->FromString(patchString))
            layout->Patch(patchFile);
    }
    screenJoystickIndex_ = (unsigned)input->AddScreenJoystick(layout, cache->GetResource<XMLFile>("UI/DefaultStyle.xml"));
    input->SetScreenJoystickVisible(screenJoystickSettingsIndex_, true);
}

void Sample::InitMouseMode(MouseMode mode)
{
    useMouseMode_ = mode;

    Input* input = GetSubsystem<Input>();

    if (GetPlatform() != "Web")
    {
        if (useMouseMode_ == MM_FREE)
            input->SetMouseVisible(true);

        Console* console = GetSubsystem<Console>();
        if (useMouseMode_ != MM_ABSOLUTE)
        {
            input->SetMouseMode(useMouseMode_);
            if (console && console->IsVisible())
                input->SetMouseMode(MM_ABSOLUTE, true);
        }
    }
    else
    {
        input->SetMouseVisible(true);
        SubscribeToEvent(E_MOUSEBUTTONDOWN, URHO3D_HANDLER(Sample, HandleMouseModeRequest));
        SubscribeToEvent(E_MOUSEMODECHANGED, URHO3D_HANDLER(Sample, HandleMouseModeChange));
    }
}

void Sample::SetLogoVisible(bool enable)
{
    if (logoSprite_)
        logoSprite_->SetVisible(enable);
}

void Sample::CreateLogo()
{
    // Get logo texture
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    Texture2D* logoTexture = cache->GetResource<Texture2D>("Textures/FishBoneLogo.png");
    if (!logoTexture)
        return;

    // Create logo sprite and add to the UI layout
    UI* ui = GetSubsystem<UI>();
    logoSprite_ = ui->GetRoot()->CreateChild<Sprite>();

    // Set logo sprite texture
    logoSprite_->SetTexture(logoTexture);

    int textureWidth = logoTexture->GetWidth();
    int textureHeight = logoTexture->GetHeight();

    // Set logo sprite scale
    logoSprite_->SetScale(256.0f / textureWidth);

    // Set logo sprite size
    logoSprite_->SetSize(textureWidth, textureHeight);

    // Set logo sprite hot spot
    logoSprite_->SetHotSpot(textureWidth, textureHeight);

    // Set logo sprite alignment
    logoSprite_->SetAlignment(HA_RIGHT, VA_BOTTOM);

    // Make logo not fully opaque to show the scene underneath
    logoSprite_->SetOpacity(0.9f);

    // Set a low priority for the logo so that other UI elements can be drawn on top
    logoSprite_->SetPriority(-100);
}

void Sample::SetWindowTitleAndIcon()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    Graphics* graphics = GetSubsystem<Graphics>();
    Image* icon = cache->GetResource<Image>("Textures/UrhoIcon.png");
    graphics->SetWindowIcon(icon);
    graphics->SetWindowTitle("Urho3D Sample");
}

void Sample::CreateConsoleAndDebugHud()
{
    // Get default style
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    XMLFile* xmlFile = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");

    // Create console
    Console* console = engine_->CreateConsole();
    console->SetDefaultStyle(xmlFile);
    console->GetBackground()->SetOpacity(0.8f);

    // Create debug HUD.
    DebugHud* debugHud = engine_->CreateDebugHud();
    debugHud->SetDefaultStyle(xmlFile);
}


void Sample::HandleKeyUp(StringHash /*eventType*/, VariantMap& eventData)
{
    using namespace KeyUp;

    int key = eventData[P_KEY].GetInt();

    // Close console (if open) or exit when ESC is pressed
    if (key == KEY_ESCAPE)
    {
        Console* console = GetSubsystem<Console>();
        if (console->IsVisible())
            console->SetVisible(false);
        else
        {
            if (GetPlatform() == "Web")
            {
                GetSubsystem<Input>()->SetMouseVisible(true);
                if (useMouseMode_ != MM_ABSOLUTE)
                    GetSubsystem<Input>()->SetMouseMode(MM_FREE);
            }
            else
                engine_->Exit();
        }
    }
}

void Sample::HandleKeyDown(StringHash /*eventType*/, VariantMap& eventData)
{
    using namespace KeyDown;

    int key = eventData[P_KEY].GetInt();

    // Toggle console with F1
    if (key == KEY_F1)
        GetSubsystem<Console>()->Toggle();

    // Toggle debug HUD with F2
    else if (key == KEY_F2)
        GetSubsystem<DebugHud>()->ToggleAll();

    // Common rendering quality controls, only when UI has no focused element
    else if (!GetSubsystem<UI>()->GetFocusElement())
    {
        Renderer* renderer = GetSubsystem<Renderer>();

        // Preferences / Pause
        if (key == KEY_SELECT && touchEnabled_)
        {
            paused_ = !paused_;

            Input* input = GetSubsystem<Input>();
            if (screenJoystickSettingsIndex_ == M_MAX_UNSIGNED)
            {
                // Lazy initialization
                ResourceCache* cache = GetSubsystem<ResourceCache>();
                screenJoystickSettingsIndex_ = (unsigned)input->AddScreenJoystick(cache->GetResource<XMLFile>("UI/ScreenJoystickSettings_Samples.xml"), cache->GetResource<XMLFile>("UI/DefaultStyle.xml"));
            }
            else
                input->SetScreenJoystickVisible(screenJoystickSettingsIndex_, paused_);
        }

        // Texture quality
        else if (key == '1')
        {
            auto quality = (unsigned)renderer->GetTextureQuality();
            ++quality;
            if (quality > QUALITY_HIGH)
                quality = QUALITY_LOW;
            renderer->SetTextureQuality((MaterialQuality)quality);
        }

        // Material quality
        else if (key == '2')
        {
            auto quality = (unsigned)renderer->GetMaterialQuality();
            ++quality;
            if (quality > QUALITY_HIGH)
                quality = QUALITY_LOW;
            renderer->SetMaterialQuality((MaterialQuality)quality);
        }

        // Specular lighting
        else if (key == '3')
            renderer->SetSpecularLighting(!renderer->GetSpecularLighting());

        // Shadow rendering
        else if (key == '4')
            renderer->SetDrawShadows(!renderer->GetDrawShadows());

        // Shadow map resolution
        else if (key == '5')
        {
            int shadowMapSize = renderer->GetShadowMapSize();
            shadowMapSize *= 2;
            if (shadowMapSize > 2048)
                shadowMapSize = 512;
            renderer->SetShadowMapSize(shadowMapSize);
        }

        // Shadow depth and filtering quality
        else if (key == '6')
        {
            ShadowQuality quality = renderer->GetShadowQuality();
            quality = (ShadowQuality)(quality + 1);
            if (quality > SHADOWQUALITY_BLUR_VSM)
                quality = SHADOWQUALITY_SIMPLE_16BIT;
            renderer->SetShadowQuality(quality);
        }

        // Occlusion culling
        else if (key == '7')
        {
            bool occlusion = renderer->GetMaxOccluderTriangles() > 0;
            occlusion = !occlusion;
            renderer->SetMaxOccluderTriangles(occlusion ? 5000 : 0);
        }

        // Instancing
        else if (key == '8')
            renderer->SetDynamicInstancing(!renderer->GetDynamicInstancing());

        // Take screenshot
        else if (key == '9')
        {
            Graphics* graphics = GetSubsystem<Graphics>();
            Image screenshot(context_);
            graphics->TakeScreenShot(screenshot);
            // Here we save in the Data folder with date and time appended
            screenshot.SavePNG(GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Screenshot_" +
                Time::GetTimeStamp().Replaced(':', '_').Replaced('.', '_').Replaced(' ', '_') + ".png");
        }
    }
}

void Sample::HandleSceneUpdate(StringHash /*eventType*/, VariantMap& eventData)
{
    PreParseXRCommand();
        
    // Move the camera by touch, if the camera node is initialized by descendant sample class
    if (touchEnabled_ && cameraNode_ &&touch_rotate_)
    {
        Input* input = GetSubsystem<Input>();
        for (unsigned i = 0; i < input->GetNumTouches(); ++i)
        {
            TouchState* state = input->GetTouch(i);
            if (!state->touchedElement_)    // Touch on empty space
            {
                if (state->delta_.x_ ||state->delta_.y_)
                {
                    Camera* camera = cameraNode_->GetComponent<Camera>();
                    if (!camera)
                        return;

                    Graphics* graphics = GetSubsystem<Graphics>();
                    yaw_ += TOUCH_SENSITIVITY * camera->GetFov() / graphics->GetHeight() * state->delta_.x_;
                    pitch_ += TOUCH_SENSITIVITY * camera->GetFov() / graphics->GetHeight() * state->delta_.y_;

                    // Construct new orientation for the camera scene node from yaw and pitch; roll is fixed to zero
                    cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));
                }
                else
                {
                    // Move the cursor to the touch position
                    Cursor* cursor = GetSubsystem<UI>()->GetCursor();
                    if (cursor && cursor->IsVisible())
                        cursor->SetPosition(state->position_);
                }
            }
        }
    }
    
}


void Sample::HandleTouchBegin(StringHash /*eventType*/, VariantMap& eventData)
{
    // On some platforms like Windows the presence of touch input can only be detected dynamically
    InitTouchInput();
    UnsubscribeFromEvent("TouchBegin");
}

// If the user clicks the canvas, attempt to switch to relative mouse mode on web platform
void Sample::HandleMouseModeRequest(StringHash /*eventType*/, VariantMap& eventData)
{
    Console* console = GetSubsystem<Console>();
    if (console && console->IsVisible())
        return;
    Input* input = GetSubsystem<Input>();
    if (useMouseMode_ == MM_ABSOLUTE)
        input->SetMouseVisible(false);
    else if (useMouseMode_ == MM_FREE)
        input->SetMouseVisible(true);
    input->SetMouseMode(useMouseMode_);
}

void Sample::HandleMouseModeChange(StringHash /*eventType*/, VariantMap& eventData)
{
    Input* input = GetSubsystem<Input>();
    bool mouseLocked = eventData[MouseModeChanged::P_MOUSELOCKED].GetBool();
    input->SetMouseVisible(!mouseLocked);
}

void Sample::BroadCastingVideo(bool cast)
{
    broadcasting_video_=cast;
    
    //打开摄像头
    char sCMD[1024];
    int devIndex=0;
    int len=0;
    if(cast)
        len=::sprintf(sCMD, "{\"cmd\":\"openCamera\",\"devIndex\":%d}",devIndex);
    else
        len=::sprintf(sCMD, "{\"cmd\":\"closeCamera\",\"devIndex\":%d}",devIndex);
    
    std::string sCommand(sCMD,len);
    if(xrGroup_)
        xrGroup_->cmd(sCommand);
}

void Sample::RecordXRCommand(std::string const& cmdStr)
{
    std::lock_guard<std::mutex> lock(cmd_mutex_);
    cmd_que_.push(cmdStr);
}

//解析命令
void Sample::PreParseXRCommand()
{
    //采用定时器可以提高新能
    std::chrono::duration<double> timeDura = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - timer_counter_);
    
    if(timeDura.count()<1) //1秒
        return ;

    timer_counter_=std::chrono::steady_clock::now();
    
    
    
    
    //检查是否开启视频
    if(clientObjectID_>0&&!tv_name_.empty())
    {
        Node* myselfNode=scene_->GetNode(clientObjectID_);
        Node* tvNode= scene_->GetChild(tv_name_.c_str(), true);
        if(myselfNode&&tvNode)
        {
            auto pos1=myselfNode->GetPosition();
            
            auto pos2=tvNode->GetPosition();
            
            float distance=pos1.DistanceToPoint(pos2);
            
            if(distance<open_camera_distn_)
            {   if(!broadcasting_video_)
                    BroadCastingVideo(true);
            }else if(distance>open_camera_distn_)
            {
                if(broadcasting_video_)
                    BroadCastingVideo(false);
                
            }
        }
    }
    
    //实时更新
    if(avReady_)
        CheckSouderChange();
        
        
    std::string cmdStr;
    {
        std::lock_guard<std::mutex> lock(cmd_mutex_);
        if(cmd_que_.size()>0)
        {
            cmdStr=cmd_que_.front();
            cmd_que_.pop();
        }else
            return ;
    }
    
    //
    try{
        ptree pt;
        std::stringstream ss(cmdStr);
        read_json(ss, pt);
        const std::string cmd=pt.get("cmd","");
        
        if("roomInit"==cmd)
        {//xrGroup_ 收到编码的opus数据后回调
            avReady_=true;
        /*
            auto cb=std::bind(&live_audio::on_encoded_frame,la_,
                              std::placeholders::_1,std::placeholders::_2,std::placeholders::_3);
            xrGroup_->setXRDataHandle(0,cb);
         */
            //开始语音通信
            if(la_)
            {
                la_->setScene(scene_,xrGroup_);
                la_->startCaptureForOpus();
            }
            
           /*
            if(2==clientObjectID_)
            {
                BroadCastingVideo(true);
            }
            */
             
            
            OnXRCommand(pt);
        }
        
    }catch (ptree_error & e) {
        //spd::error("json error where:{},{},{} ", __FILE__,__LINE__,__FUNCTION__);
    }catch (...) {
        //spd::error("unknown exception where:{},{},{} ", __FILE__,__LINE__,__FUNCTION__);
    }
    

    
    //调用虚函数让下层处理
}

void Sample::CheckSouderChange()
{
    std::set<int> currSounder;
    const PODVector<SoundSource*>&  vt=GetSubsystem<Audio>()->GetSoundSources();
    auto it=vt.Begin();
    while(it!=vt.End())
    {
        auto* souder=(*it);
        if(souder->GetAttenuation()>0.0f)
        {
            size_t nodeID=souder->GetNode()->GetID();
            if(nodeID<0xFF)
            {
                if(nodeID==clientObjectID_)
                {
                    if(bMyEcho_)
                        currSounder.insert(nodeID);//时候接受自己的音频回放
                }else
                    currSounder.insert(nodeID);
            }
                
        }
            
        ++it;
    }
    
    PODVector<int> subscribeNode=la_->GetChangeSounder(currSounder);
    
    
    if(subscribeNode.Size()>0)
    {
        std::string sounders;
        for(size_t i=0;i<subscribeNode.Size();++i)
        {
            if(!sounders.empty())
                sounders+=",";
            
            sounders+=std::to_string(subscribeNode[i]);
            
            debug_info_=sounders;
        }
        
        
        char sCMD[1024];
        int len=::sprintf(sCMD, "{\"cmd\":\"subsSounder\",\"where\":\"91_MetaBreeze\",\"sounders\":[%s]}",sounders.c_str());
        std::string sCommand(sCMD,len);
        
        if(xrGroup_)
            xrGroup_->cmd(sCommand);
    }
}

//大多数情况需要线登录
void Sample::LoginP4SP(std::string cmdLogin)
{
    if(facepower_)
        facepower_->cmd(cmdLogin);
}
//1 视频的model 名称
//2 材质通道 TV 4
void Sample::AVReady(std::string const& avServer
                     ,std::string const& verify,std::string const& nodeName,int matIndex)
{
    
    if(!nodeName.empty())
    {
        tv_name_=nodeName;
        //设置视频
        lv_=std::make_shared<LiveVideo>(context_);
        //准备视频回放
        facepower_->setVideoCallBack(std::bind(&LiveVideo::OnVideoFrame
                                                   , lv_
                                                   , std::placeholders::_1
                                                   , std::placeholders::_2
                                                   , std::placeholders::_3 ));
        
        auto on_metarial = [this,nodeName,matIndex](SharedPtr<Material> outputMaterial)
        {
            Node* tvNode= scene_->GetChild(tv_name_.c_str(), true);
            if(tvNode)
            {
                StaticModel* sm = tvNode->GetComponent<StaticModel>();
                //sm->SetMaterial(0, outputMaterial);
                sm->SetMaterial(matIndex, outputMaterial);
                
                //测试其他模型贴材质的效果
                Node* boxNode = this->scene_->GetChild("box1", true);
                if(boxNode)
                {
                    StaticModel* smBox=boxNode->GetComponent<StaticModel>();
                    smBox->SetMaterial(0,outputMaterial);
                }
            }
        };
        lv_->SetMaterailReady(on_metarial);
    }

   
    //注册命令回调
    facepower_->registerCommand(std::bind(&Sample::RecordXRCommand, this, std::placeholders::_1));
    
    
    //设置音频
    la_=std::make_shared<live_audio>(broadcasting_audio_);
    /*
    auto baseliveaudio=std::dynamic_pointer_cast<ILiveAudio>(la_);
    if(facepower_)
        facepower_->setLiveAudio(baseliveaudio);
     */
    //登陆语音视频服务器
    xrGroup_=facepower_->createXRGroup(avServer,verify);
    //注册命令回调
    xrGroup_->registerCommand(std::bind(&Sample::RecordXRCommand, this, std::placeholders::_1));
    
    
}
