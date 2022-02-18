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

#pragma once

#include <Urho3D/Engine/Application.h>
#include <Urho3D/Input/Input.h>


//音视频编解码/网络接口 add by copyleft
#include <facepower/api_facepower.h>
#include "LiveVideo.h"

#include <queue>
#include <string>
#include <thread>
#include <mutex>
#include <chrono>

//boost
#include <boost/format.hpp>
//fro xml and json
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
using namespace boost::property_tree;


namespace Urho3D
{

class Node;
class Scene;
class Sprite;

}

// All Urho3D classes reside in namespace Urho3D
using namespace Urho3D;

const float TOUCH_SENSITIVITY = 2.0f;

/// Sample class, as framework for all samples.
///    - Initialization of the Urho3D engine (in Application class)
///    - Modify engine parameters for windowed mode and to show the class name as title
///    - Create Urho3D logo at screen
///    - Set custom window title and icon
///    - Create Console and Debug HUD, and use F1 and F2 key to toggle them
///    - Toggle rendering options from the keys 1-8
///    - Take screenshot with key 9
///    - Handle Esc key down to hide Console or exit application
///    - Init touch input on mobile platform using screen joysticks (patched for each individual sample)
class Sample : public Application
{
    // Enable type information.
    URHO3D_OBJECT(Sample, Application);

public:
    /// Construct.
    explicit Sample(Context* context);

    /// Setup before engine initialization. Modifies the engine parameters.
    void Setup() override;
    /// Setup after engine initialization. Creates the logo, console & debug HUD.
    void Start() override;
    /// Cleanup after the main loop. Called by Application.
    void Stop() override;

protected:
    /// Return XML patch instructions for screen joystick layout for a specific sample app, if any.
    virtual String GetScreenJoystickPatchString() const { return String::EMPTY; }
    /// Initialize touch input on mobile platform.
    void InitTouchInput();
    /// Initialize mouse mode on non-web platform.
    void InitMouseMode(MouseMode mode);
    /// Control logo visibility.
    void SetLogoVisible(bool enable);

    /// Logo sprite.
    SharedPtr<Sprite> logoSprite_;
    /// Scene.
    SharedPtr<Scene> scene_;
    /// Camera scene node.
    SharedPtr<Node> cameraNode_;
    /// Camera yaw angle.
    float yaw_;
    /// Camera pitch angle.
    float pitch_;
    /// Flag to indicate whether touch input has been enabled.
    bool touchEnabled_;
    /// Mouse mode option to use in the sample.
    MouseMode useMouseMode_;

private:
    /// Create logo.
    void CreateLogo();
    /// Set custom window Title & Icon
    void SetWindowTitleAndIcon();
    /// Create console and debug HUD.
    void CreateConsoleAndDebugHud();
    /// Handle request for mouse mode on web platform.
    void HandleMouseModeRequest(StringHash eventType, VariantMap& eventData);
    /// Handle request for mouse mode change on web platform.
    void HandleMouseModeChange(StringHash eventType, VariantMap& eventData);
    /// Handle key down event to process key controls common to all samples.
    void HandleKeyDown(StringHash eventType, VariantMap& eventData);
    /// Handle key up event to process key controls common to all samples.
    void HandleKeyUp(StringHash eventType, VariantMap& eventData);
    /// Handle scene update event to control camera's pitch and yaw for all samples.
    void HandleSceneUpdate(StringHash eventType, VariantMap& eventData);
    /// Handle touch begin event to initialize touch input on desktop platform.
    void HandleTouchBegin(StringHash eventType, VariantMap& eventData);

    /// Screen joystick index for navigational controls (mobile platforms only).
    unsigned screenJoystickIndex_;
    /// Screen joystick index for settings (mobile platforms only).
    unsigned screenJoystickSettingsIndex_;
    /// Pause flag.
    bool paused_;
    

protected:
    void AVReady(std::string const& avServer,std::string const& verify,std::string const& nodeName="",int matIndex=0);
    void SetSpeaker(bool b)
    {
        broadcasting_audio_=b;
    }
    void SetTouchRotate(bool d)
    {
        touch_rotate_=d;
    }
    virtual void OnXRCommand(ptree pt){}
    
    void PreParseXRCommand();
    
    unsigned                            clientObjectID_{};
    FacePowerPtr                        facepower_;
private:
    void CheckSouderChange();
    void RecordXRCommand(std::string const& cmdStr);
   
    void BroadCastingVideo(bool cast=true);

    //分组的语音视频以及权限管理
    std::shared_ptr<IXRGroup>           xrGroup_;
    
    bool                                broadcasting_audio_;
    std::shared_ptr<live_audio>         la_;
    std::shared_ptr<LiveVideo>          lv_;
    
    std::queue<std::string>             cmd_que_;
    std::mutex                          cmd_mutex_;
    bool                                avReady_;
    bool                                bMyEcho_;
    
    bool                                touch_rotate_;
    

    std::string                         tv_name_;
    bool                                broadcasting_video_;
    std::chrono::steady_clock::time_point           timer_counter_;
};

#include "SampleXR.inl"
