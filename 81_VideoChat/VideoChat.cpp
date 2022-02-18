//
// Copyright (c) 2008-2019 the Urho3D project.
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

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>

#include <Urho3D/Audio/Audio.h>
#include <Urho3D/Audio/AudioEvents.h>
#include <Urho3D/Audio/Sound.h>
#include <Urho3D/Audio/SoundSource.h>

#include "VideoChat.h"

#include <Urho3D/DebugNew.h>
#include <memory>
#include <functional>


#include "url_encoder.h"
//boost
#include <boost/format.hpp>
//fro xml and json
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
using namespace boost::property_tree;

#include <fstream>

#include <iostream>

#include "VUIChat1V1.h"
#include "VUIMeeting.h"

////for test
//void callback(void *userdata, Uint8 *stream, int len) {
//    // len=samples*16/8*channels
//
//    VideoChat* pThis=(VideoChat*)userdata;
//    if(pThis)
//        pThis->OnAudioFrame(stream, len);
//
//    //soundStream_->AddData(stream, len);
//}
//using callback_type =std::function<void(void*,Uint8*,int)>;

URHO3D_DEFINE_APPLICATION_MAIN(VideoChat)

VideoChat::VideoChat(Context* context) :
    Sample(context)
    ,portrait_(false)
    ,b2DVideo_(true)
{
    VideoComponent::RegisterObject(context);
    
    //显示视频步骤
    liveV_=std::make_shared<LiveVideo>(context);
    video_box_=nullptr;
    
    
    exit_=false;
    
    if(nullptr==facepower_)
        facepower_=getFacePower();
    
    VUILayout::SetFacePower(facepower_);
}
void VideoChat::Setup()
{
    // Modify engine startup parameters
    Sample::Setup();
    engineParameters_[EP_SOUND] = true;
    //engineParameters_[EP_ORIENTATIONS] = "Portrait";
    //同时 android 中也要设置 成竖屏 AndroidManifest.xml或者代码中设置
    
    //取到启动参数
    std::string parameter;
    Vector<String> vParameter=GetArguments();
    for(auto c:vParameter)
    {
        parameter+=c.CString();
        parameter+=" ";
    }
    bootJson_=URL_TOOLS::UrlDecode(URL_TOOLS::base64_decode(parameter));
    if(bootJson_.empty())
        return ;

    try
    {
        ptree pt;
        std::stringstream ss(bootJson_);
        read_json(ss, pt);
        const std::string screen=pt.get("screen","landscape");
        if(screen=="portrait"){
            portrait_=true;
            engineParameters_[EP_ORIENTATIONS] = "Portrait";
            //同时 android 中也要设置 成竖屏 AndroidManifest.xml或者代码中设置
        }
        
        #ifdef _WIN32
           //define something for Windows (32-bit and 64-bit, this part is common)
           #ifdef _WIN64
              //define something for Windows (64-bit only)
           #else
              //define something for Windows (32-bit only)
           #endif
        #elif __APPLE__
            #include "TargetConditionals.h"
            #if TARGET_IPHONE_SIMULATOR
                 // iOS Simulator
            #elif TARGET_OS_IPHONE
                // iOS device
            #elif TARGET_OS_MAC
                // Other kinds of Mac OS
                portrait_=false;
            #else
            #   error "Unknown Apple platform"
            #endif
        #elif __ANDROID__
            // android

        #elif __linux__
            // linux
        #elif __unix__ // all unices not caught above
            // Unix
        #elif defined(_POSIX_VERSION)
            // POSIX
        #else
        #   error "Unknown compiler"

        #endif
    
        
    }catch (ptree_error & e) {
    }catch (...) {
    }
}
//
////控制语音关闭打开
////SDL_PauseAudioDevice(captureDev_, 0);//关闭语音
////SDL_PauseAudioDevice(captureDev_, 1);//打开语音
//void VideoChat::ControlAudioCapture(void(*pf)(void*,Uint8*,int) ,void* custom
//                                    ,int freq,int channels,int samples)
//{
//    if((*pf))
//    {
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
//        want.samples    = samples;          // 1024;
//        want.callback   = (*pf);
//        //want.callback   =callback;
//        want.userdata   = custom;
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
//// for play back
//void VideoChat::OnAudioFrame(uint8_t* pcm, int len)
//{
////    if(ns_==nullptr)
////        ns_=std::make_shared<webrtc_3a1v>(16000,1);
//
//    if(len>0)
//    {
//        /*
//             static std::fstream        pcmDump_;
//             if(!pcmDump_.is_open())
//                 pcmDump_.open("/Users/leeco/Music/dump_pcm.dat", std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
//             pcmDump_.write((char*)pcm,len);
//        */
//            
//            
//            //保证语音的实时性 语音的滋滋声还不清楚
//            //Return length of buffered (unplayed) sound data in seconds.
//            float buffLen=soundStream_->GetBufferLength();
//        //    if(buffLen>=2)
//        //        soundStream_->Clear();
//            soundStream_->AddData(pcm, len);
//    }
//}

void VideoChat::Start()
{
    //正常启动，这里不会执行到
    if(bootJson_.empty())
    {
        bool  creator=true;
        if(creator)
        {
            //mac 使用本地调试 需要手动登录tracker服务器 192.168.20.133
        bootJson_="{\"cmd\":\"login\",\"wait\":\"true\",\"uid\":10001,\"db\":\"http://db.ournet.club:2021/\",\"tracker\":\"p4sp://2020@192.168.0.100:9700/mj\",\"token\":\"Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1bmlxdWVfbmFtZSI6IjEwMDAxIiwibmJmIjoxNjIwOTYzMjk3LCJleHAiOjE2MjM1NTUyOTcsImlhdCI6MTYyMDk2MzI5N30.2k4ojgLzByzoHm9zfwem3mJ3xeMbgUKmG-4ZGZhvt50\",\"profile\":{\"uid\":\"10001\",\"avatar\":\"1.jpg\",\"name\":\"彭洪\",\"sex\":1,\"birthday\":\"2000-01-24\"}}";

            if(facepower_)
                facepower_->cmd(bootJson_);//登录

            //模拟用户命令
            bootJson_="{\"cmd\":\"talk\",\"type\":1,\"to\":{\"uid\":10002}}";

//            bootJson_="{\"cmd\":\"enter\",\"where\":\"teaHouse\",\"clubID\":\"system_ka5star\",\"p\":{\"name\":\"糖糖\",\"sex\":0,\"birthday\":\"2000-01-01\"},\"url\":\"p4sp://2020@192.168.0.100:9700/club_message\"}";

            //bootJson_="{\"cmd\":\"meeting\",\"sid\":2020}";//启动视频会议模式  创建
        }else{
        bootJson_="{\"cmd\":\"login\",\"wait\":\"true\",\"uid\":10002,\"db\":\"http://db.ournet.club:2021/\",\"tracker\":\"p4sp://2020@192.168.0.100:9700/mj\",\"token\":\"Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1bmlxdWVfbmFtZSI6IjEwMDAyIiwibmJmIjoxNjE1NzgzNTI2LCJleHAiOjE2MTgzNzU1MjYsImlhdCI6MTYxNTc4MzUyNn0.ka7NRmokTLPpE6wivR0g3-ab-qOu2BcsIT8WO_bPMLI\",\"profile\":{\"uid\":\"10002\",\"avatar\":\"1.jpg\",\"name\":\"彭洪\",\"sex\":1,\"birthday\":\"2000-01-24\"}}";

            if(facepower_)
                facepower_->cmd(bootJson_);//登录
            //bootJson_="{\"cmd\":\"meeting\",\"sid\":2020,\"cid\":10001}";//启动视频会议模式  参加
        }
        //对唱
        //bootJson_="{\"cmd\":\"dueto\",\"type\":1,\"to\":{\"uid\":10002}}";
    }


    // Execute base class startup
    Sample::Start();
    SetLogoVisible(false);
    
//    // Set starting position of the cursor at the rendering window center
//    auto* graphics = GetSubsystem<Graphics>();
//    graphics->SetOrientations("Portrait");
    //Log* log(GetSubsystem<Log>());
    //call_Info
    // Create the scene content
    CreateScene();
    // Create the UI content
    CreateInstructions();
    // Setup the viewport for displaying the scene
    SetupViewport();
    // Hook up to the frame update events
    SubscribeToEvents();
    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_RELATIVE);
    
//    //测试webrtc 音频 处理
//    ControlAudioCapture(callback,this,freq_,channels_,1024);
//    return ;
    
    try
    {
        //提取信息
        ptree pt;
        std::stringstream ss(bootJson_);
        read_json(ss, pt);
        const std::string cmd=pt.get("cmd","");
        if("talk"==cmd){//1v1 聊天请求方
            spLayout_=std::make_shared<VUIChat1V1>(GetContext(),true);
        }else if("-ask"==cmd){//1v1 聊天应答方
            spLayout_=std::make_shared<VUIChat1V1>(GetContext(),false);
            bootJson_="";
        }else if("meeting"==cmd)
        {
            int64_t creator_id = pt.get("cid",int64_t(0));
            spLayout_=std::make_shared<VUIMetting>(GetContext(),creator_id!=0);
        }else
        {
            spLayout_=std::make_shared<VUIChat1V1>(GetContext(),false);
        }
        //准备就绪后转发命令
        
        //创建交互界面
        if(spLayout_)
            spLayout_->LoadXmlUI(scene_,pt);

    }catch (ptree_error & e) {
    }catch (...) {
    }
    
    
    // Create music sound source
    musicSource_= scene_->CreateComponent<SoundSource>();
    musicSource_->SetSoundType(SOUND_MUSIC);
//    ResourceCache* cache = GetSubsystem<ResourceCache>();
//    musicSample_ = cache->GetResource<Sound>("Sounds/81/you_dont_go.ogg");
//    musicSample_->SetLooped(true);
    
    //准备就绪后发起请求将ui交互命令转发给facepower
    if(facepower_){
        facepower_->cmd("{\"cmd\":\"appStart\",\"appID\":81}");
        facepower_->cmd(bootJson_);
    }
}

void VideoChat::Stop(){
    if(facepower_){
//        facepower_->setAudioCapture(nullptr);
//        facepower_->setAudioCallBack(nullptr);
        facepower_->setLiveAudio(nullptr);
        facepower_->setVideoCallBack(nullptr);

        
        facepower_->cmd("{\"cmd\":\"appExit\"}");
    }
}

void VideoChat::CreateScene()
{
    scene_ = new Scene(context_);
    scene_->CreateComponent<Octree>();
        
    if(b2DVideo_)
        Load2DVideoDemo();
    else
        Load3DVideoDemo();
    
    AVReady();
    
    return ;
}

void VideoChat::AVReady()
{
    //新接口
    la_=std::make_shared<live_audio>(context_);
    auto baselivevideo=std::dynamic_pointer_cast<ILiveAudio>(la_);
    if(facepower_)
        facepower_->setLiveAudio(baselivevideo);
    
    
//    //语音回放
//    // Sound source needs a node so that it is considered enabled
//    node_ = new Node(context_);
//    auto* source = node_->CreateComponent<SoundSource>();
//    soundStream_ = new BufferedSoundStream();
//
//    // Set format: 44100 Hz, sixteen bit, mono
//    soundStream_->SetFormat(freq_, true, (channels_==2));
//    // Start playback. We don't have data in the stream yet, but the SoundSource will wait until there is data,
//    // as the stream is by default in the "don't stop at end" mode
//    source->Play(soundStream_);
//
//    //录音
//    //ControlAudioCapture(callback);
//    std::function<void(void(*)(void*,Uint8*,int) ,void* custom,int,int,int)> fn=
//    std::bind(&VideoChat::ControlAudioCapture
//              , this
//              , std::placeholders::_1
//              , std::placeholders::_2
//              , std::placeholders::_3
//              , std::placeholders::_4
//              , std::placeholders::_5);
//    facepower_->setAudioCapture(fn);
//    //    const char* cmdStr="{\"cmd\":\"testAudio\"}";
//    //    facepower_->cmd(cmdStr);
    
    

    //注册命令回调
    facepower_->registerCommand(std::bind(&VideoChat::OnCommand, this, std::placeholders::_1));
    //准备视频回放
    //ResourceCache* cache = GetSubsystem<ResourceCache>();
    //outputMaterial=cache->GetResource<Material>("Materials/TVmaterialGPUYUV.xml");
    facepower_->setVideoCallBack(std::bind(&LiveVideo::OnVideoFrame
                                               , liveV_
                                               , std::placeholders::_1
                                               , std::placeholders::_2
                                               , std::placeholders::_3 ));
    
//    //准备音频回放
//    facepower_->setAudioCallBack(std::bind(&VideoChat::OnAudioFrame
//                                           , this
//                                           , std::placeholders::_1
//                                           , std::placeholders::_2 ));
    
    
    
    auto on_metarial = [this](SharedPtr<Material> outputMaterial)
    {
        if(b2DVideo_)
        {
            if(this->video_box_)
            {
                this->video_box_->SetMaterial(outputMaterial);
                Texture*  pTexture=outputMaterial->GetTexture(TU_DIFFUSE);
                this->video_box_->SetTexture(pTexture);
                this->video_box_->SetFullImageRect();
                //this->video_box_->SetImageRect(IntRect(0, 0, 640, 360));
            }
            if(this->sprite_)
            {
                auto pic=this->sprite_;
                //sprite_->SetMaterial(outputMaterial);
                Texture*  pTexture=outputMaterial->GetTexture(TU_DIFFUSE);
                int w= pTexture->GetWidth();
                int h= pTexture->GetHeight();
                this->sprite_->SetTexture(pTexture);
                this->sprite_->SetFullImageRect();
                
                
                //获得屏幕尺寸
                int sw=this->GetSubsystem<Graphics>()->GetWidth();
                int sh=this->GetSubsystem<Graphics>()->GetHeight();
                if(this->portrait_)
                {  //如果不想图像发生变形，那么就拉伸(等比缩放适应屏幕)
                    pic->SetSize(w,h);
                    pic->SetRotation(270.0f);
                    //pic->SetPosition(0,w);
                    float scal=std::max(sw/w,sh/h);
                    pic->SetScale(scal);
                    
                    //pic->SetPosition(0,w*scal);
                    pic->SetAlignment(HA_CENTER, VA_CENTER);
                }

            }
        }else{
            Node* tvNode= this->scene_->GetChild("TV", true);
            if(tvNode)
            {
                StaticModel* sm = tvNode->GetComponent<StaticModel>();
                sm->SetMaterial(0, outputMaterial);
                

                //测试3d音频
                
                /*
                //需要一个listenr
                static SoundListener* listener;
                                listener=cameraNode_->CreateComponent<SoundListener>();
                this->GetSubsystem<Audio>()->SetListener(listener);
                
                //https://discourse.urho3d.io/t/no-sound-in-project/4499/2
                //https://discourse.urho3d.io/t/urho3d-soundsource3d-sound-does-not-start/451
                
                static SoundSource3D  *sound_source;
                sound_source = tvNode->CreateComponent<SoundSource3D>();
                
                sound_source->SetEnabled(true);
                sound_source->SetAngleAttenuation(360.0f,360.0f);
                sound_source->SetAttenuation(10);
                sound_source->SetNearDistance(5);
                sound_source->SetFarDistance(20);
                sound_source->SetGain(0.75);
                sound_source->SetPlayPosition(0);
                
                Sound* sound = this->GetSubsystem<ResourceCache>()->GetResource<Sound>("Sounds/81/Dione.ogg");
                sound->SetLooped(true);
                
                sound_source->Play(sound);
                 */
                

                
                
                if(0)
                {//通过组件技术 组织代码
                    tvc_ = tvNode->CreateComponent<VideoComponent>();
                    //tvc_->OpenFileName("bbb_theora_486kbit.ogv");
                    //tvc->OpenFileName("trailer_400p.ogv"); //error!
                    Material* mat=sm->GetMaterial(0);
                    tvc_->SetOutputModel(sm);
                }
                
                //测试其他模型贴材质的效果
                Node* boxNode = this->scene_->GetChild("box1", true);
                if(boxNode)
                {
                    StaticModel* smBox=boxNode->GetComponent<StaticModel>();
                    smBox->SetMaterial(0,outputMaterial);
                }
            }
            
        }


    };
    
    liveV_->SetMaterailReady(on_metarial);
}

void VideoChat::CreateInstructions()
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* ui = GetSubsystem<UI>();
    
    // Create a Cursor UI element because we want to be able to hide and show it at will. When hidden, the mouse cursor will
    // control the camera, and when visible, it will point the raycast target
    auto* style = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");
    SharedPtr<Cursor> cursor(new Cursor(context_));
    cursor->SetStyleAuto(style);
    ui->SetCursor(cursor);
    
    //GetSubsystem<Input>()->SetTouchEmulation(true);//模拟touch
    
    
    
    // Set starting position of the cursor at the rendering window center
    auto* graphics = GetSubsystem<Graphics>();
    cursor->SetPosition(graphics->GetWidth() / 2, graphics->GetHeight() / 2);
    

//    // Construct new Text object, set string to display and font to use
//    auto* instructionText = ui->GetRoot()->CreateChild<Text>();
//    instructionText->SetText("Use WASD keys and mouse/touch to move");
//    instructionText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
//
//    // Position the text relative to the screen center
//    instructionText->SetHorizontalAlignment(HA_CENTER);
//    instructionText->SetVerticalAlignment(VA_CENTER);
//    instructionText->SetPosition(0, ui->GetRoot()->GetHeight() / 4);
}

void VideoChat::SetupViewport()
{
    auto* renderer = GetSubsystem<Renderer>();

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen. We need to define the scene and the camera
    // at minimum. Additionally we could configure the viewport screen size and the rendering path (eg. forward / deferred) to
    // use, but now we just use full screen and default render path configured in the engine command line options
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
    renderer->SetViewport(0, viewport);
}

void VideoChat::MoveCamera(float timeStep)
{
    // Do not move if the UI has a focused element (the console)
    if (GetSubsystem<UI>()->GetFocusElement())
        return;
    
    // Right mouse button controls mouse cursor visibility: hide when pressed
    auto* ui = GetSubsystem<UI>();
    auto* input = GetSubsystem<Input>();
    ui->GetCursor()->SetVisible(!input->GetMouseButtonDown(MOUSEB_RIGHT));
    
    // Do not move if the UI has a focused element (the console)
    if (ui->GetFocusElement())
        return;
    
    // Movement speed as world units per second
    const float MOVE_SPEED = 5.0f;  //20
    // Mouse sensitivity as degrees per pixel
    const float MOUSE_SENSITIVITY = 0.1f;
    
    // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
    // Only move the camera when the cursor is hidden
    if (!ui->GetCursor()->IsVisible())
    {
        IntVector2 mouseMove = input->GetMouseMove();
        yaw_ += MOUSE_SENSITIVITY * mouseMove.x_;
        pitch_ += MOUSE_SENSITIVITY * mouseMove.y_;
        pitch_ = Clamp(pitch_, -90.0f, 90.0f);
        
        // Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
        cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));
    }
    
    // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
    if (input->GetKeyDown(KEY_W))
        cameraNode_->Translate(Vector3::FORWARD * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_S))
        cameraNode_->Translate(Vector3::BACK * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_A))
        cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_D))
        cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);
    
//    // Toggle debug geometry with space
//    if (input->GetKeyPress(KEY_SPACE))
//        drawDebug_ = !drawDebug_;
//
//    // Paint decal with the left mousebutton; cursor must be visible
//    if (ui->GetCursor()->IsVisible() && input->GetMouseButtonPress(MOUSEB_LEFT))
//        PaintDecal();
    
}

void VideoChat::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(VideoChat, HandleUpdate));
    
    SubscribeToEvent(E_KEYUP, URHO3D_HANDLER(VideoChat, HandleKeyUpExit));
}

void VideoChat::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Move the camera, scale movement with time step
    MoveCamera(timeStep);
    
    //DownloadingImage();
    

    

    //ui 选择了退出
    if(spLayout_&&spLayout_->Exit())
        engine_->Exit();
    
    if(exit_)
        engine_->Exit();
    
    //
    //ShowWaiters();
}

//Button* VideoChat::CreateButton(int x, int y, int xSize, int ySize, const String& text)
//{
//    //UIElement* root = GetSubsystem<UI>()->GetRoot();
//    ResourceCache* cache = GetSubsystem<ResourceCache>();
//    Font* font = cache->GetResource<Font>("Fonts/simfang.ttf");
//
//    // Create the button and center the text onto it
//    Button* button = video_box_->CreateChild<Button>();
//    button->SetStyleAuto();
//    button->SetPosition(x, y);
//    button->SetSize(xSize, ySize);
//
//    Text* buttonText = button->CreateChild<Text>();
//    buttonText->SetAlignment(HA_CENTER, VA_CENTER);
//    buttonText->SetFont(font, 36);//12
//    buttonText->SetText(text);
//    buttonText->SetColor(Color(1.0,0.0,0.0,1.0));
//
//    return button;
//}
void VideoChat::Load2DVideoDemo()
{

    // Create camera node
    cameraNode_ = scene_->CreateChild("Camera");
    // Set camera's position
    cameraNode_->SetPosition(Vector3(0.0f, 0.0f, -10.0f));
    
    auto* camera = cameraNode_->CreateComponent<Camera>();
    camera->SetOrthographic(true);
    
    auto* graphics = GetSubsystem<Graphics>();
    camera->SetOrthoSize((float)graphics->GetHeight() * PIXEL_SIZE);
    camera->SetZoom(1.2f * Min((float)graphics->GetWidth() / 1280.0f, (float)graphics->GetHeight() / 800.0f)); // Set zoom according to user's resolution to ensure full visibility (initial zoom (1.2) is set for full visibility at 1280x800 resolution)
    
    //graphics->SetOrientations();
    
    //创建2D视频
    UIElement* root = GetSubsystem<UI>()->GetRoot();
    video_box_=root->CreateChild<BorderImage>();
    video_box_->SetPosition(0,0);
    video_box_->SetSize(2,2);
    //video_box_->SetVisible(false);
    
    //add sprite
    SharedPtr<Sprite>  sprite(new Sprite(GetContext()));
    sprite_=sprite;
        
    if(portrait_)
    {
        //        sprite_->SetSize(graphics->GetHeight(),graphics->GetWidth());
        //        sprite_->SetRotation(90.0f);
        //        sprite_->SetPosition(graphics->GetWidth(),0);
        
        sprite_->SetSize(graphics->GetHeight(),graphics->GetWidth());
        sprite_->SetRotation(270.0f);
        sprite_->SetPosition(0,graphics->GetHeight());
        //如果不想图像发生变形，那么就拉伸(等比缩放适应屏幕)
    }else
    {
        sprite_->SetSize(graphics->GetWidth(),graphics->GetHeight());
        sprite_->SetPosition(0,0);
    }

    root->AddChild(sprite_);

}
void VideoChat::Load3DVideoDemo()
{
     ResourceCache* cache = GetSubsystem<ResourceCache>();
    
    //从配置文件加载场景
    // auto fileScene=cache->GetResource<XMLFile>("Data/prj/videochat/Scene.xml");
    File sceneFile(context_,
                   GetSubsystem<FileSystem>()->GetProgramDir() +"Data/Scenes/Scene.xml",
                   FILE_READ);
    scene_->LoadXML(sceneFile);
    cameraNode_ = scene_->GetChild("Camera", true);
    playerNode_ = scene_->GetChild("Player", true);

}

//底层发来的命令
void VideoChat::OnCommand(std::string const& cmdStr)
{
    try
    {
        ptree pt;
        std::stringstream ss(cmdStr);
        read_json(ss, pt);
        const std::string cmd=pt.get("cmd","");
        
        if("exit"==cmd)
        {
            if(facepower_){
                facepower_->setAudioCallBack(nullptr);
                facepower_->setVideoCallBack(nullptr);
                exit_=true;
            }
        }else if("watchNetwork"==cmd)
        {
//            if(layout_){
//                std::string msg=pt.get("message","");
//                auto pTxt=layout_->GetChildStaticCast<Text>("call_info",true);
//                if(pTxt)
//                    pTxt->SetText(msg.c_str());
//            }
        }else if("playAAC"==cmd)
        {
            if(musicSource_){//测试ktv背景音乐
                ResourceCache* cache = GetSubsystem<ResourceCache>();
                //Sound* music= cache->GetResource<Sound>("Sounds/81/you_dont_go.ogg");
                Sound* music= cache->GetResource<Sound>("/Users/leeco/breeze/client/urho3d-master/bin/Data/Sounds/81/you_dont_go.ogg");
                
                //music->SetLooped(true);
                musicSource_->Play(music);
                //musicSource_->SetPanning(1.0);
            }
            
        }else
        {
             spLayout_->OnCommand(pt);
        }
        //准备就绪后转发命令
    }catch (ptree_error & e) {
        //std::cout<<"utf8-bug need fix:"<<e.what()<<std::endl;
        int a=0;a++;
    }catch (...) {
        int b=0;b++;
    }
    
}

//拦截手机的按键事件 返回键or退出键
void VideoChat::HandleKeyUpExit(StringHash /*eventType*/, VariantMap& eventData)
{
    using namespace KeyUp;
    
    int key = eventData[P_KEY].GetInt();
    
    // Close console (if open) or exit when ESC is pressed
    if (key == KEY_AC_BACK||key==SDLK_RETURN||key==SDLK_RETURN2)
    {
        if(facepower_)
            facepower_->cmd("{\"cmd\":\"appExit\",\"where\":\"VideoChat::HandleKeyUpExit\"}");
        
        //engine_->Exit();
    }
}
