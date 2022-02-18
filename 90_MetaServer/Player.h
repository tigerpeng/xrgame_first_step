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

using namespace Urho3D;

class Player //: public Object
{
    //URHO3D_OBJECT(Player, Object);
public:
    /// Construct.
    explicit Player(int64_t rid,int64_t uid,Node* n);

    ~Player();

    /// Register object factory and attributes.
    //static void RegisterObject(Context* context);
    
    
    ///  返回玩家的音视频id  uint8_t 循环利用
    int64_t GetRoomID(){return roomID_;}
    int64_t GetUserID(){return uid_;}
    WeakPtr<Node> GetNode(){return node_;}
private:
    const int64_t                       roomID_;
    const int64_t                       uid_;
    
    WeakPtr<Node>                       node_;
};
