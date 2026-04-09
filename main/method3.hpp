#ifndef METHOD3_HPP_
#define METHOD3_HPP_

#include <vector>
#include <set>
#include "point.h"
#include "floatingInformation.hpp"
#include "parameters.hpp"


int md_v1=0;
int md_v2=0;
int md_v3=0;
int md_v4=0;


/**
 * @brief ノードが保存する，情報ごとの追加情報
 */
class ExtendInformation{
private:
public:
    FloatingInformation *information;
    Store<EventDataReceive,EventMapUnit> store;
    Store<EventTroughRoad,EventMapRoadUnit> roads;
    unsigned int receive_counter; // ノード毎の受信回数
    double first_receive_time;
    double last_receive_time;
    
    // TODO: add 0703
    EventMapReply reply_summary;
    Store<EventReply,EventMapReply> replies;
    bool already_posted; // 既にreplyをpostした情報かどうか，reply済みならそれ以上のreplyを追加しない．

    // TODO: add 1105 指定されているTA内で受信したかどうか
    int receive_in_original_TA;
    int TA_passing_phase; // 0:before in TA, 1:in TA, 2:after in TA
    bool not_trust;

    // TODO: add 1128
    int method7_req_receive;
    int first_trust_counter;
    bool first_trust;

    // TODO: add 1210
    int trust_counter;
    ExtendInformation();
    ~ExtendInformation() noexcept = default;
    ExtendInformation(const ExtendInformation& other) = default;
    ExtendInformation(ExtendInformation&& other) noexcept = default;
    ExtendInformation & operator=(const ExtendInformation& other) = default;
    void set_information(FloatingInformation *floating_information,double current_time);
    void share(ExtendInformation *share, std::vector<EventMapUnit> *event_map, double current_time);
};

/**
 * @brief 情報拡張の初期化
 */
ExtendInformation::ExtendInformation(){
    information=NULL;
    receive_counter=0;
    first_receive_time=0.0;
    last_receive_time=0.0;
    
    // 0725
    already_posted=false;

    // 1105
    receive_in_original_TA=0;
    not_trust=false;

    // 1128
    method7_req_receive=-1;
    first_trust_counter=0;
    first_trust=false;
}

/**
 * @brief 情報の設定
 * @param floating_information 配信されている情報
 * @param current_time 現在時刻(受信時刻)
 */
void ExtendInformation::set_information(FloatingInformation *floating_information,double current_time){
    if(information==NULL){
        information=floating_information;
        first_receive_time=current_time;
        store.GS=&information->GS;
        roads.GS=&information->GSr;
        // TODO 07-03
        replies.GS=&information->replies;
    }
    ++receive_counter;
    last_receive_time=current_time;
    // add 1203
}

/**
 * @brief 受信側の通信の記録を蓄積する
 * @param ed 蓄積する受信データ
 * @param eMap 蓄積先のデータマップ
 */
void AddProcessReceive(EventDataReceive* ed,std::vector<EventMapUnit>* eMap){
    eMap->at(ed->from_id).user_id=ed->from_id;
    eMap->at(ed->to_id).user_id=ed->to_id;
    eMap->at(ed->from_id).send_to.insert(ed->to_id);
    eMap->at(ed->to_id).receive_from.insert(ed->from_id);
    eMap->at(ed->from_id).trust=ed->information_id;
    eMap->at(ed->to_id).receive_times.push_back(ed->timestamp);

    eMap->at(ed->to_id).connection_points.push_back(ed->at);
    eMap->at(ed->to_id).sender_points.push_back(ed->send_at);
}


/**
 * @brief 受信側の通信の記録を蓄積する for shareGS
 * @param ed 蓄積する受信データ
 * @param eMap 蓄積先のデータマップ
 */
void AddProcessReceive2(std::vector<int> *IF_join_nodes,EventDataReceive* ed,std::vector<EventMapUnit>* eMap){
    size_t from_itr=0;
    size_t to_itr=0;
    size_t c=0;
    for(size_t i=0,size=IF_join_nodes->size();i<size;++i){
        size_t v=IF_join_nodes->at(i);
        if(v==ed->from_id){
            from_itr=i;
            ++c;
            if(c==2)break;
        }
        if(v==ed->to_id){
            to_itr=i;
            ++c;
            if(c==2)break;
        }
    }
    eMap->at(from_itr).user_id=ed->from_id;
    eMap->at(to_itr).user_id=ed->to_id;
    eMap->at(from_itr).send_to.insert(ed->to_id);
    eMap->at(to_itr).receive_from.insert(ed->from_id);
    eMap->at(from_itr).trust=ed->information_id;
    eMap->at(to_itr).receive_times.push_back(ed->timestamp);
    
    eMap->at(from_itr).user_itr=from_itr;
    eMap->at(to_itr).user_id=to_itr;
    eMap->at(from_itr).send_to_itr.insert(to_itr);
    eMap->at(to_itr).receive_from_itr.insert(from_itr);
    ++eMap->at(from_itr).send_trust_count.at(ed->information_id);
    eMap->at(to_itr).connection_points.push_back(ed->at);
    eMap->at(to_itr).sender_points.push_back(ed->send_at);
}


/**
 * @brief 道路情報の蓄積
 * @param e 蓄積するデータ
 * @param m 蓄積先のデータマップ
 */
void AddProcessRoad(EventTroughRoad* e,EventMapRoadUnit* m){
    m->user_id_set.insert(e->node_id);
    if(m->timestamp_max<e->timestamp || m->timestamp_max<0){
        m->timestamp_max=e->timestamp;
    }
    if(m->timestamp_min>e->timestamp || m->timestamp_min<0){
        m->timestamp_min=e->timestamp;
    }
}

void AddProcessReply(EventReply* e,EventMapReply* m){
    if(e->timestamp>m->latest_timestamp){
        m->latest_timestamp = e->timestamp;
        m->latest_isFake = e->isFake;
        m->num+=1;
    }
}

/**
 * @brief 拡張情報の共有
 * @param share 共有される拡張情報
 * @param event_map 共有したデータの蓄積先
 * @param current_time 現在時刻(受信時刻)
 */
void ExtendInformation::share(ExtendInformation *share,std::vector<EventMapUnit> *event_map,double current_time){
    if(information==NULL){
        information=share->information;
        first_receive_time=current_time;
    }
    ++receive_counter;
    last_receive_time=current_time;
    store.share(&share->store,event_map,AddProcessReceive);
}

/**
 * 観測対象情報を管理するクラス
 * RAの実情を確認するときに使う */
class ObservedInformation{
private:
public:
    struct Observe{
        int information_id; // 観測情報id
        int observe_state; //観測状態 0:未観測 1:観測済
        double observed_time; // 観測時刻
        bool isFake; // 情報が偽物かどうか
        Observe() : information_id(-1),observe_state(0),observed_time(0.0),isFake(false) {}
    };
    std::vector<Observe> observe;
    std::set<int> fake_information_ids;
    
    ObservedInformation();
    ~ObservedInformation() noexcept = default;
    ObservedInformation(const ObservedInformation& other) = default;
    ObservedInformation(ObservedInformation&& other) noexcept = default;
    ObservedInformation & operator=(const ObservedInformation& other) = default;
    void add_observe(int observe_information_id,bool isFake,double current_time);
    bool is_observed_fake(int received_information_id);
};

ObservedInformation::ObservedInformation(){
    observe.clear();
    fake_information_ids.clear();
}

/**
 * @brief 観測情報の追加
 * @param observe_information_id 観測した内容を含む情報id
 * @param isFake 情報が偽装されたものかどうか
 * @param current_time 観測時刻 */
void ObservedInformation::add_observe(int observe_information_id,bool isFake,double current_time){
    if(observe.size()<(unsigned int)observe_information_id+1){ // 観測情報の空きを追加
        observe.resize(observe_information_id+1);
    }
    if(observe.at(observe_information_id).observe_state==0){
        observe.at(observe_information_id).observe_state=1;
        observe.at(observe_information_id).isFake=isFake;
        observe.at(observe_information_id).observed_time=current_time;
        if(isFake){
            fake_information_ids.insert(observe_information_id);
        }
    }
}

bool ObservedInformation::is_observed_fake(int received_information_id){
    if(observe.size()<(unsigned int)received_information_id+1){
        observe.resize(received_information_id+1);
    }
    if(observe.at(received_information_id).observe_state==1){
        return observe.at(received_information_id).isFake;
    }
    return false;
}

/**
 * @brief ノードの情報の扱い方
 * @details ノードの情報の処理についてまとめる． */
class Method{
private:
public:
    int method;
    double receive_time; // 最新の受信時刻

    // 通信と情報の保持
    std::vector<ExtendInformation> informations; // 保持している情報 informations[information id]
    std::vector<int> trust_index; // 信用する情報　trust_index[info] : NULL|addr 齟齬のある情報毎に保持
    std::vector<int> broadcast_index; // 配信する情報 broadcast_index[n] : indexは情報に依らない
    std::vector<std::vector<int>> classification; // 保持している情報を齟齬のある情報同士でまとめたもの clasification[info]>same infomation index
    std::set<int> connected_ids; // 通信を終えたノード群 新しい情報を持っていた場合も受け取らないという点に注意
    
    // TODO: add 0703
    // 自身が観測した事実を記録する．情報毎に指定されているエリアについて記録する．
    ObservedInformation observation; // 観測情報
    std::set<int> fake_index; // 偽装されたものと判断された情報のidのset fake_index[information.id]

    // for accumulator
    std::vector<EventMapUnit> event_map; // 通信イベントの蓄積データ
    EventMapRoadUnit event_road; // 道路通過イベントの蓄積データ
    Point *user_position; // ノードの位置情報へのアクセスポインタ

    // 1212
    Store<EventDataReceive,EventMapUnit> store; // whole store with sharedGS

    Method();
    ~Method() noexcept = default; // コピーコンストラクタ
    Method(const Method& other) = default; // ムーブコンストラクタ
    Method(Method&& other) noexcept = default; // コピー代入演算子
    Method & operator=(const Method& other) = default;
    void set(int method_param);
    void set_information(FloatingInformation *information,double current_time);
    void set_position(Point *user_current_position);
    void set_trust_information(FloatingInformation *information,double current_time);
    void set_broadcast_information(FloatingInformation *information,double current_time);
    
    void update_observe(int node_id,int observe_information_id,bool isFake,double current_time);
    void update_trust_information();
    int get_trust_state();
    void malicious_disguise(int from_id,int to_id,Method *share,double current_time);
    void adopt_method(ExtendInformation *information,int from_id,int to_id,double current_time,Point *from_position);
    void share_reply(ExtendInformation *information);
    void receive_information(int from_id,int to_id,Method *share,double current_time,int update_trust);
    void record_broadcast(int from_id,double current_time);
    bool select_broadcast_information(double x,double y,int consider_all_option);
};

/**
 * @brief Methodインスタンス生成
 */
Method::Method(){
    receive_time=0.0;
    informations.resize(2); // initial FloatingInformation id is -1
    classification.resize(1);
    classification.at(0).resize(2,-1);
    trust_index.resize(2,-1);
    broadcast_index.resize(2,-1);

    event_road.timestamp_max=-1.0;
    event_road.timestamp_min=-1.0;
}

/**
 * @brief 利用する方法の設定 */
void Method::set(int method_param){
    method=method_param;
}

/**
 * @brief 情報の登録
 * @param information 登録する情報
 * @param current_time 現在時刻(受信時刻) */
void Method::set_information(FloatingInformation *information,double current_time){
    if(informations.size()<(unsigned int)information->id+1){ // ノードの保有情報の空きを追加
        informations.resize(information->id+1);
    }
    if(classification.size()<(unsigned int)information->info+1){ // 情報内容が新しければ，情報リストの空きを追加
        classification.resize(information->info+1);
    }
    if(classification.at(information->info).size()<(unsigned int)information->fake_id+1){ // 情報内容について，受信した情報のfake_id分だけ拡張
        classification.at(information->info).resize(information->fake_id+1,-1);
    }
    if(trust_index.size()<(unsigned int)information->info+1){ // 新しい情報についてどの情報を信用するかを保持する部分を拡張
        trust_index.resize(information->info+1,-1);
    }
    if(broadcast_index.capacity()<(unsigned int)information->info+1){ // 配信情報指定部分を拡張
        broadcast_index.reserve(information->info+1);
    }
    informations.at(information->id).set_information(information,current_time); // 情報の登録
    classification.at(information->info).at(information->fake_id)=information->id; // 情報の内容グループごとに整理，
}

// 1105
void Method::set_position(Point *user_current_position){
    user_position=user_current_position;
    if(use_scheme_3){ // 1105 TA通過済みで受信していないなら，元の情報に戻したかった，あんまり効果ない＋不明なバグの気配
        for(size_t x=0,size=informations.size();x<size;++x){
            ExtendInformation &e = informations.at(x);
            if(e.information==NULL)continue;
            //printf("check %d phase:%d in:%d ",e.information->id,e.TA_passing_phase,e.information->check_inTA(user_position->x,user_position->y));
            if(e.TA_passing_phase==0 && e.information->check_inTA(user_position->x,user_position->y)==true){
                //printf("phase 1 id:%d\n",e.information->id);
                e.TA_passing_phase=1;
            }else if(e.TA_passing_phase==1 && e.information->check_inTA(user_position->x,user_position->y)==false){
                /*  // ちょっと良くなる程度の工夫
                int has_conflict_information=-1; // -1:no else conflict information id
                for(int i : classification.at(e.information->info)){
                    if(i>=0 && informations.at(i).information->id!=e.information->id && informations.at(i).not_trust==false){
                        has_conflict_information=i;
                    }
                }
                */
                //printf("phase 2 id:%d\n",e.information->id);
                e.TA_passing_phase=2;
                
                if(e.receive_in_original_TA==0){
                    //if((has_conflict_information>=0 && trust_index.at(e.information->info)==e.information->id && e.receive_in_original_TA/e.information->covered_road_length<informations.at(has_conflict_information).receive_in_original_TA/informations.at(has_conflict_information).information->covered_road_length))printf("should change trust to:%d\n",has_conflict_information);
                    //printf("and cant trust info id:%d\n",e.information->id);
                    e.not_trust=true;
                    if(trust_index.at(e.information->info)==e.information->id){ // もし信用していたら
                        trust_index.at(e.information->info)=-2; // 一旦，何も信用しない，他と区別するために-2とする
                        //printf("close trust : %d\n",e.information->id);
                        for(int i : classification.at(e.information->info)){
                            if(i>=0 && informations.at(i).information->id!=e.information->id && informations.at(i).not_trust==false){
                                // 齟齬のある情報の内，信用できるものを利用する
                                //printf("change trust information %d -> %d\n",e.information->id,informations.at(i).information->id);
                                trust_index.at(e.information->info)=informations.at(i).information->id;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
}

/**
 * @brief 信用して使用する情報の設定
 * @param information 信用する情報
 * @param current_time 現在時刻 */
void Method::set_trust_information(FloatingInformation *information,double current_time){
    set_information(information,current_time);
    trust_index.at(information->info)=information->id;
    informations.at(information->id).receive_in_original_TA=100; // 1105
    informations.at(information->id).first_trust=true; // 1203
}

/**
 * @brief 配信する情報の設定，FS用
 * @param information 配信する情報
 * @param current_time 現在時刻 */
void Method::set_broadcast_information(FloatingInformation *information,double current_time){
    set_information(information,current_time);
    trust_index.at(information->info)=information->id;
    broadcast_index.clear();
    broadcast_index.push_back(information->id);
}

void Method::update_observe(int node_id,int observe_information_id,bool isFake,double current_time){
    observation.add_observe(observe_information_id,isFake,current_time);
    if(isFake && (int)informations.size()>observe_information_id){
        if(informations.at(observe_information_id).information!=NULL && informations.at(observe_information_id).already_posted==false){
            ExtendInformation *ex = &informations.at(observe_information_id);
            EventReply rep={node_id,true,current_time};
            size_t index=ex->information->replies.append(rep); // Globalに追加
            ex->replies.add(index); // Userに追加
            AddProcessReply(&rep,&ex->reply_summary);
            ex->already_posted=true;
        }
    }
}

/**
 * @brief 情報の利用状態の取得
 * @details n個の情報について，インデックスを利用して利用状況を2^nまでの整数で表現する． */
int Method::get_trust_state(){
    int state=0;
    for(size_t i=0,size=trust_index.size();i<size;++i){
        int v=trust_index[i]; // v : information id
        if(fake_index.find(v)!=fake_index.end()){ // fake_indexに含まれている場合trust_indexを-1に戻す．．
            trust_index[i]=-1;
        }else{
            if(v>=0){
                state|=(1<<v);
            }
        }
    }
    return state;
}

/**
 * @brief stepのはじめに，信用する情報を更新したい場合ここに記述する */
void Method::update_trust_information(){
}

/**
 * @details 攻撃者が初めて情報を受信した場合の処理をここに */
void Method::malicious_disguise(int from_id,int to_id,Method *share,double current_time){
    int trust=trust_index.at(share->informations.at(share->broadcast_index.at(0)).information->info);
    //store.share2(&share->store,&event_map,share->informations.at(share->broadcast_index.at(0)).information->IF_join_nodes,AddProcessReceive2);
    if(method==9){

        for(size_t i=0,size=share->store.size;i<size;++i){
            if(share->store.store.at(i)==1){
                EventDataReceive ed=share->store.GS->events.at(i);
                ed.information_id=trust; // 攻撃者の利用している情報に書き換える
                
                store.GS->append(ed);
                LOGGER_DATA_PRINT(INFO,"add fake ed %lu : %f %lu %lu %d",store.GS->size-1,store.GS->events.at(store.GS->size-1).timestamp,store.GS->events.at(store.GS->size-1).from_id,store.GS->events.at(store.GS->size-1).to_id,store.GS->events.at(store.GS->size-1).information_id);
            store.GS->require_size=MAX(store.GS->require_size,share->informations.at(share->broadcast_index.at(0)).information->IF_join_nodes->size()+1);
            event_map.resize(store.GS->require_size);
            store.add(store.GS->size-1);
            AddProcessReceive2(share->informations.at(share->broadcast_index.at(0)).information->IF_join_nodes,&ed,&event_map);
            }
        }
        EventDataReceive ed; // 新しく追加する受信イベント
        ed.from_id=from_id;
        ed.timestamp=current_time;
        ed.to_id=to_id;
        ed.information_id=trust;
        
        ed.at=*user_position;
        ed.send_at=*share->user_position;
        store.GS->append(ed);
        //LOGGER_DATA_PRINT(INFO,"add ed %p %p %lu : %f %lu %lu",store.GS,&store.GS->events.at(store.GS->size-1),store.GS->size-1,store.GS->events.at(store.GS->size-1).timestamp,store.GS->events.at(store.GS->size-1).from_id,store.GS->events.at(store.GS->size-1).to_id);
        store.GS->require_size=MAX(store.GS->require_size,share->informations.at(share->broadcast_index.at(0)).information->IF_join_nodes->size()+1);
        event_map.resize(store.GS->require_size);
        store.add(store.GS->size-1);
    }

}

/**
 * @brief 方法の適用
 * @param received_information 受信した拡張情報
 * @param from_id 送信元ノードid
 * @param to_id　受信先ノードid
 * @param current_time 現在時刻(受信時刻)
 * @param from_position (送信元の位置)
 * @details
 * 情報を受信した場合の処理 method
 * 0:未定義
 * 1:新しく受信した情報で更新
 * 2:undefined
 * 3:最初に受信した情報でのみ更新
 * 4:受信した情報を信用する．偽装された情報があればこれを信用する．
 * 5:受信回数の多いものを信用する．
 * 6:蓄積
 */
void Method::adopt_method(ExtendInformation *received_information,int from_id,int to_id,double current_time,Point *from_position){
    FloatingInformation *info=received_information->information; // received information 
    ExtendInformation &extend=informations.at(info->id); // user extend information
    int &trust=trust_index.at(info->info); // 分類された情報グループ[info]におけるtrust:information_idを設定

    if(method==0){ // 偽装された情報を優先する
        if(trust<0){trust=info->id;}
        if(informations.at(trust).information->fake_id==0 && info->fake_id==1){ // fake_id 0 then Truth else fake information
            trust=info->id; // 偽の情報に更新(fake_id==1)
        }
    }else if(method==-1){ // TAが小さい情報を優先する
        if(trust<0){trust=info->id;}
        if(informations.at(trust).information->covered_road_length > informations.at(info->id).information->covered_road_length){
            trust=info->id;
        }
    }else if(method==-2){ // TAが大きい情報を優先する
        if(trust<0){trust=info->id;}
        if(informations.at(trust).information->covered_road_length < informations.at(info->id).information->covered_road_length){
            trust=info->id;
        }
    }else if(method==1){ // 初めて受け取った情報を信用し続ける．// no replace action
        if(extend.not_trust)return;
        if(trust<0){
            trust=info->id; // 未受信であれば最初に受信した情報を利用
        }
    }else if(method==2){ // 新しく受信した情報で更新
        if(extend.not_trust)return;
        trust=info->id;
    }else if(method==3){ // 受信回数が多い情報を利用する．
        if(extend.not_trust)return;
        if(trust<0){
            trust=info->id;
        }else if(trust!=info->id){
            if(informations.at(trust).receive_counter<extend.receive_counter){ // 受信回数の比較
                trust=info->id;
            }
        }
    }else if(method==32){ // 受信回数/TA被覆道路長が大きい情報を利用する．
        if(extend.not_trust)return;
        if(trust<0){
            trust=info->id;
        }else if(trust!=info->id){
            if(informations.at(trust).receive_counter/informations.at(trust).information->covered_road_length < extend.receive_counter/extend.information->covered_road_length){ // 受信回数の比較
                trust=info->id;
            }
        }
    }else if(40<=method && method<=49){ // 方法3,方法3.2の組み合わせ,SmallSide,k=method-40
        if(extend.not_trust)return; // 工夫3
        if(trust<0){
            trust=info->id;
        }else if(trust!=info->id){
            if(informations.at(trust).method7_req_receive<0){
                if(informations.at(trust).information->covered_road_length > informations.at(info->id).information->covered_road_length){
                    // L -> S
                    informations.at(trust).method7_req_receive=0;
                    informations.at(info->id).method7_req_receive=method-40;
                }else{
                    // S -> L
                    informations.at(trust).method7_req_receive=method-40;
                    informations.at(info->id).method7_req_receive=0;
                }
            }
            if(informations.at(trust).receive_counter < (unsigned int)informations.at(trust).method7_req_receive || extend.receive_counter < (unsigned int)extend.method7_req_receive){
                if(informations.at(trust).receive_counter < extend.receive_counter){ // 受信回数の比較
                    trust=info->id;
                }
            }else{
                if(informations.at(trust).receive_counter/informations.at(trust).information->covered_road_length < extend.receive_counter/extend.information->covered_road_length){ // 受信回数の比較
                    trust=info->id;
                }
            }
        }
        
    }else if(400<=method && method<=409){ // 方法3,方法3.2の組み合わせ,BothSide,k=method-40
        if(extend.not_trust)return; // 工夫3
        if(trust<0){
            trust=info->id;
        }else if(trust!=info->id){
            if(informations.at(trust).receive_counter<(unsigned int)method-400 || extend.receive_counter<(unsigned int)method-400){
                if(informations.at(trust).receive_counter<extend.receive_counter){ // 受信回数の比較
                    trust=info->id;
                }
            }else{
                if(informations.at(trust).receive_counter/informations.at(trust).information->covered_road_length<extend.receive_counter/extend.information->covered_road_length){ // 受信回数/道路長の比較
                    trust=info->id;
                }
            }
        }
    }else if(method==50 || method==51){ // 方法3,方法3.2の組み合わせ，最初に受信したかどうかをカウントし，小さい情報のカウントが多い場合に方法3.2へ，大きい方が多い場合，方法3へ
        if(extend.not_trust)return; // 工夫3
        if(received_information->first_trust){
            ++informations.at(received_information->information->id).first_trust_counter;
        }else{
            // あらかじめ加算する
            if(method==51)--informations.at(received_information->information->id).first_trust_counter;
        }
        if(trust<0){
            if(trust==-1)informations.at(info->id).first_trust=true;
            trust=info->id;
        }else if(trust!=info->id){
            bool trust_is_small = (informations.at(trust).information->covered_road_length < informations.at(info->id).information->covered_road_length);
            bool trust_is_more_count = informations.at(trust).first_trust_counter > informations.at(info->id).first_trust_counter;
            bool use_method3v2 = (trust_is_small==trust_is_more_count);
            if(use_method3v2){
                if(informations.at(trust).receive_counter/informations.at(trust).information->covered_road_length < extend.receive_counter/extend.information->covered_road_length){ // 受信回数/被覆長の比較
                    trust=info->id;
                }
            }else{
                if(informations.at(trust).receive_counter < extend.receive_counter){ // 受信回数の比較
                    trust=info->id;
                }
            }
        }

    }else if(method==500 || method==501){ // 最初に利用された情報かどうかをカウントし，多い方を利用
        if(extend.not_trust)return; // 工夫3
        if(received_information->first_trust){
            ++informations.at(received_information->information->id).first_trust_counter;
        }else{
            // あらかじめ加算する
            if(method==501)--informations.at(received_information->information->id).first_trust_counter;
        }
        if(trust<0){
            if(trust==-1)informations.at(info->id).first_trust=true;
            trust=info->id;
        }else if(trust!=info->id){
            if(informations.at(trust).first_trust_counter<informations.at(info->id).first_trust_counter){ // 受信回数の比較
                trust=info->id;
            }
        }

    }else if(60<=method && method<=69){ // 単純に大きさで選択する方法を組み合わせる(-1,-2)
        if(extend.not_trust)return; // 工夫3
        if(trust<0){
            trust=info->id;
        }else if(trust!=info->id){
            bool count_is_enough=(informations.at(trust).receive_counter>(unsigned int)(method-60) && extend.receive_counter>(unsigned int)(method-60));
            if(count_is_enough){
                if(informations.at(trust).receive_counter/informations.at(trust).information->covered_road_length<extend.receive_counter/extend.information->covered_road_length){ // 受信回数/道路長の比較
                    trust=info->id;
                }
            }else{
                if(informations.at(trust).receive_counter<extend.receive_counter){ // 受信回数の比較
                    trust=info->id;
                }
            }
        }

    }else if(method==9){ // 蓄積による方法　trustを未指定
        int c=3;
        for(int a : *received_information->information->IF_join_nodes){
            if(a==from_id){
                c=c-1;
                if(c==0)break;
            }
            if(a==to_id){
                c=c-2;
                if(c==0)break;
            }
        }
        if(c%2==1){received_information->information->IF_join_nodes->push_back(from_id);}
        if(c>1){received_information->information->IF_join_nodes->push_back(to_id);}

        if(store.GS==NULL){
            store.GS=received_information->information->shareGS;
        }
        if(trust<0){
            trust=info->id;
        }
        //informations.at(received_information->information->id).share(received_information,&event_map,current_time);

        /*
        EventDataReceive ed; // 新しく追加する受信イベント
        ed.from_id=from_id;
        ed.timestamp=current_time;
        ed.to_id=to_id;
        ed.information_id=info->id;
        
        ed.at=*user_position;
        ed.send_at=*from_position;
        store.GS->append(ed);
        //LOGGER_DATA_PRINT(INFO,"add ed %p %p %lu : %f %lu %lu",store.GS,&store.GS->events.at(store.GS->size-1),store.GS->size-1,store.GS->events.at(store.GS->size-1).timestamp,store.GS->events.at(store.GS->size-1).from_id,store.GS->events.at(store.GS->size-1).to_id);
        store.GS->require_size=MAX(store.GS->require_size,received_information->information->IF_join_nodes->size()+1);
        event_map.resize(store.GS->require_size);
        store.add(store.GS->size-1);
        AddProcessReceive2(received_information->information->IF_join_nodes,&ed,&event_map);
        */
        

    }else if(method==999){ // 蓄積による方法　trustを未指定
        if(trust<0){
            trust=info->id;
        }
        if(received_information->information!=NULL){
            // road. 道の情報の共有．
            informations.at(received_information->information->id).roads.update(&received_information->roads,&event_road,AddProcessRoad);
        
            // share event. 受信イベントの共有
            informations.at(received_information->information->id).share(received_information,&event_map,current_time);
            
            /*
            // new event add receiver
            EventDataReceive ed; // 新しく追加する受信イベント
            ed.from_id=from_id;
            ed.timestamp=current_time;
            ed.to_id=to_id;
            ed.information_id=info->id;
            
            ed.at=*user_position;
            ed.send_at=*from_position;
            informations.at(info->id).store.GS->append(ed);
            informations.at(info->id).store.GS->require_size=MAX(informations.at(info->id).store.GS->require_size,(size_t)MAX(from_id,to_id)+1);
            event_map.resize(informations.at(info->id).store.GS->require_size);
            informations.at(info->id).store.add(informations.at(info->id).store.GS->size-1);
            AddProcessReceive(&ed,&event_map);
            */
        }
    
    }
}

/**
 * @brief replyの交換
 * @param receive_information 受信した拡張された情報，replyを含む
 */
void Method::share_reply(ExtendInformation *receive_information){
    ExtendInformation *ex = &informations.at(receive_information->information->id);
    bool before_state = ex->reply_summary.latest_isFake;
    size_t before_store_size = ex->replies.size;
    //printf("Store size shared store size: %lu isFake:%d\n",receive_information->replies.size,receive_information->reply_summary.latest_isFake);
    ex->replies.update(&receive_information->replies,&ex->reply_summary,AddProcessReply);
    if(0){
        printf("Add Reply Share store size: %lu -> %lu\n",before_store_size, ex->replies.size);
        bool after_state = ex->reply_summary.latest_isFake;
        printf("Share RESULT bf:%d or rv:%d -> af:%d",before_state,receive_information->reply_summary.latest_isFake,after_state);
        if(receive_information->reply_summary.latest_isFake!=after_state && after_state==false){
            printf("  FAIL cant update replySummary");
        }
        if(before_state==false && after_state==true && receive_information->reply_summary.latest_isFake==false){
            printf("  UNEXPECTED FAIL no reply but get True");
        }
        printf("\n");
    }
    if(ex->reply_summary.latest_isFake){
        fake_index.insert(ex->information->id);
    }else{
        fake_index.erase(ex->information->id);
    }
}

/**
 * @brief 情報の受信
 * @param from_id 送信元ノードid
 * @param to_id 受信ノードid
 * @param share 受信した情報のまとまり
 * @param current_time 現在時刻(受信時刻)
 * @param update_trust 受信した情報に基づく，更新を行うかどうか．攻撃者なら0
 */
void Method::receive_information(int from_id,int to_id,Method *share,double current_time,int update_trust){
    LOGGER_DATA_PRINT(DEBUG,"send %d->%d info size:%ld",from_id,to_id,share->broadcast_index.size());
    //FloatingInformation *info=share->trust; // 送信元が信用している情報
    std::vector<int> &received_floating_information_index=share->broadcast_index; // 送信元が配信している情報
    if(connected_ids.find(from_id)==connected_ids.end()){ // 新しいノードからの受信である場合
        connected_ids.insert(from_id);
        
        receive_time=current_time;
        
        for(int extend_index : received_floating_information_index){
            ExtendInformation *received_extend=&share->informations.at(extend_index);
            ++received_extend->information->receive_counter; // 情報のグローバル受信回数を加算
            if(received_extend->information->first_receive_time==0.0)received_extend->information->first_receive_time=current_time;
            received_extend->information->last_receive_time=current_time;
            set_information(received_extend->information,current_time);

            // 1105
            if(received_extend->information->check_inTA(user_position->x,user_position->y)){
                informations.at(received_extend->information->id).receive_in_original_TA+=1;
            }
            // 受信を記録
            if(0){
                //fprintf(stdout,"%d,%d,%f,%u,%f,%f,%f,%f\n",from_id,to_id,current_time,informations.at(received_extend->information->id).receive_counter,user_position->x,user_position->y,share->user_position->x,share->user_position->y);
            }
            if(received_extend->replies.size>0 && 0){
                printf("Connect with replies size from:%d to:%d\n",from_id,to_id);
                int num_of_reps = 0;
                for(size_t index=0;index<received_extend->replies.size;++index){
                    if(received_extend->replies.store.at(index)==1){
                        EventReply *e=&received_extend->replies.GS->events.at(index);
                        printf("  Reply id:%d t:%.0f f:%d p:%p\n",e->node_id,e->timestamp,e->isFake,(void*)e);
                        num_of_reps+=1;
                    }
                }
                printf("Num of Replies FROM : %d\n",num_of_reps);
            }

            if(share_replys)share_reply(received_extend);
            if(update_trust>0){
                adopt_method(received_extend,from_id,to_id,current_time,share->user_position);
                if(trust_index.at(received_extend->information->info)==received_extend->information->id){
                    auto rep = received_extend->information->trust_id_set.insert(to_id);
                    if(rep.second){
                        LOGGER_DATA_PRINT(DEBUG,"node %d trust info id:%d set size:%lu",to_id,received_extend->information->id,received_extend->information->trust_id_set.size());
                    }
                }
            }
            if(method==9 || method==999){
                
                if(store.GS==NULL){
                    store.GS=received_extend->information->shareGS;
                }
                if(update_trust>0){
                    store.share2(&share->store,&event_map,received_extend->information->IF_join_nodes,AddProcessReceive2);
                    //informations.at(received_information->information->id).share(received_information,&event_map,current_time);

                    EventDataReceive ed; // 新しく追加する受信イベント
                    ed.from_id=from_id;
                    ed.timestamp=current_time;
                    ed.to_id=to_id;
                    ed.information_id=received_extend->information->id;

                    ed.at=*user_position;
                    ed.send_at=*share->user_position;
                    store.GS->append(ed);
                    //LOGGER_DATA_PRINT(INFO,"add ed %p %p %lu : %f %lu %lu",store.GS,&store.GS->events.at(store.GS->size-1),store.GS->size-1,store.GS->events.at(store.GS->size-1).timestamp,store.GS->events.at(store.GS->size-1).from_id,store.GS->events.at(store.GS->size-1).to_id);
                    store.GS->require_size=MAX(store.GS->require_size,received_extend->information->IF_join_nodes->size()+1);
                    event_map.resize(store.GS->require_size);
                    store.add(store.GS->size-1);
                    AddProcessReceive2(received_extend->information->IF_join_nodes,&ed,&event_map);

                    int cnt_0=0;
                    int cnt_1=0;
                    for(EventMapUnit em : event_map){
                        if(em.send_trust_count.at(0)>0 && em.send_trust_count.at(1)==0){
                            ++cnt_0;
                        }else if(em.send_trust_count.at(0)==0 && em.send_trust_count.at(1)>0){
                            ++cnt_1;
                        }
                    }
                    
                    if(trust_index.at(received_extend->information->info)!=1 && cnt_1>cnt_0){
                        trust_index.at(received_extend->information->info)=1;
                    }else if(trust_index.at(received_extend->information->info)!=0 && cnt_1<cnt_0){
                        trust_index.at(received_extend->information->info)=0;
                    }
                    
                    
                    //method9：逆ロジック,エラー
                    /*
                    if(informations.at(0).receive_counter>0 && informations.at(1).receive_counter>0){
                        if(trust_index.at(received_extend->information->info)!=0 && cnt_1>cnt_0){
                            trust_index.at(received_extend->information->info)=0;
                        }else if(trust_index.at(received_extend->information->info)!=1 && cnt_1<cnt_0){
                            trust_index.at(received_extend->information->info)=1;
                        }
                    }
                    */
                    

                    if(trust_index.at(received_extend->information->info)==received_extend->information->id){
                        auto rep = received_extend->information->trust_id_set.insert(to_id);
                        if(rep.second){
                            LOGGER_DATA_PRINT(DEBUG,"node %d trust info id:%d set size:%lu",to_id,received_extend->information->id,received_extend->information->trust_id_set.size());
                        }
                    }
                    
                }
            }
        }
    }
}

/**
 * @brief 配信する情報の設定
 * @param x ノードのx座標
 * @param y ノードのy座標
 * @param consider_all_option 衝突するすべての情報を利用しているTAとするかどうか(1:true)
 */
bool Method::select_broadcast_information(double x,double y,int consider_all_option){
    broadcast_index.clear();
    for(int trust_information_index : trust_index){
        if(trust_information_index<0)continue;
        ExtendInformation *trust=&informations.at(trust_information_index);
        if(consider_all_option==0){// trustのTAについてノードがTAないかどうか判定
            if(trust->information->check_inTA(x,y)){
                broadcast_index.push_back(trust_information_index);
            }
        }else if(consider_all_option==1){// 齟齬のある情報全てのTAについて評価して，ひとつでもTA内にあれば，trustを配信できる
            //printf("w%p\n",trust);
            //printf("e%p\n",trust->information);
            //printf("r%p\n",trust->information->info);
            //printf("\nx%d\n",trust->information->info);
            for(int information_index : classification.at(trust->information->info)){
                if(information_index>=0 && informations.at(information_index).information->check_inTA(x,y)){
                    broadcast_index.push_back(trust_information_index);
                    break;
                }
            }
        }else if(consider_all_option==2){ // 齟齬のある情報全てのTAについて評価して，ひとつでもTA内にあり，trustのTAで受信したことがあれば，trustを配信できる
            if(trust->receive_in_original_TA==0)continue; // trustTAで受信していない
            for(int information_index : classification.at(trust->information->info)){
                if(information_index>=0 && informations.at(information_index).information->check_inTA(x,y)){
                    broadcast_index.push_back(trust_information_index);
                    break;
                }
            }
        }
    }
    for(int is_fake_index : fake_index){
        ExtendInformation *trust=&informations.at(is_fake_index);
        if(consider_all_option==0){// trustのTAについてノードがTAないかどうか判定
            if(trust->information->check_inTA(x,y)){
                broadcast_index.push_back(is_fake_index);
            }// TODO
        }else{// 齟齬のある情報全てのTAについて評価して，ひとつでもTA内にあれば，trustを配信できる
            for(int information_index : classification.at(trust->information->info)){
                if(information_index>=0 && informations.at(information_index).information->check_inTA(x,y)){
                    broadcast_index.push_back(is_fake_index);
                    break;
                }
            }
        }
    }
    return (broadcast_index.size()>0);
}

/**
 * @brief 配信の記録
 * @param current_time 現在時刻(配信時刻)
 */
void Method::record_broadcast(int from_id,double current_time){
    for(int extend_index : broadcast_index){
        ExtendInformation *extend=&informations.at(extend_index);
        if(extend->information->first_broadcast_time==0.0){extend->information->first_broadcast_time=current_time;} // 最初の配信時刻
        extend->information->last_broadcast_time=current_time; // 最終配信時刻
        ++extend->information->broadcast_counter; // 配信回数
        auto rep = extend->information->broadcast_id_set.insert(from_id);
        if(rep.second){
            LOGGER_DATA_PRINT(DEBUG,"node %d broadcast info id:%d set size:%lu",from_id,extend->information->id,extend->information->broadcast_id_set.size());
        }   
    }
}

#endif