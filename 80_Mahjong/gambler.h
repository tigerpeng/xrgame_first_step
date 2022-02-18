#pragma once
#include <string>
#include <memory>
#include <vector>
#include <map>
#include <set>


namespace Urho3D
{
    
    class Node;
    class Scene;
    class Sprite;
    
}
// All Urho3D classes reside in namespace Urho3D
using namespace Urho3D;



enum DeskDir{
    Me=0,
    Left,
    Forward,
    Right,
    Unknow
};
enum CardState{
    Catched=0,               //抓牌
    Playing,                //牌墙
    Discarding,             //出牌
    Discarded,              //出牌后
    Penged
};
enum UserAction
{
    //desk action
    ListPlayer=2021,             //列出所有玩家  1004
    CheckChips,                  //
    CheckChipsOver,              //结果
    PlayerChoiceOver,            //玩家选择飘 1 2 3  cs双向发送
    
    RoundNew,                    //一圈开始
    RoundInfo,                   //本圈的信息 多种信息
    RoundOver,                   //一圈结束
    RoundNext,                   //下一圈开始
    SplitChips,
    GameOver,                    //游戏结束 退出
    
    //user action
    ChoosePiao,                  //2031
    Zhua,                        //抓牌
    Chu,
    Chi,
    Peng,
    Gang,
    Ting,
    Liang,
    HuPai,
    Pass,                        //过
    NoResult,                    //流局
    ForceLiang,                  //强制亮牌
    BuyHorse                     //买马
};
#include <iostream>
class Desk;
class Gambler
{
public:
    Gambler(Desk* desk);
    
    void NewRound();
    void SetDir(DeskDir dir);
    DeskDir GetDir();
    void LoadCard(std::vector<int32_t>& cards);
    void LoadCard();
    
    void Card_Zhua(int32_t card);
    
    Node* Card_Chu(int32_t card); //出牌后，把牌的控制权交给桌子
    void  Card_Qi(Node* pDisCard);              //出牌没人要，桌子把控制权转给出牌者，控制移动到合适位置
    
    void Card_Peng(Node* pPeng,int32_t value);
    void Card_Gang(Node* pPeng,int32_t value,int32_t type);
    void Card_Liang(std::vector<int32_t>& cards);
    
    void Buy_Horse(int card);
    
    void CardSort();
    int32_t CanSelected(Node* pNode,bool& liangPress);
    //void Card_Relay();      //清楚当前的出牌,转移控制权
    
    int32_t GetCardValue(Node* pNode);
    
    void SetCardSelected(Node* pNode){pSelected_=pNode;}
    
    int32_t GetGangCard();
    
    //设置声音
    void SetWhoSound(std::string const& who){
        whoSound_=who;
    }
    std::string GetCardSound(int32_t card);
    std::string GetActionSound(const std::string& action);
    
    
    void SetProfile(std::string const& avatar,std::string const& profile)
    {
        avatar_=avatar;
        profile_=profile;
    }
    std::string& GetProfile(){return profile_;}
    std::string& GetAvatar(){return  avatar_;}
    
    void setLiangCards(std::map<int,std::map<int,int>>& cards){
        liangSelectCards_.clear();
        
        liangSelectCards_=cards;
        std::cout<<"---------------->亮倒调试:牌的张数="<<liangSelectCards_.size()<<std::endl;
    }
    void startLiang();
    void endLiang();
    bool isLiangPress(){return liangPress_;}
    bool getHuinfo(int card,std::map<int,int>& huPai);
//    {
//        auto it=liangSelectCards_.find(card);
//        if(it!=liangSelectCards_.end())
//        {
//            huPai=it->second;
//            return true;
//        }
//        
//        return false;
//    }
    
    void UnHideCard(Node* pNode);
    void GetKouCards(std::set<int>& cards)
    {
        for(auto c:hides_)
            if(c.second)
                cards.insert(c.first);
    }
    //胡的牌
    void SetHuCards(std::set<int>& cards);
    void ShowHuCards();
    
    void ScanDisableCards();
public:
    static std::string GetAvatar(int sex,int uid,std::string const& a="");
private:
    Desk*                             desk_;
    std::multimap<int32_t,Node*>      playing_;           //手牌
    std::vector<int32_t>              fix_;               //被固定的牌 组
 
    std::multimap<int,Node*>          liangCards_;        //亮倒的牌
    
    std::string                      whoSound_;
    
    
    std::string                     profile_;
    std::string                     avatar_;
    //账户名：
    //昵称：
    //个人信息
    
    DeskDir     dir_;
    
    int32_t                     totalQi_;
    
    int32_t                     zhuaing_;
    bool                        liang_;
    
    Node*                       pZhua_;
    int32_t                     zhuaValue_;
    
    Node*                                  pSelected_;

    bool                                   liangPress_;         //用户选择亮
    std::map<int,std::map<int,int>>        liangSelectCards_;   //用户选择 亮牌 后只能选的牌
    
    std::map<int,bool>                     hides_;
    
    std::map<int,int>                      huCards_;
};
