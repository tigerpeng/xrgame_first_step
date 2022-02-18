#include "VUIChat1V1.h"

VUIChat1V1::VUIChat1V1(Context* context,bool request)
:VUILayout(context,request)
,headSet_(true)
,videoOpen_(true)
{
}

void VUIChat1V1::LoadXmlUI(SharedPtr<Scene> scene,ptree& pt)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    UIElement* root = GetSubsystem<UI>()->GetRoot();
    auto* graphics = GetSubsystem<Graphics>();
    
    auto layout=cache->GetResource<XMLFile>("UI/81_request.xml");
    layout_=GetSubsystem<UI>()->LoadLayout(layout);
    root->AddChild(layout_);
    //layoutRequest_->SetLayout(LM_VERTICAL);
    
    
    //for talking ...
    auto rsTalking=cache->GetResource<XMLFile>("UI/81_talking.xml");
    layoutTalking_=GetSubsystem<UI>()->LoadLayout(rsTalking);
    root->AddChild(layoutTalking_);
    if(layoutTalking_)
    {
        int pianyi=10; //整体便宜5
        layoutTalking_->SetWidth(graphics->GetWidth()-pianyi);
        layoutTalking_->SetHeight(graphics->GetHeight()-pianyi);
        layoutTalking_->SetAlignment(HA_CENTER,VA_CENTER);
        
        
        auto xbtnNoise=layoutTalking_->GetChildStaticCast<Button>("btnNoise",true);
        if(xbtnNoise)
            SubscribeToEvent(xbtnNoise, E_PRESSED, URHO3D_HANDLER(VUIChat1V1,HandleBtnNoise));
        
        auto xbtnVideo=layoutTalking_->GetChildStaticCast<Button>("btnVideoOpen",true);
        if(xbtnVideo)
            SubscribeToEvent(xbtnVideo, E_PRESSED, URHO3D_HANDLER(VUIChat1V1,HandleBtnVideo));
    
        auto xbtnExit=layoutTalking_->GetChildStaticCast<Button>("btnExit",true);
        if(xbtnExit)
            SubscribeToEvent(xbtnExit, E_PRESSED, URHO3D_HANDLER(VUIChat1V1,HandleBtnExit));
    }
    
    
    //绑定按钮事件
    auto trans=root->GetChildStaticCast<UIElement>("callUI",true);
    if(trans){
        trans->SetSize(graphics->GetWidth(),graphics->GetHeight());
        trans->SetPosition(0,0);
    }

//    //麦克风按下和松开事件
//    auto xbtnMic=layout_->GetChildStaticCast<Button>("btnMike",true);
//    if(xbtnMic){
//        SubscribeToEvent(xbtnMic, E_PRESSED, URHO3D_HANDLER(VUIChat1V1,HandleBtnMicPress));
//        SubscribeToEvent(xbtnMic, E_RELEASED, URHO3D_HANDLER(VUIChat1V1,HandleBtnMicRelease));
//        xbtnMic->SetVisible(false);//被动模式 不显示语音按钮
//    }

    //调试信息
    char szBuufer[1024];
    snprintf(szBuufer,100,"窗口信息 width:%d height:%d" , graphics->GetWidth(), graphics->GetHeight());
    auto pTxt=layout_->GetChildStaticCast<Text>("call_info",true);
    if(pTxt)
    {
        //pTxt->SetText(szBuufer);
       
        //pTxt->SetColor(Color(1,0,0));
        //pTxt->SetPosition(100, 100);
    }

    //根据用户的显示询问对话
    layoutTips_=layout_->GetChildStaticCast<UIElement>("tipsInfo",true);
    if(layoutTips_)
    {
        auto xbtnAgree=layout_->GetChildStaticCast<Button>("btnAgree",true);
        if(xbtnAgree)
            SubscribeToEvent(xbtnAgree, E_PRESSED, URHO3D_HANDLER(VUIChat1V1,HandleBtnAgree));

        auto xbtnExit=layout_->GetChildStaticCast<Button>("btnExit",true);
        if(xbtnExit)
            SubscribeToEvent(xbtnExit, E_PRESSED, URHO3D_HANDLER(VUIChat1V1,HandleBtnExit));

//         //layoutTips_->SetVisible(false);
//        std::string str="是否同意用户的视频会话请求\n昵称:彭洪\n年龄:35\n这周性情不错";
//                    str="正在等待用户接听视频会话...";
//                    str="用户的费用已经消耗完毕是否继续会话？";
//        auto pTxt=layout_->GetChildStaticCast<Text>("tips_text",true);
//        pTxt->SetText(str.c_str());
    }
    
    
    // Create ring sound source
    ringSource_ = scene->CreateComponent<SoundSource>();
    // Set the sound type to music so that master volume control works correctly
    ringSource_->SetSoundType(SOUND_MUSIC);
    

    //加载音乐文件
    std::string  ringStr;
    if(request_)
        ringStr="Sounds/81/Triton.ogg";
    else
        ringStr="Sounds/81/Dione.ogg";

    Sound* music = cache->GetResource<Sound>(ringStr.c_str());
    music->SetLooped(true);
    ringSource_->Play(music);

    
    //订阅事件
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(VUIChat1V1, HandleUpdate));
    
    WelcomeScreen(pt);
}


void VUIChat1V1::WelcomeScreen(ptree& pt)
{
    std::string initText;
    char szBuufer[1024];
    if(request_)
        snprintf(szBuufer,1024,"正在呼叫用户[%s]...",pt.get("to.name","").c_str());
    else
        snprintf(szBuufer,1024,"是否接听用户[%s]的视频电话...",pt.get("from.name","").c_str());
    
    initText=szBuufer;
    
    ShowTips(initText,request_?1:2);
}
void VUIChat1V1::ShowTips(std::string const& strText,int btns)
{
    //显示提示信息
    auto pTxt=layout_->GetChildStaticCast<Text>("tips_text",true);
    if(pTxt){
        pTxt->SetColor(Color(1,0,0));
        pTxt->SetAlignment(HA_CENTER, VA_CUSTOM);
        pTxt->SetText(strText.c_str());
    }
    
    //显示按钮
    auto pBtn=layout_->GetChildStaticCast<Text>("btnAgree",true);
    if(pBtn){
        if(btns==1)
            pBtn->SetVisible(false);
        else
            pBtn->SetVisible(true);
    }
    
     auto pBtnExit=layout_->GetChildStaticCast<Text>("btnExit",true);
     if(pBtnExit)
     {
         if(btns==1)
             pBtnExit->SetAlignment(HA_CENTER, VA_CUSTOM);
         else
             pBtnExit->SetAlignment(HA_CUSTOM, VA_CUSTOM);
     }
    
    
    if(layoutTips_)
        layoutTips_->SetVisible(true);
}
void VUIChat1V1::HandleBtnNoise(StringHash eventType, VariantMap& eventData)
{
    headSet_=!headSet_;
    
    auto xbtnNoise=layoutTalking_->GetChildStaticCast<Button>("btnNoise",true);
            
    Texture2D*   texture;
    if(headSet_)
    texture=GetSubsystem<ResourceCache>()->GetResource<Texture2D>("Textures/81/headset.png");
    else
        texture=GetSubsystem<ResourceCache>()->GetResource<Texture2D>("Textures/81/headsetNo.png");
    
    xbtnNoise->SetTexture(texture);
    
    
    
    if(facepower_)
        facepower_->cmd("{\"cmd\":\"webrtc3A1V\",\"where\":\"81\"}");
    
}
void VUIChat1V1::HandleBtnVideo(StringHash eventType, VariantMap& eventData)
{
    videoOpen_=!videoOpen_;
    
    Texture2D*   texture;
    if(videoOpen_)
    texture=GetSubsystem<ResourceCache>()->GetResource<Texture2D>("Textures/81/video.png");
    else
        texture=GetSubsystem<ResourceCache>()->GetResource<Texture2D>("Textures/81/videoNo.png");
    
    auto xbtnVideo=layoutTalking_->GetChildStaticCast<Button>("btnVideoOpen",true);
    if(xbtnVideo&&texture)
        xbtnVideo->SetTexture(texture);
    
    
    if(facepower_)
        facepower_->cmd("{\"cmd\":\"videoOpen\",\"where\":\"81\"}");
}
void VUIChat1V1::HandleBtnAgree(StringHash eventType, VariantMap& eventData)
{
    if(layoutTips_)
        layoutTips_->SetVisible(false);

    if(!request_)
    {
        std::string strJson="{\"cmd\":\"-ask\",\"agree\":true,\"type\":2}";
        if(facepower_)
            facepower_->cmd(strJson);
    }
    if(ringSource_)
        ringSource_->Stop();

    //接听的按钮从此变成继续
    auto pTxt=layout_->GetChildStaticCast<Text>("txtAgree",true);
    if(pTxt)
        pTxt->SetText("继续");
    
}

void VUIChat1V1::HandleBtnExit(StringHash eventType, VariantMap& eventData)
{
    if(ringSource_)
        ringSource_->Stop();

    if(!request_)
    {//被动模式
       //同意
        std::string strJson="{\"cmd\":\"-ask\",\"agree\":false,\"type\":2}";
        if(facepower_)
            facepower_->cmd(strJson);
    }

    if(facepower_){
        facepower_->setAudioCallBack(nullptr);
        facepower_->setVideoCallBack(nullptr);

        facepower_->cmd("{\"cmd\":\"endTalk\",\"where\":\"Urho3D\"}");

        exit_=true;
    }
}


void VUIChat1V1::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    //显示错误信息
    if(!error_text_.empty())
    {
        ShowTips(error_text_,error_btns_);
        
        error_text_="";
        error_btns_=0;
    }
}
void VUIChat1V1::OnCommand(ptree& pt)
{
    const std::string cmd=pt.get("cmd","");
    
    if("chatStart"==cmd)
    {
        //关闭请求画面和提示音
        if(layoutTips_)
            layoutTips_->SetVisible(false);
        if(ringSource_)
            ringSource_->Stop();
    }else if("ask_quit"==cmd)
    {
        error_text_=pt.get("why","");       //提示的信息
        error_btns_=pt.get("btns",0);       //显示的按钮数  1 只有退出
    }
    
    VUILayout::OnCommand(pt);
}
/*
  交互过程定义
 1 请求
    用户webui，点击视频聊天按钮 发起请求----启动videotalks-- vtalks向facepower 发起请求并接受视频
    收到同意，显示视频音频
    收到拒绝，关闭
 
2 应答
   用户收到应答---网页展示是同意，还是拒绝
    1）  拒绝  直接关闭请求
    2)   同意  打开vtalks 显示视频   同时显示关闭按钮
转账
 1 发起者首次转账给对方，对方收到转账通知后同意视频
 2 后续持续转账
 3 转账延迟时间定义为     30--60秒
 4 自动转账 自动结算
 */
