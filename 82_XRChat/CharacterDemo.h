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

#include "Urho3DAll.h"

#include "Sample.h"

#include "Player.h"
#include <unordered_map>

#include <queue>
#include <string>
#include <thread>
#include <mutex>


//音视频编解码/网络接口
#include <facepower/api_facepower.h>
//音频
#include "./av/webrtc_3a1v/webrtc_3a1v.h"
#include "./av/LiveAudio.h"
#include "./av/LiveVideo.h"


namespace Urho3D
{

class Node;
class Scene;

}

class Character;
class Touch;

/// Moving character example.
/// This sample demonstrates:
///     - Controlling a humanoid character through physics
///     - Driving animations using the AnimationController component
///     - Manual control of a bone scene node
///     - Implementing 1st and 3rd person cameras, using raycasts to avoid the 3rd person camera clipping into scenery
///     - Defining attributes of a custom component so that it can be saved and loaded
///     - Using touch inputs/gyroscope for iOS/Android (implemented through an external file)
class CharacterDemo : public Sample
{
    URHO3D_OBJECT(CharacterDemo, Sample);

public:
    /// Construct.
    explicit CharacterDemo(Context* context);
    /// Destruct.
    ~CharacterDemo() override;

    /// Setup before engine initialization. Modifies the engine parameters.
    void Setup() override;
    /// Setup after engine initialization and before running the main loop.
    void Start() override;

    
protected:
    /// Return XML patch instructions for screen joystick layout for a specific sample app, if any.
    String GetScreenJoystickPatchString() const override { return
        "<patch>"
        "    <add sel=\"/element\">"
        "        <element type=\"Button\">"
        "            <attribute name=\"Name\" value=\"Button3\" />"
        "            <attribute name=\"Position\" value=\"-120 -120\" />"
        "            <attribute name=\"Size\" value=\"96 96\" />"
        "            <attribute name=\"Horiz Alignment\" value=\"Right\" />"
        "            <attribute name=\"Vert Alignment\" value=\"Bottom\" />"
        "            <attribute name=\"Texture\" value=\"Texture2D;Textures/TouchInput.png\" />"
        "            <attribute name=\"Image Rect\" value=\"96 0 192 96\" />"
        "            <attribute name=\"Hover Image Offset\" value=\"0 0\" />"
        "            <attribute name=\"Pressed Image Offset\" value=\"0 0\" />"
        "            <element type=\"Text\">"
        "                <attribute name=\"Name\" value=\"Label\" />"
        "                <attribute name=\"Horiz Alignment\" value=\"Center\" />"
        "                <attribute name=\"Vert Alignment\" value=\"Center\" />"
        "                <attribute name=\"Color\" value=\"0 0 0 1\" />"
        "                <attribute name=\"Text\" value=\"Gyroscope\" />"
        "            </element>"
        "            <element type=\"Text\">"
        "                <attribute name=\"Name\" value=\"KeyBinding\" />"
        "                <attribute name=\"Text\" value=\"G\" />"
        "            </element>"
        "        </element>"
        "    </add>"
        "    <remove sel=\"/element/element[./attribute[@name='Name' and @value='Button0']]/attribute[@name='Is Visible']\" />"
        "    <replace sel=\"/element/element[./attribute[@name='Name' and @value='Button0']]/element[./attribute[@name='Name' and @value='Label']]/attribute[@name='Text']/@value\">1st/3rd</replace>"
        "    <add sel=\"/element/element[./attribute[@name='Name' and @value='Button0']]\">"
        "        <element type=\"Text\">"
        "            <attribute name=\"Name\" value=\"KeyBinding\" />"
        "            <attribute name=\"Text\" value=\"F\" />"
        "        </element>"
        "    </add>"
        "    <remove sel=\"/element/element[./attribute[@name='Name' and @value='Button1']]/attribute[@name='Is Visible']\" />"
        "    <replace sel=\"/element/element[./attribute[@name='Name' and @value='Button1']]/element[./attribute[@name='Name' and @value='Label']]/attribute[@name='Text']/@value\">Jump</replace>"
        "    <add sel=\"/element/element[./attribute[@name='Name' and @value='Button1']]\">"
        "        <element type=\"Text\">"
        "            <attribute name=\"Name\" value=\"KeyBinding\" />"
        "            <attribute name=\"Text\" value=\"SPACE\" />"
        "        </element>"
        "    </add>"
        "</patch>";
    }
    
private:
    struct stObjIDL{
        stObjIDL():ac(0),rot(217,0,579),pos(217,0,579)
        {}
        
        uint8_t     ac;
        Quaternion  rot;
        Vector3     pos;
    };
    
    void AVReady();
    void OnCommand(std::string const& cmdStr);
    void OnMover(int64_t index,uint8_t* pData,int len);
    void SendAction(uint8_t mvByte);
    
    //add by copyleft
    void SetModel(int index,std::string const& avatar);
    Character* CreateModel(int index,std::string const& avatar="");
    
    void CreateTV();
    void CreateRobot();
    void LoadPlayer(int64_t uid);
    
    
    void PlayerLeave(int index,int64_t uid=0);
    
    void ParseCommand();
    
    
    

    //std::unordered_map<int64_t,std::shared_ptr<player>>  players_;
    

    
    
    FacePowerPtr                        facepower_;
    bool                                exit_;
    std::string                         bootJson_;
    bool                                portrait_;
    
    std::queue<std::string>             cmd_que_;
    std::mutex                          cmd_mutex_;
    

    std::shared_ptr<live_audio>         la_;
    std::shared_ptr<webrtc_3a1v>        ns_;
    std::shared_ptr<LiveVideo>          lv_;
    
    
    //
    //SharedPtr<HttpRequest>                avatarRequest_;
    //下载列表
    //std::vector<std::string>              downloadings_;

private:
    /// Create static scene content.
    void CreateScene();
    /// Create controllable character.
    void CreateCharacter(std::string const& avatar="def");
    void CreateCharacter2();
    /// Construct an instruction text to the UI.
    void CreateInstructions();
    /// Subscribe to necessary events.
    void SubscribeToEvents();
    /// Handle application update. Set controls to character.
    void HandleUpdate(StringHash eventType, VariantMap& eventData);
    /// Handle application post-update. Update camera position after character has moved.
    void HandlePostUpdate(StringHash eventType, VariantMap& eventData);

    /// Touch utility object.
    SharedPtr<Touch> touch_;
    /// The controllable character component.
    WeakPtr<Character> character_;
    /// First person camera flag.
    bool    firstPerson_;
    int     myIndex_;
};
