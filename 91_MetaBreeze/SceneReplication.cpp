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

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Input/Controls.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Network/Connection.h>
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/NetworkEvents.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/LineEdit.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>

#include <Urho3D/Audio/Audio.h>
#include <Urho3D/Audio/AudioEvents.h>
#include <Urho3D/Audio/Sound.h>
#include <Urho3D/Audio/SoundSource.h>

#include "SceneReplication.h"
#include "../90_MetaServer/Character.h"
#include "Touch.h"

//boost
#include <boost/format.hpp>
//fro xml and json
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
using namespace boost::property_tree;

#include "url_encoder.h"
#include "Mover.h"

#include "../90_MetaServer/NetworkMessage.h"
#include "../90_MetaServer/BaseInfo.h"



#include <Urho3D/DebugNew.h>

//// UDP port we will use
//static const unsigned short SERVER_PORT = 2345;
//
//// Identifier for our custom remote event we use to tell the client which object they control
//static const StringHash E_CLIENTOBJECTID("ClientObjectID");
//// Identifier for the node ID parameter in the event data
//static const StringHash P_ID("ID");
//static const StringHash HASH_ID("HASH");


//// Control bits we define
//static const unsigned CTRL_FORWARD = 1;
//static const unsigned CTRL_BACK = 2;
//static const unsigned CTRL_LEFT = 4;
//static const unsigned CTRL_RIGHT = 8;

#include <set>
#include <vector>


URHO3D_DEFINE_APPLICATION_MAIN(SceneReplication)

SceneReplication::SceneReplication(Context* context) :
    Sample(context)
    ,exit_(false)
    ,use_internal_av_(false)
    ,portrait_(false)
    ,firstPerson_(false)
{
    //默认值
    //game_server_    ="192.168.0.100:2345";
    game_server_    ="game.ournet.club:2345";
    room_id_        =1;            //自动创建
    av_server_      ="2020@192.168.0.100:9700";
        
    token_="Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1bmlxdWVfbmFtZSI6IjEwMDAxIiwibmJmIjoxNjQ0OTE4MzUxLCJleHAiOjE2NDc1MTAzNTEsImlhdCI6MTY0NDkxODM1MX0.gFc7ApkQYuv4JvOZKkRlJ5prxMvAseorRYePVi2YJhI";
    
    
    // Register factory and attributes for the Character component so it can be created via CreateComponent, and loaded / saved
    
    //Character::RegisterObject(context);
    BaseInfomation::RegisterObject(context);
    
    // Register an object factory for our custom Mover component so that we can create them to scene nodes
    context->RegisterFactory<Mover>();
}
SceneReplication::~SceneReplication()
{
//    la_.reset();
//
//    if(facepower_){
//        facepower_->setVideoCallBack(nullptr);
//        facepower_->setLiveAudio(nullptr);
//        facepower_->cmd("{\"cmd\":\"appExit\"}");
//    }
}
void SceneReplication::Setup()
{
    // Modify engine startup parameters
    Sample::Setup();
    engineParameters_[EP_SOUND]     = true;
    
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
        
        //游戏状态同步服务器
        room_id_        =pt.get("rid",room_id_);
        game_server_    =pt.get("sync",game_server_.CString()).c_str();
        av_server_      =pt.get("av",av_server_);
        token_          =pt.get("token",token_);
    }catch (ptree_error & e) {
    }catch (...) {
    }
}

void SceneReplication::Start()
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
    
    //建立头像目录
    String avatarDir=dir+"avatars/";
    if(!filesystem->DirExists(avatarDir))
        filesystem->CreateDir(avatarDir);
    
    //缓存资源
    String cacheDir=dir+"cache/";
    GetSubsystem<Network>()->SetPackageCacheDir(cacheDir);

 
//    //正常启动，这里不会执行到
//    if(bootJson_.empty())
//    {       //\"tracker\":\"p4sp://2020@tracker.ournet.club:9700/mj\"
//            //\"tracker\":\"p4sp://2020@192.168.0.100:9700/mj\"
//        bootJson_="{\"cmd\":\"login\",\"wait\":\"true\",\"uid\":10002,\"db\":\"http://db.ournet.club:2021/\",\"tracker\":\"p4sp://p4sp://2020@192.168.0.100:9700/mj\",\"token\":\"Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1bmlxdWVfbmFtZSI6IjEwMDAxIiwibmJmIjoxNjQ0OTE4MzUxLCJleHAiOjE2NDc1MTAzNTEsImlhdCI6MTY0NDkxODM1MX0.gFc7ApkQYuv4JvOZKkRlJ5prxMvAseorRYePVi2YJhI\",\"profile\":{\"uid\":\"10002\",\"avatar\":\"default\",\"name\":\"彭洪\",\"sex\":1,\"birthday\":\"2000-01-24\"}}";
//        //模拟登陆
//        if(facepower_)
//            facepower_->cmd(bootJson_);//登录
//    }

    // Execute base class startup
    Sample::Start();
    if (touchEnabled_)
        touch_ = new Touch(context_, TOUCH_SENSITIVITY);
    
    SetLogoVisible(false);
    
    //add audio video support  by copyleft
    //AVReady();
    
    // Create the scene content
    CreateScene();

    // Create the UI content
    CreateUI();

    // Setup the viewport for displaying the scene
    SetupViewport();

    // Hook up to necessary events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_RELATIVE);
    
    
    
    VariantMap eventData;
    HandleConnect({},eventData);
}

void SceneReplication::CreateScene()
{
    scene_ = new Scene(context_);

    auto* cache = GetSubsystem<ResourceCache>();

    // Create octree and physics world with default settings. Create them as local so that they are not needlessly replicated
    // when a client connects
    scene_->CreateComponent<Octree>(LOCAL);
    scene_->CreateComponent<PhysicsWorld>(LOCAL);

    // All static scene content and the camera are also created as local, so that they are unaffected by scene replication and are
    // not removed from the client upon connection. Create a Zone component first for ambient lighting & fog control.
    Node* zoneNode = scene_->CreateChild("Zone",LOCAL);
    auto* zone = zoneNode->CreateComponent<Zone>();
    zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
    zone->SetAmbientColor(Color(0.1f, 0.1f, 0.1f));
    zone->SetFogStart(100.0f);
    zone->SetFogEnd(300.0f);

    // Create a directional light without shadows
    Node* lightNode = scene_->CreateChild("DirectionalLight", LOCAL);
    lightNode->SetDirection(Vector3(0.5f, -1.0f, 0.5f));
    auto* light = lightNode->CreateComponent<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetColor(Color(0.2f, 0.2f, 0.2f));
    light->SetSpecularIntensity(1.0f);

    // Create a "floor" consisting of several tiles. Make the tiles physical but leave small cracks between them
    for (int y = -20; y <= 20; ++y)
    {
        for (int x = -20; x <= 20; ++x)
        {
            Node* floorNode = scene_->CreateChild("FloorTile", LOCAL);
            floorNode->SetPosition(Vector3(x * 20.2f, -0.5f, y * 20.2f));
            floorNode->SetScale(Vector3(20.0f, 1.0f, 20.0f));
            auto* floorObject = floorNode->CreateComponent<StaticModel>();
            floorObject->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
            floorObject->SetMaterial(cache->GetResource<Material>("Materials/Stone.xml"));

            auto* body = floorNode->CreateComponent<RigidBody>();
            body->SetFriction(1.0f);
            auto* shape = floorNode->CreateComponent<CollisionShape>();
            shape->SetBox(Vector3::ONE);
        }
    }

    // Create the camera. Limit far clip distance to match the fog
    // The camera needs to be created into a local node so that each client can retain its own camera, that is unaffected by
    // network messages. Furthermore, because the client removes all replicated scene nodes when connecting to a server scene,
    // the screen would become blank if the camera node was replicated (as only the locally created camera is assigned to a
    // viewport in SetupViewports() below)
    cameraNode_ = scene_->CreateChild("Camera", LOCAL);
    auto* camera = cameraNode_->CreateComponent<Camera>();
    camera->SetFarClip(300.0f);

    // Set an initial position for the camera scene node above the plane
    cameraNode_->SetPosition(Vector3(0.0f, 5.0f, 0.0f));
        
}

void SceneReplication::CreateUI()
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* ui = GetSubsystem<UI>();
    UIElement* root = ui->GetRoot();
    auto* uiStyle = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");
    // Set style to the UI root so that elements will inherit it
    root->SetDefaultStyle(uiStyle);

    
    
    auto* graphics = GetSubsystem<Graphics>();

    // Create a Cursor UI element because we want to be able to hide and show it at will. When hidden, the mouse cursor will
    // control the camera, and when visible, it can interact with the login UI
    SharedPtr<Cursor> cursor(new Cursor(context_));
    cursor->SetStyleAuto(uiStyle);
    ui->SetCursor(cursor);
    // Set starting position of the cursor at the rendering window center
    cursor->SetPosition(graphics->GetWidth() / 2, graphics->GetHeight() / 2);
    //add by copyleft hide cursor
    ui->GetCursor()->SetVisible(false);

    
    // Construct the instructions text element
    instructionsText_ = ui->GetRoot()->CreateChild<Text>();
//    instructionsText_->SetText(
//        "Use WASD keys to move and RMB to rotate view"
//    );
    
    instructionsText_->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
    // Position the text relative to the screen center
    instructionsText_->SetHorizontalAlignment(HA_CENTER);
    instructionsText_->SetVerticalAlignment(VA_CENTER);
    instructionsText_->SetPosition(0, graphics->GetHeight() / 4);
    // Hide until connected
    instructionsText_->SetVisible(false);

    packetsIn_ = ui->GetRoot()->CreateChild<Text>();
    packetsIn_->SetText("Packets in : 0");
    packetsIn_->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
    packetsIn_->SetHorizontalAlignment(HA_LEFT);
    packetsIn_->SetVerticalAlignment(VA_CENTER);
    packetsIn_->SetPosition(10, -10);

    packetsOut_ = ui->GetRoot()->CreateChild<Text>();
    packetsOut_->SetText("Packets out: 0");
    packetsOut_->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
    packetsOut_->SetHorizontalAlignment(HA_LEFT);
    packetsOut_->SetVerticalAlignment(VA_CENTER);
    packetsOut_->SetPosition(10, 10);
    
    myInfo_ = ui->GetRoot()->CreateChild<Text>();
    myInfo_->SetText("My indexID: 0");
    myInfo_->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
    myInfo_->SetHorizontalAlignment(HA_LEFT);
    myInfo_->SetVerticalAlignment(VA_CENTER);
    myInfo_->SetPosition(10, 30);
    
    
    

//    buttonContainer_ = root->CreateChild<UIElement>();
//    buttonContainer_->SetFixedSize(500, 20);
//    buttonContainer_->SetPosition(20, 20);
//    buttonContainer_->SetLayoutMode(LM_HORIZONTAL);
//
//    textEdit_ = buttonContainer_->CreateChild<LineEdit>();
//    textEdit_->SetStyleAuto();
//
//    connectButton_ = CreateButton("Connect", 90);
//    disconnectButton_ = CreateButton("Disconnect", 100);
//    startServerButton_ = CreateButton("Start Server", 110);

    //UpdateButtons();
}

void SceneReplication::SetupViewport()
{
    auto* renderer = GetSubsystem<Renderer>();

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
    renderer->SetViewport(0, viewport);
}

void SceneReplication::SubscribeToEvents()
{
    // Subscribe to fixed timestep physics updates for setting or applying controls
    SubscribeToEvent(E_PHYSICSPRESTEP, URHO3D_HANDLER(SceneReplication, HandlePhysicsPreStep));

    // Subscribe HandlePostUpdate() method for processing update events. Subscribe to PostUpdate instead
    // of the usual Update so that physics simulation has already proceeded for the frame, and can
    // accurately follow the object with the camera
    SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(SceneReplication, HandlePostUpdate));

    // Subscribe to button actions
    //SubscribeToEvent(connectButton_, E_RELEASED, URHO3D_HANDLER(SceneReplication, HandleConnect));
    //SubscribeToEvent(disconnectButton_, E_RELEASED, URHO3D_HANDLER(SceneReplication, HandleDisconnect));

    // Subscribe to network events
    SubscribeToEvent(E_SERVERCONNECTED, URHO3D_HANDLER(SceneReplication, HandleConnectionStatus));
    SubscribeToEvent(E_SERVERDISCONNECTED, URHO3D_HANDLER(SceneReplication, HandleConnectionStatus));
    SubscribeToEvent(E_CONNECTFAILED, URHO3D_HANDLER(SceneReplication, HandleConnectionStatus));
    SubscribeToEvent(E_NETWORKMESSAGE, URHO3D_HANDLER(SceneReplication, HandleNetworkMessage));

//    SubscribeToEvent(E_CLIENTCONNECTED, URHO3D_HANDLER(SceneReplication, HandleClientConnected));
//    SubscribeToEvent(E_CLIENTDISCONNECTED, URHO3D_HANDLER(SceneReplication, HandleClientDisconnected));
    // This is a custom event, sent from the server to the client. It tells the node ID of the object the client should control
    SubscribeToEvent(E_CLIENTOBJECTID, URHO3D_HANDLER(SceneReplication, HandleClientObjectID));
    // Events sent between client & server (remote events) must be explicitly registered or else they are not allowed to be received
    GetSubsystem<Network>()->RegisterRemoteEvent(E_CLIENTOBJECTID);
}

//Button* SceneReplication::CreateButton(const String& text, int width)
//{
//    auto* cache = GetSubsystem<ResourceCache>();
//    auto* font = cache->GetResource<Font>("Fonts/Anonymous Pro.ttf");
//
//    auto* button = buttonContainer_->CreateChild<Button>();
//    button->SetStyleAuto();
//    button->SetFixedWidth(width);
//
//    auto* buttonText = button->CreateChild<Text>();
//    buttonText->SetFont(font, 12);
//    buttonText->SetAlignment(HA_CENTER, VA_CENTER);
//    buttonText->SetText(text);
//
//    return button;
//}
//
//void SceneReplication::UpdateButtons()
//{
//    return ;
//
//    auto* network = GetSubsystem<Network>();
//    Connection* serverConnection = network->GetServerConnection();
//    bool serverRunning = network->IsServerRunning();
//
//    // Show and hide buttons so that eg. Connect and Disconnect are never shown at the same time
//    connectButton_->SetVisible(!serverConnection && !serverRunning);
//    disconnectButton_->SetVisible(serverConnection || serverRunning);
//    startServerButton_->SetVisible(!serverConnection && !serverRunning);
//    textEdit_->SetVisible(!serverConnection && !serverRunning);
//}


//
//Node* SceneReplication::CreateControllableObject()
//{
//    auto* cache = GetSubsystem<ResourceCache>();
//
//    Node* objectNode = scene_->CreateChild("Jack");
//    objectNode->SetPosition(Vector3(0.0f, 1.0f, 0.0f));
//    
//    //Then, we can create the listener.
//    // Create the listener node
//    Node *listener_node = objectNode->CreateChild("Listener node");
//    // Set listener node position
//    //listener_node->SetPosition(Vector3(0, 0, 0));
//    // Create the listener itself
//    SoundListener *listener = listener_node->CreateComponent<SoundListener>();
//    // Set the listener for that audio subsystem
//    GetSubsystem<Audio>()->SetListener(listener);
//    
//
//    // spin node
//    Node* adjustNode = objectNode->CreateChild("AdjNode");
//    adjustNode->SetRotation( Quaternion(180, Vector3(0,1,0) ) );
//
//    // Create the rendering component + animation controller
//    auto* object = adjustNode->CreateComponent<AnimatedModel>();
//    object->SetModel(cache->GetResource<Model>("Models/Mutant/Mutant.mdl"));
//    object->SetMaterial(cache->GetResource<Material>("Models/Mutant/Materials/mutant_M.xml"));
//    object->SetCastShadows(true);
//    adjustNode->CreateComponent<AnimationController>();
//
//    // Set the head bone for manual control
//    object->GetSkeleton().GetBone("Mutant:Head")->animated_ = false;
//
//    // Create rigidbody, and set non-zero mass so that the body becomes dynamic
//    auto* body = objectNode->CreateComponent<RigidBody>();
//    body->SetCollisionLayer(1);
//    body->SetMass(1.0f);
//
//    // Set zero angular factor so that physics doesn't turn the character on its own.
//    // Instead we will control the character yaw manually
//    body->SetAngularFactor(Vector3::ZERO);
//
//    // Set the rigidbody to signal collision also when in rest, so that we get ground collisions properly
//    body->SetCollisionEventMode(COLLISION_ALWAYS);
//
//    // Set a capsule shape for collision
//    auto* shape = objectNode->CreateComponent<CollisionShape>();
//    shape->SetCapsule(0.7f, 1.8f, Vector3(0.0f, 0.9f, 0.0f));
//
//    // Create the character logic component, which takes care of steering the rigidbody
//    // Remember it so that we can set the controls. Use a WeakPtr because the scene hierarchy already owns it
//    // and keeps it alive as long as it's not removed from the hierarchy
//    objectNode->CreateComponent<Character>(CreateMode::LOCAL);
//
//    return objectNode;
//}



void SceneReplication::MoveCamera()
{
    // Right mouse button controls mouse cursor visibility: hide when pressed
    auto* ui = GetSubsystem<UI>();
    auto* input = GetSubsystem<Input>();
    
    //modify by copyleft
    //ui->GetCursor()->SetVisible(false);
    //ui->GetCursor()->SetVisible(!input->GetMouseButtonDown(MOUSEB_RIGHT));


    // Mouse sensitivity as degrees per pixel
    const float MOUSE_SENSITIVITY = 0.1f;

    // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch and only move the camera
    // when the cursor is hidden
    if (!ui->GetCursor()->IsVisible())
    {
        IntVector2 mouseMove = input->GetMouseMove();
        yaw_ += MOUSE_SENSITIVITY * mouseMove.x_;
        pitch_ += MOUSE_SENSITIVITY * mouseMove.y_;
        pitch_ = Clamp(pitch_, 1.0f, 90.0f);
    }

    // Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
    cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));

    // Only move the camera / show instructions if we have a controllable object
    bool showInstructions = false;
    if (clientObjectID_)
    {
        Node* ballNode = scene_->GetNode(clientObjectID_);
        if (ballNode)
        {
            // Get camera lookat dir from character yaw + pitch
            const Quaternion& rot = ballNode->GetRotation();
            Quaternion dir = rot * Quaternion(controls_.pitch_, Vector3::RIGHT);
            
            // Turn head to camera pitch, but limit to avoid unnatural animation
            Node* headNode = ballNode->GetChild("Mutant:Head", true);
            if(headNode)
            {
                float limitPitch = Clamp(controls_.pitch_, -45.0f, 45.0f);
                Quaternion headDir = rot * Quaternion(limitPitch, Vector3(1.0f, 0.0f, 0.0f));
                // This could be expanded to look at an arbitrary target, now just look at a point in front
                Vector3 headWorldTarget = headNode->GetWorldPosition() + headDir * Vector3(0.0f, 0.0f, -1.0f);
                headNode->LookAt(headWorldTarget, Vector3(0.0f, 1.0f, 0.0f));
            }
            
            if (firstPerson_)
            {
                cameraNode_->SetPosition(headNode->GetWorldPosition() + rot * Vector3(0.0f, 0.15f, 0.2f));
                cameraNode_->SetRotation(dir);
            }else
            {
                // Third person camera: position behind the character
                Vector3 aimPoint = ballNode->GetPosition() + rot * Vector3(0.0f, 1.7f, 0.0f);

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


            
            
//            const float CAMERA_DISTANCE = 5.0f;
//            // Move camera some distance away from the ball
//            cameraNode_->SetPosition(ballNode->GetPosition() + cameraNode_->GetRotation() * Vector3::BACK * CAMERA_DISTANCE);
            
            
            showInstructions = true;
        }
    }

    instructionsText_->SetVisible(showInstructions);
}

void SceneReplication::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
    // We only rotate the camera according to mouse movement since last frame, so do not need the time step
    MoveCamera();
    
    if (packetCounterTimer_.GetMSec(false) > 1000 && GetSubsystem<Network>()->GetServerConnection())
    {
        packetsIn_->SetText("Packets  in: " + String(GetSubsystem<Network>()->GetServerConnection()->GetPacketsInPerSec()));
        packetsOut_->SetText("Packets out: " + String(GetSubsystem<Network>()->GetServerConnection()->GetPacketsOutPerSec()));
        packetCounterTimer_.Reset();
    }
    
    
    if (packetCounterTimer_.GetMSec(false) > 1000 && GetSubsystem<Network>()->GetClientConnections().Size())
    {
        int packetsIn = 0;
        int packetsOut = 0;
        auto connections = GetSubsystem<Network>()->GetClientConnections();
        for (auto it = connections.Begin(); it != connections.End(); ++it ) {
            packetsIn += (*it)->GetPacketsInPerSec();
            packetsOut += (*it)->GetPacketsOutPerSec();
        }
        packetsIn_->SetText("Packets  in: " + String(packetsIn));
        packetsOut_->SetText("Packets out: " + String(packetsOut));
        packetCounterTimer_.Reset();
    }
    
//    //设置显示信息
//    String  info="My indexID: " + String(clientObjectID_);
//    if(la_)
//            info+="  3dsource:"+String(la_->GetSubcribeSounderCount());
//    myInfo_->SetText(info);
    
    //ParseCommand(); 
}



void SceneReplication::HandlePhysicsPreStep(StringHash eventType, VariantMap& eventData)
{
    // This function is different on the client and server. The client collects controls (WASD controls + yaw angle)
    // and sets them to its server connection object, so that they will be sent to the server automatically at a
    // fixed rate, by default 30 FPS. The server will actually apply the controls (authoritative simulation.)
    auto* network = GetSubsystem<Network>();
    Connection* serverConnection = network->GetServerConnection();

    // Client: collect controls
    if (serverConnection)
    {
        auto* ui = GetSubsystem<UI>();
        auto* input = GetSubsystem<Input>();
        

        // Only apply WASD controls if there is no focused UI element
        if (!ui->GetFocusElement())
        {
            // Clear previous controls
            controls_.Set(CTRL_FORWARD | CTRL_BACK | CTRL_LEFT | CTRL_RIGHT | CTRL_JUMP, false);

            // Update controls using touch utility class
            if (touch_)
                touch_->UpdateTouches(controls_);
            
            if (!touch_ || !touch_->useGyroscope_)
            {
                controls_.Set(CTRL_FORWARD, input->GetKeyDown(KEY_W));
                controls_.Set(CTRL_BACK, input->GetKeyDown(KEY_S));
                controls_.Set(CTRL_LEFT, input->GetKeyDown(KEY_A));
                controls_.Set(CTRL_RIGHT, input->GetKeyDown(KEY_D));
            }
            controls_.Set(CTRL_JUMP, input->GetKeyDown(KEY_SPACE));
            
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
                        controls_.yaw_ += TOUCH_SENSITIVITY * camera->GetFov() / graphics->GetHeight() * state->delta_.x_;
                        controls_.pitch_ += TOUCH_SENSITIVITY * camera->GetFov() / graphics->GetHeight() * state->delta_.y_;
                    }
                }

            }else{
                controls_.yaw_   += (float)input->GetMouseMoveX() * YAW_SENSITIVITY;
                controls_.pitch_ += (float)input->GetMouseMoveY() * YAW_SENSITIVITY;
            }
            // Limit pitch
            controls_.pitch_ = Clamp(controls_.pitch_, -80.0f, 80.0f);

            
            serverConnection->SetControls(controls_);
            // In case the server wants to do position-based interest management using the NetworkPriority components, we should also
            // tell it our observer (camera) position. In this sample it is not in use, but eg. the NinjaSnowWar game uses it
            serverConnection->SetPosition(cameraNode_->GetPosition());
            
        }
    }
    
}


void SceneReplication::HandleConnect(StringHash eventType, VariantMap& eventData)
{
    auto* network = GetSubsystem<Network>();
    
    String address      = "localhost";
    unsigned short port = SERVER_PORT;

    unsigned pos=game_server_.Find(':');
    if(pos>0)
    {
        address     = game_server_.Substring(0,pos);
        String sport= game_server_.Substring(pos+1);
        
        port=atoi(sport.CString());
        
        // Connect to server, specify scene to use as a client for replication
        clientObjectID_ = 0; // Reset own object ID from possible previous connection
        
        VariantMap identity;
        identity["RoomID"]   =(unsigned long long)room_id_;
        identity["Token"]    =String(token_.c_str());
        
        /*
         {
           "username": "admin@yourStore.com",
           "customer_id": 1,
           "token": <authentication token>
         }
         */
        network->Connect(address, port, scene_,identity);

        //UpdateButtons();
    }
}

void SceneReplication::HandleDisconnect(StringHash eventType, VariantMap& eventData)
{
    auto* network = GetSubsystem<Network>();
    Connection* serverConnection = network->GetServerConnection();
    // If we were connected to server, disconnect. Or if we were running a server, stop it. In both cases clear the
    // scene of all replicated content, but let the local nodes & components (the static world + camera) stay
    if (serverConnection)
    {
        serverConnection->Disconnect();
        scene_->Clear(true, false);
        clientObjectID_ = 0;
    }
    // Or if we were running a server, stop it
    else if (network->IsServerRunning())
    {
        network->StopServer();
        scene_->Clear(true, false);
    }

    //UpdateButtons();
}


void SceneReplication::HandleConnectionStatus(StringHash eventType, VariantMap& eventData)
{
    //UpdateButtons();
}


void SceneReplication::FixRobotSound()
{
    auto* cache = GetSubsystem<ResourceCache>();
    PODVector<Node*>  robots;
    scene_->GetNodesWithTag(robots, "robotGirls");
    
    const unsigned NUM_MODELS = 30;
    const float MODEL_MOVE_SPEED = 2.0f;
    const float MODEL_ROTATE_SPEED = 100.0f;
    //const BoundingBox bounds(Vector3(-20.0f, 0.0f, -20.0f), Vector3(20.0f, 0.0f, 20.0f));
    const BoundingBox bounds(Vector3(-100.0f, 0.0f, -100.0f), Vector3(100.0f, 0.0f, 100.0f));
    for(size_t i=0;i<robots.Size();++i)
    {
        auto* modelNode=robots[i];
        
        modelNode->SetPosition(Vector3(Random(40.0f) - 20.0f, 0.0f, Random(40.0f) - 20.0f));
        modelNode->SetRotation(Quaternion(0.0f, Random(360.0f), 0.0f));

        auto* modelObject = modelNode->CreateComponent<AnimatedModel>();
        modelObject->SetModel(cache->GetResource<Model>("Models/Kachujin/Kachujin.mdl"));
        modelObject->SetMaterial(cache->GetResource<Material>("Models/Kachujin/Materials/Kachujin.xml"));
        modelObject->SetCastShadows(true);
        
        //modelNode->CreateComponent<AnimationController>();
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
        
        if(0==i)
            modelNode->Scale(5.0f);
        else if(1==i)
            modelNode->Scale(0.2f);
        
        
        
        
        // add sound
        SoundSource3D *snd_source_3d = modelNode->CreateComponent<SoundSource3D>();
        // Set near distance (if listener is within that distance, no attenuation is computed)
        snd_source_3d->SetNearDistance(5);
        // Set far distance (distance after which no sound is heard)
        snd_source_3d->SetFarDistance(20);
        //snd_source_3d->SetSoundType(SOUND_VOICE);

        //2 We also need to load a sound.

        // Load a sound. That sound will be the one that will be broadcast by the 3D sound source.
       char buf[512];
       ::sprintf(buf, "Sounds/82/%d.ogg", i%10);
        Sound *mySound = GetSubsystem<ResourceCache>()->GetResource<Sound>(buf);
        // That sound shall be played continuously.
        mySound->SetLooped(true);
        //Do not forget to play the sound, or you will hear nothing !
        // Play sound
        snd_source_3d->Play(mySound);
    }
}

void SceneReplication::HandleClientObjectID(StringHash eventType, VariantMap& eventData)
{
    auto* cache = GetSubsystem<ResourceCache>();
    
    clientObjectID_ = eventData[P_ID].GetUInt();
    String hash     = eventData[HASH_ID].GetString();
    
    if(clientObjectID_)
    {
        Node* objectNode=scene_->GetNode(clientObjectID_);
        if(objectNode)
        {
            SoundListener *listener = objectNode->CreateComponent<SoundListener>();
            if(listener)
                GetSubsystem<Audio>()->SetListener(listener);
            
            

            //设置音量
            //GetSubsystem<Audio>()->SetMasterGain(SOUND_VOICE, 0.6);

//           SoundSource3D* snd_source_3d =   objectNode->CreateComponent<SoundSource3D>();
//                snd_source_3d->SetNearDistance(3);
//                snd_source_3d->SetFarDistance(20);
        
        }
    }
    
    
    //开始创建xr 音视频组
    AVReady(av_server_,hash.CString(),"TV",4);
    
//    //登陆语音视频服务器
//    xrGroup_=facepower_->createXRGroup(av_server_id_
//                                       ,room_id_,clientObjectID_,hash.CString());
//
//    //注册命令回调
//    xrGroup_->registerCommand(std::bind(&SceneReplication::OnCommand, this, std::placeholders::_1));
//
//    //等待连接成功后，才开始采集发送数据
    
    
    

    
    
    
//    //开始音视频 设置场景
//    if(use_internal_av_)
//    {//使用游戏服务器传输交换音频
//        la_->startCapture(scene_,GetSubsystem<Network>());
//         auto cb=std::bind(&SceneReplication::OnEncodeAACFrame,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3);
//
//         facepower_->startAudioCapture(cb);
//    }else
//    {
//        //使用独立的音视频服务器
//        int64_t app_id=91;
//
//        char sCMD[1024];
//        int len=::sprintf(sCMD, "{\"cmd\":\"appStart\",\"appID\":%lld,\"rid\":%lld,\"nodeID\":%d,\"hash\":\"%s\"}",app_id,room_id_,clientObjectID_,hash.CString());
//        std::string sCommand(sCMD,len);
//
//
//        facepower_->cmd(sCommand);
//    }
}

int SceneReplication::OnEncodeAACFrame(uint8_t* buf,int len,int type)
{
    auto* network = GetSubsystem<Network>();
        
    Connection* serverConnection = network->GetServerConnection();
    if (serverConnection)
    {
        // A VectorBuffer object is convenient for constructing a message to send
        VectorBuffer msg;
        msg.Write((void*)buf, len);

        // Send the chat message as in-order and reliable
        serverConnection->SendMessage(MSG_VOICE, false, false, msg);
    }
    return 1;
}

void SceneReplication::HandleNetworkMessage(StringHash /*eventType*/, VariantMap& eventData)
{
    auto* network = GetSubsystem<Network>();

    using namespace NetworkMessage;

    int msgID = eventData[P_MESSAGEID].GetInt();
    const PODVector<unsigned char>& data = eventData[P_DATA].GetBuffer();
    // Use a MemoryBuffer to read the message data so that there is no unnecessary copying
    //MemoryBuffer msg(data);
    
    if (msgID == MSG_VOICE)
    {
        uint8_t fromID  =(uint8_t)data[0];
        uint8_t* buf    =data.Buffer()+1;
        int len         =data.Size()-1;
        
        //facepower_->decodeAudioFrame(fromID, buf, len);
    }else if(msgID==MSG_PLAYER_ENTER)
    {
        
    }else if(MSG_ERROR==msgID)
    {
        MemoryBuffer msg(data);
        String text = msg.ReadString();
        int a=0;a++;
    }
    
}


void SceneReplication::OnXRCommand(ptree pt)
{
    const std::string cmd=pt.get("cmd","");
    

}
