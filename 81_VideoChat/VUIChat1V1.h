#pragma once
#include "VUILayout.h"

class VUIChat1V1 : public VUILayout
{
public:
    VUIChat1V1(Context* context,bool request);
    
    void LoadXmlUI(SharedPtr<Scene> scene,ptree& pt) override;
    
    void OnCommand(ptree& pt) override;
private:
    void WelcomeScreen(ptree& pt);
    void ShowTips(std::string const& strText,int status=-1);
    
    void HandleUpdate(StringHash eventType, VariantMap& eventData);
    //std::shared_ptr<Derived> pointer = std::dynamic_pointer_cast<Derived>(shared_from_this());
    
    //同意 or 继续
    void HandleBtnAgree(StringHash eventType, VariantMap& eventData);
    
    //降噪
    void HandleBtnNoise(StringHash eventType, VariantMap& eventData);
    
    //打开自己的视频
    void HandleBtnVideo(StringHash eventType, VariantMap& eventData);

    
    //退出 or 取消 按钮
    void HandleBtnExit(StringHash eventType, VariantMap& eventData);
private:
    SharedPtr<UIElement>        layoutTips_;
    
    SharedPtr<UIElement>        layoutTalking_;
    
    SharedPtr<SoundSource>      ringSource_;
    
    bool                        headSet_;
    bool                        videoOpen_;
    
    std::string  error_text_;
    int          error_btns_;
};
