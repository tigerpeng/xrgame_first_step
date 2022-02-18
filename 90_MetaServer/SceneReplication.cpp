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
#include "Character.h"
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
#include "NetworkMessage.h"
#include "BaseInfo.h"
#include "Room.h"

#include <chrono>

#include <Urho3D/DebugNew.h>

#include <spdlog/spdlog.h>
#include "spdlog/sinks/basic_file_sink.h"
namespace spd = spdlog;



//// Control bits we define
//static const unsigned CTRL_FORWARD = 1;
//static const unsigned CTRL_BACK = 2;
//static const unsigned CTRL_LEFT = 4;
//static const unsigned CTRL_RIGHT = 8;

URHO3D_DEFINE_APPLICATION_MAIN(SceneReplication)

SceneReplication::SceneReplication(Context* context)
:Sample(context)
{
    // Register factory and attributes for the Character component so it can be created via CreateComponent, and loaded / saved
    Character::RegisterObject(context);
    
    BaseInfomation::RegisterObject(context);
    
    Room::RegisterObject(context);
    
    //Player::RegisterObject(context);
    
    // Register an object factory for our custom Mover component so that we can create them to scene nodes
    context->RegisterFactory<Mover>();
    
    
    
    // Set the default logger to file logger
    std::string logPath="logs/90_metaserver.log";
    std::string initMessage="";
    FileSystem * filesystem = GetSubsystem<FileSystem>();
    if(!filesystem->DirExists("logs"))
        filesystem->CreateDir("logs");
    
    auto file_logger = spdlog::basic_logger_mt("basic_logger", logPath);
    spdlog::set_default_logger(file_logger);
    
    spd::set_level(spd::level::trace);
    spd::flush_on(spdlog::level::level_enum::trace);
    spd::info("------90_MetaServer------");
    if(!initMessage.empty())
        spd::error(initMessage);
    
}

void SceneReplication::Setup()
{
    // Modify engine startup parameters
    Sample::Setup();
    engineParameters_[EP_HEADLESS]  = true;
    engineParameters_[EP_SOUND]     = false;
}

void SceneReplication::Start()
{
    // Execute base class startup
    //Sample::Start();
    
    // Create the scene content
    CreateScene();
    
    // Create the UI content
    //CreateUI();
    // Setup the viewport for displaying the scene
    //SetupViewport();

    // Hook up to necessary events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    //Sample::InitMouseMode(MM_RELATIVE);
    
    VariantMap eventData;
    HandleStartServer({},eventData);
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
    Node* zoneNode = scene_->CreateChild("Zone", LOCAL);
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
//    auto* cache = GetSubsystem<ResourceCache>();
//    auto* ui = GetSubsystem<UI>();
//    UIElement* root = ui->GetRoot();
//    auto* uiStyle = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");
//    // Set style to the UI root so that elements will inherit it
//    root->SetDefaultStyle(uiStyle);
//
//    // Create a Cursor UI element because we want to be able to hide and show it at will. When hidden, the mouse cursor will
//    // control the camera, and when visible, it can interact with the login UI
//    SharedPtr<Cursor> cursor(new Cursor(context_));
//    cursor->SetStyleAuto(uiStyle);
//    ui->SetCursor(cursor);
//    // Set starting position of the cursor at the rendering window center
//    auto* graphics = GetSubsystem<Graphics>();
//    cursor->SetPosition(graphics->GetWidth() / 2, graphics->GetHeight() / 2);
//
//    // Construct the instructions text element
//    instructionsText_ = ui->GetRoot()->CreateChild<Text>();
//    instructionsText_->SetText(
//        "Use WASD keys to move and RMB to rotate view"
//    );
//    instructionsText_->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
//    // Position the text relative to the screen center
//    instructionsText_->SetHorizontalAlignment(HA_CENTER);
//    instructionsText_->SetVerticalAlignment(VA_CENTER);
//    instructionsText_->SetPosition(0, graphics->GetHeight() / 4);
//    // Hide until connected
//    instructionsText_->SetVisible(false);
//
//    packetsIn_ = ui->GetRoot()->CreateChild<Text>();
//    packetsIn_->SetText("Packets in : 0");
//    packetsIn_->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
//    packetsIn_->SetHorizontalAlignment(HA_LEFT);
//    packetsIn_->SetVerticalAlignment(VA_CENTER);
//    packetsIn_->SetPosition(10, -10);
//
//    packetsOut_ = ui->GetRoot()->CreateChild<Text>();
//    packetsOut_->SetText("Packets out: 0");
//    packetsOut_->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
//    packetsOut_->SetHorizontalAlignment(HA_LEFT);
//    packetsOut_->SetVerticalAlignment(VA_CENTER);
//    packetsOut_->SetPosition(10, 10);
//
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
//
//    UpdateButtons();
}

void SceneReplication::SetupViewport()
{
//    auto* renderer = GetSubsystem<Renderer>();
//
//    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
//    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
//    renderer->SetViewport(0, viewport);
}

void SceneReplication::SubscribeToEvents()
{
    // Subscribe to fixed timestep physics updates for setting or applying controls
    SubscribeToEvent(E_PHYSICSPRESTEP, URHO3D_HANDLER(SceneReplication, HandlePhysicsPreStep));

    // Subscribe HandlePostUpdate() method for processing update events. Subscribe to PostUpdate instead
    // of the usual Update so that physics simulation has already proceeded for the frame, and can
    // accurately follow the object with the camera
    SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(SceneReplication, HandlePostUpdate));

//    // Subscribe to button actions
//    SubscribeToEvent(connectButton_, E_RELEASED, URHO3D_HANDLER(SceneReplication, HandleConnect));
//    SubscribeToEvent(disconnectButton_, E_RELEASED, URHO3D_HANDLER(SceneReplication, HandleDisconnect));
//    SubscribeToEvent(startServerButton_, E_RELEASED, URHO3D_HANDLER(SceneReplication, HandleStartServer));

    // Subscribe to network events
    SubscribeToEvent(E_SERVERCONNECTED, URHO3D_HANDLER(SceneReplication, HandleConnectionStatus));
    SubscribeToEvent(E_SERVERDISCONNECTED, URHO3D_HANDLER(SceneReplication, HandleConnectionStatus));
    SubscribeToEvent(E_CONNECTFAILED, URHO3D_HANDLER(SceneReplication, HandleConnectionStatus));
    SubscribeToEvent(E_CLIENTIDENTITY, URHO3D_HANDLER(SceneReplication, HandleClientIdentity));
    SubscribeToEvent(E_CLIENTCONNECTED, URHO3D_HANDLER(SceneReplication, HandleClientConnected));
    SubscribeToEvent(E_CLIENTDISCONNECTED, URHO3D_HANDLER(SceneReplication, HandleClientDisconnected));
    SubscribeToEvent(E_CLIENTSCENELOADED, URHO3D_HANDLER(SceneReplication, HandleClientSceneLoaded));
    
    SubscribeToEvent(E_NETWORKMESSAGE, URHO3D_HANDLER(SceneReplication, HandleNetworkMessage));
    
    // Events sent between client & server (remote events) must be explicitly registered or else they are not allowed to be received
    GetSubsystem<Network>()->RegisterRemoteEvent(E_CLIENTOBJECTID);
}

Button* SceneReplication::CreateButton(const String& text, int width)
{
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

    //return button;
    return nullptr;
}

void SceneReplication::UpdateButtons()
{
    return ;
    
//    auto* network = GetSubsystem<Network>();
//    Connection* serverConnection = network->GetServerConnection();
//    bool serverRunning = network->IsServerRunning();
//
//    // Show and hide buttons so that eg. Connect and Disconnect are never shown at the same time
//    connectButton_->SetVisible(!serverConnection && !serverRunning);
//    disconnectButton_->SetVisible(serverConnection || serverRunning);
//    startServerButton_->SetVisible(!serverConnection && !serverRunning);
//    textEdit_->SetVisible(!serverConnection && !serverRunning);
}

void SceneReplication::MoveCamera()
{
    return ;
//
//    // Right mouse button controls mouse cursor visibility: hide when pressed
//    auto* ui = GetSubsystem<UI>();
//    auto* input = GetSubsystem<Input>();
//    ui->GetCursor()->SetVisible(!input->GetMouseButtonDown(MOUSEB_RIGHT));
//
//    // Mouse sensitivity as degrees per pixel
//    const float MOUSE_SENSITIVITY = 0.1f;
//
//    // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch and only move the camera
//    // when the cursor is hidden
//    if (!ui->GetCursor()->IsVisible())
//    {
//        IntVector2 mouseMove = input->GetMouseMove();
//        yaw_ += MOUSE_SENSITIVITY * mouseMove.x_;
//        pitch_ += MOUSE_SENSITIVITY * mouseMove.y_;
//        pitch_ = Clamp(pitch_, 1.0f, 90.0f);
//    }
//
//    // Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
//    cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));

}

void SceneReplication::VerifyUserToken()
{
    auto it=verification_.begin();
    while(it!=verification_.end())
    {
        stAuthorization& authorize=verification_.front();
        if(!authorize.httpRequest.Null())
        {
            SharedPtr<HttpRequest> httpRequest=authorize.httpRequest;
            // Initializing HTTP request
            if (httpRequest->GetState() == HTTP_INITIALIZING)
                ;
            // An error has occurred
            else if (httpRequest->GetState() == HTTP_ERROR)
            {
                int a=0;a++;
                //text_->SetText("An error has occurred: " + httpRequest->GetError());
                //UnsubscribeFromEvent("Update");
                //URHO3D_LOGERRORF("HttpRequest error: %s", httpRequest->GetError().CString());
            }else // Get message data
            {
                if (httpRequest->GetAvailableSize() > 0)
                    authorize.message+= httpRequest->ReadLine();
                else
                {
                    spd::trace("verify user token  return msg:{}",authorize.message.CString());
                    //parse message
                    //提取用户信息
                    int64_t rid=0;
                    int64_t uid;
                    String  uname;
                    String  avatar;
                    
                    std::string errMsg;
                    
                    try{
                        ptree pt;
                        std::stringstream ss(authorize.message.CString());
                        read_json(ss, pt);
                        
                        rid  =pt.get("rid",int64_t(0));
                        if(rid>0)
                        {
                            uid     =pt.get("uid",int64_t(0));
                            uname   =pt.get("name","").c_str();
                            avatar  =pt.get("avatar","").c_str();
                        }else
                            errMsg=pt.get("errorMSG","");

                    }catch (ptree_error & e) {
                        spd::error("json error where:{},{},{} ", __FILE__,__LINE__,__FUNCTION__);
                    }catch (...) {
                        spd::error("unknown exception where:{},{},{} ", __FILE__,__LINE__,__FUNCTION__);
                    }
                    
                    if(rid>0)
                    {
                        if(rooms_[rid]==nullptr)
                            rooms_[rid]=new Room(context_); //根据房间信息创建房间
                        
                        rooms_[rid]->AddPlayer(rid,uid,uname
                                               ,authorize.connection,avatar);
                    }else{
                        //发送错误通知给客户端
                        // A VectorBuffer object is convenient for constructing a message to send
                        if(errMsg.empty())
                            errMsg="无法连接到服务器.";
                        
                        VectorBuffer msg;
                        msg.Write((void*)errMsg.data(), errMsg.size());

                        // Send the chat message as in-order and reliable
                        authorize.connection->SendMessage(MSG_ERROR, false, false, msg);
                    }
                    
                    verification_.erase(it++);
                    continue;
                }
            }
            
        }
        
        
        
        //
        std::chrono::duration<double> timeDura = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - authorize.startTime);
        if(timeDura.count()>60)
        {
            std::string errMsg="连接服务器超时.";
            VectorBuffer msg;
            msg.Write((void*)errMsg.data(), errMsg.size());
            // Send the chat message as in-order and reliable
            authorize.connection->SendMessage(MSG_ERROR, false, false, msg);
            verification_.erase(it++);
        }else
            ++it;
    }
}

void SceneReplication::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
    VerifyUserToken();

    
//    // We only rotate the camera according to mouse movement since last frame, so do not need the time step
//    MoveCamera();
//
//
//    if (packetCounterTimer_.GetMSec(false) > 1000 && GetSubsystem<Network>()->GetServerConnection())
//    {
//        packetsIn_->SetText("Packets  in: " + String(GetSubsystem<Network>()->GetServerConnection()->GetPacketsInPerSec()));
//        packetsOut_->SetText("Packets out: " + String(GetSubsystem<Network>()->GetServerConnection()->GetPacketsOutPerSec()));
//        packetCounterTimer_.Reset();
//    }
//    if (packetCounterTimer_.GetMSec(false) > 1000 && GetSubsystem<Network>()->GetClientConnections().Size())
//    {
//        int packetsIn = 0;
//        int packetsOut = 0;
//        auto connections = GetSubsystem<Network>()->GetClientConnections();
//        for (auto it = connections.Begin(); it != connections.End(); ++it ) {
//            packetsIn += (*it)->GetPacketsInPerSec();
//            packetsOut += (*it)->GetPacketsOutPerSec();
//        }
//        packetsIn_->SetText("Packets  in: " + String(packetsIn));
//        packetsOut_->SetText("Packets out: " + String(packetsOut));
//        packetCounterTimer_.Reset();
//    }
    
}

void SceneReplication::HandlePhysicsPreStep(StringHash eventType, VariantMap& eventData)
{
//    // This function is different on the client and server. The client collects controls (WASD controls + yaw angle)
//    // and sets them to its server connection object, so that they will be sent to the server automatically at a
//    // fixed rate, by default 30 FPS. The server will actually apply the controls (authoritative simulation.)
//    auto* network = GetSubsystem<Network>();
//
//    if (network->IsServerRunning())
//    {
//        const Vector<SharedPtr<Connection> >& connections = network->GetClientConnections();
//
//        for (unsigned i = 0; i < connections.Size(); ++i)
//        {
//            Connection* connection = connections[i];
//
//            if(serverObjects_[connection]==nullptr)
//                continue;       //校验用户合法性需要等待
//
//            // Get the object this connection is controlling
//            Node* playerNode = serverObjects_[connection]->GetNode();
//            if (!playerNode)
//                continue;
//
//            auto* character=playerNode->GetComponent<Character>();
//            if(character)
//                character->controls_=connection->GetControls();
//
//            playerNode->SetRotation(Quaternion(character->controls_.yaw_, Vector3::UP));
//        }
//    }
    
}


void SceneReplication::HandleDisconnect(StringHash eventType, VariantMap& eventData)
{
    auto* network = GetSubsystem<Network>();
    Connection* serverConnection = network->GetServerConnection();
    // If we were connected to server, disconnect. Or if we were running a server, stop it. In both cases clear the
    // scene of all replicated content, but let the local nodes & components (the static world + camera) stay
//    if (serverConnection)
//    {
//        serverConnection->Disconnect();
//        scene_->Clear(true, false);
//        clientObjectID_ = 0;
//    }
    // Or if we were running a server, stop it
    if (network->IsServerRunning())
    {
        network->StopServer();
        scene_->Clear(true, false);
    }

    UpdateButtons();
}

void SceneReplication::HandleStartServer(StringHash eventType, VariantMap& eventData)
{
    auto* network = GetSubsystem<Network>();
    network->StartServer(SERVER_PORT);

    UpdateButtons();
}

void SceneReplication::HandleConnectionStatus(StringHash eventType, VariantMap& eventData)
{
    UpdateButtons();
}

void SceneReplication::HandleClientIdentity(StringHash eventType, VariantMap& eventData)
{
    auto* network = GetSubsystem<Network>();
    
    using namespace ClientIdentity;
    auto connection = static_cast<Connection*>(eventData[P_CONNECTION].GetPtr());
//    URHO3D_LOGINFO("Client identity: Client " + connection->GetGUID() + " => " + connection->GetIdentity()["Name"].GetString());
    //UpdatePlayerList();
    
   
    int64_t rid     =connection->GetIdentity()["RoomID"].GetUInt64();
    String  token   =connection->GetIdentity()["Token"].GetString();
    
    //校验用户合法性
    //String url="http://localhost:5000/api/world/playerinfo?rid="+String(rid);
    String url="http://db.ournet.club:2021/api/world/playerinfo?rid="+String(rid);
    
    Vector<String> headers;
    String Authorization="Authorization:"+token;
    headers.Push(Authorization);

    stAuthorization authorize;
                    authorize.connection=connection;
                    authorize.startTime=std::chrono::steady_clock::now();
#ifdef URHO3D_SSL
    authorize.httpRequest = network->MakeHttpRequest(url,"GET",headers);
#else
    authorize.httpRequest = network->MakeHttpRequest(url,"GET",headers);
#endif
    verification_.push_back(authorize);
    
    spd::trace("verify user token  rid:{},token:{},url:{}",rid,token.CString(),url.CString());

//    int64_t uid     =connection->GetIdentity()["UserID"].GetUInt64();
//    String  profile =connection->GetIdentity()["Profile"].GetString();
}



void SceneReplication::HandleClientConnected(StringHash eventType, VariantMap& eventData)
{
    using namespace ClientConnected;
    
//    // When a client connects, assign to scene to begin scene replication
//    auto* newConnection = static_cast<Connection*>(eventData[P_CONNECTION].GetPtr());
//    
//    newConnection->SetScene(scene_);
    
    return ;

}

void SceneReplication::HandleClientDisconnected(StringHash eventType, VariantMap& eventData)
{
    using namespace ClientConnected;

    // When a client disconnects, remove the controlled object
    auto* connection = static_cast<Connection*>(eventData[P_CONNECTION].GetPtr());
    int64_t rid=connection->GetIdentity()["RoomID"].GetUInt64();
    if(rooms_[rid])
        rooms_[rid]->RemovePlayer(connection);
    
//    Node* object = serverObjects_[connection]->GetNode();
//    if (object){
//        object->Remove();
//    }
    
    //serverObjects_.Erase(connection);
}
void SceneReplication::HandleClientSceneLoaded(StringHash /*eventType*/, VariantMap& eventData)
{
    auto* network = GetSubsystem<Network>();
    using namespace ClientSceneLoaded;
    /// \todo why use GetEventSender ?
    //    dynamic_cast<Connection*>(eventData[P_CONNECTION].GetPtr());

    Connection* connection = dynamic_cast<Connection*>(GetEventSender());
    int64_t rid=connection->GetIdentity()["RoomID"].GetUInt64();
    if(rooms_[rid])
        rooms_[rid]->OnClientSceneLoaded(connection);
}

void SceneReplication::HandleNetworkMessage(StringHash /*eventType*/, VariantMap& eventData)
{
    using namespace NetworkMessage;

    auto* network = GetSubsystem<Network>();
    
    int msgID = eventData[P_MESSAGEID].GetInt();
    PODVector<unsigned char> data = eventData[P_DATA].GetBuffer();
    auto* sender = static_cast<Connection*>(eventData[P_CONNECTION].GetPtr());
    
    int64_t rid=sender->GetIdentity()["RoomID"].GetUInt64();
    if(rooms_[rid])
    {
        rooms_[rid]->OnClientNetworkMessage(sender,msgID,data);
    }
    
   
    
//    if (msgID == MSG_VOICE)
//    {
//        if (network->IsServerRunning())
//        {
//            //record first packet
//            if(fromCtrl&&fromCtrl->headerIndex_<0)
//            {
//                std::string header;
//                header.assign((char*)data.Buffer(),data.Size());
//
//                fromCtrl->headerIndex_=AddFirstPacket(header);
//            }
//
//
//            uint8_t fromID=serverObjects_[sender]->GetNode()->GetID();
//            //插入用户的索引
//            data.Insert(0,fromID);
//            VectorBuffer sendMsg;
//            sendMsg.Write(data.Buffer(),data.Size());
//
//            auto it=serverObjects_.Begin();
//            while(it!=serverObjects_.End())
//            {
//                if(1/*it->first_!=sender*/)
//                {
//                    auto* peerCtrl=it->second_->GetNode()->GetComponent<Character>();
//                    if(peerCtrl)
//                    {
//                        int status=peerCtrl->GetPeerAACStreamStatus(fromID);
//                        if(status<0)
//                            continue;;
//
//                        if(status==0)//发送header
//                        {   peerCtrl->SetPeerAACStreamStatus(fromID,1);
//                            const std::string& head=firstPackets_[fromCtrl->headerIndex_];
//                            VectorBuffer aacHeader;
//                            aacHeader.Write(head.c_str(),head.size());
//
//                            it->first_->SendMessage(MSG_VOICE, true, true, aacHeader);
//                        }
//                    }
//                    it->first_->SendMessage(MSG_VOICE, false, false, sendMsg);
//                }
//                ++it;
//            }
//        }
//    }else if(msgID==MSG_SUBCRIBE_SOUNDER)
//    {
//        PODVector<unsigned char> data = eventData[P_DATA].GetBuffer();
//
//        PODVector<int> nodeIDs((int*)data.Buffer(),data.Size()/sizeof(int));
//        auto* sender = static_cast<Connection*>(eventData[P_CONNECTION].GetPtr());
//        auto* character=serverObjects_[sender]->GetNode()->GetComponent<Character>();
//        character->UpdateSounders(nodeIDs);
//    }
    
}

void SceneReplication::CreateRobot()
{
    return ;
    
    auto* cache = GetSubsystem<ResourceCache>();
    
    //cache->AddPackageFile("/Users/leeco/xrchat/myModel.pak");
    //cache->GetResource<Model>("Models/Kachujin/Kachujin.mdl");
    // Create animated models
    const unsigned NUM_MODELS = 30;
    const float MODEL_MOVE_SPEED = 2.0f;
    const float MODEL_ROTATE_SPEED = 100.0f;
    //const BoundingBox bounds(Vector3(-20.0f, 0.0f, -20.0f), Vector3(20.0f, 0.0f, 20.0f));
    const BoundingBox bounds(Vector3(-100.0f, 0.0f, -100.0f), Vector3(100.0f, 0.0f, 100.0f));

    for (unsigned i = 0; i < NUM_MODELS; ++i)
    {
        char modelName[512];
        ::sprintf(modelName, "Jill_%d", i);
        Node* modelNode = scene_->CreateChild(modelName);
        
        //add tags
        StringVector tags;
        tags.Push("robotGirls");
        modelNode->SetTags(tags);
                
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

//        if(9>=i){
////            // add sound
//            SoundSource3D *snd_source_3d = modelNode->CreateComponent<SoundSource3D>();
//            // Set near distance (if listener is within that distance, no attenuation is computed)
//            snd_source_3d->SetNearDistance(5);
//            // Set far distance (distance after which no sound is heard)
//            snd_source_3d->SetFarDistance(20);
//            //snd_source_3d->SetSoundType(SOUND_VOICE);
//
//            //2 We also need to load a sound.
//
//            // Load a sound. That sound will be the one that will be broadcast by the 3D sound source.
//           char buf[512];
//           ::sprintf(buf, "Sounds/82/%d.ogg", i);
//            Sound *mySound = GetSubsystem<ResourceCache>()->GetResource<Sound>(buf);
//            // That sound shall be played continuously.
//            mySound->SetLooped(true);
//            //Do not forget to play the sound, or you will hear nothing !
//            // Play sound
//            snd_source_3d->Play(mySound);
//        }
    }
}
