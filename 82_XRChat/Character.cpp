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

#include <Urho3D/Core/Context.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

#include "Character.h"


#include <Urho3D/Audio/Audio.h>
#include <Urho3D/Audio/AudioEvents.h>
#include <Urho3D/Audio/Sound.h>
#include <Urho3D/Audio/SoundSource.h>

#include <spdlog/spdlog.h>
namespace spd = spdlog;

Character::Character(Context* context) :
    LogicComponent(context),
    onGround_(false),
    okToJump_(true),
    inAirTimer_(0.0f)
    ,modelPath_("Avatars/Mutant/")
    ,bStop_(false)
{
    // Only the physics update event is needed: unsubscribe from the rest for optimization
    SetUpdateEventMask(USE_FIXEDUPDATE);
}

void Character::RegisterObject(Context* context)
{
    context->RegisterFactory<Character>();

    // These macros register the class attributes to the Context for automatic load / save handling.
    // We specify the Default attribute mode which means it will be used both for saving into file, and network replication
    URHO3D_ATTRIBUTE("Controls Yaw", float, controls_.yaw_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Controls Pitch", float, controls_.pitch_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("On Ground", bool, onGround_, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE("OK To Jump", bool, okToJump_, true, AM_DEFAULT);
    URHO3D_ATTRIBUTE("In Air Timer", float, inAirTimer_, 0.0f, AM_DEFAULT);
}

void Character::Start()
{
    // Component has been inserted into its scene node. Subscribe to events now
    SubscribeToEvent(GetNode(), E_NODECOLLISION, URHO3D_HANDLER(Character, HandleNodeCollision));
}

void Character::FixedUpdate(float timeStep)
{
    /// \todo Could cache the components for faster access instead of finding them each frame
    auto* body = GetComponent<RigidBody>();
    auto* animCtrl = node_->GetComponent<AnimationController>(true);
    
    // Update the in air timer. Reset if grounded
    if (!onGround_)
        inAirTimer_ += timeStep;
    else
        inAirTimer_ = 0.0f;
    // When character has been in air less than 1/10 second, it's still interpreted as being on ground
    bool softGrounded = inAirTimer_ < INAIR_THRESHOLD_TIME;

    // Update movement & animation
    const Quaternion& rot = node_->GetRotation();
    Vector3 moveDir = Vector3::ZERO;
    const Vector3& velocity = body->GetLinearVelocity();
    // Velocity on the XZ plane
    Vector3 planeVelocity(velocity.x_, 0.0f, velocity.z_);

    if (controls_.IsDown(CTRL_FORWARD))
        moveDir += Vector3::FORWARD;
    if (controls_.IsDown(CTRL_BACK))
        moveDir += Vector3::BACK;
    if (controls_.IsDown(CTRL_LEFT))
        moveDir += Vector3::LEFT;
    if (controls_.IsDown(CTRL_RIGHT))
        moveDir += Vector3::RIGHT;

    // Normalize move vector so that diagonal strafing is not faster
    if (moveDir.LengthSquared() > 0.0f)
        moveDir.Normalize();
    
    
    if(bStop_)
    {
        bStop_=false;
        auto node=GetNode();
        node->SetPosition(stopPos_);
        node->SetRotation(stopRot_);
    }
    
    // If in air, allow control, but slower than when on ground
    body->ApplyImpulse(rot * moveDir * (softGrounded ? MOVE_FORCE : INAIR_MOVE_FORCE));

    if (softGrounded)
    {
        // When on ground, apply a braking force to limit maximum ground velocity
        Vector3 brakeForce = -planeVelocity * BRAKE_FORCE;
        body->ApplyImpulse(brakeForce);

        // Jump. Must release jump control between jumps
        if (controls_.IsDown(CTRL_JUMP))
        {
            if (okToJump_)
            {
                body->ApplyImpulse(Vector3::UP * JUMP_FORCE);
                okToJump_ = false;
                animCtrl->PlayExclusive(jump_.c_str(), 0, false, 0.2f);
            }
        }
        else
            okToJump_ = true;
    }

    if ( !onGround_ )
    {
        animCtrl->PlayExclusive(jump_.c_str(), 0, false, 0.2f);
    }
    else
    {
        // Play walk animation if moving on ground, otherwise fade it out
        if (softGrounded && !moveDir.Equals(Vector3::ZERO))
            animCtrl->PlayExclusive(walk_.c_str(), 0, true, 0.2f);
        else
            animCtrl->PlayExclusive(idl_.c_str(), 0, true, 0.2f);

        // Set walk animation speed proportional to velocity
        animCtrl->SetSpeed(walk_.c_str(), planeVelocity.Length() * 0.3f);
    }

    // Reset grounded flag for next frame
    onGround_ = false;
}

void Character::HandleNodeCollision(StringHash eventType, VariantMap& eventData)
{
    // Check collision contacts and see if character is standing on ground (look for a contact that has near vertical normal)
    using namespace NodeCollision;

    MemoryBuffer contacts(eventData[P_CONTACTS].GetBuffer());

    while (!contacts.IsEof())
    {
        Vector3 contactPosition = contacts.ReadVector3();
        Vector3 contactNormal = contacts.ReadVector3();
        /*float contactDistance = */contacts.ReadFloat();
        /*float contactImpulse = */contacts.ReadFloat();

        // If contact is below node center and pointing up, assume it's a ground contact
        if (contactPosition.y_ < (node_->GetPosition().y_ + 1.0f))
        {
            float level = contactNormal.y_;
            if (level > 0.75)
                onGround_ = true;
        }
    }
}

void Character::playPCM(uint8_t* pcm,int pcm_len,int freq,int channels)
{
    if(soundStream_==nullptr)
    {
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
