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

#include <Urho3D/Input/Controls.h>
#include <Urho3D/Scene/LogicComponent.h>
#include <Urho3D/Network/Connection.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/Scene/Scene.h>

#include <queue>

#include "Player.h"

using namespace Urho3D;

class Room: public Object
{
    URHO3D_OBJECT(Room, Object);
public:
    /// Construct.
    explicit Room(Context* context);

    ~Room();
    
    /// Register object factory and attributes.
    static void RegisterObject(Context* context);
public:
    void AddPlayer(int64_t rid,int64_t uid,String& name,Connection* c,String const& avatar);
    void RemovePlayer(Connection* c);
    
    void OnClientSceneLoaded(StringHash /*eventType*/, VariantMap& eventData);
    void OnClientSceneLoaded(Connection* c);
    void OnClientNetworkMessage(Connection* sender,int msgID,PODVector<unsigned char> data);
private:
    unsigned ProduceVoiceID();
    void     ReuseVoiceID(unsigned id);
    
    void HandlePhysicsPreStep(StringHash eventType, VariantMap& eventData);
    void CreateScene();
    std::shared_ptr<Player> CreateControllableObject(int64_t rid,int64_t uid,String& name,String const& avatar="");
    void CreateTV();
    

    
private:
    /// Scene.
    SharedPtr<Scene>                                        scene_;
    
    std::queue<unsigned>                                    playerVoiceIDs_;
    ///
    HashMap<Connection*, std::shared_ptr<Player> >          serverObjects_;
    
    
    //SharedPtr<HttpRequest>                                  httpRequest_;
};
