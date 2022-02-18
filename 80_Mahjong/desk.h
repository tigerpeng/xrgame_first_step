#pragma once
#include <boost/asio.hpp>
#include "ws_client_async.h"
#include "gambler.h"
#include <queue>


#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <thread>

//fro xml and json
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
using namespace boost::property_tree;

#include <facepower/api_facepower.h>
#include <p4sp/api_p4sp.h>



namespace Urho3D
{
    
    class Node;
    class Scene;
    class Sprite;
}
// All Urho3D classes reside in namespace Urho3D
using namespace Urho3D;

enum NTouch{
    NT_UP,
    NT_DOWN,
    NT_LEFT,
    NT_RIGHT,
    NT_CLICK
};

//这样设计的初衷是想分离游戏客户端算法跟游戏引擎，以便以后能切换引擎
//但是......虚函数太多了,必须把跟引擎交互抽象出来，减少虚函数以减少移植工作量
class Desk//:public std::enable_shared_from_this<Desk>
{
public:
    Desk(int32_t threads=1);
    ~Desk();
    
    void ConnectNetWork(std::string const& url="",std::string const& firstCMD="");
    void StopNetWork();
    
    virtual void NewRound();
    virtual void GameReady(std::string const& jsonInfo)=0;
    virtual void SetPlayerInfo(std::vector<std::string>& infos,std::vector<std::string>& avatars,int64_t rid,int myPos,std::string const& rToken,std::vector<DeskDir> dirs)=0;
    
    virtual void SetDeskInfo(std::string const& name,
                std::string const& infoStatic,int cardNumber=-1)=0;
    virtual void SetDeskInfo(DeskDir player,int cards=-1)=0;
    virtual void CreateButtons(std::string const& btns)=0;
    virtual Node* CreateMahJong(int32_t card,Vector3 pos,Quaternion rota=Quaternion(0, 0, 0))=0;
    virtual void ModifyNode(Node* pNode,int32_t value)=0;
    virtual void PlayMjSound(std::string const&  soundFile,int pos=-1)=0;
    
    virtual void ShowRoundResult(ptree& pt,std::string const& title="")=0;
    virtual void ShowGameResult(int excess,std::vector<std::string>& vPlayers)=0;
    virtual void ShowGameOver(ptree& pt)=0;
    virtual void ShowButton(std::string const& btns,int32_t seconds)=0;
    
   
    virtual void ShowHuInfo(std::map<int,int>& huPai)=0;    //显示自己胡的牌信息
    virtual void ShowTingInfo(std::map<int,int>& huPai)=0;  //现实听牌信息 不能打的牌
    
    virtual void AddMark(Node* pNode,int value,std::string const& mark)=0;
    
    virtual void ShowErrorAndExit(std::string const& message){};
    
    //调整玩家的位置 语音位置 和系统播放声音位置
    //
    //int idOffset  服务器对玩家索引的偏移 因为语音服务器不允许 idIndex=0

    
    
        
    void ParseCommand();
    void SendCommand(std::string& cmd);

    
    void NodeTouch(Node* pNode,NTouch dir);
    void SendAction(std::string const& actionName);

    
    
    void OnWebsocketData(boost::system::error_code ec,std::string const&strText);
    
    
    float GetCardWidth(){return cardWidth_;}
    float GetCardHeight(){return cardHeight_;}
    float GetCardDepth(){return cardDepth_;}
    
    Vector3 GetCardWallStart(DeskDir dir);
    Vector3 GetCardPos_Zhua(DeskDir dir);
    Vector3 GetCardPos_Chu(DeskDir dir);
    Vector3 GetCardPos_Qi(DeskDir dir);
    Vector3 GetCardPos_Fix(DeskDir dir,int32_t size,int32_t i);
    Vector3 GetCardPos_Liang(DeskDir dir);

    
    int GetCardCount(int c)
    {
        return publicCards_.count(c);
    }
    int GetCardWallQiCol()
    {
        if(playerCount_==2)
            return 10;
        return 6;
    }
    
    bool is_ting_card(int card)
    {
        if(cardsTing_.size()>0&&cardsTing_.find(card)!=cardsTing_.end())
            return true;
        else
            return false;
    }
    
    std::map<int,std::map<int,int>>& GetCardsLiang()
    {
        return cardsLing_;
    }
private:
    void  onStream(std::error_code const& failure,std::shared_ptr<IStream> pStream);
    
     std::shared_ptr<IStream>    stream_;
private:
    float   GetFixStartLength();
    DeskDir GetDirection(int32_t pos);
    
    void  AddPublicCard(int card)
    {
        publicCards_.insert(card);
        gamblers_[myPos_]->ShowHuCards();
    }
    std::string GetActionSound(const std::string& action);

    


private:
    std::thread                                 net_thread_;

    
    
    std::shared_ptr<ws_session>                 server_;
    
    //玩家数组和服务器返回的 玩家列表对应  
    std::vector<std::shared_ptr<Gambler>>       gamblers_;
    
    std::shared_ptr<Gambler>                    playering_;
    Node*                                       pCardDiscard_;
    
    std::mutex                                  mutex_;
    std::queue<std::string>                     messages_;
    
    std::multiset<int>                          publicCards_;//出牌计算器
    
    std::set<int>                               cardsTing_; //听牌,所有人不能打的牌
    //调试不显示牌的原因
    std::map<int,std::map<int,int>>             cardsLing_; //我的亮牌信息

    
    
    int32_t             playerCount_;
    int32_t             myPos_;
    int32_t             bankerPos_;
    int32_t             leftPos_;
    int32_t             rightPos_;
    

    float             cardWidth_;
    float             cardHeight_;
    float             cardDepth_;
    
    float             dist1_;
    float             dist2_;
    float             adjustMe_;
    
    
    FacePowerPtr              facepower_;
};
