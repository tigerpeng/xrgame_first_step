#pragma once
/*
  视频会议
 */
#include "VUILayout.h"

class VUIMetting : public VUILayout
                 //, public std::enable_shared_from_this<VUIMetting>
{
public:
    VUIMetting(Context* context,bool request);
    
    void LoadXmlUI(SharedPtr<Scene> scene,ptree& pt) override;
    void OnCommand(ptree& pt) override;

    //std::shared_ptr<Derived> pointer = std::dynamic_pointer_cast<Derived>(shared_from_this());
    
private:
    //meeting
    void SubscribeToEvents();
    void HandleUpdate(StringHash eventType, VariantMap& eventData);
    
    void HandlebtnRiseHand(StringHash eventType, VariantMap& eventData);
    void ShowWaiters();
    void DownloadingImage();
    void HandleWaiterPress(StringHash eventType, VariantMap& eventData);
    void HandleWaiterRelease(StringHash eventType, VariantMap& eventData);

    
    void HandleTouchBegin(StringHash /*eventType*/, VariantMap& eventData);
    void HandleTouchEnd(StringHash eventType, VariantMap& eventData);
    void HandleTouchMove(StringHash eventType, VariantMap& eventData);
    

    int64_t GetPictureUID(IntVector2 pt);
    
private:
    SharedPtr<HttpRequest>                      imgRequest_;
    std::multimap<std::string,Button*>          downloading_;
    
    IntVector2                                  touchStart_;
    String                                      touchUserID_;

    //麦序
    std::shared_ptr<std::list<std::string>>     pLstWaiters_;
    std::mutex                                  mtx_;
};
