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

//#include "Sample.h"
#include "../SampleXR.h"





////音频
//#include "./av/webrtc_3a1v/webrtc_3a1v.h"
//#include "./av/LiveAudio.h"
//#include "./av/LiveVideo.h"


namespace Urho3D
{

class Button;
class Connection;
class Scene;
class Text;
class UIElement;

}


class Touch;
/// Scene network replication example.
/// This sample demonstrates:
///     - Creating a scene in which network clients can join
///     - Giving each client an object to control and sending the controls from the clients to the server
///       where the authoritative simulation happens
///     - Controlling a physics object's movement by applying forces
class SceneReplication : public Sample
{
    URHO3D_OBJECT(SceneReplication, Sample);

public:
    /// Construct.
    explicit SceneReplication(Context* context);
    
    ~SceneReplication();

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
        "    <replace sel=\"/element/element[./attribute[@name='Name' and @value='Button1']]/element[./attribute[@name='Name' and @value='Label']]/attribute[@name='Text']/@value\">跳跃</replace>"
        "    <add sel=\"/element/element[./attribute[@name='Name' and @value='Button1']]\">"
        "        <element type=\"Text\">"
        "            <attribute name=\"Name\" value=\"KeyBinding\" />"
        "            <attribute name=\"Text\" value=\"SPACE\" />"
        "        </element>"
        "    </add>"
        "</patch>";
    }
private:
    /// Construct the scene content.
    void CreateScene();
    /// Construct instruction text and the login / start server UI.
    void CreateUI();
    /// Set up viewport.
    void SetupViewport();
    /// Subscribe to update, UI and network events.
    void SubscribeToEvents();
//    /// Create a button to the button container.
//    Button* CreateButton(const String& text, int width);
//    /// Update visibility of buttons according to connection and server status.
//    void UpdateButtons();
//    /// Create a controllable ball object and return its scene node.
//    Node* CreateControllableObject();
    /// Read input and move the camera.
    void MoveCamera();
    /// Handle the physics world pre-step event.
    void HandlePhysicsPreStep(StringHash eventType, VariantMap& eventData);
    /// Handle the logic post-update event.
    void HandlePostUpdate(StringHash eventType, VariantMap& eventData);
    /// Handle pressing the connect button.
    void HandleConnect(StringHash eventType, VariantMap& eventData);
    /// Handle pressing the connect button.
    void HandleDisconnect(StringHash eventType, VariantMap& eventData);
    /// Handle connection status change (just update the buttons that should be shown.)
    void HandleConnectionStatus(StringHash eventType, VariantMap& eventData);
//    /// Handle a client connecting to the server.
//    void HandleClientConnected(StringHash eventType, VariantMap& eventData);
//    /// Handle a client disconnecting from the server.
//    void HandleClientDisconnected(StringHash eventType, VariantMap& eventData);
    /// Handle remote event from server which tells our controlled object node ID.
    void HandleClientObjectID(StringHash eventType, VariantMap& eventData);
    
    
    int  OnEncodeAACFrame(uint8_t* buf,int len,int type);
    void CheckSouderChange();
    void HandleNetworkMessage(StringHash /*eventType*/, VariantMap& eventData);
    
    
    /// Mapping from client connections to controllable objects.
   // HashMap<Connection*, WeakPtr<Node> > serverObjects_;
   
//    /// Button container element.
//    SharedPtr<UIElement> buttonContainer_;
//    /// Server address line editor element.
//    SharedPtr<LineEdit> textEdit_;
//    /// Connect button.
//    SharedPtr<Button> connectButton_;
//    /// Disconnect button.
//    SharedPtr<Button> disconnectButton_;
//    /// Start server button.
//    SharedPtr<Button> startServerButton_;
    
    
    /// Instructions text.
    SharedPtr<Text> instructionsText_;
    /// ID of own controllable object (client only.)

    /// Packets in per second
    SharedPtr<Text> packetsIn_;
    /// Packets out per second
    SharedPtr<Text> packetsOut_;
    SharedPtr<Text> myInfo_;
    /// Packet counter UI update timer
    Timer packetCounterTimer_;
    
    
    Controls controls_;
    /// Touch utility object.
    SharedPtr<Touch> touch_;
    /// First person camera flag.
    bool firstPerson_;
    
    
    
  
//add by copyleft
private:
    void OnXRCommand(ptree pt) override;
    
    /// del
    void FixRobotSound();



    bool                                exit_;
    std::string                         bootJson_;
    bool                                portrait_;
    
    
    String                              game_server_;       //游戏服务器
    int64_t                             room_id_;           //房间id
    std::string                         av_server_;      //音视频服务器id
    std::string                         token_;

    

    
        

    
    
    //使用内置的音频引擎
    bool                                use_internal_av_;
        
    SharedPtr<SoundSource>              backgroudMusic_;
    
    

};
