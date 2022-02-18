#include "VUILayout.h"

FacePowerPtr    VUILayout::facepower_=nullptr;
VUILayout::VUILayout(Context* context,bool request):Object(context)
,request_(request)
,exit_(false)
,subChange_(false)
{
    SubscribeToEvent(E_SCENEPOSTUPDATE, URHO3D_HANDLER(VUILayout, HandleUpdate));
}
void VUILayout::SetFacePower(FacePowerPtr fp)
{
    facepower_=fp;
}
void VUILayout::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    if(subChange_)
    {
        subChange_=false;
        
        if(layout_&&subtitle_==nullptr)
        {   //动态创建字幕
            subtitle_=layout_->CreateChild<Text>();
            subtitle_->SetAlignment(HA_CENTER, VA_BOTTOM); //VA_CUSTOM
            subtitle_->SetColor(Color(1,0,0));
            
            ResourceCache* cache = GetSubsystem<ResourceCache>();
            Font* fontChina = cache->GetResource<Font>("Fonts/simfang.ttf");
            subtitle_->SetFont(fontChina, 36);//12
        }
        
        if(subtitle_)
           subtitle_->SetText(strSubTitle_.c_str());
    }
}

void VUILayout::OnCommand(ptree& pt)
{
    const std::string cmd=pt.get("cmd","");
    
    if("showSubtitle"==cmd)
    {
        //显示字幕
        strSubTitle_=pt.get("text","");
        subChange_=true;
    }
}
