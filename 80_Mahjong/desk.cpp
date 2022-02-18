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
#include <Urho3D/Input/Input.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>

#include <iostream>
#include "desk.h"

//#include <Urho3D/Engine/Engine.h>

//for websocket
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <boost/format.hpp>


#include "url.hpp"


//调整手牌高度
#define HAND_CARDS_Y         4.0f

//麻将机器人算法
//http://mahjongjoy.com/mahjong_bots_ai_algorithm_cs.shtml


boost::asio::io_context           _ioc;
void thread_loop()
{
   _ioc.run();
}



Desk::Desk(int32_t threads)
{
   cardWidth_   =3.45f;
   cardHeight_  =4.6f;
   cardDepth_   =2.2f;
    
    
    dist1_=3*cardWidth_;                                //摆牌距离
    dist2_=dist1_+5*cardHeight_+cardDepth_;             //
    
    //我的调整位置
    adjustMe_=1.5*cardDepth_;
}
Desk::~Desk()
{
    StopNetWork();
    
    if(facepower_)
        facepower_->cmd("{\"cmd\":\"appExit\"}");
}
void Desk::StopNetWork()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if(stream_)
    {
        stream_->close();
        stream_=nullptr;
    }
    
    if(server_)
    {
        server_->stop();
        server_=nullptr;
        
        _ioc.stop();
        net_thread_.join();
    }
}
//本桌新一轮开始
void Desk::NewRound()
{
    pCardDiscard_=nullptr;
    playering_=nullptr;
    
    
    for(size_t i=0;i<gamblers_.size();i++)
        gamblers_[i]->NewRound();
    
    gamblers_.clear();
    publicCards_.clear();
    
    cardsTing_.clear();
    cardsLing_.clear();
}

//url 链接的服务器地址 p4sp://2020@192.168.0.100:9700/mj
//firstCMD 链接参数 用户token 桌号，游戏类型等信息
void Desk::ConnectNetWork(std::string const& url,std::string const& firstCMD)
{
    if(nullptr==facepower_){
        facepower_=getFacePower();
        
        facepower_->cmd("{\"cmd\":\"appStart\",\"appID\":80}");
        
        
        #if defined _WIN32

        #elif defined  __APPLE__

        #elif defined __ANDROID__
                //登录 p4sp
                facepower_->cmd(firstCMD);//mac 下调试没有宿主程序启动 因此需要自己登录
        #endif
    }
    
    //登录游戏服务器
    P4SPPtr      p4sp=std::static_pointer_cast<ITransport>(facepower_->getP4SP());
    if(p4sp&&!stream_)
    {
        std::string header;
        header = "GET /mj HTTP/1.1\r\n";
        header+="Accept: *\r\n";
        header+="\r\n";
        header+=firstCMD;
        
        auto onStream=std::bind(&Desk::onStream,this,std::placeholders::_1,std::placeholders::_2);
        //UDP 协议
        p4sp->httpRequest(url,header,onStream);
        
        return ;
    }
    

    if(nullptr==server_)
    {

//        server_=std::make_shared<ws_session>(io_context_.io_context()
//                                             ,std::bind(&Desk::OnWebsocketData,this
//                                             ,std::placeholders::_1,std::placeholders::_2));
        
        
        server_=std::make_shared<ws_session>(_ioc
                                             ,std::bind(&Desk::OnWebsocketData,this
                                             ,std::placeholders::_1,std::placeholders::_2));
        
        
        Url    parseUrl(url.c_str());
        std::string host        = parseUrl.GetHost();
        std::string port        = parseUrl.GetPort();
        std::string target      = parseUrl.GetPath() + parseUrl.GetQuery();
        
        server_->run(host.c_str(),port.c_str(),target.c_str(),firstCMD.c_str());
        
        //开始线程
        net_thread_=std::thread(thread_loop);
        
    }
}

void  Desk::onStream(std::error_code const& failure,std::shared_ptr<IStream> pStream)
{
    if (failure){
        std::cerr <<"stream 错误:"<< failure.message() << std::endl;
        ShowErrorAndExit("网络出现故障而中断，请退出稍后再试！");
    }else{
        if(stream_==nullptr)
            stream_=pStream;
        
        
        std::string body=pStream->getBody();
        if(!body.empty())
        {
            std::lock_guard<std::mutex> lock(mutex_);
            messages_.push(body);
            //parse_command(body);
        }

        std::cout<<"Desk::onStream  data:"<<body<<std::endl;
    }
}

//掉线自动重连
void Desk::OnWebsocketData(boost::system::error_code ec,std::string const&strText)
{
    if(ec)
    {//发送错误，重连服务器
        if(server_)
            server_->stop();
        
    }else{
        std::cout<<strText<<std::endl;
        
        std::lock_guard<std::mutex> lock(mutex_);
        messages_.push(strText);
    }
}
void Desk::SendCommand(std::string& cmd)
{
    if(stream_)
        stream_->send(cmd);
    else if(server_)
        server_->write(cmd);
}

void PtreeToMultiMap(ptree& ptRemove,std::map<int,std::map<int,int>>& removes)
{
    //2 层 map 遍历
    for (boost::property_tree::ptree::iterator it = ptRemove.begin(); it != ptRemove.end(); ++it)
    {
        std::string key     = it->first;//key ID
        
        std::map<int,int>  huCards;
        ptree     vitems   = it->second;
        for (boost::property_tree::ptree::iterator it2 = vitems.begin(); it2 != vitems.end(); ++it2)
        {
            std::string key2     = it2->first;//key ID
            int32_t     value   = it2->second.get_value<int32_t>();
            
            huCards.insert(std::make_pair(atoi(key2.c_str()),value));
        }
        
        removes.insert(std::make_pair(atoi(key.c_str()), huCards));
    }
}
void PtreeToSet(ptree& ptArray,std::set<int>& array)
{
    BOOST_FOREACH(boost::property_tree::ptree::value_type &v,ptArray)
    {
        int32_t value=v.second.get_value<int32_t>();
        array.insert(value);
    }
}
std::string Desk::GetActionSound(const std::string& action)
{
    std::string soundFile=str(boost::format("Sounds/mj/game/%s.ogg")% action);
    return soundFile;
}

//获得星座
std::string getConstell(int month,int day){
    if(month<=0)
        month=1;
    if(day<=0)
        day=1;
        
    std::string constells[12][2] = {
    { "摩羯座","水瓶座" },//一月
    { "水瓶座","双鱼座" },//二月
    { "双鱼座","白羊座" },//三月
    { "白羊座","金牛座" },//四月
    { "金牛座","双子座" },//五月
    { "双子座","巨蟹座" },//六月
    { "巨蟹座","狮子座" },//七月
    { "狮子座","处女座" },//八月
    { "处女座","天秤座" },//九月
    { "天秤座","天蝎座" },//十月
    { "天蝎座","射手座" },//十一月
    { "射手座","摩羯座" },//十一月
    };
    //每个月有两个星座，数组中的值对应每个月中两个星座的分割日期
    int constell_dates[]{ 20,19,21,20,21,22,23,23,23,24,23,22 };
    
    return constells[month-1][day / constell_dates[month]];
}

void Desk::ParseCommand()
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    while(messages_.size()>0)
    {
        const std::string strText=messages_.front();
        messages_.pop();
        
        std::cout<<"---->"<<strText<<std::endl;
        
        
        try
        {
            ptree pt;
            std::stringstream ss(strText);
            read_json(ss, pt);
            const int32_t action = pt.get("action",0);
            const std::string cmd = pt.get("cmd","");
            
            if(0==action)
            {
                if("error"==cmd)
                {//发生错误 退出游戏
                    std::string message=pt.get("message","");
                    ShowErrorAndExit(message);
                }
                return ;
            }else if(UserAction::ListPlayer==action)
            {
                //遍历数组
                int   totalPlayers      =pt.get("total",3);
                std::string deskName    =pt.get("deskProfile.name","");
                int excess              =pt.get("deskProfile.excess",2);
                GameReady(strText);
                
                 PlayMjSound(GetActionSound("game_list"));
            }else if(UserAction::CheckChips==action||UserAction::CheckChipsOver==action)
            {       //仅显示消息
                    GameReady(strText);
            }else if(UserAction::RoundNew==action)
            {
                NewRound();
                
                PlayMjSound(GetActionSound("game_start"));
                
                //获得语音房间的校验码
                std::string roomToken=pt.get("roomToken","");
                
                std::string buttons= pt.get("buttons","");
                ptree gamblerArray = pt.get_child("players");  // get_child得到数组对象
                BOOST_FOREACH(boost::property_tree::ptree::value_type &v, gamblerArray)
                {
                    int64_t uid         =v.second.get("id",int64_t(0));
                    std::string name    =v.second.get("name","");
                    std::string avatar  =v.second.get("avatar","");
                    std::string birthday=v.second.get("birthday","");
                    int piaoBase        =v.second.get("piaoBase",0);
                    int score           =v.second.get("score",0);
                    int sex             =v.second.get("sex",0);
                    
                   
                    
                    //使用默认头像
                    //if(avatar.empty())
                    //强制使用动画头像
                    avatar=Gambler::GetAvatar(sex,uid);
                    
                    //计算年龄和星座
                    int age=18;
                    int year, month, day;
                        year=1997;
                        month=day=1;
                    sscanf(birthday.c_str(), "%d-%d-%d", &year, &month, &day);
                    time_t t = time(NULL);
                    struct tm* today=localtime(&t);
                    age=today->tm_year+1900-year;
                            //星座
                    std::string constell=getConstell(month,day);

//                    //计算年龄
//                    std::tm tm = {};
//                    //strptime("Thu Jan 9 2014 12:35:34", "%a %b %d %Y %H:%M:%S", &tm);
//                    strptime(birthday.c_str(),"%Y-%m-%d", &tm);
//                    
//                    std::chrono::system_clock::time_point now=std::chrono::system_clock::now();
//                    time_t tt = std::chrono::system_clock::to_time_t(now);
//                    //std::tm utc_tm = *gmtime(&tt);
//                    std::tm local_tm = *localtime(&tt);
//                    years=local_tm.tm_year-tm.tm_year;
                    

                    std::shared_ptr<Gambler> gambler=std::make_shared<Gambler>(this);
                    gamblers_.push_back(gambler);
                    
                    std::string profile=str(boost::format("%s\n飘:%d\n%s,%d,%s")%name%piaoBase%(sex==1?"男":"女")%age%constell);
                    gambler->SetProfile(avatar, profile);
                    
                    //设置配音
                    int maxSound=1;   //配音索引上限 ，男女应该一样
                    std::random_device rd;
                    std::mt19937 gen(rd());
                    std::uniform_int_distribution<> dis(0,maxSound);//区间包含开始和结束
                
                    int rnd=dis(gen);
                    if(sex<0)
                        sex=0;
                    else if(sex>1)
                        sex=1;
                    
                    std::string sound=str(boost::format("%d_%d") %sex%rnd);//第一个数字为性别 0 女 1 男 第二个数字为人物所以 0，1 2，3....
                    gambler->SetWhoSound(sound);
                    
                }

                // 遍历数组
                std::vector<int32_t>  cards;
                ptree card_array = pt.get_child("cards");  // get_child得到数组对象
                BOOST_FOREACH(boost::property_tree::ptree::value_type &v,card_array)
                {
                    int32_t value=v.second.get_value<int32_t>();
                    cards.push_back(value);
                    //                std::stringstream s;
                    //                write_json(s, v.second);
                    //                std::string image_item = s.str();
                }
                int64_t      roomID     =pt.get("deskProfile.rid",0);
                std::string  deskName   =pt.get("deskProfile.name","");
                int excess              =pt.get("deskProfile.excess",2);
                std::string round       =pt.get("deskProfile.round","");

                
                playerCount_    =pt.get("total",-1);           //游戏人数     3
                myPos_          =pt.get("pos",-1);             //我所处的位置  0-3
                bankerPos_      =pt.get("banker",-1);          //庄
                //计算各个玩家的位置
                gamblers_[myPos_]->SetDir(DeskDir::Me);      //设置我的位置
                int32_t myIndex=myPos_;
                if(myIndex>-1&&myIndex<playerCount_&&playerCount_==(int32_t)gamblers_.size())
                {
                    if(2==playerCount_)
                    {//二人麻将 不区分顺逆向
                        int32_t nextPos=myPos_;
                        if(++nextPos>=playerCount_)
                            nextPos=0;
                        gamblers_[nextPos]->SetDir(DeskDir::Forward);
                    }else if(3==playerCount_)
                    {//三人麻将 设置方位
                        leftPos_=myIndex+1;
                        if(leftPos_>2)
                            leftPos_=0;
                        
                        rightPos_=myIndex-1;
                        if(rightPos_<0)
                            rightPos_=2;
                        
                        gamblers_[leftPos_]->SetDir(DeskDir::Left);
                        gamblers_[rightPos_]->SetDir(DeskDir::Right);
                    }else if(4==playerCount_)
                    {//4人麻将 0 1 2 3
                        int32_t nextPos=myPos_;
                        for(int k=0;k<4;++k)
                        {
                            DeskDir dir=DeskDir::Me;
                            if(k==0)
                                dir=DeskDir::Me;
                            else if(k==1)
                                dir=DeskDir::Left;
                            else if(k==2)
                                dir=DeskDir::Forward;
                            else if(k==3)
                                dir=DeskDir::Right;
                            
                            gamblers_[nextPos]->SetDir(dir);
                            
                            if(++nextPos>=playerCount_)
                                nextPos=0;
                        }
                    }
                }
                
                //发牌
                for(int i=0;i<playerCount_;++i)
                {
                    if(i==myPos_)
                        gamblers_[myPos_]->LoadCard(cards);
                    else
                        gamblers_[i]->LoadCard();
                    
                }
                //

               //桌面信息
               std::string deskInfo=str(boost::format("底分:%d\n局数:%s") % excess % round);
               SetDeskInfo(deskName,deskInfo,-1);
                
               CreateButtons(buttons);
                
                
                //玩家信息 从我的郑伟开始顺时针
                std::vector<std::string>  infoes;
                std::vector<std::string>  avatars;
                std::vector<DeskDir>      seats;
                int pNext=myPos_;
                for(int i=0;i<playerCount_;++i)
                {
                    infoes.push_back(gamblers_[pNext]->GetProfile());
                    avatars.push_back(gamblers_[pNext]->GetAvatar());
                    seats.push_back(gamblers_[pNext]->GetDir());
                    
                    if(++pNext>=playerCount_)
                        pNext=0;
                }
                
                //2022.2.17 增加座位
                //roomID
                SetPlayerInfo(infoes,avatars,roomID,myPos_,roomToken,seats);
                
            }else if(UserAction::Zhua<=action)
            {
                int32_t  from =pt.get("from",-1);   //当前用户
                int32_t  peer=pt.get("peer",-1);    //对家--上一个出牌的用户
                int32_t  card =pt.get("card",-1);
                int32_t  card2 =pt.get("card2",0);
                int32_t  cardRemains =pt.get("cardRemains",108);
                DeskDir dir=(DeskDir)GetDirection(from);
                SetDeskInfo(dir,cardRemains);
                if(UserAction::Zhua==action)
                {//抓牌/发牌
  
                    PlayMjSound(GetActionSound("card_zhua"));
                    
                    //余牌信息
                    SetDeskInfo("","",cardRemains);
                    
                    if(playering_)
                        playering_->Card_Qi(pCardDiscard_);
                    
                    playering_=gamblers_[from];
                    playering_->Card_Zhua(card);
                    pCardDiscard_=nullptr;
                    
                    //思考时间倒记时开始
                    if(DeskDir::Me==dir){
                        std::string btns=pt.get("btns","");
                        int32_t btnTime =pt.get("btnTime",0);
                        
                        //亮选牌
                        if(btns.find("亮")!=std::string::npos)
                        {
                            //std::map<int,std::map<int,int>> removes;
                            PtreeToMultiMap(pt.get_child("liangCards"),cardsLing_);

                            gamblers_[DeskDir::Me]->setLiangCards(cardsLing_);
                        }
                        
                        ShowButton(btns, btnTime);
                    }
                }else if(UserAction::Chu==action)
                {//出牌
                    PlayMjSound(GetActionSound("card_chu"));
                    
                    AddPublicCard(card);//统计出牌，亮牌
                    
                    if(playering_&&pCardDiscard_)
                        playering_->Card_Qi(pCardDiscard_);
                    
                    playering_  =gamblers_[from];
                    pCardDiscard_=playering_->Card_Chu(card);
                    
                    PlayMjSound(playering_->GetCardSound(card),from);
                    
                    //显示按钮
                    std::string btns=pt.get("btns","");
                    int32_t btnTime =pt.get("btnTime",0);
                    if(DeskDir::Me==dir){
                        if(btns.empty())
                            ShowButton("", 0);               //关闭进度提示
                        else
                            ShowButton(btns, btnTime);
                    }else
                    {   //碰 杠 胡
                        if(!btns.empty())
                            ShowButton(btns, btnTime);
                    }
                    
                    //如果亮倒了有胡，那么自动胡牌？放在服务端？
                    //if(gamblers_[myPos_]->)
                    
                }else if(UserAction::Peng==action)
                {//碰
                    for(int i=0;i<2;++i)
                        AddPublicCard(card);
                    
                    DeskDir dir=(DeskDir)GetDirection(from);
                    SetDeskInfo(dir,-1);
                    
                    gamblers_[from]->Card_Peng(pCardDiscard_,card);
                    pCardDiscard_=nullptr;
                    
                    PlayMjSound(gamblers_[from]->GetActionSound("peng"),from);
                                       

                    //思考时间倒记时开始
                    if(DeskDir::Me==dir){
                        std::string btns=pt.get("btns","");
                        int32_t btnTime =pt.get("btnTime",0);
                        //亮选牌
                        if(btns.find("亮")!=std::string::npos)
                        {
                            PtreeToMultiMap(pt.get_child("liangCards"),cardsLing_);
                            gamblers_[DeskDir::Me]->setLiangCards(cardsLing_);
                        }
                        ShowButton(btns, btnTime);
                    }
                }else if(UserAction::Gang==action)
                {//杠
                    PlayMjSound(GetActionSound("card_gang"));
                    
                    int type=card2; //杠的类型 -4 暗杠 -3 点杠 -1 碰杠
                    if(type==-1)
                        AddPublicCard(card);
                    else if(type==-3)
                    {
                        for(int i=0;i<3;++i)
                            AddPublicCard(card);
                    }else if(type==-4)
                    {
                        for(int i=0;i<4;++i)
                            AddPublicCard(card);
                    }
                     
                    gamblers_[from]->Card_Gang(pCardDiscard_,card,type);
                    pCardDiscard_=nullptr;
                    PlayMjSound(gamblers_[from]->GetActionSound("gang"),from);
                    
                    
                    
                    
                    //显示按钮
                    std::string btns=pt.get("btns","");
                    int32_t btnTime =pt.get("btnTime",0);
                    if(DeskDir::Me==dir){
                        if(btns.empty())
                            ShowButton("", 0);               //关闭进度提示
                        else
                            ShowButton(btns, btnTime);
                    }else
                    {   //碰 杠 胡
                        if(!btns.empty())
                            ShowButton(btns, btnTime);
                    }
                    
//                    if(DeskDir::Me==dir){
//                        ShowButton("",0);
//                    }
                }else if(UserAction::Liang==action)
                {//亮
                    
                    PlayMjSound(GetActionSound("card_chu"));
                    //PlayMjSound(GetActionSound("card_liang"));
                    PlayMjSound(gamblers_[from]->GetActionSound("liang"),from);
                    
                    // 遍历数组
                    std::vector<int32_t>  cards;
                    ptree card_array = pt.get_child("pubCards");  // get_child得到数组对象
                    BOOST_FOREACH(boost::property_tree::ptree::value_type &v,card_array)
                    {
                        int32_t value=v.second.get_value<int32_t>();
                        cards.push_back(value);
                    }
                    
                    //统计亮的牌
                    for(auto p:cards)
                        AddPublicCard(p);
                    
                    
                    std::set<int32_t>  huCards;
                    card_array = pt.get_child("huCards");  // get_child得到数组对象
                    BOOST_FOREACH(boost::property_tree::ptree::value_type &v,card_array)
                    {
                        int32_t value=v.second.get_value<int32_t>();
                        huCards.insert(value);
                    }
                    
                    //亮倒到牌
                    gamblers_[from]->Card_Liang(cards);
                    
                    
                    if(DeskDir::Me==dir){
                        gamblers_[from]->SetHuCards(huCards);
                    }
                    
                }else if(UserAction::HuPai==action)
                {//胡

                    //step 1 展示牌  推倒
                    //step 2 game over 展示结果
                    int winner=pt.get("winner",0);
                    int firer=pt.get("firer",0);
                    from=winner;
                    
                    PlayMjSound(GetActionSound("card_hu"),from);
                    
                    //指向赢家
                    SetDeskInfo((DeskDir)GetDirection(winner),-1);
                    
                    
                    std::random_device rd;
                    std::mt19937 gen(rd());
                    std::uniform_int_distribution<>  disID(1, 3);
                    //区间包含开始和结束
                    std::string huStr="hupai"+std::to_string(disID(gen));
                    PlayMjSound(gamblers_[winner]->GetActionSound(huStr.c_str()));
                    //金币的声音
                    PlayMjSound(GetActionSound("win_coins"));
                    
                    
                    int index=0;
                    ptree array = pt.get_child("cards");
                    BOOST_FOREACH(const boost::property_tree::ptree::value_type &v, array)
                    {
                        // v.first is the name of the child.
                        // v.second is the child tree.
                        
                        std::vector<int32_t>  cards;
                        ptree     vCards   = v.second;
                        BOOST_FOREACH(const boost::property_tree::ptree::value_type &v2, vCards)
                        {
                            int  card= v2.second.get_value<int>();
                            cards.push_back(card);
                        }
                        gamblers_[index]->Card_Liang(cards);
                        
                        ++index;
                    }
                    
                    
                    /* 只亮倒赢家的牌
                    std::vector<int32_t>  cards;
                    ptree card_array = pt.get_child("cards");  // get_child得到数组对象
                    BOOST_FOREACH(boost::property_tree::ptree::value_type &v,card_array)
                    {
                        int32_t value=v.second.get_value<int32_t>();
                        cards.push_back(value);
                    }
                    gamblers_[winner]->Card_Liang(cards);
                    */
                    
                    ShowButton("", 0);
                }else if(UserAction::BuyHorse==action)
                {//买马
                    PlayMjSound(GetActionSound("card_chu"));
                    gamblers_[myPos_]->Buy_Horse(card);
                    

                    PlayMjSound(GetActionSound("buy_horse"));
                    
                }else if(UserAction::ForceLiang==action)
                {//强制亮牌
                    PlayMjSound(GetActionSound("card_liang"),from);
                    // 遍历数组
                    std::vector<int32_t>  cardsLiang;
                    ptree card_array = pt.get_child("linagCards");  // get_child得到数组对象
                    BOOST_FOREACH(boost::property_tree::ptree::value_type &v,card_array)
                    {
                        int32_t value=v.second.get_value<int32_t>();
                        cardsLiang.push_back(value);
                    }
                    
                    //统计亮的牌
                    for(auto p:cardsLiang)
                        AddPublicCard(p);
                    
                    //亮倒到牌
                    gamblers_[from]->Card_Liang(cardsLiang);
                    
                    PlayMjSound(gamblers_[from]->GetActionSound("liang"));
                }
            }else if(UserAction::RoundInfo==action)
            {
                //得到听的牌并显示出来
                std::string type=pt.get("type","");
                if("ting"==type)
                {
                    // 遍历数组得到听的牌
                    std::vector<int32_t>  ting_cards;
                    ptree ting_array = pt.get_child("tings");   //get_child得到数组对象
                    BOOST_FOREACH(boost::property_tree::ptree::value_type &v,ting_array)
                    {
                        int32_t value=v.second.get_value<int32_t>();
                        ting_cards.push_back(value);
                        
                        cardsTing_.insert(value);
                    }
                    
                    //
                    gamblers_[myPos_]->ScanDisableCards();
                    
                    
                    //
                    std::map<int,int> tings;
                    for(auto it : ting_cards)
                        tings.insert(std::make_pair(it,4-this->GetCardCount(it)));
                    ShowTingInfo(tings);
                }

            }else if(UserAction::RoundOver==action)
            {
                PlayMjSound(GetActionSound("game_over"));
                
                int winner=pt.get("winner",2);
                if(winner==myPos_)
                    ShowRoundResult(pt,"赢");
                else
                    ShowRoundResult(pt,"输");
            }else if(UserAction::CheckChipsOver==action)
            {

                
            }
            else if(UserAction::GameOver==action)
            {
                ShowGameOver(pt);
            }


        }catch (ptree_error & e) {
            
            std::cout<<"ParseCommand json解析错误:"<<e.what();
            int a=0;a++;
            //spd::get("console")->warn("解析json异常捕获 text:{},what:{}",strText,e.what());
        }catch (...) {
            //spd::get("console")->warn("解析json未知的异常text:{}",strText);
        }
        
    }
}

DeskDir Desk::GetDirection(int32_t pos)
{
    if(pos>-1&&pos<gamblers_.size())
        return gamblers_[pos]->GetDir();
    
    return DeskDir::Unknow;
}

Vector3 Desk::GetCardPos_Zhua(DeskDir dir)
{
    Vector3 pos(0.f,0.f,0.f);
    if(DeskDir::Left==dir)
    {
        pos.x_=-dist2_ - cardDepth_;
        pos.z_=-7*cardWidth_;
    }else if(DeskDir::Right==dir)
    {
        pos.x_=dist2_+cardDepth_;
        pos.z_=7*cardWidth_;
    }else if(DeskDir::Me==dir)
    {
        pos.x_=7*cardWidth_;
        pos.z_=-dist2_ -cardDepth_ -adjustMe_;
        
        //调整牌的高度
        pos.y_=HAND_CARDS_Y;
    }else if(DeskDir::Forward==dir)
    {   //me的反方向
        pos.x_=-7*cardWidth_;
        pos.z_=dist2_+cardDepth_;
    }
    return pos;
}
Vector3 Desk::GetCardPos_Liang(DeskDir dir)
{
    Vector3 pos(0.f,0.f,0.f);
    pos=GetCardPos_Zhua(dir);
    
    //调整位置(被抓牌挡住了 me 向上移动1.0f)
    if(DeskDir::Left==dir)
    {
        pos.x_+=1.0f;
    }else if(DeskDir::Right==dir)
    {
        pos.x_-=1.0f;
    }else if(DeskDir::Me==dir)
    {
        pos.z_+=1.f;
    }else if(DeskDir::Forward==dir)
    {   //me的反方向
        pos.z_-=1.f;
    }
    

    return pos;
}

Vector3 Desk::GetCardWallStart(DeskDir dir)
{
    Vector3 pos(0.f,0.f,0.f);
    if(DeskDir::Left==dir)
    {
        pos.x_=-dist2_ - cardDepth_;
        pos.z_=-5*cardWidth_;
    }else if(DeskDir::Right==dir)
    {
        pos.x_=dist2_+cardDepth_;
        pos.z_=5*cardWidth_;
    }else if(DeskDir::Me==dir)
    {
        pos.x_=5*cardWidth_;
        pos.z_=-dist2_ -cardDepth_ - adjustMe_;
        
        //调整牌的高度
        pos.y_=HAND_CARDS_Y;
    }else if(DeskDir::Forward==dir)
    {
        pos.x_=-5*cardWidth_;
        pos.z_=dist2_+cardDepth_;
    }
    return pos;
}

Vector3 Desk::GetCardPos_Chu(DeskDir dir)
{
    Vector3 pos(0.f,0.f,0.f);
    if(DeskDir::Left==dir)
    {
        pos.x_=-dist1_ - 4*cardHeight_;
    }else if(DeskDir::Right==dir)
    {
        pos.x_=dist1_+4*cardHeight_;
    }else if(DeskDir::Me==dir)
    {
        pos.z_=-dist1_ - 4*cardHeight_;
    }else if(DeskDir::Forward==dir)
    {
        pos.z_=dist1_+4*cardHeight_;
    }
    return pos;
}

//弃
Vector3 Desk::GetCardPos_Qi(DeskDir dir)
{
    int cardsCount=3;
    if(2==playerCount_)
        cardsCount=5;
    
    Vector3 pos(0.f,0.f,0.f);
    if(DeskDir::Left==dir)
    {
        pos.x_+=-dist1_-cardHeight_;
        pos.z_+=cardsCount*cardWidth_;
    }else if(DeskDir::Right==dir)
    {
        pos.x_+=dist1_+cardHeight_;
        pos.z_+=-cardsCount*cardWidth_;
    }else if(DeskDir::Me==dir)
    {
        pos.x_+=-cardsCount*cardWidth_;
        pos.z_+=-dist1_-cardHeight_;
    }else if(DeskDir::Forward==dir)
    {
        pos.x_+=cardsCount*cardWidth_;
        pos.z_+=dist1_+cardHeight_;
    }
    return pos;
}
float  Desk::GetFixStartLength()
{
    return dist2_-cardWidth_;
}
Vector3 Desk::GetCardPos_Fix(DeskDir dir,int32_t size,int32_t i)
{
    Vector3 pos(0.f,0.f,0.f);
    if(DeskDir::Left==dir)
    {
        pos.x_+=-GetFixStartLength()-cardHeight_;
        pos.z_+=GetFixStartLength()-size*3*cardWidth_-i*cardWidth_;
    }else if(DeskDir::Right==dir)
    {
        pos.x_+=GetFixStartLength()+cardHeight_;
        pos.z_+=-GetFixStartLength()+size*3*cardWidth_+i*cardWidth_;
    }else if(DeskDir::Me==dir)
    {
        pos.x_+=-GetFixStartLength()+size*3*cardWidth_+i*cardWidth_;
        pos.z_+=-GetFixStartLength()-cardHeight_;
    }else if(DeskDir::Forward==dir)
    {
        pos.x_+=GetFixStartLength()-size*3*cardWidth_+i*cardWidth_;
        pos.z_+=GetFixStartLength()+cardHeight_;
    }
    return pos;
}


//用户选择牌
void Desk::NodeTouch(Node* pNode,NTouch dir)
{
    if(gamblers_.size()==0)
        return ;

    if(dir==NTouch::NT_DOWN)
    {
        if(gamblers_[myPos_]->isLiangPress())
            gamblers_[myPos_]->UnHideCard(pNode);
    }else if(dir==NTouch::NT_UP)
    {//出牌动作
        bool bLiangSelect=false;
        int32_t value=gamblers_[myPos_]->CanSelected(pNode,bLiangSelect);
        if(value>0)
        {
            std::string  res;
            if(bLiangSelect)
            {
                std::set<int> hideCards;
                gamblers_[myPos_]->GetKouCards(hideCards);
                
                std::stringstream ss;          //定义流ss
                auto it=hideCards.begin();
                while(it!=hideCards.end())
                {
                    if(it!=hideCards.begin())
                        ss<<",";
                    ss<<*it;
                    
                    ++it;
                }
                ss >> res; //将流输出到字符串中
                
            }

            std::string cmd=str(boost::format("{\"action\":%d,\"card\":%d,\"kouCards\":[%s]}") % (bLiangSelect?UserAction::Liang : UserAction::Chu) %value % res);
            SendCommand(cmd);
            gamblers_[myPos_]->endLiang();
            return ;
        }
    }else if(dir==NTouch::NT_CLICK){//查询动作
        bool bLiangSelect=false;
        int32_t value=gamblers_[myPos_]->CanSelected(pNode,bLiangSelect);
        if(bLiangSelect&&value>0)
        {
            std::map<int,int> temp;
            bool has=gamblers_[myPos_]->getHuinfo(value,temp);
            if(has)
                ShowHuInfo(temp);
        }
    }
}
void Desk::SendAction(std::string const& actionName)
{
    UserAction action;
    int32_t card=-1;
    //这里的命令转换，以后移动到服务端 让客户端更具有通用性
    if("过"==actionName)
    {
        ShowButton("", 0);
        action=UserAction::Pass;
    }else if("碰"==actionName)
    {
        ShowButton("", 0);
        action=UserAction::Peng;
    }else if("杠"==actionName)
    {
        ShowButton("", 0);
        action=UserAction::Gang;
        //暗杠 明杠
        if(playering_==gamblers_[myPos_])
            card=playering_->GetGangCard();
        else
            card=0;
    }else if("亮"==actionName)
    {
        gamblers_[myPos_]->startLiang();
        return ;
    }else if("不亮"==actionName)
    {
        //ShowButton("", 0);
        gamblers_[myPos_]->endLiang();
        gamblers_[myPos_]->CardSort();
        return ;
    }else if("胡"==actionName)
    {
        ShowButton("", 0);
        action=UserAction::HuPai;
        //先检查是否能胡牌
    }else if("继续游戏"==actionName||"继续"==actionName)
    {
         ShowButton("", 0);
         action=UserAction::RoundNext;
    }

    std::string cmd=str(boost::format("{\"action\":%d,\"card\":%d}") % action %card);
    SendCommand(cmd);
}
