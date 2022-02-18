#include "VUIMeeting.h"

static const StringHash VAR_WAITERID("waiterID");


VUIMetting::VUIMetting(Context* context,bool request)
:VUILayout(context,request)
{
    
}

void VUIMetting::OnCommand(ptree& pt)
{
    const std::string cmd=pt.get("cmd","");
    
    if("showWaiters"==cmd)
    {
        std::lock_guard<std::mutex> lck(mtx_);
        pLstWaiters_=std::make_shared<std::list<std::string>>();
        
        //strJson="{\"cmd\":\"showWaiters\",\"waiters\":[10001,10002,10003,10004,10005,10006]}";
        ptree btnArray = pt.get_child("waiters");  // get_child得到数组对象
        BOOST_FOREACH(boost::property_tree::ptree::value_type &v,btnArray)
        {
            std::string value=v.second.get_value<std::string>();
            pLstWaiters_->push_back(value);
        }
    }
    
    
    VUILayout::OnCommand(pt);
}



void VUIMetting::LoadXmlUI(SharedPtr<Scene> scene,ptree& pt)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    UIElement* root = GetSubsystem<UI>()->GetRoot();
    auto* graphics = GetSubsystem<Graphics>();
    
     auto layoutXml=cache->GetResource<XMLFile>("UI/81_meeting.xml");
     layout_=GetSubsystem<UI>()->LoadLayout(layoutXml);
     root->AddChild(layout_);
    
    auto trans=root->GetChildStaticCast<UIElement>("meetingUI",true);
    if(trans){
        trans->SetSize(graphics->GetWidth(),graphics->GetHeight());
        trans->SetPosition(0,0);
    }
    
    auto btnRiseHand=layout_->GetChildStaticCast<Button>("btnRiseHand",true);
    if(btnRiseHand)
    {
        SubscribeToEvent(btnRiseHand, E_PRESSED, URHO3D_HANDLER(VUIMetting,HandlebtnRiseHand));
        
//            if(!request_) //创建者不需要举手按钮
//                btnRiseHand->SetVisible(false);
    }
    
    SubscribeToEvents();
}

void VUIMetting::DownloadingImage()
{
    static const size_t MaxBufferSize=65535;
    static char*        pBufferStart=nullptr;
    static size_t       BufferSize=0;
    
    auto*  network=GetSubsystem<Network>();
    if(imgRequest_.Null())
    {
        if(downloading_.size()>0)
        {
            auto it=downloading_.begin();
            imgRequest_=network->MakeHttpRequest(it->first.c_str());
            if(pBufferStart==nullptr)
                pBufferStart=new char[MaxBufferSize];
            BufferSize=0;
        }else if(pBufferStart!=nullptr)
        {
            delete []pBufferStart;
            pBufferStart=nullptr;
        }
    }else{
        HttpRequestState  state=imgRequest_->GetState();
        if(state==HTTP_ERROR)
        {//PASS
            auto it=downloading_.begin();
            if(it!=downloading_.end())
            {
                //使用缺省头像
                Texture2D*   texture=GetSubsystem<ResourceCache>()->GetResource<Texture2D>("Textures/81/avatar.jpg");
                Button* pImg=it->second;
                if(pImg)
                    pImg->SetTexture(texture);
                
                downloading_.erase(it);
                imgRequest_.Reset();
            }
        }else
        {
            if(state==HTTP_OPEN)
                 BufferSize+=imgRequest_->Read((void*)(pBufferStart+BufferSize), MaxBufferSize-BufferSize);
            
            if(BufferSize>=MaxBufferSize)
            {// 不下载超过 65k的图片  pass
                //使用缺省头像
                Texture2D*   texture=GetSubsystem<ResourceCache>()->GetResource<Texture2D>("Textures/81/avatar.jpg");
                Button* pImg=downloading_.begin()->second;
                if(pImg)
                    pImg->SetTexture(texture);
                
                downloading_.erase(downloading_.begin());
                imgRequest_.Reset();
                return ;
            }

                
            if(state==HTTP_CLOSED)
            {
                BufferSize+=imgRequest_->Read((void*)(pBufferStart+BufferSize), MaxBufferSize-BufferSize);
                MemoryBuffer  buf(pBufferStart,BufferSize);
                
                SharedPtr<Image>  img(new Image(context_));
                img->Load(buf);
                //img to texture
                SharedPtr<Texture2D> texture(new Texture2D(context_));
                texture->SetData(img);
                //texture to imageborder
                Button* pImg=downloading_.begin()->second;
                if(pImg)
                    pImg->SetTexture(texture);
                
                
                //for next
                downloading_.erase(downloading_.begin());
                imgRequest_.Reset();
            }
            
        }
    }
}

void VUIMetting::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(VUIMetting, HandleUpdate));
    
    //touch event
    SubscribeToEvent(E_TOUCHBEGIN, URHO3D_HANDLER(VUIMetting, HandleTouchBegin));
    SubscribeToEvent(E_TOUCHEND, URHO3D_HANDLER(VUIMetting, HandleTouchEnd));
    //SubscribeToEvent(E_TOUCHMOVE, URHO3D_HANDLER(VideoChat, HandleTouchMove));
}

void VUIMetting::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    DownloadingImage();
 
    ShowWaiters();
}


//meeting btnRiseHand
void VUIMetting::HandleTouchBegin(StringHash eventType, VariantMap& eventData)
{
    using namespace TouchBegin;
    touchStart_= IntVector2(eventData[P_X].GetInt(), eventData[P_Y].GetInt());
}
void VUIMetting::HandleTouchEnd(StringHash eventType, VariantMap& eventData)
{
    if(touchStart_==IntVector2(0,0))
        return ;

    using namespace TouchBegin;
    IntVector2 touchEnd = IntVector2(eventData[P_X].GetInt(), eventData[P_Y].GetInt());

    std::string strCMD;
    int dy=touchEnd.y_-touchStart_.y_;
    int dx=touchEnd.x_-touchStart_.x_;
    
    if(!touchUserID_.Empty())
    {
        if(std::abs(dy)>100)
        {//删除用户
            char szBuufer[1024];
            snprintf(szBuufer,1024,"{\"cmd\":\"waiterRemove\",\"to\":%s,\"where\":\"meeting\"}",touchUserID_.CString());
            strCMD=szBuufer;
        }else if(std::abs(dx)>100)
        {//上麦
            char szBuufer[1024];
            snprintf(szBuufer,1024,"{\"cmd\":\"waiterOnStage\",\"to\":%s,\"where\":\"meeting\"}",touchUserID_.CString());
            strCMD=szBuufer;
        }
        
        if(!strCMD.empty())
        {
            if(facepower_)
                facepower_->cmd(strCMD);
            
//            //可以不用 让系统消息去删除
//            auto waiterRc=layout_->GetChildStaticCast<UIElement>("waiters",true);
//            if(waiterRc&&touchBtn_)
//            {
//                waiterRc->RemoveChild(touchBtn_);
//                touchBtn_=nullptr;
//            }
        }
        touchUserID_="";
    }else
    {
//        auto pText=layout_->GetChildStaticCast<Text>("debug_info",true);
//        if(pText){
//
//            char szBuufer[1024];
//            snprintf(szBuufer,1024,"鼠标起点未知x=%d,y=%d",touchStart_.x_,touchStart_.y_);
//            pText->SetText(szBuufer);
//        }
        if(dy>50)
        {//下视频
            int64_t uid=GetPictureUID(touchStart_);
            if(uid>0&&facepower_)
            {
                char szBuufer[1024];
                snprintf(szBuufer,1024,"{\"cmd\":\"offStage\",\"to\":%lld,\"where\":\"meeting\"}",uid);
                facepower_->cmd(szBuufer);
            }
        }
    }
    touchStart_=IntVector2(0,0);
}
// 这个事件不会用 只好用上面的两个配合起来   估计 TouchMove 是持续性事件 不停的发
void VUIMetting::HandleTouchMove(StringHash eventType, VariantMap& eventData)
{
}

void VUIMetting::HandlebtnRiseHand(StringHash eventType, VariantMap& eventData)
{
    if(facepower_){
        facepower_->cmd("{\"cmd\":\"raiseHand\",\"where\":\"meeting\"}");
    }
}

void VUIMetting::ShowWaiters()
{
    std::lock_guard<std::mutex> lck(mtx_);
    if(pLstWaiters_==nullptr)
        return ;
        
    auto waiterRc=layout_->GetChildStaticCast<UIElement>("waiters",true);
    if(waiterRc)
    {
        if(pLstWaiters_)
            waiterRc->RemoveAllChildren();
        
        int i=0;
        int picSize=100;
        for(auto it:(*pLstWaiters_))
        {
            Button* button=waiterRc->CreateChild<Button>();
            button->SetPosition(0,(picSize+15)*i++);
            button->SetSize(IntVector2(picSize,picSize));
            //button->SetAlignment(HA_CUSTOM, VA_CENTER);
            //button->SetBorder(IntRect(10,10,10,10));
            button->SetVar(VAR_WAITERID,it.c_str());
            
            
            //"http://n.sinaimg.cn/sinakd20122/164/w438h526/20200421/7472-isqivxf8894253.jpg";
            std::string url=facepower_->getDBWebSite();
            url+="img/a/";
            url+=it.c_str();
            url+=".jpg";
            
            downloading_.insert(std::make_pair(url, button));
            SubscribeToEvent(button, E_PRESSED, URHO3D_HANDLER(VUIMetting, HandleWaiterPress));
            SubscribeToEvent(button, E_RELEASED, URHO3D_HANDLER(VUIMetting, HandleWaiterRelease));
        }
        
        pLstWaiters_->clear();
        pLstWaiters_.reset();
    }
}
void VUIMetting::HandleWaiterPress(StringHash eventType, VariantMap& eventData)
{
   Button* touchBtn_= static_cast<Button*>(GetEventSender());
    if(touchBtn_)
        touchUserID_= touchBtn_->GetVar(VAR_WAITERID).GetString();
}
void VUIMetting::HandleWaiterRelease(StringHash eventType, VariantMap& eventData)
{   //show profile
//    touchBtn_= static_cast<Button*>(GetEventSender());
//    if(touchBtn_){
//        touchUserID_= touchBtn_->GetVar(VAR_WAITERID).GetString();
//    }
    
    //测试
    char szBuufer[1024];
    snprintf(szBuufer,1024,"{\"cmd\":\"waiterOnStage\",\"to\":%s,\"where\":\"meeting\"}",touchUserID_.CString());
    if(facepower_)
        facepower_->cmd(szBuufer);
}
int64_t  VUIMetting::GetPictureUID(IntVector2 point)
{
    auto* graphics = GetSubsystem<Graphics>();
    int width=graphics->GetWidth();
    int height=graphics->GetHeight();
    
    //获得视频区域
     std::string rectJson;//"{\"pics\":[[10001,0,0,320,240],[10002,100,200,320,400]]}";
     if(facepower_)
         facepower_->getPictureRects(rectJson);

    try
    {
        ptree pt;
        std::stringstream ss(rectJson);
        read_json(ss, pt);
        
        ptree arrayPics = pt.get_child("pics");  // get_child得到数组对象
        BOOST_FOREACH(boost::property_tree::ptree::value_type &v,arrayPics)
        {                   //v.first;
            ptree  subRect= v.second;
            
            float lat[5];
            int i=0;
            int64_t uid;
            int x,y,w,h;
            BOOST_FOREACH(boost::property_tree::ptree::value_type &v2,subRect)
            {
                if(i==0)
                    uid=v2.second.get_value<int64_t>();
                else
                    lat[i]=v2.second.get_value<float>();
                
                i++;
            }
            
            x=lat[1]*width;
            y=lat[2]*height;
            w=lat[3]*width;
            h=lat[4]*height;
            
            IntRect  rect(x,y,x+w,y+h);
            
            if(uid>0&&rect.IsInside(point))
                return uid;
        }

    }catch (ptree_error & e) {
    }catch (...) {
    }
    
    return 0;
}

