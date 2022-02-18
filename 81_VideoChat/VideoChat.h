//
// Copyright (c) 2008-2019 the Urho3D project.
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

#include <map>
#include <list>
#include <string>
#include "VideoComponent.h"
#include "LiveVideo.h"
#include "VUILayout.h"

#include <facepower/api_facepower.h>

#include <SDL/SDL.h>
#include "./webrtc_3a1v/webrtc_3a1v.h"
#include "LiveAudio.h"


namespace Urho3D
{

class Node;
class Scene;
class HttpRequest;

}

enum VIDEO_TYP{
    TALKS_REQUEST=0,
    TALKS_RESPONSE,
    MEETING_CREATOR,
    MEETING_PARTNER,
    EMOTION_REQUEST,
    EMOTION_RESPONSE,
    DUETO_REQUEST,
    DUETO_RESPONSE
};
/// Static 3D scene example.
/// This sample demonstrates:
///     - Creating a 3D scene with static content
///     - Displaying the scene using the Renderer subsystem
///     - Handling keyboard and mouse input to move a freelook camera
class VideoChat : public Sample
{
    URHO3D_OBJECT(VideoChat, Sample);

public:
    /// Construct.
    explicit VideoChat(Context* context);

    /// Setup before engine initialization. Modifies the engine parameters.
    void Setup() override;
    /// Setup after engine initialization and before running the main loop.
    void Start() override;
    
    void Stop() override;
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
    /// Construct an instruction text to the UI.
    void CreateInstructions();
    /// Set up a viewport for displaying the scene.
    void SetupViewport();
    /// Read input and moves the camera.
    void MoveCamera(float timeStep);
    /// Subscribe to application-wide logic update events.
    void SubscribeToEvents();
    /// Handle the logic update event.
    void HandleUpdate(StringHash eventType, VariantMap& eventData);
    
    void HandleKeyUpExit(StringHash /*eventType*/, VariantMap& eventData);
    
    //event
    void OnCommand(std::string const&cmd);
public:
    //void OnAudioFrame(uint8_t* pcm, int len);
private:
    void OnVideoFrame(unsigned char* plane[3],int y_info[3],int uv_info[3]);
    bool InitTexture();
    void UpdatePlaneTextures();
    
    
//    void ControlAudioCapture(void(*pf)(void*,Uint8*,int),void* custom=nullptr,int freq=44100,int channels=2,int samples=1024);
    
    //准备音视频
    void Load2DVideoDemo();
    void Load3DVideoDemo();
    void AVReady();
    


    //add
    //Button* CreateButton(int x, int y, int xSize, int ySize, const String& text);
//    void RequestVTalks(std::string const& peerID);
//    void ResponseVTalks(std::string const& peerID);
    
//    //按钮事件
//    void HandleStart1(StringHash eventType, VariantMap& eventData);
//    void HandleStart2(StringHash eventType, VariantMap& eventData);
//    void HandleRequestAudio(StringHash eventType, VariantMap& eventData);
//    void HandleRequestVideo(StringHash eventType, VariantMap& eventData);
        
//    //meeting
//    void HandlebtnRiseHand(StringHash eventType, VariantMap& eventData);
//    void ShowWaiters();
//    void DownloadingImage();
//    void HandleWaiterPress(StringHash eventType, VariantMap& eventData);
//    void HandleWaiterRelease(StringHash eventType, VariantMap& eventData);
//
//
//    void HandleTouchBegin(StringHash /*eventType*/, VariantMap& eventData);
//    void HandleTouchEnd(StringHash eventType, VariantMap& eventData);
//    void HandleTouchMove(StringHash eventType, VariantMap& eventData);
//
//
//    int64_t GetPictureUID(IntVector2 pt);
private:
    FacePowerPtr                    facepower_;
    bool                            exit_;
    std::string                     bootJson_;
    bool                            portrait_;

    

    //显示视频
    std::shared_ptr<LiveVideo>      liveV_;//real time video
    
    bool                            b2DVideo_;
    //for 2D video
    BorderImage*                    video_box_;
    SharedPtr<Sprite>               sprite_;
    //for 3D video
    SharedPtr<Node>                 playerNode_;
    SharedPtr<VideoComponent>       tvc_;
    
//    //audio
//    // Scene node for the sound component.
//    SharedPtr<Node>     node_;
//    SDL_AudioDeviceID   captureDev_;
//    
//    int                 freq_;
//    int                 channels_;
//    int                 audioCaptureIndex_;
    
    //micphone
    //SharedPtr<BufferedSoundStream>              soundStream_;
    
    

    
    std::shared_ptr<VUILayout>      spLayout_;
    
    //layout
    //SharedPtr<UIElement>        layout_;//统一ui界面的操作
    //SharedPtr<SoundSource>      ringSource_;
    
//    std::string  error_text_;
//    int          error_btns_;
    
        
    //music
    SharedPtr<SoundSource>      musicSource_;
    
//    
//    SharedPtr<Text>             subtitle_;
//    std::string                 strSubTitle_;
//    bool                        subChange_;
    
    std::shared_ptr<webrtc_3a1v>     ns_;
    
    
    //
    std::shared_ptr<live_audio>      la_;
};
