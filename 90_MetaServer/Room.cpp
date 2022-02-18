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

#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/AnimationState.h>
#include <Urho3D/Scene/Scene.h>

#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Graphics/AnimationController.h>

//boost
#include <boost/format.hpp>
//fro xml and json
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
using namespace boost::property_tree;

#include <spdlog/spdlog.h>
namespace spd = spdlog;


#include <boost/compute/detail/sha1.hpp>

#include "Room.h"
#include "Character.h"
#include "Touch.h"
#include "BaseInfo.h"
#include "NetworkMessage.h"

#include "jwt/jwt.hpp"
//验证用户token是否合法
std::string key="THIS IS USED TO SIGN AND VERIFY JWT TOKENS, REPLACE IT WITH YOUR OWN SECRET, IT CAN BE ANY STRING";
int64_t verifyToken(std::string const& bt)
{
    std::string token=bt;
    size_t ptS=token.find(" ");//去掉Bearer 前缀
    if(ptS!=std::string::npos)
    {
        ptS++;
        token=token.substr(ptS,token.length()-ptS);
    }
    
    int64_t UserId;
    //算法参考
    //https://github.com/arun11299/cpp-jwt
    using namespace jwt::params;
    //auto key = "THIS IS USED TO SIGN AND VERIFY JWT TOKENS, REPLACE IT WITH YOUR OWN SECRET, IT CAN BE ANY STRING"; //Secret to use for the algorithm
    //std::string key=lobby_mj::GetLobby()->get_secret();
    try
    {
        //Decode
        auto dec_obj = jwt::decode(token, algorithms({"HS256"}), secret(key));
        std::cout << dec_obj.header() << std::endl;
        std::cout << dec_obj.payload() << std::endl;
        
        /*
         {"alg":"HS256","typ":"JWT"}   //dec_obj.header()
         {"exp":1571625330,"iat":1571020530,"nbf":1571020530,"unique_name":"10001"} // dec_obj.payload()
         */
        ptree pt;
        std::stringstream ss;
        ss<<dec_obj.payload();
        read_json(ss, pt);
        
        UserId= pt.get("unique_name",0);
        //过期时间
        uint64_t exp=pt.get("exp",0);
        
        //spd::trace("verify user token  id:{}",uid_);
        
        return UserId;
    }catch (ptree_error & e) {
        //spd::trace("json parse exception");
    }catch (...) {
        //spd::trace("json parse unknown exception 解析 token 异常!");
    }
    return UserId;
}

std::string CreateAudioRoomJWT(int64_t rid,int64_t uid,int64_t ndx)
{
    using namespace jwt::params;
    
    //Create JWT object
    //jwt::jwt_object obj{algorithm("HS256"), payload({{"name","penghong"},{"birthday","2001-1-1"}}), secret(key)};
    jwt::jwt_object obj{algorithm("HS256"), payload({}), secret(key)};

    obj.add_claim("rid",rid)
        .add_claim("uid",uid)
        .add_claim("ndx",ndx)
        .add_claim("exp", std::chrono::system_clock::now() + std::chrono::seconds{60});
    
    //Get the encoded string/assertion
    auto enc_str = obj.signature();
    //std::cout << enc_str << std::endl;
    std::stringstream ss;
    ss<<enc_str;
    return ss.str();
}
std::string DecodeAudioRoomJWT(std::string const&token)
{
    using namespace jwt::params;
    auto dec_obj = jwt::decode(token, algorithms({"HS256"}), secret(key));
    std::cout<<dec_obj.payload()<<std::endl;
    
    std::stringstream ss;
    ss<<dec_obj.payload();
    
    return ss.str();
}





Room::Room(Context* context)
:Object(context)
{
    CreateScene();
    
    SubscribeToEvent(E_PHYSICSPRESTEP, URHO3D_HANDLER(Room, HandlePhysicsPreStep));
}

Room::~Room()
{

}
void Room::RegisterObject(Context* context)
{
    context->RegisterFactory<Room>();
}

void Room::HandlePhysicsPreStep(StringHash eventType, VariantMap& eventData)
{
//    // This function is different on the client and server. The client collects controls (WASD controls + yaw angle)
//    // and sets them to its server connection object, so that they will be sent to the server automatically at a
//    // fixed rate, by default 30 FPS. The server will actually apply the controls (authoritative simulation.)
//    auto* network = GetSubsystem<Network>();

    
    auto it=serverObjects_.Begin();
    while(it!=serverObjects_.End())
    {
        Connection* connection =it->first_;
        Node* playerNode=it->second_->GetNode();
        if(playerNode!=nullptr)
        {
            auto* character=playerNode->GetComponent<Character>();
            if(character)
                character->controls_=connection->GetControls();
            
            playerNode->SetRotation(Quaternion(character->controls_.yaw_, Vector3::UP));
        }
        
        ++it;
    }
    
}

///to do
void Room::AddPlayer(int64_t rid,int64_t uid,String& name,Connection* c,String const& avatar)
{
    c->SetScene(scene_);
    // Then create a controllable object for that client
    serverObjects_[c] = CreateControllableObject(rid,uid,name,avatar);
}
void Room::RemovePlayer(Connection* c)
{
    int nodeID=serverObjects_[c]->GetNode()->GetID();
    if(nodeID>=2&&nodeID<=0xff)
        ReuseVoiceID(nodeID);
    
    serverObjects_.Erase(c);
}


void Room::CreateScene()
{
    auto* cache = GetSubsystem<ResourceCache>();
    
    scene_ = new Scene(context_);
    
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

//    // Create the camera. Limit far clip distance to match the fog
//    // The camera needs to be created into a local node so that each client can retain its own camera, that is unaffected by
//    // network messages. Furthermore, because the client removes all replicated scene nodes when connecting to a server scene,
//    // the screen would become blank if the camera node was replicated (as only the locally created camera is assigned to a
//    // viewport in SetupViewports() below)
//    cameraNode_ = scene_->CreateChild("Camera", LOCAL);
//    auto* camera = cameraNode_->CreateComponent<Camera>();
//    camera->SetFarClip(300.0f);
//
//    // Set an initial position for the camera scene node above the plane
//    cameraNode_->SetPosition(Vector3(0.0f, 5.0f, 0.0f));
//    
//
    
    //定义玩家的id范围
    if(playerVoiceIDs_.size()==0)
    {
        for(size_t i=2;i<=0xff;++i)
        {
            playerVoiceIDs_.push(i);
            scene_->CreateChild("Placeholder",REPLICATED);
        }
        scene_->RemoveChildren(true, false, false);
    }
    
    CreateTV();

    //CreateRobot();
}

unsigned Room::ProduceVoiceID()
{
    if(playerVoiceIDs_.size()>0)
    {
        unsigned newID=playerVoiceIDs_.front();
        playerVoiceIDs_.pop();
        return newID;
    }
    return 1; //返回错误
}
void Room::ReuseVoiceID(unsigned id)
{
    playerVoiceIDs_.push(id);
}
//add by copyleft for vrchat
//左手坐标系. X, Y和Z轴的正方向指向向右,向上和向前. 旋转以顺时针方向为正.
//创建视频墙
void Room::CreateTV()
{
    auto* cache = GetSubsystem<ResourceCache>();

    float scale = 3.0f;

    Node* objectNode = scene_->CreateChild("TV");
    //objectNode->SetPosition(Vector3(Random(180.0f) - 90.0f, Random(10.0f) + 10.0f, Random(180.0f) - 90.0f));
    objectNode->SetPosition(Vector3(0.0f, 0.0f,5.0f));
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
void Room::OnClientSceneLoaded(StringHash /*eventType*/, VariantMap& eventData)
{
    
}
void Room::OnClientSceneLoaded(Connection* c)
{
    if(serverObjects_[c])
    {
        VariantMap remoteEventData;
        remoteEventData[P_ID] = serverObjects_[c]->GetNode()->GetID();
            //token=hash(roomid+userid+nodeid+"yoyo151210")
//            std::string s;
//            s=std::to_string(serverObjects_[c]->GetRoomID());
//            s+=std::to_string(serverObjects_[c]->GetUserID());
//            s+=std::to_string(serverObjects_[c]->GetNode()->GetID());
//
////            std::hash<std::string> hash_fn;
////            size_t hash =hash_fn(s);
//            boost::compute::detail::sha1 sha1 {s};
//            std::string str { sha1 };
//
        std::string str=CreateAudioRoomJWT(serverObjects_[c]->GetRoomID()
                                           ,serverObjects_[c]->GetUserID()
                                           ,serverObjects_[c]->GetNode()->GetID());
        
        
        remoteEventData[HASH_ID]=String(str.c_str());
        
        c->SendRemoteEvent(E_CLIENTOBJECTID, true, remoteEventData);
    }
    
}
void Room::OnClientNetworkMessage(Connection* sender,int msgID,PODVector<unsigned char> data)
{
    auto* fromCtrl=serverObjects_[sender]->GetNode()->GetComponent<Character>();
    
}
std::shared_ptr<Player> Room::CreateControllableObject(int64_t rid,int64_t uid,String& name,String const& avatar)
{
    auto* cache = GetSubsystem<ResourceCache>();
        
    unsigned VID=ProduceVoiceID();
    
    char modelName[512];
    ::sprintf(modelName, "Player_%d", VID);
    Node* objectNode = scene_->CreateChild(modelName,REPLICATED,VID);
    
    assert(objectNode->GetID()==VID);
    std::shared_ptr<Player> player=std::make_shared<Player>(rid,uid,objectNode);
    objectNode->AddTag("Player");
    objectNode->SetPosition(Vector3(0.0f, 1.0f, 0.0f));

    // spin node
    Node* adjustNode = objectNode->CreateChild("AdjNode");
    adjustNode->SetRotation( Quaternion(180, Vector3(0,1,0) ) );

    // Create the rendering component + animation controller
    auto* object = adjustNode->CreateComponent<AnimatedModel>();
    
    // Create the character logic component, which takes care of steering the rigidbody
    // Remember it so that we can set the controls. Use a WeakPtr because the scene hierarchy already owns it
    // and keeps it alive as long as it's not removed from the hierarchy
    auto* character=objectNode->CreateComponent<Character>(CreateMode::LOCAL);
    
    
    bool needDownloadAvatar=false;
    
    String modelPath;
    String modelFile;
    if(avatar.Empty())
    {
        modelPath="Models/def/";
        modelFile="Models/def/a.mdl";
    }else{
        auto* fileSystem=GetSubsystem<FileSystem>();
        
        modelPath="avatars/"+avatar+"/";
        modelFile=String("avatars/"+avatar+"/a.mdl");
        
        if(!fileSystem->FileExists(modelFile))
        {
            modelFile="Models/def/a.mdl";
            modelPath="Models/def/";
            
            needDownloadAvatar=true;
        }
    }
        
    
    //object->SetModel(cache->GetResource<Model>(modelFile.c_str()));
    Model* m=cache->GetResource<Model>(modelFile);
    if(nullptr!=m)
    {
        character->SetAvatar(modelPath);
        
        object->SetModel(m);
        const size_t slots=object->GetBatches().Size();
        if(slots<1)
        {
            String mats=modelPath+"Materials/0.xml";
            object->SetMaterial(cache->GetResource<Material>(mats));
        }else{
            for(size_t i=0;i<slots;++i)
            {
               // Materials
                char matlist[128];
                ::sprintf(matlist, "Materials/%d.xml", (int)i);
                String mats=modelPath+matlist;
                
                object->SetMaterial(i,cache->GetResource<Material>(mats));
            }
        }
    }else{
        //不存在...
        
    }
    //object->SetMaterial(cache->GetResource<Material>("Models/Mutant/Materials/mutant_M.xml"));
    
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
    //objectNode->CreateComponent<Character>(CreateMode::LOCAL);
    
    //创建3d声源
    SoundSource3D*  snd_source_3d =objectNode->CreateComponent<SoundSource3D>();
                    snd_source_3d->SetSoundType(SOUND_VOICE);
                    snd_source_3d->SetNearDistance(3);
                    snd_source_3d->SetFarDistance(20);
    
    //add custom
    auto* info=objectNode->CreateComponent<BaseInfomation>();
    if(info)
    {
        info->name_=name;
        //通过异步http请求获得其他用户信息
        info->age_=40;
        
        Node* adjNode = objectNode->CreateChild("AdjText");
        
        int height=2;
        adjNode->SetPosition(Vector3(0,height,0));
        
        
        
        auto* titleText = adjNode->CreateComponent<Text3D>();
        titleText->SetFaceCameraMode(FaceCameraMode::FC_LOOKAT_XYZ);
        //titleText->SetFont(cache->GetResource<Font>("Fonts/BlueHighway.sdf"), 30);
        titleText->SetFont(cache->GetResource<Font>("Fonts/simfang.ttf"), 12);
        titleText->SetText(info->name_);
        titleText->SetHorizontalAlignment(HA_CENTER);
        titleText->SetVerticalAlignment(VA_TOP);
        
    }
    
    


    return player;
}
