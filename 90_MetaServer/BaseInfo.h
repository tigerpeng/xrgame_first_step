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
#include <Urho3D/Scene/LogicComponent.h>


#include "Urho3DAll.h"
#include <memory>
#include <thread>
#include <mutex>
#include <map>
#include <SDL/SDL.h>


using namespace Urho3D;


/// Character component, responsible for physical movement according to controls, as well as animation.
class BaseInfomation : public LogicComponent
{
    URHO3D_OBJECT(BaseInfomation, LogicComponent);

public:
    /// Construct.
    explicit BaseInfomation(Context* context)
    :LogicComponent(context)
    {
        
    }

    /// Register object factory and attributes.
    static void RegisterObject(Context* context)
    {
        context->RegisterFactory<BaseInfomation>();
        
        URHO3D_ATTRIBUTE("Player Name",String,name_,String::EMPTY,AM_DEFAULT );
        URHO3D_ATTRIBUTE("Player Age", int, age_, 11, AM_DEFAULT);
    }

    /// Handle startup. Called by LogicComponent base class.
    void Start() override
    {
        
    }
    /// Handle physics world update. Called by LogicComponent base class.
    void FixedUpdate(float timeStep) override
    {
        
    }
    
public:
    String          name_;
    int             age_;
private:
    
};
