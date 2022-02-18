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

#pragma once

#include <string>
#include <Urho3D/Input/Controls.h>
#include <Urho3D/Scene/LogicComponent.h>


#include "Urho3DAll.h"
#include <memory>
#include <thread>
#include <mutex>
#include <SDL/SDL.h>


using namespace Urho3D;

const unsigned CTRL_FORWARD = 1;
const unsigned CTRL_BACK = 2;
const unsigned CTRL_LEFT = 4;
const unsigned CTRL_RIGHT = 8;
const unsigned CTRL_JUMP = 16;

const float MOVE_FORCE = 0.8f; //0.8f
const float INAIR_MOVE_FORCE = 0.02f;
const float BRAKE_FORCE = 0.2f;
const float JUMP_FORCE = 7.0f;
const float YAW_SENSITIVITY = 0.1f;
const float INAIR_THRESHOLD_TIME = 0.1f;

/// Character component, responsible for physical movement according to controls, as well as animation.
class Character : public LogicComponent
{
    URHO3D_OBJECT(Character, LogicComponent);

public:
    /// Construct.
    explicit Character(Context* context);

    /// Register object factory and attributes.
    static void RegisterObject(Context* context);

    /// Handle startup. Called by LogicComponent base class.
    void Start() override;
    /// Handle physics world update. Called by LogicComponent base class.
    void FixedUpdate(float timeStep) override;
    
    
    
    
    //add by copyleft
    void playPCM(uint8_t* pcm,int pcm_len,int freq,int channels);
    
    void SetAvatar(std::string const& path)
    {
        modelPath_=path;
        
        idl_    =modelPath_+"idl.ani";
        walk_   =modelPath_+"walk.ani";
        jump_   =modelPath_+"jump.ani";
    }
    void SetStop(Vector3& pos,Quaternion& rot)
    {
        bStop_=true;
        stopPos_=pos;
        stopRot_=rot;
    }
    /// Movement controls. Assigned by the main program each frame.
    Controls controls_;
private:
    /// Handle physics collision event.
    void HandleNodeCollision(StringHash eventType, VariantMap& eventData);

    /// Grounded flag for movement.
    bool onGround_;
    /// Jump flag.
    bool okToJump_;
    /// In air timer. Due to possible physics inaccuracy, character can be off ground for max. 1/10 second and still be allowed to move.
    float inAirTimer_;
    
    
    std::string modelPath_;
    
    std::string idl_;
    std::string walk_;
    std::string jump_;
    
    bool        bStop_;
    Vector3     stopPos_;
    Quaternion  stopRot_;
    
    
    SharedPtr<BufferedSoundStream>              soundStream_;
};
