//
// Copyright (c) 2008-2021 the Urho3D project.
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
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Input/Controls.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>

#include "Mover.h"
#include "Character.h"
#include "CharacterDemo.h"
#include "Touch.h"


#include <memory>
#include <functional>
//#include "url_encoder.h"
//boost
#include <boost/format.hpp>
//fro xml and json
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
using namespace boost::property_tree;

#include "url_encoder.h"
#include <Urho3D/DebugNew.h>


#include <spdlog/spdlog.h>
namespace spd = spdlog;
#include "spdlog/sinks/basic_file_sink.h"

const std::string getCurrentSystemTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm* ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900,(int)ptm->tm_mon + 1,(int)ptm->tm_mday,
            (int)ptm->tm_hour,(int)ptm->tm_min,(int)ptm->tm_sec);
    return std::string(date);
}


URHO3D_DEFINE_APPLICATION_MAIN(CharacterDemo)

CharacterDemo::CharacterDemo(Context* context)
    :Sample(context)
    ,exit_(false)
    ,portrait_(false)
    ,firstPerson_(false)
    ,myIndex_(-1)
{
    // Register factory and attributes for the Character component so it can be created via CreateComponent, and loaded / saved
    Character::RegisterObject(context);
    
    // Register an object factory for our custom Mover component so that we can create them to scene nodes
    context->RegisterFactory<Mover>();
    

    facepower_=getFacePower();
    
    
    
    
    //日志
    std::string logPath;
    std::string initMessage;
    
#ifdef _WIN32
   //define something for Windows (32-bit and 64-bit, this part is common)
            logPath="c:/xrchat.log";
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
            //mac 本地调试会启动多个 实例 因此必须用时间去分开
            logPath="/Users/leeco/Downloads/xrchat";
            logPath+=getCurrentSystemTime();
            logPath+=".txt";
    #else
    #   error "Unknown Apple platform"
    #endif
#elif __ANDROID__
    // android
            logPath="/mnt/sdcard/xrchat.txt";
#elif __linux__
    // linux
#elif __unix__ // all unices not caught above
    // Unix
#elif defined(_POSIX_VERSION)
    // POSIX
#else
#   error "Unknown compiler"
    logPath="~/xrchat.log";
#endif
    
    
    // Set the default logger to file logger
    auto file_logger = spdlog::basic_logger_mt("basic_logger", logPath);
    spdlog::set_default_logger(file_logger);
    
    spd::set_level(spd::level::trace);
    spd::flush_on(spdlog::level::level_enum::trace);
    spd::info("------xrchat------2020.7.9");
    if(!initMessage.empty())
        spd::error(initMessage);
}

//CharacterDemo::~CharacterDemo() = default;
CharacterDemo::~CharacterDemo()
{
    la_.reset();
    
    if(facepower_){
        facepower_->setLiveAudio(nullptr);
        facepower_->setVideoCallBack(nullptr);
        facepower_->cmd("{\"cmd\":\"appExit\"}");
    }
}

void CharacterDemo::Setup()
{
    // Modify engine startup parameters
    Sample::Setup();
    engineParameters_[EP_SOUND] = true;
    
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

void CharacterDemo::AVReady()
{
    la_=std::make_shared<live_audio>(scene_);
    auto baselivevideo=std::dynamic_pointer_cast<ILiveAudio>(la_);
    if(facepower_)
        facepower_->setLiveAudio(baselivevideo);
    
    lv_=std::make_shared<LiveVideo>(context_);

    //准备视频回放
    //ResourceCache* cache = GetSubsystem<ResourceCache>();
    //outputMaterial=cache->GetResource<Material>("Materials/TVmaterialGPUYUV.xml");
    facepower_->setVideoCallBack(std::bind(&LiveVideo::OnVideoFrame
                                               , lv_
                                               , std::placeholders::_1
                                               , std::placeholders::_2
                                               , std::placeholders::_3 ));
    
    auto on_metarial = [this](SharedPtr<Material> outputMaterial)
    {
        Node* tvNode= this->scene_->GetChild("TV", true);
        if(tvNode)
        {
            StaticModel* sm = tvNode->GetComponent<StaticModel>();
            //sm->SetMaterial(0, outputMaterial);
            sm->SetMaterial(4, outputMaterial);
            
            //测试其他模型贴材质的效果
            Node* boxNode = this->scene_->GetChild("box1", true);
            if(boxNode)
            {
                StaticModel* smBox=boxNode->GetComponent<StaticModel>();
                smBox->SetMaterial(0,outputMaterial);
            }
            

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
        }
    };
    lv_->SetMaterailReady(on_metarial);
    
    
    
    //注册命令回调
    facepower_->registerCommand(std::bind(&CharacterDemo::OnCommand, this, std::placeholders::_1));
    
//    facepower_->registerMover(std::bind(&CharacterDemo::OnMover, this, std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
    
}


//#include <Urho3D/IO/File.h>
//#include <Urho3D/IO/FileSystem.h>

void CharacterDemo::Start()
{
    FileSystem * filesystem = GetSubsystem<FileSystem>();
    
    String dir=filesystem->GetUserDocumentsDir();
    #ifdef __ANDROID__
        dir=String(facepower_->getStoragePath().c_str());
    #endif
    
    dir+="xrchat/";
    if(!filesystem->DirExists(dir))
        filesystem->CreateDir(dir);
    //增加 系统允许的存储目录为资源搜索目录
    auto* cache = GetSubsystem<ResourceCache>();
    cache->AddResourceDir(dir.CString());
    
    dir+="avatars/";
    if(!filesystem->DirExists(dir))
        filesystem->CreateDir(dir);
    
    
    
    

    //正常启动，这里不会执行到
    if(bootJson_.empty())
    {
        bool  creator=true;
        if(creator)
        {
       
                        //mac 使用本地调试 需要手动登录tracker服务器 192.168.20.133
            #ifdef _WIN32
               //define something for Windows (32-bit and 64-bit, this part is common)
                storageHome="d:/xrchat/";
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
                        bootJson_="{\"cmd\":\"login\",\"wait\":\"true\",\"uid\":10002,\"db\":\"http://db.ournet.club:2021/\",\"tracker\":\"p4sp://2020@192.168.0.100:9700/mj\",\"token\":\"Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1bmlxdWVfbmFtZSI6IjEwMDAyIiwibmJmIjoxNjM1NzkwNDQ4LCJleHAiOjE2MzgzODI0NDgsImlhdCI6MTYzNTc5MDQ0OH0.tyu6rnU4yc7hJvwLnIJEV1bprUbHX-OfQTUiMa2LJPc\",\"profile\":{\"uid\":\"10001\",\"avatar\":\"1.jpg\",\"name\":\"彭洪\",\"sex\":1,\"birthday\":\"2000-01-24\"}}";
                #else
                #   error "Unknown Apple platform"
                #endif
            #elif __ANDROID__
                        bootJson_="{\"cmd\":\"login\",\"wait\":\"true\",\"uid\":10001,\"db\":\"http://db.ournet.club:2021/\",\"tracker\":\"p4sp://2020@192.168.0.100:9700/mj\",\"token\":\"Beawrer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1bmlxdWVfbmFtZSI6IjEwMDAxIiwibmJmIjoxNjM0MjcwNTE4LCJleHAiOjE2MzY4NjI1MTgsImlhdCI6MTYzNDI3MDUxOH0.5sRh9BlISj7sLqV-oLMlay_TNz45NR9QXXQOU9gQ_xg\",\"profile\":{\"uid\":\"10001\",\"avatar\":\"1.jpg\",\"name\":\"彭洪\",\"sex\":1,\"birthday\":\"2000-01-24\"}}";
            #elif __linux__

            #elif __unix__ // all unices not caught above

            #elif defined(_POSIX_VERSION)
                // POSIX
            #else
            #   error "Unknown compiler"

            #endif
            

            if(facepower_)
                facepower_->cmd(bootJson_);//登录

            //模拟用户命令 聊天
            //bootJson_="{\"cmd\":\"talk\",\"type\":1,\"to\":{\"uid\":10002}}";


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
    if (touchEnabled_)
        touch_ = new Touch(context_, TOUCH_SENSITIVITY);

    //add
    AVReady();
    

    // Create static scene content
    CreateScene();
    


    // Create the controllable character
    //CreateCharacter("Kachujin");// //Mutant
    
    //创建视频
    CreateTV();
    //创建机器人
    //CreateRobot();
    //加载其他用户
    //LoadPlayer(100);
    //CreateCharacter2();

    // Create the UI content
    CreateInstructions();

    // Subscribe to necessary events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_RELATIVE);
    
    
    
    //准备就绪后发起请求将ui交互命令转发给facepower
    if(facepower_){
        facepower_->cmd("{\"cmd\":\"appStart\",\"appID\":82,\"serverID\":2020,\"roomID\":10001}");
        facepower_->cmd(bootJson_);
    }
}

void CharacterDemo::CreateRobot()
{
    auto* cache = GetSubsystem<ResourceCache>();
    // Create animated models
    const unsigned NUM_MODELS = 30;
    const float MODEL_MOVE_SPEED = 2.0f;
    const float MODEL_ROTATE_SPEED = 100.0f;
    //const BoundingBox bounds(Vector3(-20.0f, 0.0f, -20.0f), Vector3(20.0f, 0.0f, 20.0f));
    const BoundingBox bounds(Vector3(-100.0f, 0.0f, -100.0f), Vector3(100.0f, 0.0f, 100.0f));

    for (unsigned i = 0; i < NUM_MODELS; ++i)
    {
        Node* modelNode = scene_->CreateChild("Jill");
        modelNode->SetPosition(Vector3(Random(40.0f) - 20.0f, 0.0f, Random(40.0f) - 20.0f));
        modelNode->SetRotation(Quaternion(0.0f, Random(360.0f), 0.0f));

        auto* modelObject = modelNode->CreateComponent<AnimatedModel>();
        modelObject->SetModel(cache->GetResource<Model>("Models/Kachujin/Kachujin.mdl"));
        modelObject->SetMaterial(cache->GetResource<Material>("Models/Kachujin/Materials/Kachujin.xml"));
        modelObject->SetCastShadows(true);

        // Create an AnimationState for a walk animation. Its time position will need to be manually updated to advance the
        // animation, The alternative would be to use an AnimationController component which updates the animation automatically,
        // but we need to update the model's position manually in any case
        auto* walkAnimation = cache->GetResource<Animation>("Models/Kachujin/Kachujin_Walk.ani");

        AnimationState* state = modelObject->AddAnimationState(walkAnimation);
        // The state would fail to create (return null) if the animation was not found
        if (state)
        {
            // Enable full blending weight and looping
            state->SetWeight(1.0f);
            state->SetLooped(true);
            state->SetTime(Random(walkAnimation->GetLength()));
        }

        // Create our custom Mover component that will move & animate the model during each frame's update
        auto* mover = modelNode->CreateComponent<Mover>();
        mover->SetParameters(MODEL_MOVE_SPEED, MODEL_ROTATE_SPEED, bounds);
        
        
        
        //
        if(0==i)
            modelNode->Scale(5.0f);
        else if(1==i)
            modelNode->Scale(0.2f);
        
        if(9>=i){
            // add sound
            //1 First, we create our 3D sound source.
            // Create the sound source node
            Node *sound_node = modelNode->CreateChild("Sound source node");
            //Node *sound_node = scene_->CreateChild("Sound source node");
            // Set the node position (sound will come from that position also)
            //sound_node->SetPosition(Vector3(10,-20,45));
            // Create the sound source
            SoundSource3D *snd_source_3d = sound_node->CreateComponent<SoundSource3D>();
            // Set near distance (if listener is within that distance, no attenuation is computed)
            snd_source_3d->SetNearDistance(5);
            // Set far distance (distance after which no sound is heard)
            snd_source_3d->SetFarDistance(20);
            
            //2 We also need to load a sound.

                // Load a sound. That sound will be the one that will be broadcast by the 3D sound source.
               char buf[512];
               ::sprintf(buf, "Sounds/82/%d.ogg", i);
                Sound *my_sound = GetSubsystem<ResourceCache>()->GetResource<Sound>(buf);
                // That sound shall be played continuously.
                my_sound->SetLooped(true);
                //Do not forget to play the sound, or you will hear nothing !
                // Play sound
                snd_source_3d->Play(my_sound);
        }
    }
}

void CharacterDemo::CreateScene()
{
    auto* cache = GetSubsystem<ResourceCache>();

    scene_ = new Scene(context_);

    // Create scene subsystem components
    scene_->CreateComponent<Octree>();
    scene_->CreateComponent<PhysicsWorld>();

    // Create camera and define viewport. We will be doing load / save, so it's convenient to create the camera outside the scene,
    // so that it won't be destroyed and recreated, and we don't have to redefine the viewport on load
    cameraNode_ = new Node(context_);
    auto* camera = cameraNode_->CreateComponent<Camera>();
    camera->SetFarClip(300.0f);
    GetSubsystem<Renderer>()->SetViewport(0, new Viewport(context_, scene_, camera));

    // Create static scene content. First create a zone for ambient lighting and fog control
    Node* zoneNode = scene_->CreateChild("Zone");
    auto* zone = zoneNode->CreateComponent<Zone>();
    zone->SetAmbientColor(Color(0.15f, 0.15f, 0.15f));
    zone->SetFogColor(Color(0.5f, 0.5f, 0.7f));
    zone->SetFogStart(100.0f);
    zone->SetFogEnd(300.0f);
    zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));

    // Create a directional light with cascaded shadow mapping
    Node* lightNode = scene_->CreateChild("DirectionalLight");
    lightNode->SetDirection(Vector3(0.3f, -0.5f, 0.425f));
    auto* light = lightNode->CreateComponent<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetCastShadows(true);
    light->SetShadowBias(BiasParameters(0.00025f, 0.5f));
    light->SetShadowCascade(CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));
    light->SetSpecularIntensity(0.5f);

    // Create the floor object
    Node* floorNode = scene_->CreateChild("Floor");
    floorNode->SetPosition(Vector3(0.0f, -0.5f, 0.0f));
    floorNode->SetScale(Vector3(200.0f, 1.0f, 200.0f));
    auto* object = floorNode->CreateComponent<StaticModel>();
    object->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    object->SetMaterial(cache->GetResource<Material>("Materials/Stone.xml"));

    auto* body = floorNode->CreateComponent<RigidBody>();
    // Use collision layer bit 2 to mark world scenery. This is what we will raycast against to prevent camera from going
    // inside geometry
    body->SetCollisionLayer(2);
    auto* shape = floorNode->CreateComponent<CollisionShape>();
    shape->SetBox(Vector3::ONE);

    // Create mushrooms of varying sizes
    const unsigned NUM_MUSHROOMS = 60;
    for (unsigned i = 0; i < NUM_MUSHROOMS; ++i)
    {
        Node* objectNode = scene_->CreateChild("Mushroom");
        objectNode->SetPosition(Vector3(Random(180.0f) - 90.0f, 0.0f, Random(180.0f) - 90.0f));
        objectNode->SetRotation(Quaternion(0.0f, Random(360.0f), 0.0f));
        objectNode->SetScale(2.0f + Random(5.0f));
        auto* object = objectNode->CreateComponent<StaticModel>();
        object->SetModel(cache->GetResource<Model>("Models/Mushroom.mdl"));
        object->SetMaterial(cache->GetResource<Material>("Materials/Mushroom.xml"));
        object->SetCastShadows(true);

        auto* body = objectNode->CreateComponent<RigidBody>();
        body->SetCollisionLayer(2);
        auto* shape = objectNode->CreateComponent<CollisionShape>();
        shape->SetTriangleMesh(object->GetModel(), 0);
    }

    // Create movable boxes. Let them fall from the sky at first
    const unsigned NUM_BOXES = 100;
    for (unsigned i = 0; i < NUM_BOXES; ++i)
    {
        float scale = Random(2.0f) + 0.5f;

        Node* objectNode = scene_->CreateChild("Box");
        objectNode->SetPosition(Vector3(Random(180.0f) - 90.0f, Random(10.0f) + 10.0f, Random(180.0f) - 90.0f));
        objectNode->SetRotation(Quaternion(Random(360.0f), Random(360.0f), Random(360.0f)));
        objectNode->SetScale(scale);
        auto* object = objectNode->CreateComponent<StaticModel>();
        object->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
        object->SetMaterial(cache->GetResource<Material>("Materials/Stone.xml"));
        object->SetCastShadows(true);

        auto* body = objectNode->CreateComponent<RigidBody>();
        body->SetCollisionLayer(2);
        // Bigger boxes will be heavier and harder to move
        body->SetMass(scale * 2.0f);
        auto* shape = objectNode->CreateComponent<CollisionShape>();
        shape->SetBox(Vector3::ONE);
    }
}

//add by copyleft for vrchat
//左手坐标系. X, Y和Z轴的正方向指向向右,向上和向前. 旋转以顺时针方向为正.
//创建视频墙
void CharacterDemo::CreateTV()
{
    auto* cache = GetSubsystem<ResourceCache>();

    float scale = 3.0f;

    Node* objectNode = scene_->CreateChild("TV");
    //objectNode->SetPosition(Vector3(Random(180.0f) - 90.0f, Random(10.0f) + 10.0f, Random(180.0f) - 90.0f));
    objectNode->SetPosition(Vector3(0.0f, 0.0f,-1.0f));
    //objectNode->SetRotation(Quaternion(0.0F, 0.0f, -90.0f));
    
    //objectNode->SetRotation(Quaternion(Random(360.0f), Random(360.0f), Random(360.0f)));
    //objectNode->SetScale(scale);

    auto* object = objectNode->CreateComponent<StaticModel>();
    object->SetModel(cache->GetResource<Model>("Models/yoyoTV.mdl"));
    //object->SetModel(cache->GetResource<Model>("/Users/leeco/3D/model/Models/Plane.mdl"));
    //object->SetMaterial(cache->GetResource<Material>("Materials/Stone.xml"));
    object->SetCastShadows(true);

    auto* body = objectNode->CreateComponent<RigidBody>();
    body->SetCollisionLayer(2);
    // Bigger boxes will be heavier and harder to move
    body->SetMass(scale * 2.0f);

    auto* shape = objectNode->CreateComponent<CollisionShape>();
    shape->SetBox(Vector3::ONE);
}
//#include <IO/FileSystem.h>
void CharacterDemo::CreateCharacter(std::string const& avatar)
{
    auto* cache = GetSubsystem<ResourceCache>();


    //加载模型
    std::string modelPath;
    if("def"==avatar||""==avatar)
        modelPath="Models/def/";
    else
        modelPath="avatars/"+avatar+"/";
    
    
    Node* objectNode = scene_->CreateChild("player_0");
    objectNode->SetPosition(Vector3(0.0f, 1.0f, 0.0f));
    
    //Then, we can create the listener.
    // Create the listener node
    Node *listener_node = objectNode->CreateChild("Listener node");
    // Set listener node position
    //listener_node->SetPosition(Vector3(0, 0, 0));
    // Create the listener itself
    SoundListener *listener = listener_node->CreateComponent<SoundListener>();
    // Set the listener for that audio subsystem
    GetSubsystem<Audio>()->SetListener(listener);
    
    

    // spin node
    Node* adjustNode = objectNode->CreateChild("AdjNode");
    adjustNode->SetRotation(Quaternion(180, Vector3(0,1,0) ) );

    
    
    // Create the rendering component + animation controller
    auto* object = adjustNode->CreateComponent<AnimatedModel>();

    object->SetCastShadows(true);
    adjustNode->CreateComponent<AnimationController>();

    // Set the head bone for manual control
    auto h=object->GetSkeleton().GetBone("Mutant:Head");
    if(h)
        h->animated_ = false;

    // Create rigidbody, and set non-zero mass so that the body becomes dynamic
    auto* body = objectNode->CreateComponent<RigidBody>();
    body->SetCollisionLayer(1);
    body->SetMass(1.0f);

    // Set zero angular factor so that physics doesn't turn the character on its own.
    // Instead we will control the character yaw manually
    body->SetAngularFactor(Vector3::ZERO);

    // Set the rigidbody to signal collision also when in rest, so that we get ground collisions properly
    body->SetCollisionEventMode(COLLISION_ALWAYS);

    // Set a capsule shape for collision
    auto* shape = objectNode->CreateComponent<CollisionShape>();
    shape->SetCapsule(0.7f, 1.8f, Vector3(0.0f, 0.9f, 0.0f));

    // Create the character logic component, which takes care of steering the rigidbody
    // Remember it so that we can set the controls. Use a WeakPtr because the scene hierarchy already owns it
    // and keeps it alive as long as it's not removed from the hierarchy
   
    character_ = objectNode->CreateComponent<Character>();
    
    //character_->SetAvatar(modelPath);
    SetModel(0,avatar);
}

void CharacterDemo::CreateCharacter2()
{
    auto* cache = GetSubsystem<ResourceCache>();

    Node* objectNode = scene_->CreateChild("Jack2");
    objectNode->SetPosition(Vector3(0.0f, 2.0f, 0.0f));
    
    
//    //Then, we can create the listener.
//    // Create the listener node
//    Node *listener_node = objectNode->CreateChild("Listener node");
//    // Set listener node position
//    //listener_node->SetPosition(Vector3(0, 0, 0));
//    // Create the listener itself
//    SoundListener *listener = listener_node->CreateComponent<SoundListener>();
//    // Set the listener for that audio subsystem
//    GetSubsystem<Audio>()->SetListener(listener);
    
    

    // spin node
    Node* adjustNode = objectNode->CreateChild("AdjNode");
    adjustNode->SetRotation(Quaternion(180, Vector3(0,1,0) ) );

    // Create the rendering component + animation controller
    auto* object = adjustNode->CreateComponent<AnimatedModel>();
    
//    object->SetModel(cache->GetResource<Model>("Models/Mutant/Mutant.mdl"));
//    object->SetMaterial(cache->GetResource<Material>("Models/Mutant/Materials/mutant_M.xml"));
    object->SetModel(cache->GetResource<Model>("Platforms/Models/BetaLowpoly/Beta.mdl"));
    object->SetMaterial(0, cache->GetResource<Material>("Platforms/Materials/BetaBody_MAT.xml"));
    object->SetMaterial(1, cache->GetResource<Material>("Platforms/Materials/BetaBody_MAT.xml"));
    object->SetMaterial(2, cache->GetResource<Material>("Platforms/Materials/BetaJoints_MAT.xml"));
    
    
    object->SetCastShadows(true);
    adjustNode->CreateComponent<AnimationController>();

    // Set the head bone for manual control
    //object->GetSkeleton().GetBone("Mutant:Head")->animated_ = false;

    // Create rigidbody, and set non-zero mass so that the body becomes dynamic
    auto* body = objectNode->CreateComponent<RigidBody>();
    //body->SetCollisionLayer(ColLayer_Character);
    //body->SetCollisionMask(ColMask_Character);
    body->SetCollisionLayer(1);
    body->SetMass(1.0f);

    // Set zero angular factor so that physics doesn't turn the character on its own.
    // Instead we will control the character yaw manually
    body->SetAngularFactor(Vector3::ZERO);

    // Set the rigidbody to signal collision also when in rest, so that we get ground collisions properly
    body->SetCollisionEventMode(COLLISION_ALWAYS);

    // Set a capsule shape for collision
    auto* shape = objectNode->CreateComponent<CollisionShape>();
    shape->SetCapsule(0.7f, 1.8f, Vector3(0.0f, 0.9f, 0.0f));

    // Create the character logic component, which takes care of steering the rigidbody
    // Remember it so that we can set the controls. Use a WeakPtr because the scene hierarchy already owns it
    // and keeps it alive as long as it's not removed from the hierarchy
   // character_ = objectNode->CreateComponent<Character>();
}

void CharacterDemo::CreateInstructions()
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* ui = GetSubsystem<UI>();

    // Construct new Text object, set string to display and font to use
    auto* instructionText = ui->GetRoot()->CreateChild<Text>();
    instructionText->SetText(
        "Use WASD keys and mouse/touch to move\n"
        "Space to jump, F to toggle 1st/3rd person\n"
        "F5 to save scene, F7 to load"
    );
    instructionText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
    // The text has multiple rows. Center them in relation to each other
    instructionText->SetTextAlignment(HA_CENTER);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, ui->GetRoot()->GetHeight() / 4);
}

void CharacterDemo::SubscribeToEvents()
{
    // Subscribe to Update event for setting the character controls before physics simulation
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(CharacterDemo, HandleUpdate));

    // Subscribe to PostUpdate event for updating the camera position after physics simulation
    SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(CharacterDemo, HandlePostUpdate));

    // Unsubscribe the SceneUpdate event from base class as the camera node is being controlled in HandlePostUpdate() in this sample
    UnsubscribeFromEvent(E_SCENEUPDATE);
}

void CharacterDemo::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    ParseCommand();
    
    
    using namespace Update;

    auto* input = GetSubsystem<Input>();

    if (character_)
    {
//        // Clear previous controls
        character_->controls_.Set(CTRL_FORWARD | CTRL_BACK | CTRL_LEFT | CTRL_RIGHT | CTRL_JUMP, false);

        // Update controls using touch utility class
        if (touch_)
            touch_->UpdateTouches(character_->controls_);

        // Update controls using keys
        auto* ui = GetSubsystem<UI>();
        if (!ui->GetFocusElement())
        {
            if (!touch_ || !touch_->useGyroscope_)
            {
                character_->controls_.Set(CTRL_FORWARD, input->GetKeyDown(KEY_W));
                character_->controls_.Set(CTRL_BACK, input->GetKeyDown(KEY_S));
                character_->controls_.Set(CTRL_LEFT, input->GetKeyDown(KEY_A));
                character_->controls_.Set(CTRL_RIGHT, input->GetKeyDown(KEY_D));
            }
            character_->controls_.Set(CTRL_JUMP, input->GetKeyDown(KEY_SPACE));
            
            // Add character yaw & pitch from the mouse motion or touch input
            if (touchEnabled_)
            {
                for (unsigned i = 0; i < input->GetNumTouches(); ++i)
                {
                    TouchState* state = input->GetTouch(i);
                    if (!state->touchedElement_)    // Touch on empty space
                    {
                        auto* camera = cameraNode_->GetComponent<Camera>();
                        if (!camera)
                            return;

                        auto* graphics = GetSubsystem<Graphics>();
                        character_->controls_.yaw_ += TOUCH_SENSITIVITY * camera->GetFov() / graphics->GetHeight() * state->delta_.x_;
                        character_->controls_.pitch_ += TOUCH_SENSITIVITY * camera->GetFov() / graphics->GetHeight() * state->delta_.y_;
                    }
                }
            }
            else
            {
                character_->controls_.yaw_ += (float)input->GetMouseMoveX() * YAW_SENSITIVITY;
                character_->controls_.pitch_ += (float)input->GetMouseMoveY() * YAW_SENSITIVITY;
            }
            // Limit pitch
            character_->controls_.pitch_ = Clamp(character_->controls_.pitch_, -80.0f, 80.0f);
            // Set rotation already here so that it's updated every rendering frame instead of every physics frame
            character_->GetNode()->SetRotation(Quaternion(character_->controls_.yaw_, Vector3::UP));

            // Switch between 1st and 3rd person
            if (input->GetKeyPress(KEY_F)){
                firstPerson_ = !firstPerson_;
                
              Vector3 pos= character_->GetNode()->GetPosition();
              Quaternion rot= character_->GetNode()->GetRotation();
            }
               

            // Turn on/off gyroscope on mobile platform
            if (touch_ && input->GetKeyPress(KEY_G))
                touch_->useGyroscope_ = !touch_->useGyroscope_;

            
            //把character_ 动作序列化出去
            uint8_t mvByte=0;
            if(input->GetKeyDown(KEY_W))
                mvByte|=0x1;
            if(input->GetKeyDown(KEY_S))
                mvByte|=0x2;
            if(input->GetKeyDown(KEY_A))
                mvByte|=0x4;
            if(input->GetKeyDown(KEY_D))
                mvByte|=0x8;
            if(input->GetKeyDown(KEY_SPACE))
                mvByte|=0x10;
                
            SendAction(mvByte);
            
            // Check for loading / saving the scene
            if (input->GetKeyPress(KEY_F5))
            {
                File saveFile(context_, GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Scenes/CharacterDemo.xml", FILE_WRITE);
                scene_->SaveXML(saveFile);
            }
            if (input->GetKeyPress(KEY_F7))
            {
                File loadFile(context_, GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Scenes/CharacterDemo.xml", FILE_READ);
                scene_->LoadXML(loadFile);
                // After loading we have to reacquire the weak pointer to the Character component, as it has been recreated
                // Simply find the character's scene node by name as there's only one of them
                Node* characterNode = scene_->GetChild("Jack", true);
                if (characterNode)
                    character_ = characterNode->GetComponent<Character>();
            }
        }
        

    }
}

void CharacterDemo::SendAction(uint8_t dirByte)
{
    static Quaternion   rotDir;
    static  Vector3     mvDir(127.f,0.f,579.f);
    static stObjIDL     curStatus;
    if(!facepower_)
        return ;
    
    //旋转方向是否变化
    curStatus.rot=character_->GetNode()->GetRotation();
    curStatus.pos=character_->GetNode()->GetPosition();
    if(curStatus.ac!=dirByte||curStatus.rot!=rotDir)
    {
        curStatus.ac=dirByte;
        //状态发生了变化
        int sendBytes=0;
        uint8_t& mvByte=curStatus.ac;
        
        if(curStatus.rot!=rotDir)
        {
            rotDir=curStatus.rot;
            
            mvByte|=0x20;
            
            sendBytes=sizeof(stObjIDL)-sizeof(Vector3);
        }else if(0==mvByte)
            sendBytes=sizeof(stObjIDL);
        else
            sendBytes=1;

        //OnMover(100,(uint8_t*)&curStatus,sendBytes);//for test
        //facepower_->sendCustomData(3,(uint8_t*)&curStatus,sendBytes);
    }
}

void CharacterDemo::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
    if (!character_)
        return;

    Node* characterNode = character_->GetNode();

    // Get camera lookat dir from character yaw + pitch
    const Quaternion& rot = characterNode->GetRotation();
    Quaternion dir = rot * Quaternion(character_->controls_.pitch_, Vector3::RIGHT);

    // Turn head to camera pitch, but limit to avoid unnatural animation
    Node* headNode = characterNode->GetChild("Mutant:Head", true);
    if(headNode)
    {
        float limitPitch = Clamp(character_->controls_.pitch_, -45.0f, 45.0f);
        Quaternion headDir = rot * Quaternion(limitPitch, Vector3(1.0f, 0.0f, 0.0f));
        // This could be expanded to look at an arbitrary target, now just look at a point in front
        Vector3 headWorldTarget = headNode->GetWorldPosition() + headDir * Vector3(0.0f, 0.0f, -1.0f);
        headNode->LookAt(headWorldTarget, Vector3(0.0f, 1.0f, 0.0f));
    }


    if (firstPerson_)
    {
        cameraNode_->SetPosition(headNode->GetWorldPosition() + rot * Vector3(0.0f, 0.15f, 0.2f));
        cameraNode_->SetRotation(dir);
    }
    else
    {
        // Third person camera: position behind the character
        Vector3 aimPoint = characterNode->GetPosition() + rot * Vector3(0.0f, 1.7f, 0.0f);

        // Collide camera ray with static physics objects (layer bitmask 2) to ensure we see the character properly
        Vector3 rayDir = dir * Vector3::BACK;
        float rayDistance = touch_ ? touch_->cameraDistance_ : CAMERA_INITIAL_DIST;
        PhysicsRaycastResult result;
        scene_->GetComponent<PhysicsWorld>()->RaycastSingle(result, Ray(aimPoint, rayDir), rayDistance, 2);
        if (result.body_)
            rayDistance = Min(rayDistance, result.distance_);
        rayDistance = Clamp(rayDistance, CAMERA_MIN_DIST, CAMERA_MAX_DIST);

        cameraNode_->SetPosition(aimPoint + rayDir * rayDistance);
        cameraNode_->SetRotation(dir);
    }
}
void CharacterDemo::LoadPlayer(int64_t index)
{
   // std::string profile="{\"uid\":10001,\"name\":\"微风吹过\",\"model\":\"Models/Mutant/Mutant.mdl\",\"materials\":[\"Models/Mutant/Materials/mutant_M.xml\"]}";
    
    // std::string profile="{\"uid\":10001,\"name\":\"微风吹过\",\"model\":\"Models/Kachujin/Kachujin.mdl\",\"materials\":[\"Models/Kachujin/Materials/Kachujin.xml\"],\"idl\":\"\",\"walk\":\"Models/Kachujin/Kachujin_Walk.ani\",\"jump\":\"\"}";
    
    //机器人
    auto* character=CreateModel(index,"BetaLowpoly");
}

Character* CharacterDemo::CreateModel(int index,std::string const& avatar)
{
    auto* cache = GetSubsystem<ResourceCache>();

    char modelName[512];
    ::sprintf(modelName, "player_%d", index);
    Node* objectNode = scene_->CreateChild(modelName,REPLICATED,index);
    
    objectNode->SetPosition(Vector3(0.0f, 0.0f, 0.0f));
    
    
    if(index==myIndex_)
    {
        //Then, we can create the listener.
        // Create the listener node
        Node *listener_node = objectNode->CreateChild("Listener node");
        // Set listener node position
        //listener_node->SetPosition(Vector3(0, 0, 0));
        // Create the listener itself
        SoundListener *listener = listener_node->CreateComponent<SoundListener>();
        // Set the listener for that audio subsystem
        GetSubsystem<Audio>()->SetListener(listener);
        
    }

    
    

    // spin node
    Node* adjustNode = objectNode->CreateChild("AdjNode");
    adjustNode->SetRotation(Quaternion(180, Vector3(0,1,0) ) );

    
    // Create the rendering component + animation controller
    auto* object = adjustNode->CreateComponent<AnimatedModel>();

    object->SetCastShadows(true);
    adjustNode->CreateComponent<AnimationController>();

    // Set the head bone for manual control
    auto h=object->GetSkeleton().GetBone("Mutant:Head");
    if(h)
        h->animated_ = false;

    // Create rigidbody, and set non-zero mass so that the body becomes dynamic
    auto* body = objectNode->CreateComponent<RigidBody>();
    body->SetCollisionLayer(1);
    body->SetMass(1.0f);

    // Set zero angular factor so that physics doesn't turn the character on its own.
    // Instead we will control the character yaw manually
    body->SetAngularFactor(Vector3::ZERO);

    // Set the rigidbody to signal collision also when in rest, so that we get ground collisions properly
    body->SetCollisionEventMode(COLLISION_ALWAYS);

    // Set a capsule shape for collision
    auto* shape = objectNode->CreateComponent<CollisionShape>();
    shape->SetCapsule(0.7f, 1.8f, Vector3(0.0f, 0.9f, 0.0f));

    
    // Create the character logic component, which takes care of steering the rigidbody
    // Remember it so that we can set the controls. Use a WeakPtr because the scene hierarchy already owns it
    // and keeps it alive as long as it's not removed from the hierarchy
    auto* c= objectNode->CreateComponent<Character>();
    
    //设置模型
    SetModel(index, avatar);
    return c;
}

void CharacterDemo::SetModel(int indexID,std::string const& avatar)
{
    auto* cache = GetSubsystem<ResourceCache>();
    //get node
    Node* objectNode=scene_->GetChild(indexID);
    if(objectNode)
    {
        auto* character=objectNode->GetComponent<Character>();
        
        Node* adjustNode = objectNode->GetChild("AdjNode");
        auto* object = adjustNode->GetComponent<AnimatedModel>();
        
        std::string modelPath;
        if("def"==avatar||""==avatar)
            modelPath="Models/def/";
        else
            modelPath="avatars/"+avatar+"/";
        
        std::string modelFile=modelPath+"a.mdl";
        //auto* file=scene_->GetSubsystem<FileSystem>();
        
        Model* m=cache->GetResource<Model>(modelFile.c_str());
        if(nullptr!=m)
        {
            character->SetAvatar(modelPath);
            
            object->SetModel(m);
            const size_t slots=object->GetBatches().Size();
            if(slots<1)
            {
                std::string mats=modelPath+"Materials/0.xml";
                object->SetMaterial(cache->GetResource<Material>(mats.c_str()));
            }else{
                for(size_t i=0;i<slots;++i)
                {
                   // Materials
                    char matlist[128];
                    ::sprintf(matlist, "Materials/%d.xml", (int)i);
                    std::string mats=modelPath+matlist;
                    
                    object->SetMaterial(i,cache->GetResource<Material>(mats.c_str()));
                }
            }
        }else{//头像不存在 通知网络层 从服务器下载模型
            
            FileSystem * filesystem = GetSubsystem<FileSystem>();
            String dir=filesystem->GetUserDocumentsDir();
            #ifdef __ANDROID__
                dir=String(facepower_->getStoragePath().c_str());
            #endif
            
            dir+="xrchat/avatars/";


            char sCommand[1024];
            int len=::sprintf(sCommand, "{\"cmd\":\"avatarDownload\",\"index\":%lld,\"avatar\":\"%s\",\"path\":\"%s\"}", indexID,avatar.c_str(),dir.CString());
                        
            //facepower_->sendCustomData(2, (uint8_t*)sCommand, len);
            
            //先使用缺省的头像 等下载完成后再更改回来
            SetModel(indexID,"");
        }
    }
}


void CharacterDemo::PlayerLeave(int index,int64_t uid)
{
    auto* node=scene_->GetChild(index);
    if(node)
    {
        node->Remove();
    }
}
//对象移动数据
void CharacterDemo::OnMover(int64_t index,uint8_t* pData,int len)
{
        auto* node=scene_->GetChild(index);
        if(nullptr==node)
            return ;

        stObjIDL*  idlStatus=(stObjIDL*)pData;
        
        auto* character=node->GetComponent<Character>();
    
        if(nullptr==character)
            return ;
    

        character_->controls_.Set(CTRL_FORWARD | CTRL_BACK | CTRL_LEFT | CTRL_RIGHT | CTRL_JUMP, false);
        
        if(idlStatus->ac!=0)
        {
            uint8_t& mvByte=idlStatus->ac;
            
            if(mvByte&0x1)
                character_->controls_.Set(CTRL_FORWARD,true);
            if(mvByte&0x2)
                character_->controls_.Set(CTRL_BACK,true);
            if(mvByte&0x4)
                character_->controls_.Set(CTRL_LEFT,true);
            if(mvByte&0x8)
                character_->controls_.Set(CTRL_RIGHT,true);
            
            if(mvByte&0x10)
                character_->controls_.Set(CTRL_JUMP,true);
            //        if(mvByte&0x20&&node_!=nullptr)
            //            node_->SetRotation(rot);
            
        }else{//stop
            
        }
}

//底层发来的命令
void CharacterDemo::OnCommand(std::string const& cmdStr)
{
    std::lock_guard<std::mutex> lock(cmd_mutex_);
    cmd_que_.push(cmdStr);
}

void CharacterDemo::ParseCommand()
{
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

            
        }else if("playerInit"==cmd)
        {
            myIndex_=pt.get("selfIndex",-1);
            
            ptree vPlayers = pt.get_child("players");
            for (boost::property_tree::ptree::iterator it = vPlayers.begin(); it != vPlayers.end(); ++it)
            {
                //std::string key     = it->first;//key ID
                ptree player        = it->second;
            
                int index   = player.get("index",0);
                int64_t uid = player.get("uid",int64_t(0));
                
                std::string avatar  =player.get("avatar","");
                std::string name    =player.get("name","");
                
                //avatar="Kachujin";
                
                auto* character=CreateModel(index,avatar);
                if(index==myIndex_)
                    character_=character;// it is me
//                else
//                    LoadPlayer(index,uid,character);//other player
            }
            
            
        }else if("playerEnter"==cmd)
        {
            int index=pt.get("player.index",0);
            int64_t uid=pt.get("player.uid",int64_t(0));
            std::string avatar=pt.get("player.avatar","");
            std::string name=pt.get("player.name","");
            
            auto* character=CreateModel(index,avatar);
            //LoadPlayer(index,uid,character);//other player
            
            //通知该节点，我的最新位置
            if(facepower_&&character_)
            {
                Vector3    pos=character_->GetNode()->GetPosition();
                Quaternion rot=character_->GetNode()->GetRotation();

                char buf[2048];
                int size=::sprintf(buf, "{\"cmd\":\"playerPos\",\"to\":%d,\"pos\":{\"x\":%f,\"y\":%f,\"z\":%f},\"rot\":{\"w\":%f,\"x\":%f,\"y\":%f,\"z\":%f}}"
                          ,index,pos.x_,pos.y_,pos.z_,rot.w_,rot.x_,rot.y_,rot.z_);
                
                //facepower_->sendCustomData(2,(uint8_t*)buf,size);
            }
        }else if("playerLeave"==cmd)
        {
            int index=pt.get("player.index",0);
            int64_t uid=pt.get("player.uid",int64_t(0));

        }else if("playerPos"==cmd)
        {
            Vector3    pos;
            pos.x_=pt.get("pos.x",0.0f);
            pos.y_=pt.get("pos.y",0.0f);
            pos.z_=pt.get("pos.z",0.0f);
            
            Quaternion rot;
            rot.w_=pt.get("rot.w",0.0f);
            rot.x_=pt.get("rot.x",0.0f);
            rot.y_=pt.get("rot.y",0.0f);
            rot.z_=pt.get("rot.z",0.0f);
            
            auto* node=character_->GetNode();
            if(node)
            {
                node->SetPosition(pos);
                node->SetRotation(rot);
            }
        }else if("avatarDownloaded"==cmd)
        {
            int index=pt.get("index",0);
            std::string avatar=pt.get("avatar","");
            //重新设置头像
            SetModel(index, avatar);
        }
        else
        {

        }
        //准备就绪后转发命令
    }catch (ptree_error & e) {
        //std::cout<<"utf8-bug need fix:"<<e.what()<<std::endl;
        int a=0;a++;
    }catch (...) {
        int b=0;b++;
    }
    
}


