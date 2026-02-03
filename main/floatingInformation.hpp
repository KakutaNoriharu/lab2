#ifndef FLOATING_INFORMATION_HPP_
#define FLOATING_INFORMATION_HPP_

#include <vector>
#include "point.h"
#include "store.hpp"
#include "../utility/logger.h"
#include "../utility/utility.h"
#include "area.hpp"

/**
 * todo : 2025-05-01 蓄積データに基づいた配信が可能になるような調整
 * TA,RAについて，簡単に調整できるように
 * 準備する
 */

/**
 * @brief 受信イベントデータ
 */
typedef struct EventDataReceive{
    size_t from_id; // id required non negative value (id>=0)
    size_t to_id; // id required non negative value (id>=0)
    Point at; // receive event occurred position
    int information_id; // received information id
    double timestamp; // received time
    Point send_at; // sender sended position
}EventDataReceive;

/**
 * @brief 利用情報切り替えイベント */
typedef struct EventDataChangeTrust{
    size_t node_id; //! id required non negative value
    int pre_trust_information_id; // information id
    int new_trust_information_id; // information id
    double timestamp;
}EventDataChangeTrust;

/**
 * @brief 道路区間通過イベント */
typedef struct EventTroughRoad{
    size_t road_id; // 通過したroadのid
    size_t node_id; // roadを通過したnodeのid
    double timestamp; // イベント時刻
}EventTroughRoad;

/** TODO: add 07-03
 * @brief 配信されている情報へのリプライの構造体 */
typedef struct EventReply{
    int node_id; // replied node id
    bool isFake; // if True information is fake else original information
    double timestamp; // replied timestamp
}EventReply;

/** TODO: add 07-03
 * @brief 保有するリプライ情報の総括 */
typedef struct EventMapReply{
    size_t num; // the number of reply
    bool latest_isFake; // latest reply content;
    double latest_timestamp; // latest reply timestamp
    EventMapReply() : num(0),latest_isFake(false),latest_timestamp(-1.0) {}
}EventMapReply;

/**
 * @brief ノードの通信イベント情報 */
typedef struct EventMapUnit{
    size_t user_id; // ノードのid
    int trust; // 信用情報id
    size_t user_itr; // このノードが保存されている通し番号
    std::vector<int> send_trust_count; // ノードが送った情報のカウンタ
    std::vector<double> receive_times; // ノードが受信した時刻リスト
    std::vector<Point> connection_points; // ノードが通信した位置リスト(古いものから)
    std::vector<Point> sender_points; // 通信時の送信元の位置リスト(古いものから)
    std::set<size_t> send_to; // このノードが送信した宛先ノードのidリスト
    std::set<size_t> send_to_itr; // このノードが送信した宛先のノードの通し番号
    std::set<size_t> receive_from; // このノードが受信したノードのidリスト
    std::set<size_t> receive_from_itr; // このノードが受信したノードの通し番号リスト(receive_fromとは順不同)

    EventMapUnit() : user_id(0),trust(-1),user_itr(0)/*,timestamp(0.0)*/ {
        send_trust_count.resize(2,0);
    }
}EventMapUnit;

/**
 * @brief 道路単位でのイベント記録 */
typedef struct EventMapRoadUnit{
    std::set<size_t> user_id_set; // 道路を通過したノードのidリスト
    double timestamp_min; // 最初に通過した時刻
    double timestamp_max; // 最後に通過した時刻
    EventMapRoadUnit() : timestamp_min(-1.0),timestamp_max(-1.0) {}
}EventMapRoadUnit;

/**
 * @brief IFで配信される情報
 */
class FloatingInformation{
private:
public:
    int id; // information identifier
    int fake_id; // if 0 true infomation, or more fake information
    int info; // information, if same each other, one is fake

    std::vector<Area> TA; // TA information
    size_t TA_size; // size of TA vector
    std::vector<Area> RA; // Restricted Area (start,end,form:rect) node should not to enter
    size_t RA_size; // size of RA vector

    double TA_area_size; // area of TA [m^2]

    GlobalStore<EventDataReceive> GS; // 受信データの蓄積
    GlobalStore<EventTroughRoad> GSr; // 道路通過データの蓄積

    // todo:add 1211
    GlobalStore<EventDataReceive> *shareGS; // 受信データの蓄積
    std::vector<int> *IF_join_nodes;

    double first_broadcast_time;
    double last_broadcast_time;
    double first_receive_time;
    double last_receive_time;
    long int broadcast_counter;
    long int receive_counter;
    int trust_node_counter;
    int max_trust_node_counter;
    std::set<int> trust_id_set;
    std::set<int> broadcast_id_set;

    // TODO: add 0703
    GlobalStore<EventReply> replies;

    // TODO: add 1106
    double covered_road_length;

    FloatingInformation();
    FloatingInformation(int information_id,int information,int sub_information_id);
    FloatingInformation(int information_id,int information,int sub_information_id,double center_x,double center_y,double TA_scale_x,double TA_scale_y);
    ~FloatingInformation() noexcept = default;
    FloatingInformation(const FloatingInformation& other) = default;
    FloatingInformation(FloatingInformation&& other) noexcept = default;
    FloatingInformation & operator=(const FloatingInformation& other) = default;

    void initialize();
    void set_information(int infomation_id,int sub_information_id,int information);
    void add_TA_range(double center_x,double center_y,double TA_scale_x,double TA_scale_y);
    void add_TA_square(double start_x,double start_y,double end_x,double end_y);
    void add_RA_range(double center_x,double center_y,double TA_scale_x,double TA_scale_y);
    void add_RA_square(double start_x,double start_y,double end_x,double end_y);
    bool check_inTA(double x,double y);
    bool check_inRA(double x,double y);
    void describe();
};

FloatingInformation::FloatingInformation(){
    initialize();
}

FloatingInformation::FloatingInformation(int information_id,int information,int sub_information_id){
    initialize();
    set_information(information_id,sub_information_id,information);
}

FloatingInformation::FloatingInformation(int information_id,int information,int sub_information_id,double center_x,double center_y,double TA_scale_x,double TA_scale_y){
    initialize();
    set_information(information_id,sub_information_id,information);
    add_TA_range(center_x,center_y,TA_scale_x,TA_scale_y);
}

void FloatingInformation::initialize(){
    id=-1;
    fake_id=-1;
    info=-1;
    TA_size=0;
    RA_size=0;
    TA.clear();
    RA.clear();

    first_broadcast_time=0;
    last_broadcast_time=0;
    first_receive_time=0;
    last_receive_time=0;
    broadcast_counter=0;
    receive_counter=0;
    trust_node_counter=0;
    max_trust_node_counter=0;

    // 1106
    covered_road_length=0.0;
}

void FloatingInformation::set_information(int information_id,int sub_information_id,int information){
    id=information_id;
    fake_id=sub_information_id;
    info=information;
}

void FloatingInformation::add_TA_range(double center_x,double center_y,double TA_scale_x,double TA_scale_y){
    Area ta=createArea(center_x,center_y,TA_scale_x,TA_scale_y);
    TA.push_back(ta);
    TA_size=TA.size();
    TA_area_size=calculate_union_area_discretized(TA);
}

void FloatingInformation::add_TA_square(double start_x,double start_y,double end_x,double end_y){
    Area ta=createAreaByCorners(start_x,start_y,end_x,end_y);
    TA.push_back(ta);
    TA_size=TA.size();
    TA_area_size=calculate_union_area_discretized(TA);
}

void FloatingInformation::add_RA_range(double center_x,double center_y,double RA_scale_x,double RA_scale_y){
    Area ra=createArea(center_x,center_y,RA_scale_x,RA_scale_y);
    RA.push_back(ra);
    RA_size=RA.size();
}

void FloatingInformation::add_RA_square(double start_x,double start_y,double end_x,double end_y){
    Area ta=createAreaByCorners(start_x,start_y,end_x,end_y);
    RA.push_back(ta);
    RA_size=RA.size();
}

bool FloatingInformation::check_inTA(double x,double y){
    for(unsigned int i=0;i<TA_size;++i){
        Area &ta = TA.at(i);
        if(onArea(ta,x,y)) return true;
    }
    return false;
}

bool FloatingInformation::check_inRA(double x,double y){
    for(unsigned int i=0;i<RA_size;++i){
        Area &ta = RA.at(i);
        if(onArea(ta,x,y)) return true;
    }
    return false;
}

void FloatingInformation::describe(){
    printf("Floating information:%d  info:%d(%d)  TA size:%lu\n",id,info,fake_id,TA_size);
}

#endif