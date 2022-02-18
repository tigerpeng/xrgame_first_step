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

#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/AnimationState.h>
#include <Urho3D/Scene/Scene.h>

#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Graphics/AnimationController.h>


#include "Player.h"

Player::Player(int64_t rid,int64_t uid,Node* n)
:roomID_(rid)
,uid_(uid)
,node_(n)
//Object(context)
{

    //SubscribeToEvent(E_PHYSICSPRESTEP, URHO3D_HANDLER(Player, HandlePhysicsPrestep));
}

Player::~Player()
{
    if(node_)
        node_->Remove();
}
//void Player::RegisterObject(Context* context)
//{
//    context->RegisterFactory<Player>();
//}

//to do
