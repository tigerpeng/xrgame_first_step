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

#include "gambler.h"
#include "desk.h"
#include <boost/format.hpp>

const Quaternion MEROT(0, 90, 30);
const Quaternion ME_PA(0, 90, -90);//趴
#define PA_OFFSET               1.0f
#define LIANG_OFFSET            1.0f

#define MY_CARDS_WALL_Y         0.f


int canHu(int cardAdd,const std::multiset<int>& cards)
{
    std::multiset<int32_t>    result;
    result=cards;
    if(cardAdd>0)
        result.insert(cardAdd);
    
    bool        is_hu=false;
    int         jiang=0;
    int         nong=0;
    //第一步 找到所有可能为将的牌（一对）
    std::set<int32_t>  pairs;
    auto iter=result.rbegin();
    while(iter!=result.rend())
    {
        int count=result.count(*iter);
        if(count>=2)
        {
            if(count==4&&pairs.find(*iter)==pairs.end())
                nong++;
            
            pairs.insert(*iter);
        }
        
        iter++;
    }
    
    //第二步 对将牌进行具体情况的分析
    if(pairs.size()+nong==7)
    {//7 对胡牌
        if(nong==0)
            return -14;
        else if(nong>0)
            return -nong; //龙7对，双龙7对
    }else{
        for(auto p : pairs)
        {
            jiang=p;
            
            std::multiset<int32_t>    tmp=result;
            auto it1=tmp.find(jiang);
            auto it2=it1;++it2;
            auto it3=it2;
            tmp.erase(it1);
            tmp.erase(it2);//删除一对将
            
            if(tmp.size()==0)
            {
                is_hu=true;
                break;
            }
            
            //剩下的组成刻字或顺子
            for(int j=0;j<4;j++)//最多4次
            {
                it1=tmp.begin();
                if(it1==tmp.end())
                    break;
                
                const int32_t value=*it1;
                if(tmp.count(*it1)>=3)
                {   //刻字优先剔除
                    tmp.erase(tmp.begin());
                    tmp.erase(tmp.begin());
                    tmp.erase(tmp.begin());
                }else if(value<30)
                {   //筒条万才能组成顺子
                    it2=tmp.find(value+1);
                    it3=tmp.find(value+2);
                    if(it2!=tmp.end()&&it3!=tmp.end())
                    {
                        tmp.erase(it1);
                        tmp.erase(it2);
                        tmp.erase(it3);
                    }
                }
            }
            //判断剩下牌的个数
            if(tmp.size()==0)
            {
                is_hu=true;
                break;
            }
        }
    }
    
    if(is_hu)
        return jiang;
    else
        return 0;
}

//手里的牌是  3*N+1
bool canLiang(const std::multiset<int>& cards,std::set<int>& perhaps)
{
    //加的一张牌可能取值
    auto it=cards.begin();
    while(it!=cards.end())
    {
        int value=*it;
        perhaps.insert(value);
        if(value<30)
        {
            int valuePre=value-1;
            int valueNex=value+1;
            if(valuePre%10!=0)
                perhaps.insert(valuePre);
            if(valueNex%10!=0)
                perhaps.insert(valueNex);
        }
        it++;
    }
    
    auto itAdd=perhaps.begin();
    while(itAdd!=perhaps.end())
    {
        int result=canHu(*itAdd,cards);
        if(result==0)
        {
            auto itDel=itAdd;
            itAdd++;
            perhaps.erase(itDel);
            continue;
        }
        
        itAdd++;
    }
    
    if(perhaps.size()>0)
        return true;
    
    return false;
}

//手里的牌正好 3*n+2
bool canLiangII(std::multiset<int>& myCards,std::map<int,std::set<int>>& removes
                ,Desk* desk)
{
    std::set<int> tempPerhaps;
    std::map<int,int> pa;
    
    if((myCards.size()-2)%3!=0)
         return false;
    
    std::multiset<int> temp;
    auto it=myCards.begin();
    while(it!=myCards.end())
    {
        int value=*it;
        temp=myCards;
        temp.erase(temp.find(value));
        
        if(canLiang(temp,tempPerhaps))
        {
            //
            if(!desk->is_ting_card(value))
                removes.insert(std::make_pair(value, tempPerhaps));
            //检查能否有趴下的牌
            
        }
        it++;
    }
    
    if(removes.size()>0)
        return true;
    
    return false;
}

Gambler::Gambler(Desk* desk)
:desk_(desk)
,totalQi_(0)
,liang_(false)
,pZhua_(nullptr)
,zhuaValue_(0)
,pSelected_(nullptr)
,liangPress_(false)
{
    
}
void Gambler::NewRound()
{
    pSelected_=nullptr;
    pZhua_=nullptr;
    
    totalQi_=0;
    zhuaing_=0;
    zhuaValue_=0;
    liang_=false;
    liangPress_=false;
    
    fix_.clear();
    playing_.clear();
    liangSelectCards_.clear();
    std::cout<<"liangSelectCards_------>debug NewRound"<<std::endl;
    
}
void Gambler::SetDir(DeskDir dir)
{
    dir_=dir;
}
DeskDir Gambler::GetDir()
{
    return dir_;
}
std::string Gambler::GetAvatar(int sex,int uid,std::string const& a)
{
    return str(boost::format("%d_%d.jpg")%sex%(uid%12));
}
//我自己的牌墙
void Gambler::LoadCard(std::vector<int32_t>& cards)
{
    Quaternion rota=MEROT;//Quaternion(0, 90, 0);;
    Vector3 pos=desk_->GetCardWallStart(dir_);
    for(int32_t i=0;i<(int32_t)cards.size();i++)
    {
        int32_t value=cards[i];
        Node* pCard=desk_->CreateMahJong(value, pos,rota);
        playing_.insert(std::make_pair(value,pCard));
        
        pos.x_-=desk_->GetCardWidth();
    }
    //洗牌
    CardSort();
}
void Gambler::LoadCard()
{
    Quaternion rota;
    if(DeskDir::Right==dir_)
        rota=Quaternion(0,0,0);
    else if(DeskDir::Me==dir_)
        rota=MEROT;//Quaternion(0, 90, 0);
    else if(DeskDir::Left==dir_)
        rota=Quaternion(0, 180, 0);
    else if(DeskDir::Forward==dir_)
        rota=Quaternion(0, -90, 0);

    Vector3 pos=desk_->GetCardWallStart(dir_);
    for(int32_t i=0;i<13;i++)
    {
       Node* pCard=desk_->CreateMahJong(0, pos,rota);
       playing_.insert(std::make_pair(0,pCard));
        
        if(DeskDir::Left==dir_)
            pos.z_+=desk_->GetCardWidth();
        else  if(DeskDir::Right==dir_)
             pos.z_-=desk_->GetCardWidth();
        else if(DeskDir::Forward==dir_)
             pos.x_+=desk_->GetCardWidth();

    }
}
//整理牌
void Gambler::CardSort()
{
    Vector3 pos=desk_->GetCardWallStart(dir_);
    if(DeskDir::Me==dir_)
    {
        auto iter=playing_.rbegin();
        while(iter!=playing_.rend())
        {
            Node* pCard=iter->second;
            pCard->SetPosition(pos);
            pos.x_-=desk_->GetCardWidth();
            
            iter++;
        }
    }else if(DeskDir::Left==dir_)
    {
        auto iter=playing_.begin();
        while(iter!=playing_.end())
        {
            Node* pCard=iter->second;
            pCard->SetPosition(pos);
            pos.z_+=desk_->GetCardWidth();
            
            iter++;
        }
    }else if(DeskDir::Right==dir_)
    {
        auto iter=playing_.begin();
        while(iter!=playing_.end())
        {
            Node* pCard=iter->second;
            pCard->SetPosition(pos);
            pos.z_-=desk_->GetCardWidth();
            
            iter++;
        }
    }else if(DeskDir::Forward==dir_)
    {
        auto iter=playing_.rbegin();
        while(iter!=playing_.rend())
        {
            Node* pCard=iter->second;
            pCard->SetPosition(pos);
            pos.x_+=desk_->GetCardWidth();
            
            iter++;
        }
    }
}
//抓牌
void Gambler::Card_Zhua(int32_t card)
{
    zhuaValue_=card;      //记录抓的牌
    
    Vector3 pos=desk_->GetCardPos_Zhua(dir_);
    
    Quaternion rota;
    if(DeskDir::Right==dir_)
        rota=Quaternion(0,0,0);
    else if(DeskDir::Me==dir_)
        rota=MEROT;//Quaternion(0, 90, 0);
    else if(DeskDir::Left==dir_)
        rota=Quaternion(0, 180, 0);
    else if(DeskDir::Forward==dir_)
        rota=Quaternion(0, -90, 0);
    
    pZhua_=desk_->CreateMahJong(card,pos,rota);
    playing_.insert(std::make_pair(card, pZhua_));
    
    //打禁用标识
    if(desk_->is_ting_card(card))
        desk_->AddMark(pZhua_, card, "no");
    
    if(liang_)
        pSelected_=pZhua_;
}
//出牌
Node* Gambler::Card_Chu(int32_t card)
{
    endLiang();
    
   Node* pDisCard=nullptr;
    int32_t value;
    if(DeskDir::Left==dir_||DeskDir::Right==dir_||DeskDir::Forward==dir_)
        value=0;
    else if(DeskDir::Me==dir_)
        value=card;
    
    if(DeskDir::Me==dir_)
    {
        bool finded=false;
        if(pSelected_)
        {//优先出选中的节点
            auto it=playing_.begin();
            while(it!=playing_.end())
            {
                if(it->second==pSelected_&&it->first==value)
                {
                    pDisCard=it->second;
                    playing_.erase(it);
                    finded=true;
                    break;
                }
                it++;
            }
        }
        
        if(!finded)
        {
            auto iter=playing_.find(value);
            if(iter!=playing_.end())
            {
                pDisCard=iter->second;
                playing_.erase(iter);
            }
        }

    }else {
        
        if(!liang_)
        {//随机一张牌
            int32_t index=Random((int32_t)playing_.size());
            auto iter=playing_.begin();
            while(iter!=playing_.end())
            {
                if(index==0)
                {
                    pDisCard=iter->second;
                    playing_.erase(iter);
                    break;
                }
                
                index--;
                iter++;
            }
        }else if(pSelected_){//亮倒后只能出 抓的牌
                auto it=playing_.begin();
                while(it!=playing_.end())
                {
                    if(it->second==pSelected_)
                    {
                        pDisCard=it->second;
                        playing_.erase(it);
                        break;
                    }
                    it++;
                }
        }else {
            if(playing_.size()>0){
                pDisCard=playing_.begin()->second;
                playing_.erase(playing_.begin());
            }
        }

        if(pDisCard)
            desk_->ModifyNode(pDisCard, card);
    }

    if(pDisCard)
    {//旋转
        if(DeskDir::Left==dir_)
        {
            pDisCard->SetPosition(desk_->GetCardPos_Chu(dir_));
            pDisCard->SetRotation(Quaternion(0, 180, 90));
        }
        else if(DeskDir::Right==dir_)
        {   pDisCard->SetPosition(desk_->GetCardPos_Chu(dir_));
            pDisCard->SetRotation(Quaternion(0, 0, 90));
        }
        else if(DeskDir::Me==dir_)
        {   pDisCard->SetPosition(desk_->GetCardPos_Chu(dir_));
            pDisCard->SetRotation(Quaternion(0, 90, 90));
        }else if(DeskDir::Forward==dir_)
        {
            pDisCard->SetPosition(desk_->GetCardPos_Chu(dir_));
            pDisCard->SetRotation(Quaternion(0, -90, 90));
        }
    }else{
        int a=0;a++;//未找到牌
    }
    
    CardSort();//add by copyleft
    zhuaValue_=0;
    return pDisCard;
}
//void Gambler::Card_Relay()
//{
//    //pDisCarding_=nullptr;
//}
void Gambler::Card_Qi(Node* pDisCard)
{
    desk_->PlayMjSound(GetActionSound("card_qi"));
    
    int cols=desk_->GetCardWallQiCol();
    Vector3 pos=desk_->GetCardPos_Qi(dir_);
    if(pDisCard)
    {
        int8_t row =totalQi_/cols;
        int8_t col =totalQi_%cols;
        if(DeskDir::Left==dir_)
        {
            pos.x_-=row*desk_->GetCardHeight();
            pos.z_-=col*desk_->GetCardWidth();
        }
        else if(DeskDir::Right==dir_)
        {
            pos.x_+=row*desk_->GetCardHeight();
            pos.z_+=col*desk_->GetCardWidth();
        }
        else if(DeskDir::Me==dir_)
        {
            pos.x_+=col*desk_->GetCardWidth();
            pos.z_-=row*desk_->GetCardHeight();
        }else if(DeskDir::Forward==dir_)
        {
            pos.x_-=col*desk_->GetCardWidth();
            pos.z_+=row*desk_->GetCardHeight();
        }
        pDisCard->SetPosition(pos);
        totalQi_++;
    }
    
    //洗牌
    CardSort();
}
int32_t Gambler::GetCardValue(Node* pNode)
{
    int32_t value=-1;
    auto iter=playing_.begin();
    while(iter!=playing_.end())
    {
        if(iter->second==pNode)
        {
            value=iter->first;
            break;
        }
        iter++;
    }
    return value;
}
//碰牌
void Gambler::Card_Peng(Node* pPeng,int32_t value)
{
    std::vector<Node*> vNode;
    vNode.push_back(pPeng);
    
    for(int i=0;i<2;i++)
    {
        auto iter=playing_.find(DeskDir::Me==dir_?value:0);
        if(iter!=playing_.end())
        {
            Node* pFind=iter->second;
            if(DeskDir::Me!=dir_)
                desk_->ModifyNode(pFind, value);
            
            vNode.push_back(pFind);
            playing_.erase(iter);
        }
    }

    if(vNode.size()==3)
    {
        for(size_t i=0;i<vNode.size();i++)
        {
            Vector3 pos=desk_->GetCardPos_Fix(dir_,fix_.size(),i);
            Node* pDel=vNode[i];
            
            if(pDel)
            {
                //旋转
                if(DeskDir::Left==dir_)
                    pDel->SetRotation(Quaternion(0, 180, 90));
                else if(DeskDir::Right==dir_)
                    pDel->SetRotation(Quaternion(0, 0, 90));
                else if(DeskDir::Me==dir_)
                    pDel->SetRotation(Quaternion(0, 90, 90));
                else if(DeskDir::Forward==dir_)
                    pDel->SetRotation(Quaternion(0, -90, 90));
                
                pDel->SetPosition(pos);
            }

        }
        fix_.push_back(value);
    }
}

//杠
void Gambler::Card_Gang(Node* pGang,int32_t value,int32_t type)
{
    std::vector<Node*> vNode;
    //点杠
    if(pGang!=nullptr&&type==-3)
        vNode.push_back(pGang);
    
    //将手里的牌删除，移动到 vNode
    for(int i=0;i<std::abs(type);i++)
    {
        auto iter=playing_.find(DeskDir::Me==dir_?value:0);
        if(iter!=playing_.end())
        {
            Node* pFind=iter->second;
            if(DeskDir::Me!=dir_)
                desk_->ModifyNode(pFind, value);
            
            vNode.push_back(pFind);
            playing_.erase(iter);
        }
    }
    
    //设置位置
    if(vNode.size()==4)
    {
        Vector3 Pos2;
        for(size_t i=0;i<vNode.size();i++)
        {
            Vector3 pos=desk_->GetCardPos_Fix(dir_,fix_.size(),i);
            Node* pDel=vNode[i];
            
            if(i==1)
                Pos2=pos;
            else if(i==3)
            {
                pos.x_=Pos2.x_;
                pos.z_=Pos2.z_;
                pos.y_+=desk_->GetCardDepth();
            }

            //旋转
            if(DeskDir::Left==dir_)
                pDel->SetRotation(Quaternion(0, 180, 90));
            else if(DeskDir::Right==dir_)
                pDel->SetRotation(Quaternion(0, 0, 90));
            else if(DeskDir::Me==dir_)
                pDel->SetRotation(Quaternion(0, 90, 90));
            else if(DeskDir::Forward==dir_)
                pDel->SetRotation(Quaternion(0, -90, 90));
            
            pDel->SetPosition(pos);
        }
        fix_.push_back(value);
    }else if(vNode.size()==1){//碰杠
        Node* pDel=vNode[0];
        
        Vector3 pos;
        for(int i=0;i<fix_.size();i++)
        {
            if(fix_[i]==value)
            {
                pos=desk_->GetCardPos_Fix(dir_,i,1);
            }
        }
        
        pos.y_+=desk_->GetCardDepth();
        
        //旋转
        if(DeskDir::Left==dir_)
            pDel->SetRotation(Quaternion(0, 180, 90));
        else if(DeskDir::Right==dir_)
            pDel->SetRotation(Quaternion(0, 0, 90));
        else if(DeskDir::Me==dir_)
            pDel->SetRotation(Quaternion(0, 90, 90));
        else if(DeskDir::Forward==dir_)
            pDel->SetRotation(Quaternion(0, -90, 90));
        
        pDel->SetPosition(pos);
    }
}

void Gambler::Card_Liang(std::vector<int32_t>& cards)
{
    if(liang_&&pSelected_)
    {//第2 次亮牌为胡牌
        //旋转亮倒
        if(DeskDir::Left==dir_)
            pSelected_->SetRotation(Quaternion(0, 180, 90));
        else if(DeskDir::Right==dir_)
            pSelected_->SetRotation(Quaternion(0, 0, 90));
        else if(DeskDir::Forward==dir_)
            pSelected_->SetRotation(Quaternion(0, -90, 90));
        else if(DeskDir::Me==dir_)
            pSelected_->SetRotation(Quaternion(0, 90, 90));
    }
    
    //筛选出 亮的牌
    std::multimap<int,Node*> lingNodes;
    
    liang_=true;
    if(DeskDir::Me==dir_)
    {
        for(auto c:cards)
        {
            auto it=playing_.find(c);
            if(it!=playing_.end())
            {
                lingNodes.insert(std::make_pair(c, it->second));
                playing_.erase(it);
            }
        }
    }else
    {
       for(auto c:cards)
       {
           auto it=playing_.begin();
           if(it!=playing_.end())
           {
               Node* pLiang=it->second;
               desk_->ModifyNode(pLiang, c);
               
               //旋转亮倒
               if(DeskDir::Left==dir_)
                   pLiang->SetRotation(Quaternion(0, 180, 90));
               else if(DeskDir::Right==dir_)
                   pLiang->SetRotation(Quaternion(0, 0, 90));
               else if(DeskDir::Forward==dir_)
                   pLiang->SetRotation(Quaternion(0, -90, 90));
               
               lingNodes.insert(std::make_pair(c, pLiang));
               playing_.erase(it);
           }
       }
    }
    
    
    
    //对 亮倒的牌进行排序 修复牌面 修改位置和旋转
    Vector3 pos=desk_->GetCardPos_Liang(dir_);
    auto iter=lingNodes.rbegin();
    while(iter!=lingNodes.rend())
    {
        if(DeskDir::Left==dir_)
            pos.z_+=desk_->GetCardWidth();
        else if(DeskDir::Right==dir_)
            pos.z_-=desk_->GetCardWidth();
        else if(DeskDir::Forward==dir_)
            pos.x_+=desk_->GetCardWidth();
        else if(DeskDir::Me==dir_)
            pos.x_-=desk_->GetCardWidth();
        
        
        
        Node* pCard=iter->second;
        pCard->SetPosition(pos);

        
        desk_->ModifyNode(pCard, iter->first);
        
        if(DeskDir::Left==dir_)
            pCard->SetRotation(Quaternion(0, 180, 90));
        else if(DeskDir::Right==dir_)
            pCard->SetRotation(Quaternion(0, 0, 90));
        else if(DeskDir::Forward==dir_)
            pCard->SetRotation(Quaternion(0, -90, 90));
        else if(DeskDir::Me==dir_)
            pCard->SetRotation(Quaternion(0, 90, 90));
        
        iter++;
    }
    
    if(liangCards_.size()>0)
    {  //之前已经亮过牌了重新排序
        if(DeskDir::Left==dir_)
            pos.z_+=1.f;
        else if(DeskDir::Right==dir_)
            pos.z_-=1.f;
        else if(DeskDir::Forward==dir_)
            pos.x_+=1.f;
        else if(DeskDir::Me==dir_)
            pos.x_-=1.f;
        
        auto itLast=liangCards_.rbegin();
        while(itLast!=liangCards_.rend())
        {
            if(DeskDir::Left==dir_)
                pos.z_+=desk_->GetCardWidth();
            else if(DeskDir::Right==dir_)
                pos.z_-=desk_->GetCardWidth();
            else if(DeskDir::Forward==dir_)
                pos.x_+=desk_->GetCardWidth();
            else if(DeskDir::Me==dir_)
                pos.x_-=desk_->GetCardWidth();
            
            Node* pCard=itLast->second;
            pCard->SetPosition(pos);

            desk_->ModifyNode(pCard, itLast->first);
            
            if(DeskDir::Left==dir_)
                pCard->SetRotation(Quaternion(0, 180, 90));
            else if(DeskDir::Right==dir_)
                pCard->SetRotation(Quaternion(0, 0, 90));
            else if(DeskDir::Forward==dir_)
                pCard->SetRotation(Quaternion(0, -90, 90));
            else if(DeskDir::Me==dir_)
                pCard->SetRotation(Quaternion(0, 90, 90));
            
            itLast++;
        }
        
    }else
        liangCards_=lingNodes;
    
}

std::string Gambler::GetCardSound(int32_t card){
    std::string soundFile=str(boost::format("Sounds/mj/%s/%d.ogg") % whoSound_ % card);
    return soundFile;
}
std::string Gambler::GetActionSound(const std::string& action)
{
    std::string soundFile=str(boost::format("Sounds/mj/%s/%s.ogg") % whoSound_ % action);
    return soundFile;
}
//得到gang 牌
int32_t Gambler::GetGangCard()
{
    int card=0;
    int gangCount=0;
    auto iter=playing_.begin();
    while (iter!=playing_.end()) {
        if(playing_.count(iter->first)==4)
        {
            card=iter->first;
            gangCount++;
        }
        
        iter++;
    }
    if(gangCount==1)
        return card;
    else if(gangCount>1)
        return -gangCount;
    else{
        
        int pengCount=0;
        for(int i=0;i<fix_.size();i++)
        {
            if(fix_[i]==zhuaing_)
                pengCount++;
        }
        if(pengCount==3)
            return -1;
        else
            return  0;
    }
}
int32_t Gambler::CanSelected(Node* pNode,bool& liangPress)
{
    if(liang_)
    {//亮倒后，只能出 抓的牌
        std::string cmd=str(boost::format("{\"action\":%d,\"card\":%d}") % UserAction::Chu % zhuaValue_);
        desk_->SendCommand(cmd);
        return 0;
    }
    
    liangPress=liangPress_;
    if(liangPress)
    {
        Vector3 pos=pNode->GetPosition();
        if(pos.y_>0)
        {
            int32_t value=GetCardValue(pNode);
            if(value>0)
            {
                SetCardSelected(pNode);
                
                //liangPress_=false;
                //liangSelectCards_.clear();
                return value;
            }
        }else
            return 0;
    }
    
    
    int32_t value=GetCardValue(pNode);
    if(value>0)
        SetCardSelected(pNode);
    
    return value;
}
void Gambler::startLiang()
{
    liangPress_=true;
    
    {
        hides_.clear();
        //检查趴的的牌
        std::multiset<int> cards;
        for(auto c:playing_)
            cards.insert(c.first);
        
        auto it=cards.begin();
        while(it!=cards.end())
        {
            if(cards.count(*it)>=3)
            {//3个头
                std::multiset<int> tmp=cards;
                for(int i=0;i<3;++i)
                    tmp.erase(tmp.find(*it));
                //检查胡的牌
                std::map<int,std::set<int>> removes;
                if(canLiangII(tmp,removes,desk_))
                {
                    hides_.insert(std::make_pair(*it, false));
                }
            }
            ++it;
        }
        //给麻将打标签  扣/趴
        for(auto k:hides_)
        {
            auto itc=playing_.find(k.first);
            if(itc!=playing_.end())
            {
                for(int i=0;i<3;i++)
                {
                    desk_->AddMark(itc->second, itc->first,"kou");
                    ++itc;
                }
            }
        }
    }

    
    //清除状态
    auto iter=playing_.begin();
    while (iter!=playing_.end()) {
        
            Node*  pSelect=iter->second;
            Vector3 pos=pSelect->GetPosition();
            //pos.y_=MY_CARDS_WALL_Y;
            pSelect->SetPosition(pos);
 
        iter++;
    }
    
    if(liangSelectCards_.size()==0)
    {
        std::cout<< "亮倒的牌为空" <<std::endl;
        //主要原因是本机打出的牌，服务器尚未通知亮倒
        int i=0;i++;
        //检查胡的牌
        //检查趴的的牌
        std::multiset<int> cards;
        for(auto c:playing_)
            cards.insert(c.first);
        
        std::map<int,std::set<int>> removes;
        if(canLiangII(cards,removes,desk_))
        {
            for(auto f:removes)
            {
                auto rfind=playing_.rbegin();
                while(rfind!=playing_.rend())
                {
                    if(rfind->first==f.first)
                    {
                        Vector3 pos=rfind->second->GetPosition();
                        pos.y_+=PA_OFFSET;
                        rfind->second->SetPosition(pos);
                        break;
                    }
                    ++rfind;
                }
            }
        }
    }
    
    //让可选的牌跳出来 供用户选择
    iter=playing_.begin();
    while (iter!=playing_.end())
    {
        if(liangSelectCards_.find(iter->first)!=liangSelectCards_.end())
        {
            Node*  pSelect=iter->second;
            Vector3 pos=pSelect->GetPosition();
            pos.y_+=1.f;
            pSelect->SetPosition(pos);
        }
        iter++;
    }
}
void Gambler::endLiang()
{
    if(liangPress_)
    {
        liangPress_=false;
        //清理状态
        liangSelectCards_.clear();
        std::cout<<"liangSelectCards_------>debug"<<std::endl;
        
        std::map<int,int> null;
        
        if(dir_==DeskDir::Me)
            desk_->ShowHuInfo(null);

        if(hides_.size()>0)
        {
            //清理标记
            for(auto c:playing_)
            {
                if(hides_.find(c.first)!=hides_.end())
                    desk_->ModifyNode(c.second, c.first);
            }
            hides_.clear();
        }

    }
    
    if(!liang_)
        liangPress_=false;
    

}

//选择趴下
void Gambler::UnHideCard(Node* pNode)
{
    int32_t value=GetCardValue(pNode);
    
    {
        //选择取消 刻字
        auto it=hides_.find(value);
        if(it!=hides_.end()&&playing_.count(value)>=3)
        {
            if(it->second==false)
            {//后移动
                Node* pCard[3];
                auto c=playing_.find(value);
                pCard[0]=c->second;
                ++c;
                pCard[1]=c->second;
                ++c;
                pCard[2]=c->second;
                for(int i=0;i<3;++i)
                {
                    Vector3 pos=pCard[i]->GetPosition();
                    pos.z_-=PA_OFFSET;
                    pCard[i]->SetPosition(pos);
                }
            }else
            {//前移动
                Node* pCard[3];
                auto c=playing_.find(value);
                pCard[0]=c->second;
                ++c;
                pCard[1]=c->second;
                ++c;
                pCard[2]=c->second;
                for(int i=0;i<3;++i)
                {
                    Vector3 pos=pCard[i]->GetPosition();
                    pos.z_+=PA_OFFSET;
                    pCard[i]->SetPosition(pos);
                }
            }
            
            it->second=!it->second;
        }
    }

    
    //全部下来
    auto iter=playing_.begin();
    while(iter!=playing_.end())
    {
        Vector3 pos=iter->second->GetPosition();
        //pos.y_=MY_CARDS_WALL_Y;
        iter->second->SetPosition(pos);
        
        ++iter;
    }
    //
    
    {
        //检查趴的的牌
        std::multiset<int> cards;
        for(auto c:playing_)
            cards.insert(c.first);
        
        for(auto d:hides_)
        {
            if(d.second==true)
                for(int i=0;i<3;++i)
                {
                    auto del=cards.find(d.first);
                    if(del!=cards.end())
                        cards.erase(del);
                }
        }
        
        //检查胡的牌
        std::map<int,std::set<int>> removes;
        if(canLiangII(cards,removes,desk_))
        {
            for(auto f:removes)
            {
                auto rfind=playing_.rbegin();
                while(rfind!=playing_.rend())
                {
                    if(rfind->first==f.first)
                    {
                        Vector3 pos=rfind->second->GetPosition();
                        pos.y_+=PA_OFFSET;
                        rfind->second->SetPosition(pos);
                        break;
                    }
                    ++rfind;
                }
            }

        }
        //修改提示
    }
}

void Gambler::SetHuCards(std::set<int>& cards)
{
    for(auto c:cards)
        huCards_.insert(std::make_pair(c, 0));
    
    //只显示自己的信息
    if(dir_==DeskDir::Me)
        desk_->ShowHuInfo(huCards_);
}
void Gambler::ShowHuCards()
{
    if(liang_&&dir_==DeskDir::Me)
    {
        auto it=huCards_.begin();
        while(it!=huCards_.end())
        {
            int count=4-desk_->GetCardCount(it->first);
                count-=playing_.count(it->first);
            if(count<0)
                count=0;
            it->second=count;
            
            ++it;
        }
        
        desk_->ShowHuInfo(huCards_);
    }
}

bool Gambler::getHuinfo(int card,std::map<int,int>& huPai)
{
    auto  cardLing=desk_->GetCardsLiang();

    auto it=cardLing.find(card);
    if(it!=cardLing.end())
    {
        huPai=it->second;
        return true;
    }
    
    return false;
}
void Gambler::ScanDisableCards()
{
    auto it=playing_.begin();
    while(it!=playing_.end())
    {
        if(desk_->is_ting_card(it->first))
            desk_->AddMark(it->second, it->first, "no");
        
        ++it;
    }
    
}
void Gambler::Buy_Horse(int card)
{
    Quaternion rota;
    if(DeskDir::Right==dir_)
        rota=Quaternion(0,0,0);
    else if(DeskDir::Me==dir_)
        rota=MEROT;//Quaternion(0, 90, 0);
    else if(DeskDir::Left==dir_)
        rota=Quaternion(0, 180, 0);
    else if(DeskDir::Forward==dir_)
        rota=Quaternion(0, -90, 0);
    
    int w=desk_->GetCardWidth()/2;
    int h=desk_->GetCardHeight()/2;
    auto nodeHorse=desk_->CreateMahJong(card,Vector3(-w,0.f,-h),rota);
    
    if(nodeHorse)
    {
        if(DeskDir::Left==dir_)
            nodeHorse->SetRotation(Quaternion(0, 180, 90));
        else if(DeskDir::Right==dir_)
            nodeHorse->SetRotation(Quaternion(0, 0, 90));
        else if(DeskDir::Forward==dir_)
            nodeHorse->SetRotation(Quaternion(0, -90, 90));
        else if(DeskDir::Me==dir_)
            nodeHorse->SetRotation(Quaternion(0, 90, 90));
    }
}
