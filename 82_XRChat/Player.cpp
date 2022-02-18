#include "Player.h"
#include "Mover.h"

#include <Urho3D/Audio/Audio.h>
#include <Urho3D/Audio/AudioEvents.h>
#include <Urho3D/Audio/Sound.h>
#include <Urho3D/Audio/SoundSource.h>


#include <facepower/api_facepower.h>
#include "Character.h"

//boost
#include <boost/format.hpp>
//fro xml and json
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
using namespace boost::property_tree;

#include <spdlog/spdlog.h>
namespace spd = spdlog;


 

std::string player::save_path_="/Users/leeco/Pictures/";
player::player(int index,int64_t uid,SharedPtr<Scene> scene)
:index_(index)
,uid_(uid)
,scene_(scene)
{
    
}
player::~player()
{
    destroy();
}
//void setModel(SharedPtr<Scene>&scene_
//              ,Node* objectNode
//              ,std::string const& avatar)
//{
//
//    auto* cache =  scene_->GetSubsystem<ResourceCache>();
//
//    auto* character=objectNode->GetComponent<Character>();
//
//    Node* adjustNode = objectNode->GetChild("AdjNode");
//    auto* object = adjustNode->GetComponent<AnimatedModel>();
//
//
//    std::string modelPath;
//    if("def"==avatar||""==avatar)
//        modelPath="Models/def/";
//    else
//        modelPath="avatars/"+avatar+"/";
//
//    std::string modelFile=modelPath+"a.mdl";
//    //auto* file=scene_->GetSubsystem<FileSystem>();
//
//    Model* m=cache->GetResource<Model>(modelFile.c_str());
//    if(nullptr!=m)
//    {
//        character->SetAvatar(modelPath);
//
//        object->SetModel(m);
//        const size_t slots=object->GetBatches().Size();
//        if(slots<1)
//        {
//            std::string mats=modelPath+"Materials/0.xml";
//            object->SetMaterial(cache->GetResource<Material>(mats.c_str()));
//        }else{
//            for(size_t i=0;i<slots;++i)
//            {
//               // Materials
//                char matlist[128];
//                ::sprintf(matlist, "Materials/%d.xml", (int)i);
//                std::string mats=modelPath+matlist;
//
//                object->SetMaterial(i,cache->GetResource<Material>(mats.c_str()));
//            }
//        }
//    }else{//从网络下载模型 下载完成后设置模型
//        auto fp=getFacePower();
//    }
//
//}

void player::attachModel(Character* c,std::string const&name)
{
    character_=c;
    
    char buf[512];
    if(scene_)
    {
        ::sprintf(buf, "player_%lld", (int64_t)index_);
        node_=scene_->GetChild(buf);
    }
}

//加载静态声音文件
void player::loadStaticSound()
{
    Node* modelNode=nullptr;
    char buf[512];
    if(scene_)
    {
        ::sprintf(buf, "player_%lld", uid_);
        modelNode=scene_->GetChild(buf);
    }
    // add sound
    //1 First, we create our 3D sound source.
    // Create the sound source node
    Node *sound_node = modelNode->CreateChild("Sound source node");
    // Set the node position (sound will come from that position also)
    //sound_node->SetPosition(Vector3(10,-20,45));
    // Create the sound source
    SoundSource3D *snd_source_3d = sound_node->CreateComponent<SoundSource3D>();
    // Set near distance (if listener is within that distance, no attenuation is computed)
    snd_source_3d->SetNearDistance(1);
    // Set far distance (distance after which no sound is heard)
    snd_source_3d->SetFarDistance(15);

    //2 We also need to load a sound.
    // Load a sound. That sound will be the one that will be broadcast by the 3D sound source.
    ::sprintf(buf, "Sounds/82/%d.ogg", 0);
    Sound *my_sound = scene_->GetSubsystem<ResourceCache>()->GetResource<Sound>(buf);
    // That sound shall be played continuously.
    my_sound->SetLooped(true);
    //Do not forget to play the sound, or you will hear nothing !
    // Play sound
    snd_source_3d->Play(my_sound);
}
void player::destroy()
{
    if(character_)
    {
        auto* node=character_->GetNode();
        
        auto* scene=node->GetScene();
        if(scene)
            scene->RemoveChild(node);
    }
//    if(scene_)
//    {
//        char buf[512];
//        ::sprintf(buf, "player_%lld", uid_);
//        Node* node=scene_->GetChild(buf);
//        scene_->RemoveChild(node);
//    }
}


void player::playPCM(uint8_t* pcm,int pcm_len,int freq,int channels)
{
    if(soundStream_==nullptr)
    {
        spd::trace("play pcm:{}",index_);
        //语音回放
        // Sound source needs a node so that it is considered enabled
        soundStream_ = new BufferedSoundStream();
        // Set format: 44100 Hz, sixteen bit, mono
        soundStream_->SetFormat(freq, true, (channels==2));
        // Start playback. We don't have data in the stream yet, but the SoundSource will wait until there is data,
        if(node_)
        {
            // add sound
            //1 First, we create our 3D sound source.
            // Create the sound source node
            Node *sound_node = node_->CreateChild("3DSound source node");
            // Set the node position (sound will come from that position also)
            //sound_node->SetPosition(Vector3(10,-20,45));
            // Create the sound source
            SoundSource3D *snd_source_3d = sound_node->CreateComponent<SoundSource3D>();
            // Set near distance (if listener is within that distance, no attenuation is computed)
            snd_source_3d->SetNearDistance(1);
            // Set far distance (distance after which no sound is heard)
            snd_source_3d->SetFarDistance(15);

            //Do not forget to play the sound, or you will hear nothing !
            // Play sound
            snd_source_3d->Play(soundStream_);
        }
    }

    if(pcm_len>0&&soundStream_)
    {
            //保证语音的实时性 语音的滋滋声还不清楚
            //Return length of buffered (unplayed) sound data in seconds.
        float buffLen=soundStream_->GetBufferLength();
        //    if(buffLen>=2)
        //        soundStream_->Clear();
        soundStream_->AddData(pcm, pcm_len);
    }
}

void player::playMovement(uint8_t& ac,Quaternion& rot,Vector3& pos)
{
    if(nullptr==character_)
        return ;
    
    //        // Clear previous controls
    character_->controls_.Set(CTRL_FORWARD | CTRL_BACK | CTRL_LEFT | CTRL_RIGHT | CTRL_JUMP, false);
    
    if(0==ac)
    {   //stop
        //pos.z_+=2.0f; //for test only
        
        //character_->SetStop(pos,rot);
        
        spd::trace("stop start  ps:x:{}y:{}z:{}",pos.x_,pos.y_,pos.z_);
        if(node_!=nullptr)
        {
            char str[20];
            
            sprintf(str,"%.2f", pos.x_);
            pos.x_=(float)(atof(str));
            
            sprintf(str,"%.2f", pos.y_);
            pos.y_=(float)(atof(str));
            
            sprintf(str,"%.2f", pos.z_);
            pos.z_=(float)(atof(str));

            
            spd::trace("crash test begin");
            
            node_->SetPosition(pos);

            spd::trace("crash test end ----->no crash!");


           // node_->SetRotation(rot);
        }else{
            spd::trace("error node ptr is empty in player::playMovement");
        }
        spd::trace("stop end");
    }else{
        uint8_t& mvByte=ac;
        
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
    }
}

