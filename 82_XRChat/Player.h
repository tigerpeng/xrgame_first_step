#pragma once
#include "Urho3DAll.h"

#include <memory>
#include <thread>
#include <mutex>
#include <SDL/SDL.h>

//#include <facepower/api_facepower.h>

class Mover;

class Character;

class player : public std::enable_shared_from_this<player>
{
public:
    player(int index_,int64_t uid,SharedPtr<Scene> scene);
    ~player();
    
public:
    
    void attachModel(Character* c,std::string const&name="");
//    //new method
//    void create(std::string const& profile="");
//    void changeModel(std::string const& model);
    

    void loadStaticSound();
    void playPCM(uint8_t* pcm,int pcm_len,int freq,int channels);
    void playMovement(uint8_t& ac,Quaternion& rot,Vector3& pos);
    void destroy();
    
    static void set_path(std::string const&path)
    {
        save_path_=path;
    }
        
private:
    const int                                   index_;
    const int64_t                               uid_;
    
    SharedPtr<Scene>                            scene_;

    //for play back
    SharedPtr<Node>                             node_;
    SharedPtr<BufferedSoundStream>              soundStream_;
    
    
    
    /// The controllable character component.
    /// The controllable character component.
    WeakPtr<Character>                          character_;
    
    //SharedPtr<Mover>                            mover_;
    static std::string                          save_path_;
};
