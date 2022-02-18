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

#include "Sample.h"
#include "Player.h"

#include <string>
#include <vector>
#include <queue>
#include <unordered_map>
#include <map>
#include <list>
#include <chrono>

namespace Urho3D
{

class Button;
class Connection;
class Scene;
class Text;
class UIElement;

}

class Touch;
class Room;
/// Scene network replication example.
/// This sample demonstrates:
///     - Creating a scene in which network clients can join
///     - Giving each client an object to control and sending the controls from the clients to the server
///       where the authoritative simulation happens
///     - Controlling a physics object's movement by applying forces
class SceneReplication : public Sample
{
    URHO3D_OBJECT(SceneReplication, Sample);

    struct stAuthorization{
        SharedPtr<HttpRequest>                  httpRequest;
        String                                  message;
        SharedPtr<Connection>                   connection;
        std::chrono::steady_clock::time_point   startTime;
    };
public:
    /// Construct.
    explicit SceneReplication(Context* context);

    /// Setup before engine initialization. Modifies the engine parameters.
    void Setup() override;
    /// Setup after engine initialization and before running the main loop.
    void Start() override;

protected:
    /// Return XML patch instructions for screen joystick layout for a specific sample app, if any.
    String GetScreenJoystickPatchString() const override { return
        "<patch>"
        "    <add sel=\"/element/element[./attribute[@name='Name' and @value='Hat0']]\">"
        "        <attribute name=\"Is Visible\" value=\"false\" />"
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
    /// Create a button to the button container.
    Button* CreateButton(const String& text, int width);
    /// Update visibility of buttons according to connection and server status.
    void UpdateButtons();
//    /// Create a controllable ball object and return its scene node.
//    std::shared_ptr<Player> CreateControllableObject();
    /// Read input and move the camera.
    void MoveCamera();
    /// Handle the physics world pre-step event.
    void HandlePhysicsPreStep(StringHash eventType, VariantMap& eventData);
    /// Handle the logic post-update event.
    void HandlePostUpdate(StringHash eventType, VariantMap& eventData);

    /// Handle pressing the disconnect button.
    void HandleDisconnect(StringHash eventType, VariantMap& eventData);
    /// Handle pressing the start server button.
    void HandleStartServer(StringHash eventType, VariantMap& eventData);
    /// Handle connection status change (just update the buttons that should be shown.)
    void HandleConnectionStatus(StringHash eventType, VariantMap& eventData);
    
    ///  Handle Identity
    void HandleClientIdentity(StringHash eventType, VariantMap& eventData);
    /// Handle a client connecting to the server.
    void HandleClientConnected(StringHash eventType, VariantMap& eventData);
    /// Handle a client disconnecting from the server.
    void HandleClientDisconnected(StringHash eventType, VariantMap& eventData);
    
    void HandleNetworkMessage(StringHash /*eventType*/, VariantMap& eventData);
    void HandleClientSceneLoaded(StringHash /*eventType*/, VariantMap& eventData);
    
    void CreateRobot();
    
    void VerifyUserToken();


    /// Mapping from client connections to controllable objects.
    //HashMap<Connection*, WeakPtr<Node> > serverObjects_;

    
//
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
//    /// Instructions text.
//    SharedPtr<Text> instructionsText_;
//    /// ID of own controllable object (client only.)
//    unsigned clientObjectID_{};
//    /// Packets in per second
//    SharedPtr<Text> packetsIn_;
//    /// Packets out per second
//    SharedPtr<Text> packetsOut_;
//    /// Packet counter UI update timer
//    Timer packetCounterTimer_;
    
    //校验用户token 返回用户信息
    std::list<stAuthorization>  verification_;
    
    //多场景支持
    std::unordered_map<int64_t,SharedPtr<Room>>      rooms_;
    
    
    
    //校验用户信息
    std::map<int64_t,SharedPtr<HttpRequest>>         wait_for_cheking_;
};
