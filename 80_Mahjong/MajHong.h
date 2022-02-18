//
// Copyright (c) 2008-2017 the Urho3D project.
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

#include "desk.h"
#include <chrono>
#include <map>
#include <string>

namespace Urho3D
{
class Drawable;
class Node;
class Scene;
    
class Button;
class Slider;
}


class Mahjong : public Sample,public Desk
{
    URHO3D_OBJECT(Mahjong, Sample);

public:
    /// Construct.
    Mahjong(Context* context);

    /// Setup before engine initialization. Modifies the engine parameters.
    void Setup() override;
    /// Setup after engine initialization and before running the main loop.
    void Start() override;
    void Stop() override;
    
    
    //for desk
    void NewRound() override;
    void GameReady(std::string const& jsonInfo) override;
    void SetPlayerInfo(std::vector<std::string>& infos,std::vector<std::string>& avatars,int64_t rid,int myPos,std::string const& rToken,std::vector<DeskDir> dirs) override;
    
    void SetDeskInfo(std::string const& name
                     ,std::string const& infoStatic
                     ,int cardNumber=-1);
    //void LoadReady();
    void SetDeskInfo(DeskDir player,int cards=-1) override;
    void CreateButtons(std::string const& btns) override;
    Node* CreateMahJong(int32_t card,Vector3 pos,Quaternion rota=Quaternion(0, 0, 0)) override;
    void ModifyNode(Node* pNode,int32_t value) override;
    void PlayMjSound(std::string const&  soundFile,int pos=-1) override;
    void ShowRoundResult(ptree& pt,std::string const& title="") override;
    void ShowGameResult(int excess ,std::vector<std::string>& vPlayers) override;
    void ShowButton(std::string const& btns,int32_t seconds) override;
    
    void ShowGameOver(ptree& pt) override;
    
    void ShowHuInfo(std::map<int,int>& huPai) override;
    void ShowTingInfo(std::map<int,int>& huPai) override;
    
    void AddMark(Node* pNode,int value,std::string const& mark) override;
    
    void ShowErrorAndExit(std::string const& message) override;

protected:
    SoundSource* musicSource_;
    
    /// Return XML patch instructions for screen joystick layout for a specific sample app, if any.
    String GetScreenJoystickPatchString() const override { return
        "<patch>"
        "    <add sel=\"/element/element[./attribute[@name='Name' and @value='Hat0']]\">"
        "        <attribute name=\"Is Visible\" value=\"false\" />"
        "    </add>"
        "</patch>";
    }

private:
    /// Create the UI and subscribes to UI events.
    void CreateUI();
    /// Create a button at position with specified text in it.
    Button* CreateButton(int x, int y, int xSize, int ySize, const String& text);
    /// Create a horizontal slider with specified text above it.
    Slider* CreateSlider(int x, int y, int xSize, int ySize, const String& text);
    
    ProgressBar* CreateProgressBar(int x, int y, int xSize, int ySize);
    /// Handle a sound effect button click.
    //void HandlePlaySound(StringHash eventType, VariantMap& eventData);
    /// Handle "play music" button click.
    void HandlePlayMusic(StringHash eventType, VariantMap& eventData);
    void HandleBntSpeakerOn(StringHash eventType, VariantMap& eventData);
    void HandleBntSpeakerOff(StringHash eventType, VariantMap& eventData);
    
    /// Handle sound effects volume slider change.
    //void HandleSoundVolume(StringHash eventType, VariantMap& eventData);
    /// Handle music volume slider change.
    void HandleMusicVolume(StringHash eventType, VariantMap& eventData);
   
    
    
    
    //added
    void CreateScene();
    /// Set up a viewport for displaying the scene.
    void SetupViewport();
    /// Subscribe to application-wide logic update events.
    void SubscribeToEvents();
    /// Read input and moves the camera.
    void MoveCamera(float timeStep);
    /// Handle the logic update event.
    void HandleUpdate(StringHash eventType, VariantMap& eventData);
    
    void HandleTouchBegin(StringHash /*eventType*/, VariantMap& eventData);
    void HandleTouchEnd(StringHash eventType, VariantMap& eventData);
    void HandleTouchMove(StringHash eventType, VariantMap& eventData);
    bool Raycast(IntVector2& touchPos,float maxDistance, Vector3& hitPos, Drawable*& hitDrawable);
    void SelectCard(IntVector2& touchPos,NTouch dir);
    
    void HandleAction(StringHash eventType, VariantMap& eventData);
    
    void HandlePiaoAction(StringHash eventType, VariantMap& eventData);
    
    void SetCardFree(int number);
    
    IntRect Create2DMahjong(UIElement* pParent,int value,int state,int xPos,int yPos,float scal);

    
    Texture2D* GetAvatarTexture(std::string& avatar);
    //退出和继续游戏
    void HandleKeyUpExit(StringHash /*eventType*/, VariantMap& eventData);
    void HandleExit(StringHash eventType, VariantMap& eventData);
    void HandleShare(StringHash eventType, VariantMap& eventData);
    void HandleContinue(StringHash eventType, VariantMap& eventData);
    
    void CreateRoundResultRow(SharedPtr<UIElement> pRoot,int index,bool zhuang,std::string& avatar,std::string const& name
                              ,std::string const&piaoDesp,std::vector<int> cardsPub,std::vector<int> cardsHand,int card
                              ,std::string const&total,std::string const&score,std::string const&result="");


    
    std::chrono::steady_clock::time_point               timerEnd_;
    bool                                                countDown_;
    
    bool                                                bMusicOn_;

    
    SharedPtr<UIElement>                                layoutDesk_;
    SharedPtr<UIElement>                                layoutReady_;
    SharedPtr<UIElement>                                layoutPlayerInfo_;
    SharedPtr<UIElement>                                roundResult_;

    
    SharedPtr<Text>                                     deskName_;
    SharedPtr<Text>                                     deskInfo_;

    
    std::map<std::string,Button*>                       btnMaps_;
    ProgressBar*                                        pProgressBar_;
    
    IntVector2                                          touchStart_;
    
    StaticModel*                                        deskObject_;
    
    std::string                                         avServer_;
    std::string                                         token_;
    
    bool                                                bAdjustCamera_;
    
    //调整音量
    float     volume_music_,volume_effect_;
};


