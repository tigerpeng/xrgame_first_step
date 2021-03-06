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

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Zone.h>


#include <Urho3D/Audio/Audio.h>
#include <Urho3D/Audio/AudioEvents.h>
#include <Urho3D/Audio/Sound.h>
#include <Urho3D/Audio/SoundSource.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/ValueAnimation.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Slider.h>
#include <Urho3D/UI/ProgressBar.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>

////add 2d sprite
//#include <Urho3D/Urho2D/Sprite2D.h>
//#include <Urho3D/Urho2D/StaticSprite2D.h>
//#include <Urho3D/Urho2D/StretchableSprite2D.h>

#include "MajHong.h"
#include "Rotator.h"

#include <Urho3D/DebugNew.h>

//boost
#include <boost/format.hpp>
//fro xml and json
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
using namespace boost::property_tree;


//#include <SDL/SDL.h>

//???????????????
#include <iostream>
#include <vector>
#include <iterator>
#include <regex>
#include "url_encoder.h"

/*
 ???delim????????????????????????????????????in??????????????????????????????????????????
 delim ?????????????????????????????????
 */
std::vector<std::string> s_split(const std::string& in, const std::string& delim) {
    std::regex re{ delim };
    // ?????? std::vector::vector (InputIterator first, InputIterator last,const allocator_type& alloc = allocator_type())
    // ????????????,?????????????????????
    return std::vector<std::string> {
        std::sregex_token_iterator(in.begin(), in.end(), re, -1),
        std::sregex_token_iterator()
    };
}

static const StringHash VAR_BUTTONVALUE("ButtonValue");
static const StringHash VAR_PIAOVALUE("PiaoValue");

URHO3D_DEFINE_APPLICATION_MAIN(Mahjong)

Mahjong::Mahjong(Context* context)
:Sample(context)
,musicSource_(0)
,countDown_(false)
,bMusicOn_(false)
,bAdjustCamera_(false)
{
    // Register an object factory for our custom Rotator component so that we can create them to scene nodes
    context->RegisterFactory<Rotator>();
    
    //avserer
    SetSpeaker(false);
    volume_music_=0;
    volume_effect_=0;
}

void Mahjong::Setup()
{
    // Modify engine startup parameters
    Sample::Setup();
    engineParameters_[EP_SOUND] = true;
}
void Mahjong::Stop()
{
    
}

void Mahjong::Start()
{   //????????????
    std::string serverURL;
    std::string parameter;
    Vector<String> vParameter=GetArguments();
    for(auto c:vParameter)
    {
        parameter+=c.CString();
        parameter+=" ";
    }
    
    //????????????
    std::string strJson=URL_TOOLS::UrlDecode(URL_TOOLS::base64_decode(parameter));
   
    if(strJson.empty())
    {   //mac ?????????????????? //\"tblNO\":6000,
        //type ?????????????????? 2 ??? 3 ???
        strJson="{\"cmd\":\"login\",\"type\":3,\"wait\":\"true\",\"uid\":10001,\"db\":\"http://db.ournet.club:2021/\",\"tracker\":\"p4sp://2020@tracker.ournet.club:9700/mj\",\"token\":\"Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1bmlxdWVfbmFtZSI6IjEwMDAxIiwibmJmIjoxNjQ0OTEyNjM5LCJleHAiOjE2NDc1MDQ2MzksImlhdCI6MTY0NDkxMjYzOX0.Zrr4ctQfSUs_uNPrrwMlZZI10xOHGiPY4PvKdruiaH0\",\"profile\":{\"uid\":\"10001\",\"avatar\":\"1.jpg\",\"name\":\"??????\",\"sex\":0,\"birthday\":\"2000-01-24\"}}";
    }
    
    try
    {
        ptree pt;
        std::stringstream ss(strJson);
        read_json(ss, pt);
        serverURL=pt.get("tracker","p4sp://2020@tracker.ournet.club:9700/mj");
        avServer_=pt.get("av","2020@tracker.ournet.club:9700");
        
        //?del
        token_=pt.get("token",token_);
    }catch (ptree_error & e) {
    }catch (...) {
    }
    
    // Execute base class startup
    Sample::Start();        //??????
    SetLogoVisible(false);
    SetTouchRotate(false);   //????????????????????????

    NewRound();
    ConnectNetWork(serverURL,strJson);
}
//????????????
void Mahjong::GameReady(std::string const& jsonInfo)
{
    Node* node=scene_->GetChild("Dice");
    if(node)
        node->SetPosition(Vector3(0.f, 15.0f,0.f));
    
    UIElement* root         = GetSubsystem<UI>()->GetRoot();
    Graphics* graphics      = GetSubsystem<Graphics>();
    ResourceCache* cache    = GetSubsystem<ResourceCache>();
    
    if(layoutReady_==nullptr)
    {
        if(layoutPlayerInfo_)
            layoutPlayerInfo_->RemoveAllChildren();
        
        if(roundResult_)
            roundResult_->SetVisible(false);
//        //????????????
//        GetSubsystem<UI>()->Clear();
//        if(scene_)
//            scene_->Clear();
        

        //??????xml ??????
        auto layout=cache->GetResource<XMLFile>("UI/mj_gameReady.xml");
        layoutReady_=GetSubsystem<UI>()->LoadLayout(layout);
        root->AddChild(layoutReady_);
        
        //???????????? ??????
        auto ready=layoutReady_->GetChildStaticCast<UIElement>("WIN_PIAO",true);
        if(ready)
            ready->SetVisible(false);
        //??????????????????
        for(int k=0;k<4;k++)
        {
            std::string btnName=str(boost::format("BTN_P%d")%k);
            Button* pButton=layoutReady_->GetChildStaticCast<Button>(btnName.c_str(),true);
            SubscribeToEvent(pButton, E_PRESSED, URHO3D_HANDLER(Mahjong, HandlePiaoAction));
        }
        //????????????
        Button* pButton=layoutReady_->GetChildStaticCast<Button>("BTN_LEAVE",true);
        SubscribeToEvent(pButton, E_PRESSED, URHO3D_HANDLER(Mahjong, HandleExit));
        
        //????????????
        for(int j=0;j<4;j++)
        {
            std::string subWinName=str(boost::format("winR_%d")%j);
            auto ready=layoutReady_->GetChildStaticCast<UIElement>(subWinName.c_str(),true);
            ready->SetVisible(false);
        }
    }
    
    try
    {
        ptree pt;
        std::stringstream ss(jsonInfo);
        read_json(ss, pt);
        const int32_t action = pt.get<int32_t>("action");
        
        if(UserAction::CheckChips==action||UserAction::CheckChipsOver==action)
        {
            std::string message=pt.get("message","");
            auto txt=layoutReady_->GetChildStaticCast<Text>("TXT_TITLE",true);
            if(txt)
                txt->SetText(message.c_str());
            
            return ;
        }
        

        std::string deskName    =pt.get("deskProfile.name","");
        int excess              =pt.get("deskProfile.name.excess",2);
        
        size_t   totalPlayers   =pt.get("total",3);
        ptree gambler_array     =pt.get_child("players");  // get_child??????????????????
        
        std::string tilte=pt.get("title","");
        std::string message=pt.get("message","");
        
        
        //??????
        auto txt=layoutReady_->GetChildStaticCast<Text>("TXT_TITLE",true);
        if(txt)
            txt->SetText(tilte.c_str());
        //??????
           
        int i=0;
        BOOST_FOREACH(boost::property_tree::ptree::value_type &v, gambler_array)
        {
            int64_t uid         =v.second.get("id",int64_t(0));
            std::string name    =v.second.get("name","");
            int sex             =v.second.get("sex",0);
            std::string avatar  =v.second.get("avatar","");
                        avatar  =Gambler::GetAvatar(sex,uid,avatar);
            
            //????????????
            std::string subWinName=str(boost::format("winR_%d")%i);
            auto ready=layoutReady_->GetChildStaticCast<UIElement>(subWinName.c_str(),true);
            ready->SetVisible(true);
            //????????????
            std::string txtName=str(boost::format("txt_%d")%i);
            auto txt=layoutReady_->GetChildStaticCast<Text>(txtName.c_str(),true);
            txt->SetText(name.c_str());

            //????????????
            std::string picName=str(boost::format("pic_%d")%i);
            auto pic=layoutReady_->GetChildStaticCast<BorderImage>(picName.c_str(),true);
            if(pic)
                pic->SetTexture(GetAvatarTexture(avatar));
            
            i++;
        }
        
        //????????? ??????
        if(gambler_array.size()==totalPlayers)
        {
            auto ready=layoutReady_->GetChildStaticCast<UIElement>("WIN_PIAO",true);
            ready->SetVisible(true);
            
            std::vector<int> piaoBase;
            ptree piaoArray     =pt.get_child("deskProfile.piaoBase");
            BOOST_FOREACH(boost::property_tree::ptree::value_type &v,piaoArray)
            {
                 int32_t piaoFen=v.second.get_value<int32_t>();
                 piaoBase.push_back(piaoFen);
            }
            
            for(size_t k=0;k<piaoBase.size();++k)
            {
                std::string txtName=str(boost::format("piao_%d")%k);
                auto txt=layoutReady_->GetChildStaticCast<Text>(txtName.c_str(),true);
                if(txt)
                {
                    std::string txtValue=str(boost::format("??? %d")%piaoBase[k]);
                    if(piaoBase[k]==0)
                        txtValue="??? ???";
                    txt->SetText(txtValue.c_str());
                    
                    //??????
                    std::string btnName=str(boost::format("BTN_P%d")%k);
                    Button* pButton=layoutReady_->GetChildStaticCast<Button>(btnName.c_str(),true);
                    
                    if(pButton){
                        //???????????????
                        std::string varValue=str(boost::format("%d")%k);
                        pButton->SetVar(VAR_PIAOVALUE,varValue.c_str());
                    }
                }

            }
         }
    }catch (ptree_error & e) {
    }catch (...) {
    }
    
}
//??????????????????
void Mahjong::NewRound()
{
    Desk::NewRound();
    //for game_ready
    layoutReady_=nullptr;
    //????????????
    GetSubsystem<UI>()->Clear();
    if(scene_)
        scene_->Clear();
    
    
    CreateScene();
    SetupViewport();
    CreateUI();

    if(!bAdjustCamera_)
        GetSubsystem<Input>()->SetTouchEmulation(true);//??????touch
    
    //    // Set the mouse mode to use in the sample
    //   // Sample::InitMouseMode(MM_FREE);  //????????????
    //    // Set the mouse mode to use in the sample
    //    Sample::InitMouseMode(MM_RELATIVE);
    
    // Create music sound source
    musicSource_ = scene_->CreateComponent<SoundSource>();
    // Set the sound type to music so that master volume control works correctly
    musicSource_->SetSoundType(SOUND_MUSIC);
    if(bMusicOn_)
    {
        ResourceCache* cache = GetSubsystem<ResourceCache>();
        Sound* music = cache->GetResource<Sound>("Music/mj/paiju.ogg");//paiju//Music/Ninja Gods.ogg
        music->SetLooped(true);
        musicSource_->Play(music);
    }
    
    ////????????????????????????
    //if(!bMusicOn_){
    //    StringHash eventType;
    //    VariantMap eventData;
    //    HandlePlayMusic(eventType,eventData);
    //}
    
    // Hook up to the frame update events
    SubscribeToEvents();
    
    
//
//    //?????????????????? ??????
//    UIElement* root         = GetSubsystem<UI>()->GetRoot();
//    Graphics* graphics      = GetSubsystem<Graphics>();
//    ResourceCache* cache    = GetSubsystem<ResourceCache>();
//
//    auto layout=cache->GetResource<XMLFile>("UI/mj_GameOver.xml");
//    SharedPtr<UIElement> pGameOver=GetSubsystem<UI>()->LoadLayout(layout);
//    root->AddChild(pGameOver);
//    IntVector2 maxSize=pGameOver->GetSize();
//    maxSize.x_=graphics->GetWidth();
//    pGameOver->SetSize(maxSize);
//
//    std::string sTitle  ="???????????????";
//    std::string sLeft   ="??????????????? ????????????";
//    std::string sRight  ="?????? 8/8\n?????? 1198\n2019-3-16 12:13:45";
//    auto pTitle=pGameOver->GetChildStaticCast<Text>("title",true);
//    if(pTitle){
//        pTitle->SetText(sTitle.c_str());
//        pTitle->SetTextEffect(TE_STROKE);
//        pTitle->SetEffectStrokeThickness(5);
//    }
//    auto pLeft=pGameOver->GetChildStaticCast<Text>("left",true);
//    if(pLeft)
//        pLeft->SetText(sLeft.c_str());
//    auto pRight=pGameOver->GetChildStaticCast<Text>("right",true);
//    if(pRight)
//        pRight->SetText(sRight.c_str());
//
//    auto pAvatar=pGameOver->GetChildStaticCast<BorderImage>("avatar",true);
//    if(pAvatar)
//    {
//        std::string avatar="10.jpg";
//        pAvatar->SetTexture(GetAvatarTexture(avatar));
//    }
//
//    auto pName=pGameOver->GetChildStaticCast<Text>("profile",true);
//    if(pName)
//    {
//        std::string name=str(boost::format("%s")%"????????????");
//        pName->SetText(name.c_str());
//    }
//    auto pTxt=pGameOver->GetChildStaticCast<Text>("result",true);
//    if(pTxt)
//    {
//         std::string info=str(boost::format("????????????:    %2d\n????????????:    %2d\n????????????:    %2d\n????????????:    %2d\n????????????:    %2d")
//                    %18%2%3%4%5);
//         pTxt->SetText(info.c_str());
////         pTxt->SetTextEffect(TE_STROKE);
////         pTxt->SetEffectStrokeThickness(2);
//     }
//    auto pScore=pGameOver->GetChildStaticCast<Text>("score",true);
//    if(pScore)
//    {
//        std::string score=str(boost::format("%d")%99);
//        pScore->SetText(score.c_str());
//        pTitle->SetTextEffect(TE_STROKE);
//        pTitle->SetEffectStrokeThickness(4);
//    }
//
//    //????????????
//    int winner=0;
//    std::string colName=str(boost::format("col_%d") % winner);
//    UIElement* pCol=pGameOver->GetChildStaticCast<UIElement>(colName.c_str(),true);
//    if(pCol)
//    {
//        BorderImage* pWinner=pCol->CreateChild<BorderImage>();
//        if(pWinner)
//        {
//            pWinner->SetSize(450,130);
//            pWinner->SetPosition(0,-100);
//            pWinner->SetTexture(cache->GetResource<Texture2D>("Textures/mj/winner.png"));
//            pWinner->SetAlignment(HA_CENTER, VA_TOP);
//        }
//    }
//
//
//    int paoer=0;
//    std::string colPao=str(boost::format("col_%d") % paoer);
//    UIElement* pColPao=pGameOver->GetChildStaticCast<UIElement>(colPao.c_str(),true);
//    if(pColPao)
//    {
//        BorderImage* pPaoer=pColPao->CreateChild<BorderImage>();
//        if(pPaoer)
//        {
//            pPaoer->SetSize(125,125);
//            pPaoer->SetPosition(0,0);
//            pPaoer->SetTexture(cache->GetResource<Texture2D>("Textures/mj/paoshou.png"));
//            pPaoer->SetAlignment(HA_RIGHT, VA_TOP);
//        }
//    }
}
//????????????????????????
void Mahjong::ShowGameOver(ptree& pt)
{
    //2021.4.20 ?????????--->??????????????????????????????????????????
    engine_->Exit();
    return ;
    
    
    if(roundResult_)
        roundResult_->SetVisible(false);
    
    UIElement* root         = GetSubsystem<UI>()->GetRoot();
    Graphics* graphics      = GetSubsystem<Graphics>();
    ResourceCache* cache    = GetSubsystem<ResourceCache>();
    
    auto layout=cache->GetResource<XMLFile>("UI/mj_GameOver.xml");
    SharedPtr<UIElement> pGameOver=GetSubsystem<UI>()->LoadLayout(layout);
    root->AddChild(pGameOver);
    pGameOver->SetSize(graphics->GetWidth(),graphics->GetHeight());
    
    //??????????????????
    auto xBotton=pGameOver->GetChildStaticCast<Button>("share",true);
    SubscribeToEvent(xBotton, E_PRESSED, URHO3D_HANDLER(Mahjong, HandleShare));
    auto xBottonReturn=pGameOver->GetChildStaticCast<Button>("return",true);
    SubscribeToEvent(xBottonReturn, E_PRESSED, URHO3D_HANDLER(Mahjong, HandleExit));
    
    
    try
    {
        //??????
        std::string sTitle  =pt.get("title","");
        std::string sLeft   =pt.get("left","");
        std::string sRight  =pt.get("right","");
        auto pTitle=pGameOver->GetChildStaticCast<Text>("title",true);
        if(pTitle){
            pTitle->SetText(sTitle.c_str());
            pTitle->SetTextEffect(TE_STROKE);
            pTitle->SetEffectStrokeThickness(5);
        }
        auto pLeft=pGameOver->GetChildStaticCast<Text>("left",true);
        if(pLeft)
            pLeft->SetText(sLeft.c_str());
        auto pRight=pGameOver->GetChildStaticCast<Text>("right",true);
        if(pRight)
            pRight->SetText(sRight.c_str());
        
        for(int i=0;i<4;++i)
        {
            std::string colName=str(boost::format("col_%d") % i);
            UIElement* pCol=pGameOver->GetChildStaticCast<UIElement>(colName.c_str(),true);
            pCol->SetVisible(false);
        }
        
        
        
        int winner=-1;
        int paoer=-1;
        
        int i=0;
        ptree players = pt.get_child("players");
        //??????????????????  //????????????640

        int maxScore=0;
        int maxDianPao=0;
        
        int dist=0;
        int xStart=0;
        BOOST_FOREACH(boost::property_tree::ptree::value_type &v,players)
        {
            std::string avatar  =v.second.get("profile.avatar","");
            std::string name    =v.second.get("profile.name","");
            std::string acc     =v.second.get("profile.acc","");
            
            std::string info=str(boost::format("????????????:    %2d\n????????????:    %2d\n????????????:    %2d\n????????????:    %2d\n????????????:    %2d")
                                 %v.second.get("result.zimo",0)%v.second.get("result.hupai",0)%v.second.get("result.dianpao",0)%v.second.get("result.gangming",0)%v.second.get("result.gangan",0));
            
            std::string score=str(boost::format("%d")%v.second.get("result.score",0));
            
            if(v.second.get("result.score",0)>maxScore)
            {
                maxScore=v.second.get("result.score",0);
                winner=i;
            }
            
            if(v.second.get("result.dianpao",0)>maxDianPao)
            {
                maxDianPao=v.second.get("result.dianpao",0);
                paoer=i;
            }
            //?????????
            std::string colName=str(boost::format("col_%d") % i);
            UIElement* pCol=pGameOver->GetChildStaticCast<UIElement>(colName.c_str(),true);
            if(pCol)
            {
                IntVector2 size=pCol->GetSize();
                IntVector2 pos=pCol->GetPosition();
                if(xStart==0)
                {
                    dist=graphics->GetWidth()-players.size()*size.x_;
                    if(dist>0)
                    {
                        int cols=players.size()-1+1.5*2;
                        dist/=cols;
                    }else
                        dist=1;
                        
                    xStart=1.5*dist;
                }
                //???????????????????????????
                pos.x_=xStart+i*(size.x_+dist);
                pCol->SetPosition(pos);
                pCol->SetAlignment(HA_CUSTOM, VA_CENTER);
                pCol->SetVisible(true);
                
                
                
                auto pAvatar=pCol->GetChildStaticCast<BorderImage>("avatar",true);
                if(pAvatar)
                    pAvatar->SetTexture(GetAvatarTexture(avatar));
                
                auto pName=pCol->GetChildStaticCast<Text>("profile",true);
                if(pName)
                    pName->SetText(name.c_str());
                
                auto pInfo=pCol->GetChildStaticCast<Text>("result",true);
                if(pInfo)
                    pInfo->SetText(info.c_str());
                
                auto pScore=pCol->GetChildStaticCast<Text>("score",true);
                if(pScore){
                    pScore->SetText(score.c_str());
                    pTitle->SetTextEffect(TE_STROKE);
                    pTitle->SetEffectStrokeThickness(4);
                }
            }
            
            ++i;
        }
        
        //?????????
        if(winner>0)
        {
            std::string colName=str(boost::format("col_%d") % winner);
            UIElement* pCol=pGameOver->GetChildStaticCast<UIElement>(colName.c_str(),true);
            if(pCol)
            {
                BorderImage* pWinner=pCol->CreateChild<BorderImage>();
                if(pWinner)
                {
                    pWinner->SetSize(450,130);
                    pWinner->SetPosition(0,-100);
                    pWinner->SetTexture(cache->GetResource<Texture2D>("Textures/mj/winner.png"));
                    pWinner->SetAlignment(HA_CENTER, VA_TOP);
                }
            }
        }
        //????????????
        if(paoer>0)
        {
            std::string colPao=str(boost::format("col_%d") % paoer);
            UIElement* pColPao=pGameOver->GetChildStaticCast<UIElement>(colPao.c_str(),true);
            if(pColPao)
            {
                BorderImage* pPaoer=pColPao->CreateChild<BorderImage>();
                if(pPaoer)
                {
                    pPaoer->SetSize(125,125);
                    pPaoer->SetPosition(0,0);
                    pPaoer->SetTexture(cache->GetResource<Texture2D>("Textures/mj/paoshou.png"));
                    pPaoer->SetAlignment(HA_RIGHT, VA_TOP);
                }
            }
        }
   
    }catch (ptree_error & e) {
    }catch (...) {
    }
}

void Mahjong::SetDeskInfo(std::string const& name
                          ,std::string const& infoStatic
                          ,int cardNumber)
{
    if(!name.empty())
        deskName_->SetText(name.c_str());
    
    if(!infoStatic.empty())
        deskInfo_->SetText(infoStatic.c_str());


}
void Mahjong::SetDeskInfo(DeskDir player,int cards)
{
    ResourceCache* cache    = GetSubsystem<ResourceCache>();
    
    if(cards>0&&cards<100)
        SetCardFree(cards);
    //1 ?????? 0 ??? 4 left 3 up
    deskObject_->SetMaterial(0,cache->GetResource<Material>("Models/mahjong/Materials/desk1.xml"));//???
    deskObject_->SetMaterial(1,cache->GetResource<Material>("Models/mahjong/Materials/desk2.xml"));//???
    deskObject_->SetMaterial(3,cache->GetResource<Material>("Models/mahjong/Materials/desk4.xml"));//???
    deskObject_->SetMaterial(4,cache->GetResource<Material>("Models/mahjong/Materials/desk5.xml"));//???
    
    if(player==DeskDir::Me)
    {
        SharedPtr<Material> mat=deskObject_->GetMaterial(0)->Clone();
        Texture2D*   texture=cache->GetResource<Texture2D>("Textures/mj/nanh.jpg");
        mat->SetTexture(TextureUnit::TU_DIFFUSE, texture);
        deskObject_->SetMaterial(0,mat);
    }else if(player==DeskDir::Right)
    {
        SharedPtr<Material> mat=deskObject_->GetMaterial(1)->Clone();
        Texture2D*   texture=cache->GetResource<Texture2D>("Textures/mj/dongh.jpg");
        mat->SetTexture(TextureUnit::TU_DIFFUSE, texture);
        deskObject_->SetMaterial(1,mat);
    }else if(player==DeskDir::Left)
    {
        SharedPtr<Material> mat=deskObject_->GetMaterial(4)->Clone();
        Texture2D*   texture=cache->GetResource<Texture2D>("Textures/mj/xih.jpg");
        mat->SetTexture(TextureUnit::TU_DIFFUSE, texture);
        deskObject_->SetMaterial(4,mat);
    }else if(player==DeskDir::Forward)
    {
        SharedPtr<Material> mat=deskObject_->GetMaterial(3)->Clone();
        Texture2D*   texture=cache->GetResource<Texture2D>("Textures/mj/beih.jpg");
        mat->SetTexture(TextureUnit::TU_DIFFUSE, texture);
        deskObject_->SetMaterial(3,mat);
    }
    
}
Texture2D* Mahjong::GetAvatarTexture(std::string & avatar)
{
    Texture2D* pTexture=nullptr;
    if(avatar.find("http://")==std::string::npos)
    {
        avatar="Textures/mj/a/"+avatar;
        pTexture=GetSubsystem<ResourceCache>()->GetResource<Texture2D>(avatar.c_str());
    }else
    {//??????????????????
        
    }
    
    return pTexture;
}

#define table_length    45
//???????????? ???????????? ??????????????? ????????????
void Mahjong::SetPlayerInfo(std::vector<std::string>& infos,std::vector<std::string>& avatars,int64_t rid,int myPos,std::string const& rToken,std::vector<DeskDir> dirs)
{
    //???????????????????????????????????????????????????;
    clientObjectID_=myPos+2;
    //??????????????????
    AVReady(avServer_,rToken,"");
    
    //??????????????????
    Node* desktNode=scene_->GetChild("Desk");
    if(desktNode)
    {
        auto posDesk=desktNode->GetPosition();
        
        size_t nextPos=myPos;
        for(size_t i=0;i<dirs.size();++i)
        {
            size_t playerIndex=nextPos+2;
            size_t playerSound=playerIndex+10;
            Node* objectNode=scene_->GetNode(playerIndex);
            Node* soundNode=scene_->GetNode(playerSound);
            if(objectNode)
            {
                Vector3 newPos=posDesk;
                if(i==0)
                {
                    newPos.y_=-table_length;
                    
                    //????????????
                    SoundListener *listener = objectNode->CreateComponent<SoundListener>();
                    if(listener)
                        GetSubsystem<Audio>()->SetListener(listener);
                }else
                {
                    if(dirs.size()==2)
                    {
                        if(i==1)
                            newPos.z_=table_length;
                        
                    }else if(dirs.size()==3)
                    {
                        if(i==1)
                            newPos.x_=-table_length;
                        else if(i==2)
                            newPos.x_=table_length;
                        
                    }else if(dirs.size()==4)
                    {
                        if(i==1)
                            newPos.x_=-table_length;
                        else if(i==2)
                            newPos.z_=table_length;
                        else if(i==3)
                            newPos.x_=table_length;
                        
                    }
                }
                    
                
                objectNode->SetPosition(newPos);
                if(soundNode)
                    soundNode->SetPosition(newPos);
            }
            
            if(++nextPos>=dirs.size())
                nextPos=0;
        }
    }

    
    
    
    
    Node* node=scene_->GetChild("Dice");
    if(node)
        node->SetPosition(Vector3(0.f, 100.f,0.f));
    
    UIElement* root         = GetSubsystem<UI>()->GetRoot();
    Graphics* graphics      = GetSubsystem<Graphics>();
    ResourceCache* cache    = GetSubsystem<ResourceCache>();
    
    //??????xml ??????
    auto layout=cache->GetResource<XMLFile>("UI/mj_info.xml");
    layoutPlayerInfo_=GetSubsystem<UI>()->LoadLayout(layout);
    root->AddChild(layoutPlayerInfo_);
    auto trans=root->GetChildStaticCast<UIElement>("layer_info",true);
    if(trans){
        trans->SetSize(graphics->GetWidth(),graphics->GetHeight());
        trans->SetPosition(0,0);
    }

    auto huTest=layoutPlayerInfo_->GetChildStaticCast<UIElement>("WIN_HUTEST",true);
    huTest->SetVisible(false);
    
    auto cardsTing=layoutPlayerInfo_->GetChildStaticCast<UIElement>("CARD_TING",true);
    cardsTing->SetVisible(false);

    //
    for(int i=0;i<4;++i)
    {
        std::string win=str(boost::format("WP_%d")%i);
        auto pWin=layoutPlayerInfo_->GetChildStaticCast<UIElement>(win.c_str(),true);
        pWin->SetVisible(false);
    }
    
    if(2==infos.size())
    {
        auto pWin0=layoutPlayerInfo_->GetChildStaticCast<UIElement>("WP_0",true);
        auto pWin2=layoutPlayerInfo_->GetChildStaticCast<UIElement>("WP_2",true);
        pWin0->SetVisible(true);
        pWin2->SetVisible(true);
        
        //??????
        auto pTxt=layoutPlayerInfo_->GetChildStaticCast<Text>("p_0",true);
        pTxt->SetText(infos[0].c_str());
        auto pTxt2=layoutPlayerInfo_->GetChildStaticCast<Text>("p_2",true);
        pTxt2->SetText(infos[1].c_str());
        
        auto pAvatar=layoutPlayerInfo_->GetChildStaticCast<BorderImage>("avatar_0",true);
        pAvatar->SetTexture(GetAvatarTexture(avatars[0]));
        auto pAvatar2=layoutPlayerInfo_->GetChildStaticCast<BorderImage>("avatar_2",true);
        pAvatar2->SetTexture(GetAvatarTexture(avatars[1]));
        
        
    }else if(3==infos.size())
    {
        auto pWin0=layoutPlayerInfo_->GetChildStaticCast<UIElement>("WP_0",true);
        auto pWin1=layoutPlayerInfo_->GetChildStaticCast<UIElement>("WP_1",true);
        auto pWin3=layoutPlayerInfo_->GetChildStaticCast<UIElement>("WP_3",true);
        pWin0->SetVisible(true);
        pWin1->SetVisible(true);
        pWin3->SetVisible(true);
        
        auto pTxt=layoutPlayerInfo_->GetChildStaticCast<Text>("p_0",true);
        pTxt->SetText(infos[0].c_str());
        auto pTxt1=layoutPlayerInfo_->GetChildStaticCast<Text>("p_1",true);
        pTxt1->SetText(infos[1].c_str());
        auto pTxt3=layoutPlayerInfo_->GetChildStaticCast<Text>("p_3",true);
        pTxt3->SetText(infos[2].c_str());
        
        //??????
        auto pAvatar=layoutPlayerInfo_->GetChildStaticCast<BorderImage>("avatar_0",true);
        pAvatar->SetTexture(GetAvatarTexture(avatars[0]));
        auto pAvatar2=layoutPlayerInfo_->GetChildStaticCast<BorderImage>("avatar_1",true);
        pAvatar2->SetTexture(GetAvatarTexture(avatars[1]));
        auto pAvatar3=layoutPlayerInfo_->GetChildStaticCast<BorderImage>("avatar_3",true);
        pAvatar3->SetTexture(GetAvatarTexture(avatars[2]));
        
    }else if(4==infos.size())
    {
        for(int i=0;i<4;++i)
        {
            std::string win=str(boost::format("WP_%d")%i);
            std::string txt=str(boost::format("p_%d")%i);
            std::string avatar=str(boost::format("avatar_%d")%i);
            
            auto pWin=layoutPlayerInfo_->GetChildStaticCast<UIElement>(win.c_str(),true);
            pWin->SetVisible(true);
            
            auto pTxt=layoutPlayerInfo_->GetChildStaticCast<Text>(txt.c_str(),true);
            if(pTxt)
                pTxt->SetText(infos[i].c_str());
            
            auto pAvatar=layoutPlayerInfo_->GetChildStaticCast<BorderImage>(avatar.c_str(),true);
            if(pAvatar)
                pAvatar->SetTexture(GetAvatarTexture(avatars[i]));
        }
    }
}
void Mahjong::CreateUI()
{
    UIElement* root = GetSubsystem<UI>()->GetRoot();
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    XMLFile* uiStyle = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");
    // Set style to the UI root so that elements will inherit it
    root->SetDefaultStyle(uiStyle);
    
    Font* fontChina = cache->GetResource<Font>("Fonts/simfang.ttf");
    Graphics* graphics = GetSubsystem<Graphics>();
    
    if(bAdjustCamera_)
    {
        //?????????????????????
        SharedPtr<Cursor> cursor(new Cursor(context_));
        cursor->SetStyleAuto(uiStyle);
        GetSubsystem<UI>()->SetCursor(cursor);
        // Set starting position of the cursor at the rendering window center
        cursor->SetPosition(graphics->GetWidth() / 2, graphics->GetHeight() / 2);
    }
    
    //??????xml ??????
    auto layout=cache->GetResource<XMLFile>("UI/mj_desk.xml");
    layoutDesk_=GetSubsystem<UI>()->LoadLayout(layout);
    root->AddChild(layoutDesk_);
    auto trans=root->GetChildStaticCast<UIElement>("desk_info",true);
    if(trans){
        trans->SetSize(graphics->GetWidth(),graphics->GetHeight());
        trans->SetPosition(0,0);
    }
    
    //???????????? ??????
    auto imgBotton=layoutDesk_->GetChildStaticCast<Button>("btnMusic",true);
    if(imgBotton)
    {
        imgBotton->SetPressedOffset(0,0);
        SubscribeToEvent(imgBotton, E_PRESSED, URHO3D_HANDLER(Mahjong, HandlePlayMusic));
    }
    auto btnSpeaker=layoutDesk_->GetChildStaticCast<Button>("btnSpeaker",true);
    if(btnSpeaker)
    {
        //btnSpeaker->SetPressedOffset(0,0);
        SubscribeToEvent(btnSpeaker, E_PRESSED, URHO3D_HANDLER(Mahjong, HandleBntSpeakerOn));
        SubscribeToEvent(btnSpeaker, E_RELEASED, URHO3D_HANDLER(Mahjong, HandleBntSpeakerOff));
        
        
    }
    auto imgExit=layoutDesk_->GetChildStaticCast<Button>("btnExit",true);
    if(imgExit)
    {
        imgExit->SetPressedOffset(0,0);
        SubscribeToEvent(imgExit, E_PRESSED, URHO3D_HANDLER(Mahjong, HandleExit));
    }
    

    
    
    //???????????? ??????
    deskName_=layoutDesk_->GetChildStaticCast<Text>("desk_name",true);
    
//    deskName_ = root->CreateChild<Text>();
//    deskName_->SetPosition(0,0);
//    deskName_->SetAlignment(HA_CENTER, VA_TOP);
//    deskName_->SetFont(fontChina, 36);//12
//    deskName_->SetText("?????????|??????:108");
    
    //???????????? ??????
    deskInfo_=layoutDesk_->GetChildStaticCast<Text>("round_info",true);
//    deskInfo_ =root->CreateChild<Text>();
//    deskInfo_->SetPosition(0,0);
//    deskInfo_->SetAlignment(HA_RIGHT, VA_TOP);
//    deskInfo_->SetFont(fontChina, 20);//12
//    deskInfo_->SetText("??????:2\n??????:0/8");
    

    

    // Create buttons for playing/stopping music
//    Button* button = CreateButton(graphics->GetWidth()-20-240, 160, 240, 80, "????????????");
//    SubscribeToEvent(button, E_RELEASED, URHO3D_HANDLER(Mahjong, HandlePlayMusic));
    
//    button = CreateButton(360, 120, 240, 80, "????????????");
//    SubscribeToEvent(button, E_RELEASED, URHO3D_HANDLER(Mahjong, HandleStopMusic));
    
    
    //Audio* audio = GetSubsystem<Audio>();
    // Create sliders for controlling sound and music master volume
    //Slider* slider = CreateSlider(20, 140, 200, 20, "????????????");
    //slider->SetValue(audio->GetMasterGain(SOUND_EFFECT));
    //SubscribeToEvent(slider, E_SLIDERCHANGED, URHO3D_HANDLER(Mahjong, HandleSoundVolume));

//    Slider* slider = CreateSlider(20, 200, 200, 20, "????????????");
//    slider->SetValue(audio->GetMasterGain(SOUND_MUSIC));
//    SubscribeToEvent(slider, E_SLIDERCHANGED, URHO3D_HANDLER(Mahjong, HandleMusicVolume));
    
    
    
//    // ??????????????????
//    int32_t xPos=graphics->GetWidth()-4 * 180;
//    int32_t yPos=GetSubsystem<Graphics>()->GetHeight()-300;
//    for (unsigned i = 0; i < NUM_ACTIONS; ++i)
//    {
//        Button* button = CreateButton(i * 180 + xPos, yPos, 140, 80, actionNames[i]);
//        // Store the sound effect resource name as a custom variable into the button
//        button->SetVar(VAR_SOUNDRESOURCE, actionNames[i]);
//        SubscribeToEvent(button, E_PRESSED, URHO3D_HANDLER(Mahjong, HandleAction));
//
//        pButten_[i]=button;
//        pButten_[i]->SetVisible(false);
//    }
    

    
    

    
    //deskName_->SetColor(Color(1,0,0,1));??????????????????

    //????????????
//    //https://zh.fonts2u.com/mahjong-plain.%E5%AD%97%E4%BD%93
//     Font* mahjong = cache->GetResource<Font>("Fonts/mahjong.ttf");
//     deskName_ = root->CreateChild<Text>();
//     deskName_->SetPosition(0,0);
//     deskName_->SetAlignment(HA_CENTER, VA_TOP);
//     deskName_->SetFont(mahjong, 80);//12
//     deskName_->SetText("111,x 1 ???");
//    //deskName_->SetColor(Color(1,0,0,1));??????????????????




    
    
    
//    //????????????
//    auto layout=cache->GetResource<XMLFile>("UI/mj_test.xml");
//    SharedPtr<UIElement> huInfo_=GetSubsystem<UI>()->LoadLayout(layout);
//    root->AddChild(huInfo_);
//
//    BorderImage* pImgFrame=huInfo_->CreateChild<BorderImage>();
//    pImgFrame->SetPosition(0,0);
//    pImgFrame->SetTexture(cache->GetResource<Texture2D>("Textures/mj/frm.png"));
//    pImgFrame->SetSize(IntVector2(80,128));
//    pImgFrame->SetAlignment(HA_CUSTOM, VA_CENTER);
//
//
//    BorderImage* pImage=pImgFrame->CreateChild<BorderImage>();
//    //pImage->SetPosition(100,100);
//    pImage->SetTexture(cache->GetResource<Texture2D>("Textures/mj/41.png"));
//    pImage->SetSize(IntVector2(72,100));
//    pImage->SetAlignment(HA_CENTER, VA_CENTER);
//
//    Text* pNumber= huInfo_->CreateChild<Text>();
//    IntVector2 posTmp=pImgFrame->GetPosition();
//    posTmp.x_+=80;
//    posTmp.y_+=128-36;
//    pNumber->SetPosition(posTmp);
//    pNumber->SetFont(fontChina, 36);//12
//    pNumber->SetText("X2");
//    //pNumber->SetAlignment(HA_CENTER, VA_BOTTOM);

}

IntRect Mahjong::Create2DMahjong(UIElement* pParent,int value,int state,int xPos,int yPos,float scal)
{
    IntRect rc;
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    IntVector2 frm=IntVector2(80,128);//??????????????????
    
    BorderImage* pImgFrame=pParent->CreateChild<BorderImage>();
    pImgFrame->SetPosition(xPos,0);
    pImgFrame->SetTexture(cache->GetResource<Texture2D>("Textures/mj/frm.png"));
    pImgFrame->SetSize(IntVector2(frm.x_*scal,frm.y_*scal));
    pImgFrame->SetAlignment(HA_CUSTOM, VA_CENTER);
    //child
    IntVector2 pic=IntVector2(72,100);
    std::string svalue=str(boost::format("Textures/mj/%d.jpg")%value);
    BorderImage* pImage=pImgFrame->CreateChild<BorderImage>();
    pImage->SetTexture(cache->GetResource<Texture2D>(svalue.c_str()));
    pImage->SetSize(IntVector2(pic.x_*scal,pic.y_*scal));
    pImage->SetAlignment(HA_CENTER, VA_CENTER);
    pImage->SetParent(pImgFrame);
    
    IntVector2 pos=pImgFrame->GetPosition();
    IntVector2 size=pImgFrame->GetSize();
    
    rc.left_=pos.x_;
    rc.top_=pos.y_;
    rc.right_=rc.left_+size.x_;
    rc.bottom_=rc.top_+size.y_;
    return rc;
}
//?????????????????????
void Mahjong::ShowHuInfo(std::map<int,int>& huPai)
{

    if(layoutPlayerInfo_)
    {
        Graphics* graphics = GetSubsystem<Graphics>();
        
        auto huTest=layoutPlayerInfo_->GetChildStaticCast<UIElement>("WIN_HUTEST",true);
        if(huTest)
        {
            if(huPai.size()==0)
            {
                huTest->SetVisible(false);
                return ;
            }

            huTest->RemoveAllChildren();
            //huTest->SetPosition(360, graphics->GetHeight()-400);
            
            
            ResourceCache* cache = GetSubsystem<ResourceCache>();
            Font* fontChina = cache->GetResource<Font>("Fonts/simfang.ttf");
            //????????????
            Text* pTxt_hu= huTest->CreateChild<Text>();
            pTxt_hu->SetPosition(10,10);
            pTxt_hu->SetAlignment(HA_CUSTOM, VA_CENTER);
            pTxt_hu->SetFont(fontChina,24);//12
            pTxt_hu->SetText("???:");
            
            int grouWidth=120; //??????
            int xPos=60;
            IntVector2 size=huTest->GetSize();
            size.x_=grouWidth*huPai.size()+xPos;
            huTest->SetSize(size);
            
            for(auto c:huPai)
            {
                IntRect rc=Create2DMahjong(huTest,c.first,0,xPos,0,0.6);
                
                std::string txt=str(boost::format("x%d")%c.second);
                Text* pNumber= huTest->CreateChild<Text>();
                IntVector2 posTmp;
                posTmp.x_+=rc.left_+rc.Width() +2;
                posTmp.y_+=rc.bottom_-40;
                pNumber->SetPosition(posTmp);
                pNumber->SetFont(fontChina, 36);//12
                pNumber->SetText(txt.c_str());
                
                
                xPos+=grouWidth;
            }
            huTest->SetVisible(true);
        }
    }
}

//???????????? ???????????????
void Mahjong::ShowTingInfo(std::map<int,int>& huPai)
{

    if(layoutPlayerInfo_)
    {
        Graphics* graphics = GetSubsystem<Graphics>();
        
        auto cardTing=layoutPlayerInfo_->GetChildStaticCast<UIElement>("CARD_TING",true);
        if(cardTing)
        {
            if(huPai.size()==0)
            {
                cardTing->SetVisible(false);
                return ;
            }

            cardTing->RemoveAllChildren();
            //huTest->SetPosition(360, graphics->GetHeight()-400);
            
            
            ResourceCache* cache = GetSubsystem<ResourceCache>();
            Font* fontChina = cache->GetResource<Font>("Fonts/simfang.ttf");
            //????????????
            Text* pTxt_hu= cardTing->CreateChild<Text>();
            pTxt_hu->SetPosition(10,10);
            pTxt_hu->SetAlignment(HA_CUSTOM, VA_CENTER);
            pTxt_hu->SetFont(fontChina,24);//12
            pTxt_hu->SetText("???:");
            
            int grouWidth=120; //??????
            int xPos=60;
            IntVector2 size=cardTing->GetSize();
            size.x_=grouWidth*huPai.size()+xPos;
            cardTing->SetSize(size);
            
            for(auto c:huPai)
            {
                IntRect rc=Create2DMahjong(cardTing,c.first,0,xPos,0,0.6);
                
                //????????????????????? ???????????????
                std::string txt="";//str(boost::format("x%d")%c.second);
                Text* pNumber= cardTing->CreateChild<Text>();
                IntVector2 posTmp;
                posTmp.x_+=rc.left_+rc.Width() +2;
                posTmp.y_+=rc.bottom_-40;
                pNumber->SetPosition(posTmp);
                pNumber->SetFont(fontChina, 36);//12
                pNumber->SetText(txt.c_str());
                
                
                xPos+=grouWidth;
            }
            cardTing->SetVisible(true);
        }
    }
}

void Mahjong::CreateButtons(std::string const& btns)
{
    auto btnArray = s_split(btns, "[\\s,;?]+");
    
    // ??????????????????
    Graphics* graphics = GetSubsystem<Graphics>();
    int32_t xPos=graphics->GetWidth()-btnArray.size() * 180;
    
    int32_t yPos=graphics->GetHeight()-300;
    for (unsigned i = 0; i < btnArray.size(); ++i)
    {
        Button* button = CreateButton(i * 180 + xPos, yPos, 140, 80, btnArray[i].c_str());
        // Store the sound effect resource name as a custom variable into the button
        button->SetVar(VAR_BUTTONVALUE, btnArray[i].c_str());
        SubscribeToEvent(button, E_PRESSED, URHO3D_HANDLER(Mahjong, HandleAction));
        
        button->SetVisible(false);
        btnMaps_[btnArray[i]]=button;
    }

    //??????????????????
    pProgressBar_=CreateProgressBar(xPos,yPos-40,btnArray.size() * 180-40,20);
    pProgressBar_->SetVisible(false);
    
}
void Mahjong::ShowButton(std::string const& btns,int32_t seconds)
{
    if(btns.empty())
    {
        for(auto c:btnMaps_)
            c.second->SetVisible(false);
    }else
    {
        for(auto o:btnMaps_)
            o.second->SetVisible(false);
        
        auto btnArray = s_split(btns, "[\\s,;?]+");
        for(auto c:btnArray)
        {
            if(!c.empty()){
                btnMaps_[c]->SetVisible(true);
                if(c=="???")
                {
                    auto pTxt=btnMaps_[c]->GetChildStaticCast<Text>("ShowOrNo",true);
                    if(pTxt)
                        pTxt->SetText("???");
                }
            }
        }
    }

    //???????????????
    if(seconds>0)
    {
        countDown_=true;
        if(pProgressBar_)
        {
            pProgressBar_->SetVisible(true);
            pProgressBar_->SetRange(seconds*1000);
        }
        timerEnd_=std::chrono::steady_clock::now()+std::chrono::milliseconds(seconds*1000);
    }else
    {
        countDown_=false;
        if(pProgressBar_)
            pProgressBar_->SetVisible(false);

        for(auto c:btnMaps_)
            c.second->SetVisible(false);
    }
}

Button* Mahjong::CreateButton(int x, int y, int xSize, int ySize, const String& text)
{
    UIElement* root = GetSubsystem<UI>()->GetRoot();
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    Font* font = cache->GetResource<Font>("Fonts/simfang.ttf");
    
    // Create the button and center the text onto it
    Button* button = root->CreateChild<Button>();
    button->SetStyleAuto();
    button->SetPosition(x, y);
    button->SetSize(xSize, ySize);

    Text* buttonText = button->CreateChild<Text>();
    buttonText->SetName("ShowOrNo");
    buttonText->SetAlignment(HA_CENTER, VA_CENTER);
    buttonText->SetFont(font, 36);//12
    buttonText->SetText(text);
    

    return button;
}

Slider* Mahjong::CreateSlider(int x, int y, int xSize, int ySize, const String& text)
{
    UIElement* root = GetSubsystem<UI>()->GetRoot();
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    Font* font = cache->GetResource<Font>("Fonts/Anonymous Pro.ttf");

    // Create text and slider below it
    Text* sliderText = root->CreateChild<Text>();
    sliderText->SetPosition(x, y);
    sliderText->SetFont(font, 12);
    sliderText->SetText(text);

    Slider* slider = root->CreateChild<Slider>();
    slider->SetStyleAuto();
    slider->SetPosition(x, y + 20);
    slider->SetSize(xSize, ySize);
    // Use 0-1 range for controlling sound/music master volume
    slider->SetRange(1.0f);

    return slider;
}

ProgressBar* Mahjong::CreateProgressBar(int x, int y, int xSize, int ySize)
{
    UIElement* root = GetSubsystem<UI>()->GetRoot();
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    
    Font* font = cache->GetResource<Font>("Fonts/Anonymous Pro.ttf");
    
    // Create the button and center the text onto it
    ProgressBar*  bar = root->CreateChild<ProgressBar>();
    bar->SetStyleAuto();
    bar->SetPosition(x, y);
    bar->SetSize(xSize, ySize);
    //bar->SetFont(font, 36);//12
    bar->SetShowPercentText(false);
//    bar->SetRange(100);
//    bar->SetValue(50);
    
    return bar;
}

//void Mahjong::HandlePlaySound(StringHash eventType, VariantMap& eventData)
//{
//    Button* button = static_cast<Button*>(GetEventSender());
//    const String& soundResourceName = button->GetVar(VAR_SOUNDRESOURCE).GetString();
//
//    // Get the sound resource
//    ResourceCache* cache = GetSubsystem<ResourceCache>();
//    Sound* sound = cache->GetResource<Sound>(soundResourceName);
//
//    if (sound)
//    {
//        // Create a SoundSource component for playing the sound. The SoundSource component plays
//        // non-positional audio, so its 3D position in the scene does not matter. For positional sounds the
//        // SoundSource3D component would be used instead
//        SoundSource* soundSource = scene_->CreateComponent<SoundSource>();
//        // Component will automatically remove itself when the sound finished playing
//        soundSource->SetAutoRemoveMode(REMOVE_COMPONENT);
//        soundSource->Play(sound);
//        // In case we also play music, set the sound volume below maximum so that we don't clip the output
//        soundSource->SetGain(0.75f);
//    }
//}

void Mahjong::HandlePlayMusic(StringHash eventType, VariantMap& eventData)
{
    bMusicOn_=!bMusicOn_;
    if(bMusicOn_)
    {
        ResourceCache* cache = GetSubsystem<ResourceCache>();
        Sound* music = cache->GetResource<Sound>("Music/mj/paiju.ogg");//paiju//Music/Ninja Gods.ogg
        music->SetLooped(true);
        musicSource_->Play(music);
    }else
        musicSource_->Stop();
}
void Mahjong::HandleBntSpeakerOn(StringHash eventType, VariantMap& eventData)
{
    //?????????????????????
    volume_music_=GetSubsystem<Audio>()->GetMasterGain(SOUND_MUSIC);
    volume_effect_=GetSubsystem<Audio>()->GetMasterGain(SOUND_EFFECT);
    
    GetSubsystem<Audio>()->SetMasterGain(SOUND_EFFECT, volume_effect_*0.2);
    GetSubsystem<Audio>()->SetMasterGain(SOUND_MUSIC, volume_music_*0.2);
    
    SetSpeaker(true);
}
void Mahjong::HandleBntSpeakerOff(StringHash eventType, VariantMap& eventData)
{
    SetSpeaker(false);
    
    //?????????????????????
    GetSubsystem<Audio>()->SetMasterGain(SOUND_EFFECT, volume_effect_);
    GetSubsystem<Audio>()->SetMasterGain(SOUND_MUSIC, volume_music_);
    
}

//
//void Mahjong::HandleSoundVolume(StringHash eventType, VariantMap& eventData)
//{
//    using namespace SliderChanged;
//
//    float newVolume = eventData[P_VALUE].GetFloat();
//    GetSubsystem<Audio>()->SetMasterGain(SOUND_EFFECT, newVolume);
//}
//
//void Mahjong::HandleMusicVolume(StringHash eventType, VariantMap& eventData)
//{
//    using namespace SliderChanged;
//
//    float newVolume = eventData[P_VALUE].GetFloat();
//    GetSubsystem<Audio>()->SetMasterGain(SOUND_MUSIC, newVolume);
//}

#include <Urho3D/Scene/ObjectAnimation.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/ValueAnimation.h>

//????????????
void Mahjong::CreateScene()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    scene_ = new Scene(context_);
    
    // Create octree, use default volume (-1000, -1000, -1000) to (1000, 1000, 1000)
    // Also create a DebugRenderer component so that we can draw debug geometry
    scene_->CreateComponent<Octree>();
    //scene_->CreateComponent<DebugRenderer>();
    
    /*          z
                |
                |
                |------>x
     */
    
    
    //???????????????????????????????????????id
    //??????4?????????????????? 4????????? 0???1???2???3???  0=??? ?????????
    //??????????????????????????????id ???2??????
    for(int i=2;i<6;++i)
    {
        //?????????????????? id
        Node* playerVoiceNode = scene_->CreateChild("MahjongPlayers",LOCAL,i);
        SoundSource3D*  snd_source_3d =playerVoiceNode->CreateComponent<SoundSource3D>();
                        snd_source_3d->SetSoundType(SOUND_VOICE);
                        snd_source_3d->SetNearDistance(80);
                        snd_source_3d->SetFarDistance(200);
        
        //???????????????????????????
        Node* playerSoundNode = scene_->CreateChild("SystemSound",LOCAL,10+i);
        snd_source_3d =playerSoundNode->CreateComponent<SoundSource3D>();
                        snd_source_3d->SetSoundType(SOUND_EFFECT);
                        snd_source_3d->SetNearDistance(80);
                        snd_source_3d->SetFarDistance(200);
        
    }

    
    
    
    //    ????????????
    // Create scene node & StaticModel component for showing a static plane
    Node* planeNode = scene_->CreateChild("Plane");
    planeNode->SetScale(Vector3(100.0f, 2.0f, 100.0f));
    planeNode->SetPosition(Vector3(0.f, -2.f,0.f));
    StaticModel* planeObject = planeNode->CreateComponent<StaticModel>();
    planeObject->SetModel(cache->GetResource<Model>("Models/Plane.mdl"));
    planeObject->SetMaterial(cache->GetResource<Material>("Materials/StoneTiledH.xml"));

    
    //??????????????????
    Node* deskNode = scene_->CreateChild("Desk");
    deskNode->SetPosition(Vector3(0.f, -1.1f,0.f));
    deskNode->SetRotation(Quaternion(0, 90, 0));
    deskNode->SetScale(1.2);
    deskObject_ = deskNode->CreateComponent<StaticModel>();
    deskObject_->SetModel(cache->GetResource<Model>("Models/mahjong/mjDesk.mdl"));
    //???????????? ???????????? ?????? blender export for urho  Material text list
    deskObject_->SetMaterial(0,cache->GetResource<Material>("Models/mahjong/Materials/desk1.xml"));//???
    deskObject_->SetMaterial(1,cache->GetResource<Material>("Models/mahjong/Materials/desk2.xml"));//???
    deskObject_->SetMaterial(2,cache->GetResource<Material>("Models/mahjong/Materials/desk3.xml"));//??????
    deskObject_->SetMaterial(3,cache->GetResource<Material>("Models/mahjong/Materials/desk4.xml"));//???
    deskObject_->SetMaterial(4,cache->GetResource<Material>("Models/mahjong/Materials/desk5.xml"));//???
    deskObject_->SetMaterial(5,cache->GetResource<Material>("Models/mahjong/Materials/desk7.xml"));//??????
    deskObject_->SetMaterial(6,cache->GetResource<Material>("Models/mahjong/Materials/desk6.xml"));//??????
    deskObject_->SetMaterial(7,cache->GetResource<Material>("Models/mahjong/Materials/desk0.xml"));//??????
    

    // Create a Zone component for ambient lighting & fog control
    Node* zoneNode = scene_->CreateChild("Zone");
    Zone* zone = zoneNode->CreateComponent<Zone>();
    zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
    zone->SetAmbientColor(Color(0.5f, 0.5f, 0.5f));
    zone->SetFogColor(Color(0.4f, 0.5f, 0.8f));
    zone->SetFogStart(100.0f);
    zone->SetFogEnd(300.0f);

    
    // Create a directional light to the world. Enable cascaded shadows on it
    Node* lightNode = scene_->CreateChild("DirectionalLight");
    lightNode->SetDirection(Vector3(0.6f, -1.0f, 0.8f));
    Light* light = lightNode->CreateComponent<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetCastShadows(true);
    light->SetColor(Color(0.5f, 0.5f, 0.5f));
    light->SetShadowBias(BiasParameters(0.00025f, 0.5f));
    // Set cascade splits at 10, 50 and 200 world units, fade shadows out at 80% of maximum shadow distance
    light->SetShadowCascade(CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));
    

    
    //??????????????????
    Node* lightNode2 = scene_->CreateChild("PointLight");
    lightNode2->SetPosition(Vector3(0.0f, 50.0f, -70.0f));//-70
    
    auto* light2 = lightNode2->CreateComponent<Light>();
    light2->SetLightType(LIGHT_POINT);
    light2->SetRange(120.0f);
    light2->SetColor(Color(0.5f, 0.5f, 0.5f));
    light2->SetBrightness(3.0f);
    
    //??????????????????
    Node* lightNode3 = scene_->CreateChild("PointLight");
    lightNode3->SetPosition(Vector3(0.0f, 50.0f, 70.0f));
    auto* light3 = lightNode3->CreateComponent<Light>();
    light3->SetLightType(LIGHT_POINT);
    light3->SetRange(120.0f);
    light3->SetColor(Color(0.5f, 0.5f, 0.5f));
    light3->SetBrightness(3.0f);

    
    /*
    //???????????????
    Node* lightNode_winner = scene_->CreateChild("PointLight");
    lightNode_winner->SetPosition(Vector3(20.0f, 50.0f, 0.0f));
    
    auto* light_winner = lightNode_winner->CreateComponent<Light>();
    light_winner->SetLightType(LIGHT_POINT);
    light_winner->SetRange(70.0f);
    light_winner->SetBrightness(10.0f);
    
    // Create light animation
    SharedPtr<ObjectAnimation> lightAnimation(new ObjectAnimation(context_));
    
    // Create light color animation
    SharedPtr<ValueAnimation> colorAnimation(new ValueAnimation(context_));
    colorAnimation->SetKeyFrame(0.0f, Color::WHITE);
    colorAnimation->SetKeyFrame(1.0f, Color::RED);
    colorAnimation->SetKeyFrame(2.0f, Color::YELLOW);
    colorAnimation->SetKeyFrame(3.0f, Color::GREEN);
    colorAnimation->SetKeyFrame(4.0f, Color::WHITE);
    // Set Light component's color animation
    lightAnimation->AddAttributeAnimation("@Light/Color", colorAnimation);

    // Apply light animation to light node
    lightNode_winner->SetObjectAnimation(lightAnimation);
    */
    

    

    
    
    // Create the camera. Limit far clip distance to match the fog
    cameraNode_ = scene_->CreateChild("Camera");
    Camera* camera = cameraNode_->CreateComponent<Camera>();
    camera->SetFarClip(300.0f);
    
//    // Set an initial position for the camera scene node above the plane
//    //3???????????????
//    cameraNode_->SetPosition(Vector3(2.0f, 40.0f, -55.f));//????????????
//    cameraNode_->SetRotation(Quaternion(45.f, yaw_, 0.0f)); //45

    
//    //2???????????????
//    cameraNode_->SetPosition(Vector3(2.0f, 54.4f, -56.45f));//????????????
//    cameraNode_->SetRotation(Quaternion(42, yaw_, 0.0f)); //45
    
    cameraNode_->SetPosition(Vector3(0.0f, 47.7f, -62.7f));//????????????
    cameraNode_->SetRotation(Quaternion(45.f, yaw_, 0.0f)); //45
    
     if (GetPlatform() == "Android" || GetPlatform() == "iOS")
     {
         cameraNode_->SetPosition(Vector3(0.0f, 45.0f, -60.0f));//????????????
     }else{
        cameraNode_->SetPosition(Vector3(0.0f, 47.7f, -70.7f));//????????????
     }
   
    //y ??????????????????
//    Quaternion rot=cameraNode_->GetRotation();
//    rot.y_-=0.15f; //?????????
//    rot.y_+=0.15f; //?????????
//    cameraNode_->SetRotation(rot);


    
    //cameraNode_->SetPosition(Vector3(2.0f, 50.0f, -70.f));//macbookpro ????????????
    


    //??????????????????
    Node* diceNode = scene_->CreateChild("Dice");
    diceNode->SetPosition(Vector3(0.f, 2.5f,0.f));
    diceNode->SetRotation(Quaternion(0, 90, 0));

    diceNode->SetScale(5);
    StaticModel* diceObject = diceNode->CreateComponent<StaticModel>();
    diceObject->SetModel(cache->GetResource<Model>("Models/mahjong/mjDice.mdl"));
    //???????????? ?????? blender export for urho
    diceObject->SetMaterial(0,cache->GetResource<Material>("Models/mahjong/Materials/dice2.xml"));
    diceObject->SetMaterial(1,cache->GetResource<Material>("Models/mahjong/Materials/dice1.xml"));
    diceObject->SetMaterial(2,cache->GetResource<Material>("Models/mahjong/Materials/dice0.xml"));
    diceObject->SetMaterial(3,cache->GetResource<Material>("Models/mahjong/Materials/dice3.xml"));
    auto* rotator = diceNode->CreateComponent<Rotator>();
    rotator->SetRotationSpeed(Vector3(50.0f, 20.0f, 30.0f));
    
    



    

    
//
//    {
//            Node* boxNode = scene_->CreateChild("MahJong");
//            boxNode->SetPosition(Vector3(0.f, 0.f,-30.f));
//            boxNode->SetRotation(Quaternion(0, 90, 45));
//            //??????????????????
//            //boxNode->SetRotation(Quaternion(0, 90, 0));//????????????
//            //boxNode->SetRotation(Quaternion(0, 180, 0));//????????????
//
//            //boxNode->SetRotation(Quaternion(0, -90, 0));//??????????????????
//            //boxNode->SetRotation(Quaternion(0, -90, 90));//??????????????????
//            // Orient using random pitch, yaw and roll Euler angles
//            //boxNode->SetRotation(Quaternion(Random(360.0f), Random(360.0f), Random(360.0f)));
//            StaticModel* boxObject = boxNode->CreateComponent<StaticModel>();
//            boxObject->SetModel(cache->GetResource<Model>("Models/mahjong/mj.mdl"));
//            boxObject->SetMaterial(1,cache->GetResource<Material>("Models/mahjong/Materials/White.xml"));
//            boxObject->SetMaterial(2,cache->GetResource<Material>("Models/mahjong/Materials/Green.xml"));
//            boxObject->SetMaterial(0,cache->GetResource<Material>("Models/mahjong/Materials/13.xml"));
//
//
//    }
//
    //boxNode->SetRotation(Quaternion(0, 90, 90));
    //boxNode->SetRotation(Quaternion(0, 90, 0));
    
//    {
//        //???????????????
//        Node* mjNode2 = scene_->CreateChild("MahJong");
//        mjNode2->SetPosition(Vector3(0.f, 4.86f,0.f));//  ??????4.85 ?????? 3.45
//        mjNode2->SetRotation(Quaternion(0, 90, 0));
//
//        StaticModel* mj2 = mjNode2->CreateComponent<StaticModel>();
//        mj2->SetModel(cache->GetResource<Model>("Models/mahjong/mj.mdl"));
//        mj2->SetMaterial(1,cache->GetResource<Material>("Models/mahjong/Materials/White.xml"));
//        mj2->SetMaterial(2,cache->GetResource<Material>("Models/mahjong/Materials/Green.xml"));
//        mj2->SetMaterial(0,cache->GetResource<Material>("Models/mahjong/Materials/1.xml"));
//
//        mj2->SetCastShadows(true);
//    }


}
void Mahjong::SetupViewport()
{
    Renderer* renderer = GetSubsystem<Renderer>();
    
    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
    renderer->SetViewport(0, viewport);
}
void Mahjong::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(Mahjong, HandleUpdate));
    
    
    
    SubscribeToEvent(E_TOUCHBEGIN, URHO3D_HANDLER(Mahjong, HandleTouchBegin));
    SubscribeToEvent(E_TOUCHEND, URHO3D_HANDLER(Mahjong, HandleTouchEnd));
    
    SubscribeToEvent(E_KEYUP, URHO3D_HANDLER(Mahjong, HandleKeyUpExit));
    
    //SubscribeToEvent(E_TOUCHMOVE, URHO3D_HANDLER(Mahjong, HandleTouchMove));
}

void Mahjong::HandleTouchBegin(StringHash eventType, VariantMap& eventData)
{
    using namespace TouchBegin;
    IntVector2 screenPos = IntVector2(eventData[P_X].GetInt(), eventData[P_Y].GetInt());
    int touchID = eventData[P_TOUCHID].GetInt();
    // UI-element touched
    if (GetSubsystem<UI>()->GetElementAt(screenPos, true))
        return;
    // Interaction with scene objects
    SelectCard(screenPos,NTouch::NT_CLICK);
    touchStart_=screenPos;
    
}
void Mahjong::HandleTouchEnd(StringHash eventType, VariantMap& eventData)
{
    if(touchStart_==IntVector2(0,0))
        return ;
    
    using namespace TouchBegin;
    IntVector2 touchEnd = IntVector2(eventData[P_X].GetInt(), eventData[P_Y].GetInt());
    
    int dy=touchEnd.y_-touchStart_.y_;
    if(std::abs(dy)>20)
    {
        if (GetSubsystem<UI>()->GetElementAt(touchStart_, true))
            return;
        // Interaction with scene objects
        SelectCard(touchStart_,(dy>0? NTouch::NT_DOWN:NTouch::NT_UP));
    }
    
    touchStart_=IntVector2(0,0);
    
}

// ????????????????????? ????????????????????????????????????   ?????? TouchMove ?????????????????? ????????????
void Mahjong::HandleTouchMove(StringHash eventType, VariantMap& eventData)
{
//    using namespace TouchBegin;
//    IntVector2 screenPos = IntVector2(eventData[P_X].GetInt(), eventData[P_Y].GetInt());
//    int touchID = eventData[P_TOUCHID].GetInt();
//    // UI-element touched
//    if (GetSubsystem<UI>()->GetElementAt(screenPos, true))
//        return;
//    // Interaction with scene objects
//    SelectCard(screenPos);
    
//    using namespace MouseMove;
//    auto x = eventData[P_X].GetInt();
//    auto y = eventData[P_Y].GetInt();
//
//    auto dx =eventData[P_DX].GetInt();
//    auto dy =eventData[P_DY].GetInt();
//
//    if(std::abs(dy)>0)
//    {
//        IntVector2 screenPos(x,y);
//        if (GetSubsystem<UI>()->GetElementAt(screenPos, true))
//            return;
//        // Interaction with scene objects
//        SelectCard(screenPos,(dy>0? NTouch::NT_DOWN:NTouch::NT_UP));
//    }
//    //auto* graphics = GetSubsystem<Graphics>();
}

void Mahjong::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;
    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();
    // Move the camera, scale movement with time step
    MoveCamera(timeStep);
    
    
    ParseCommand();

    if(countDown_&&pProgressBar_)
    {
        std::chrono::steady_clock::time_point  timeNow=std::chrono::steady_clock::now();
        int32_t count=std::chrono::duration_cast<std::chrono::milliseconds>(timerEnd_ - timeNow).count();
        
        if(count>0)
        {
            pProgressBar_->SetValue(count);
        }else if(count<0){
            //??????????????? ????????????
            pProgressBar_->SetVisible(false);
            countDown_=false;
            
            for(auto c:btnMaps_)
                c.second->SetVisible(false);
        }
    }
}
void Mahjong::SelectCard(IntVector2& touchPos,NTouch dir)
{
    Vector3 hitPos;
    Drawable* hitDrawable;
    
    if (Raycast(touchPos,250.0f, hitPos, hitDrawable))
    {
        // Check if target scene node already has a DecalSet component. If not, create now
        Node* targetNode = hitDrawable->GetNode();
        if(targetNode&&targetNode->GetName()=="MahJong")
        {
            NodeTouch(targetNode,dir);
        }
    }
}
bool Mahjong::Raycast(IntVector2& pos,float maxDistance, Vector3& hitPos, Drawable*& hitDrawable)
{
    hitDrawable = 0;
    //UI* ui = GetSubsystem<UI>();
    //IntVector2 pos = ui->GetCursorPosition();
//    // Check the cursor is visible and there is no UI element in front of the cursor
//    if (!ui->GetCursor()->IsVisible() || ui->GetElementAt(pos, true))
//        return false;
    
    Graphics* graphics = GetSubsystem<Graphics>();
    Camera* camera = cameraNode_->GetComponent<Camera>();
    Ray cameraRay = camera->GetScreenRay((float)pos.x_ / graphics->GetWidth(), (float)pos.y_ / graphics->GetHeight());
    // Pick only geometry objects, not eg. zones or lights, only get the first (closest) hit
    PODVector<RayQueryResult> results;
    RayOctreeQuery query(results, cameraRay, RAY_TRIANGLE, maxDistance, DRAWABLE_GEOMETRY);
    scene_->GetComponent<Octree>()->RaycastSingle(query);
    if (results.Size())
    {
        RayQueryResult& result = results[0];
        hitPos = result.position_;
        hitDrawable = result.drawable_;
        return true;
    }
    
    
    return false;
}

void Mahjong::MoveCamera(float timeStep)
{
    
    if(!bAdjustCamera_)
        return ;
    // Right mouse button controls mouse cursor visibility: hide when pressed
    UI* ui = GetSubsystem<UI>();
    Input* input = GetSubsystem<Input>();
    
    if(input&&ui)
    {
//        //?????????????????????crateui ??????
//        ui->GetCursor()->SetVisible(!input->GetMouseButtonDown(MOUSEB_RIGHT));
//        // Do not move if the UI has a focused element (the console)
//        if (ui->GetFocusElement())
//            return;
//        // Movement speed as world units per second
        const float MOVE_SPEED = 20.0f;
        
        
        // Mouse sensitivity as degrees per pixel
        const float MOUSE_SENSITIVITY = 0.1f;
        // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
        // Only move the camera when the cursor is hidden
        if (bAdjustCamera_/*||!ui->GetCursor()->IsVisible()*/)
        {
            IntVector2 mouseMove = input->GetMouseMove();
            yaw_ += MOUSE_SENSITIVITY * mouseMove.x_;
            pitch_ += MOUSE_SENSITIVITY * mouseMove.y_;
            pitch_ = Clamp(pitch_, -90.0f, 90.0f);

            // Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
            cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));
        }
        
        
        // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
        if (input->GetKeyDown(KEY_W))
            cameraNode_->Translate(Vector3::FORWARD * MOVE_SPEED * timeStep);
        if (input->GetKeyDown(KEY_S))
            cameraNode_->Translate(Vector3::BACK * MOVE_SPEED * timeStep);
        if (input->GetKeyDown(KEY_A))
            cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
        if (input->GetKeyDown(KEY_D))
            cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);
        if(deskName_)
        {
            Vector3  pos=cameraNode_->GetPosition();
            Quaternion qua=cameraNode_->GetRotation();
            std::string spos=str(boost::format("x:%f y:%f z:%f  rot: %f %f %f")%pos.x_%pos.y_%pos.z_%qua.x_%qua.y_%qua.z_);
            deskName_->SetText(spos.c_str());
        }
        
        //    // Paint decal with the left mousebutton; cursor must be visible
        //    if (ui->GetCursor()->IsVisible() && input->GetMouseButtonPress(MOUSEB_LEFT))
        //        SelectCard();
        
        //SelectCard();
    }

}

//


//add by copyleft
Node* Mahjong::CreateMahJong(int32_t card,Vector3 pos,Quaternion rota)
{
    if(card<=0)
    {
        int a=0;a++;
    }
    
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    
    Node* mjNode = scene_->CreateChild("MahJong");
    mjNode->SetPosition(pos);
    mjNode->SetRotation(rota);
    
    //????????????
    
    StaticModel* mj = mjNode->CreateComponent<StaticModel>();
    mj->SetModel(cache->GetResource<Model>("Models/mahjong/mj.mdl"));
    mj->SetMaterial(1,cache->GetResource<Material>("Models/mahjong/Materials/White.xml"));
    mj->SetMaterial(2,cache->GetResource<Material>("Models/mahjong/Materials/Green.xml"));
    std::string cardFace=str(boost::format("Models/mahjong/Materials/%d.xml") % card);
    mj->SetMaterial(0,cache->GetResource<Material>(cardFace.c_str()));
    //
    mj->SetCastShadows(true);
    
    return mjNode;
}
void Mahjong::ModifyNode(Node* pNode,int32_t value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    
    std::string cardFace=str(boost::format("Models/mahjong/Materials/%d.xml") % value);
    //????????????
    StaticModel* tmp=pNode->GetComponent<StaticModel>();
    if(tmp)
        tmp->SetMaterial(0,cache->GetResource<Material>(cardFace.c_str()));
}
void Mahjong::PlayMjSound(std::string const&  soundFile,int pos)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    Sound* sound = cache->GetResource<Sound>(soundFile.c_str());
    if (sound)
    {
        if(pos>-1)
        {
            //?????? ??????
            int seatIndex=12+pos;
            Node* objectNode=scene_->GetNode(seatIndex);
            if(objectNode)
            {
                SoundSource3D *snd_3d=objectNode->GetComponent<SoundSource3D>();
                //snd_3d->SetAutoRemoveMode(REMOVE_COMPONENT);
                snd_3d->Play(sound);
            }
            
        }else{
            //?????? ??????
            // Create a SoundSource component for playing the sound. The SoundSource component plays
            // non-positional audio, so its 3D position in the scene does not matter. For positional sounds the
            // SoundSource3D component would be used instead
            SoundSource* soundSource = scene_->CreateComponent<SoundSource>();
            // Set the sound type to music so that master volume control works correctly
            soundSource->SetSoundType(SOUND_EFFECT);
            
            // Component will automatically remove itself when the sound finished playing
            soundSource->SetAutoRemoveMode(REMOVE_COMPONENT);
            soundSource->Play(sound);
            // In case we also play music, set the sound volume below maximum so that we don't clip the output
            soundSource->SetGain(0.75f);
        }
    }
}
void Mahjong::HandleAction(StringHash eventType, VariantMap& eventData)
{
    Button* button = static_cast<Button*>(GetEventSender());
    const String& soundResourceName = button->GetVar(VAR_BUTTONVALUE).GetString();
    
    //????????????????????????
    std::string btnName=soundResourceName.CString();
    if(btnName=="???")
    {   //????????? ???????????????
        auto pTxt=button->GetChildStaticCast<Text>("ShowOrNo",true);
        std::string showName=pTxt->GetText().CString();
        if(showName=="???")
            pTxt->SetText("??????");
        else if(showName=="??????")
            pTxt->SetText("???");
        SendAction(showName);
        return ;
    }

    SendAction(soundResourceName.CString());
}

//?????? ?????????????????????
void Mahjong::HandlePiaoAction(StringHash eventType, VariantMap& eventData)
{
    Button* button = static_cast<Button*>(GetEventSender());
    const String& piaoValue = button->GetVar(VAR_PIAOVALUE).GetString();
    int piaoBase=atoi(piaoValue.CString());
    
    std::string cmd=str(boost::format("{\"action\":%d,\"card\":%d}")%UserAction::ChoosePiao%piaoBase);
    SendCommand(cmd);
    
//    if(layoutReady_)
//        layoutReady_->SetVisible(false);
}

//?????????????????? ????????????
void Mahjong::ShowGameResult(int excess ,std::vector<std::string>& vPlayers)
{
    //??????xml ??????
    UIElement* root = GetSubsystem<UI>()->GetRoot();
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    
    auto layout=cache->GetResource<XMLFile>("UI/mj_gameResult.xml");
    SharedPtr<UIElement> layoutRoot=GetSubsystem<UI>()->LoadLayout(layout);
    root->AddChild(layoutRoot);
    //layoutRoot->SetOpacity(.6);
    
    auto xBotton=layoutRoot->GetChildStaticCast<Button>("BTN_EXIT",true);
    SubscribeToEvent(xBotton, E_PRESSED, URHO3D_HANDLER(Mahjong, HandleExit));
    
    auto xBottonContinue=layoutRoot->GetChildStaticCast<Button>("BTN_CONTINUE",true);
    SubscribeToEvent(xBottonContinue, E_PRESSED, URHO3D_HANDLER(Mahjong, HandleContinue));
    
    auto window=root->GetChildStaticCast<UIElement>("WIN_RESULT",true);
    if(window)
        window->SetVisible(true);
    


    
    //UIElement::SetBlendMode();
    //layoutRoot->setblend
    
    //????????????
//    auto text=layoutRoot->GetChildStaticCast<Text>("TXT_TITLE",true);
//    text->SetText("??????\n??????");

    if(vPlayers.size()>=3)
    {
        auto T1=layoutRoot->GetChildStaticCast<Text>("TXT_PLAYER1",true);
        T1->SetText(vPlayers[0].c_str());
        
        auto T2=layoutRoot->GetChildStaticCast<Text>("TXT_PLAYER2",true);
        T2->SetText(vPlayers[1].c_str());
        
        auto T3=layoutRoot->GetChildStaticCast<Text>("TXT_PLAYER3",true);
        T3->SetText(vPlayers[2].c_str());
        //??????????????????
    }

}

//????????????
void Mahjong::HandleExit(StringHash eventType, VariantMap& eventData)
{
    if(roundResult_)
        roundResult_->SetVisible(false);
    
    StopNetWork();
    engine_->Exit();
}
//?????????????????? ?????????????????????
void Mahjong::HandleShare(StringHash eventType, VariantMap& eventData)
{
    /*
    if(roundResult_)
        roundResult_->SetVisible(false);
     */
    engine_->Exit();
}
//????????????
void Mahjong::HandleContinue(StringHash eventType, VariantMap& eventData)
{
    SendAction("????????????");
    
    UIElement* root = GetSubsystem<UI>()->GetRoot();
    auto window=root->GetChildStaticCast<UIElement>("WIN_Round",true);
    if(window)
        window->SetVisible(false);
}
void Mahjong::SetCardFree(int number)
{
    if(number>0&&number<100)
    {
        ResourceCache* cache = GetSubsystem<ResourceCache>();
        Texture2D* pTexture=cache->GetResource<Texture2D>("Textures/mj/center.jpg");
        Image*   pNumber=cache->GetResource<Image>("Textures/mj/free.jpg");
        
        int index=number/10;
        pTexture->SetData(0, 10, 16, 22, 32, pNumber->GetSubimage(IntRect(22*index,0,22*(index+1),32))->GetData());
        index=number%10;
        pTexture->SetData(0, 32,16,22, 32, pNumber->GetSubimage(IntRect(22*index,0,22*(index+1),32))->GetData());
        deskObject_->GetMaterial(5)->SetTexture(TextureUnit::TU_DIFFUSE, pTexture);
    }
}
void Mahjong::AddMark(Node* pNode,int value,std::string const& mark)
{
    //????????????
    StaticModel* mj=pNode->GetComponent<StaticModel>();
    if(mj)
    {
        ResourceCache* cache = GetSubsystem<ResourceCache>();
        
//        std::string  strMat=str(boost::format("Models/mahjong/Materials/%d.xml") % value);
//        SharedPtr<Material> mat=cache->GetTempResource<Material>(strMat.c_str());
        
        std::string  v1=str(boost::format("Textures/mj/%d.jpg") % value);
        //Texture2D*   texture=cache->GetResource<Texture2D>(v1.c_str());//Texture2D ?
        Image*   pFace=cache->GetResource<Image>(v1.c_str());
        Image*   pLogo=nullptr;
        if("kou"==mark)
            pLogo=cache->GetResource<Image>("Textures/mj/pa.png");
        else if("no"==mark)
            pLogo=cache->GetResource<Image>("Textures/mj/no.png");
        if(pLogo)
        {
            const int w=pLogo->GetWidth();
            const int h=pLogo->GetHeight();
            Color tr=pLogo->GetPixel(w-1, h-1);//get transparent
            for(int i=0;i<w;i++)
            {
                for(int j=0;j<h;j++)
                {
                    Color cr=pLogo->GetPixel(i, j);
                    if(cr!=tr)
                        pFace->SetPixel(i, j, cr);
                }
            }
        }
        

        
        SharedPtr<Texture2D> texture(new Texture2D(context_));
        texture->SetData(pFace);
        
        

        SharedPtr<Material> mat=mj->GetMaterial(0)->Clone();
        mat->SetTexture(TextureUnit::TU_DIFFUSE,texture);
        
        mj->SetMaterial(0,mat);
    }
    
}

//?????????
void Mahjong::CreateRoundResultRow(SharedPtr<UIElement> pRoot,int index,bool zhuang,std::string& avatar,std::string const& name
                                   ,std::string const&piaoDesp,std::vector<int> cardsPub,std::vector<int> cardsHand,int card
                                   ,std::string const&total,std::string const&score,std::string const&result)
{
    int posX;
    int posY;
    
//    UIElement* root         = GetSubsystem<UI>()->GetRoot();
//    Graphics* graphics      = GetSubsystem<Graphics>();
//    ResourceCache* cache    = GetSubsystem<ResourceCache>();
//    //??????xml ??????
//    auto layout=cache->GetResource<XMLFile>("UI/mj_gameRow.xml");
//    auto roundResult=GetSubsystem<UI>()->LoadLayout(layout);
//    root->AddChild(roundResult);

    std::string rawName=str(boost::format("row_%d") % index);
    auto pRow=pRoot->GetChildStaticCast<UIElement>(rawName.c_str(),true);
    if(pRow)
    {
        BorderImage* pAvatar=pRow->GetChildStaticCast<BorderImage>("avatar",true);
        if(pAvatar)
            pAvatar->SetTexture(GetAvatarTexture(avatar));

        Text* pName=pRow->GetChildStaticCast<Text>("playerName",true);
        pName->SetText(name.c_str());
        
        Text* pPiao=pRow->GetChildStaticCast<Text>("piaoBase",true);
        pPiao->SetText(piaoDesp.c_str());
        
        Text* pScoreT=pRow->GetChildStaticCast<Text>("scoreTotal",true);
        pScoreT->SetText(total.c_str());
        
        Text* pScoreS=pRow->GetChildStaticCast<Text>("scoreSub",true);
        pScoreS->SetText(score.c_str());
        
        Text* pResult=pRow->GetChildStaticCast<Text>("resultDsp",true);
        pResult->SetText(result.c_str());

        //???????????????
        auto pCards=pRow->GetChildStaticCast<UIElement>("cards",true);
        pCards->RemoveAllChildren();
        
        IntVector2 size=pCards->GetSize();
        IntRect rc;
        rc.top_=0;
        rc.left_=size.x_-82;
        //??????
        if(card>0)
        {//?????? ????????????   ??? 82x128
            rc=Create2DMahjong(pCards, card, 0,rc.left_, rc.top_, 0.8);
        }
        
        //rc.left_-=120; //?????????????????????
        //??????
        rc.left_-=80+40;
        auto rit=cardsHand.rbegin();
        while(rit!=cardsHand.rend())
        {
            rc=Create2DMahjong(pCards, *rit, 0,rc.left_, rc.top_, 0.8);
            rc.left_-=rc.Width();
            
            ++rit;
        }
        //??????
        rc.left_=0;
        rc.top_=8;
        int oc=0;
        for(auto c:cardsPub)
        {
            if(oc!=0)
            {
                if(oc>0&&oc!=c)
                    rc.left_+=rc.Width()*1.2;
                else
                    rc.left_+=rc.Width();
            }
            rc=Create2DMahjong(pCards, c, 0,rc.left_, rc.top_, 0.6);
            oc=c;
        }
    }
}
//???????????????????????????
void Mahjong::ShowRoundResult(ptree& pt,std::string const& title)
{
    //????????????
    int escess=pt.get("escess",2);

    int winner=pt.get("winner",2);
    int firer=pt.get("firer",2);
    
    
    UIElement* root         = GetSubsystem<UI>()->GetRoot();
    Graphics* graphics      = GetSubsystem<Graphics>();
    ResourceCache* cache    = GetSubsystem<ResourceCache>();
    //??????xml ??????
    auto layout=cache->GetResource<XMLFile>("UI/mj_gameRow.xml");
    roundResult_=GetSubsystem<UI>()->LoadLayout(layout);
    root->AddChild(roundResult_);
    roundResult_->SetVisible(true);
    

    
    Text* pTitle=roundResult_->GetChildStaticCast<Text>("title",true);
    pTitle->SetText(title.c_str());
    
    std::string sTime=pt.get("time","2019-3-3 13:00:00");
    int round=pt.get("round",0);
    int roundT=pt.get("roundT",0);
    int deskNO=pt.get("deskNO",0);
    std::string roundDesp=str(boost::format("??????:%d/%d???\n?????????:%d\n%S") % round % roundT % deskNO%sTime);
    
    Text* pDesp=roundResult_->GetChildStaticCast<Text>("desp",true);
    pDesp->SetText(roundDesp.c_str());
    
    //????????????
    for(int i=0;i<4;i++)
    {
        std::string rowName=str(boost::format("row_%d") % i);
        UIElement* pRow=roundResult_->GetChildStaticCast<Text>(rowName.c_str(),true);
        if(pRow)
            pRow->SetVisible(false);
    }
    
    
    
    auto xBotton=roundResult_->GetChildStaticCast<Button>("btnShare",true);
    SubscribeToEvent(xBotton, E_PRESSED, URHO3D_HANDLER(Mahjong, HandleShare));
    
    auto xBottonContinue=roundResult_->GetChildStaticCast<Button>("btnContinue",true);
    SubscribeToEvent(xBottonContinue, E_PRESSED, URHO3D_HANDLER(Mahjong, HandleContinue));
    
    
    // ????????????
    int i=0;
    ptree gambler_array = pt.get_child("players");  // get_child??????????????????
    BOOST_FOREACH(boost::property_tree::ptree::value_type &v, gambler_array)
    {
        int64_t uid          =v.second.get("profile.id",int64_t(0));
        int64_t sex          =v.second.get("profile.sex",0);
        std::string uname    =v.second.get("profile.name","");
        std::string uavatar  =v.second.get("profile.avatar","");
            uavatar=Gambler::GetAvatar(sex, uid,uavatar);
        
        //?????????????????????
        std::string result  =v.second.get("result","");
        int piaoBase        =v.second.get("piaoBase",0);
        int piao            =v.second.get("scores.piao",0);
        int gang            =v.second.get("scores.gang",0);
        int fan             =v.second.get("scores.fan",0);
        int hu              =v.second.get("scores.hu",0);
        int horse           =v.second.get("scores.horse",0);
        
        int card=v.second.get("card",0);
        
        std::vector<int32_t>  cardsHand;
        ptree cardArray = v.second.get_child("cardsHand");  // get_child??????????????????
        BOOST_FOREACH(boost::property_tree::ptree::value_type &v,cardArray)
        {
            int32_t value=v.second.get_value<int32_t>();
            cardsHand.push_back(value);
        }
        
        std::vector<int32_t>  cardsPub;
        cardArray = v.second.get_child("cardsPub");  // get_child??????????????????
        BOOST_FOREACH(boost::property_tree::ptree::value_type &v,cardArray)
        {
            int32_t value=v.second.get_value<int32_t>();
            cardsPub.push_back(value);
        }
        
        std::string total=str(boost::format("??????:%d") % (hu+piao+gang+horse));
        std::string subScore=str(boost::format("???:%d ???:%d ???:%d ???:%d ???:%d") % piao % gang % horse % fan %hu);
       
        std::string piaoDesp=str(boost::format("???:%d  %s") % piaoBase % result);
        bool zhuang=false;
        
        bool haveWin=false;
        if(winner==i)
            haveWin=true;
            
        std::string ziMoOrDianpao="";
        if(winner==i&&firer==winner)
            ziMoOrDianpao="??????";
        else if(firer==i)
            ziMoOrDianpao="??????";
        
        
        //?????????
        std::string rowName=str(boost::format("row_%d") % i);
        UIElement* pRow=roundResult_->GetChildStaticCast<UIElement>(rowName.c_str(),true);
        if(pRow)
            pRow->SetVisible(true);
        
        //???????????????
        CreateRoundResultRow(roundResult_,i++,zhuang,uavatar,uname,piaoDesp,cardsPub,cardsHand,card,total,subScore,ziMoOrDianpao);
    }
    
}
#include <Urho3D/UI/MessageBox.h>
//?????????????????? ????????????
void Mahjong::ShowErrorAndExit(std::string const& message)
{
    //GetSubsystem<UI>()->GetCursor()->SetVisible(true);
    UIElement* root         = GetSubsystem<UI>()->GetRoot();
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    
    if(layoutReady_)
        layoutReady_->SetVisible(false);
    if(layoutPlayerInfo_)
        layoutPlayerInfo_->SetVisible(false);
    if(roundResult_)
        roundResult_->SetVisible(false);

    
    auto layout=cache->GetResource<XMLFile>("UI/mj_MessageBox.xml");
    SharedPtr<UIElement> layoutRoot=GetSubsystem<UI>()->LoadLayout(layout);
    root->AddChild(layoutRoot);
    //layoutRoot->SetOpacity(.6);
    
    if(layoutRoot)
    {
        auto title=layoutRoot->GetChildStaticCast<Text>("TitleText",true);
        title->SetText("??????!");
        auto msg=layoutRoot->GetChildStaticCast<Text>("MessageText",true);
        msg->SetText(message.c_str());
        
        
        auto okBotton=layoutRoot->GetChildStaticCast<Button>("OkButton",true);
        SubscribeToEvent(okBotton, E_PRESSED, URHO3D_HANDLER(Mahjong, HandleExit));
    }
}
void Mahjong::HandleKeyUpExit(StringHash /*eventType*/, VariantMap& eventData)
{
    using namespace KeyUp;
    
    int key = eventData[P_KEY].GetInt();
    
    // Close console (if open) or exit when ESC is pressed
    if (key == KEY_AC_BACK||key==SDLK_RETURN||key==SDLK_RETURN2)
    {
        engine_->Exit();
    }
}

/*
 // ?????? loadxml ??????urho ??????
 https://dins.site/coding-lib-urho3d-ui-chs/
 
 https://zh.fonts2u.com/mahjong-plain.%E5%AD%97%E4%BD%93
 ??????????????????
 1 a
 2 s
 3 d
 4 f
 5 g
 6 h
 7 j
 8 k
 9 i
 11 z
 12 x
 13 c
 14 v
 15 b
 16 n
 17 m
 18 ,
 19 .
 31 7
 33 6
 35 5
 */
