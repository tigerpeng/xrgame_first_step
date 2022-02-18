#pragma once
#include <memory>
#include <string>
//fro xml and json
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
using namespace boost::property_tree;

#include "Urho3DAll.h"
#include <facepower/api_facepower.h>

//std::dynamic_pointer_cast
class VUILayout : public Object
                //, public std::enable_shared_from_this<VUILayout>
{
    URHO3D_OBJECT(VUILayout, Object);
public:
    VUILayout(Context* context,bool request);
    
    virtual void LoadXmlUI(SharedPtr<Scene> scene,ptree& pt) {}
    virtual void OnCommand(ptree& pt);
public:
    bool Exit(){return exit_;}
    static void SetFacePower(FacePowerPtr fp);
private:
   void HandleUpdate(StringHash eventType, VariantMap& eventData);
protected:
    bool                        request_;               //请求方
    bool                        exit_;
    SharedPtr<UIElement>        layout_;                //统一ui界面的操作
        
    static FacePowerPtr         facepower_;
private:
    //字幕
    SharedPtr<Text>             subtitle_;
    std::string                 strSubTitle_;
    bool                        subChange_;
};
