#ifndef NODE_HPP_
#define NODE_HPP_
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <random>
#include "map_enterable.hpp"
#include "movement.hpp"
#include "floatingInformation.hpp"
#include "parameters.hpp"
#include "method3.hpp"

class Node{
private:
public:
    enum NODE_TYPE{
        NORMAL,
        MALICIOUS,
        FIXED_SOURCE,
    };

    int id;
    NODE_TYPE type; // NORMAL or MALICIOUS
    double velocity; // [m/s]
    double step; // [m] velocity*Delta_T
    int status; // ノードの状態段階を保存，攻撃者が停止する場合に利用．
    Movement mv;
    
    MapEnterable *map;
    Edge *route;
    EdgeDirection enter_direction;
    size_t table_id; // map table id  to select nodes route 0:L|D,1:D|R,2:R|U,3:U|L

    Method md;

    bool sendable;
    
    Node();
    Node(int node_id,int method_type,NODE_TYPE node_type,double stay_point_x,double stay_point_y); // for FS
    Node(int node_id,int method_type,Point start,Point goal,double remain_second,double node_velocity,double rnd_val,double turn_left); // for first Information carry node and MALICIOUS node
    Node(int node_id,int method_type,Edge *enter_point,MapEnterable *map,double remain_second,double node_velocity,std::mt19937& rng, std::uniform_real_distribution<>& dist,double turn_left); // for normal Node
    ~Node() noexcept = default;
    Node(const Node& other) = default;
    Node(Node&& other) noexcept = default;
    Node & operator=(const Node& other) = default;
    void check_position();
    void update_goal(std::mt19937& rng, std::uniform_real_distribution<>& dist);
    void update(std::mt19937& rng, std::uniform_real_distribution<>& dist);
    void connection(Checker *checker,Node *node,double current_time);
    void describe();
};

/* 空の初期化 */
Node::Node(){
    id=0;
    type=NORMAL;
    velocity=1.0;
    map=nullptr;
    sendable=false;
    route=nullptr;
    enter_direction=LEFT;
    table_id=(size_t)enter_direction;
}

/* 定位置で静止する初期化 for FS */
Node::Node(int node_id,int method_type,NODE_TYPE node_type,double stay_point_x,double stay_point_y){
    id=node_id;
    type=node_type;
    status=0;
    mv.type=Movement::STAY;
    mv.position.x=stay_point_x;
    mv.position.y=stay_point_y;
    route=nullptr;
    sendable=false;
    md.set(method_type);
}

/* 移動を伴う場合の初期化 Pointにより始点，目的地を指定 for first information carry node and MALICIOUS node */
Node::Node(int node_id,int method_type,Point start,Point goal,double remain_second,double node_velocity,double rnd_val,double turn_left){
    id=node_id;
    type=NORMAL;
    velocity=node_velocity;
    step=velocity*Dt;
    status=0;
    map=nullptr;
    enter_direction=LEFT;
    route=nullptr;
    sendable=false;
    mv.set_movement(start,goal,step,remain_second*velocity);
    table_id=(RND_PROB(turn_left,rnd_val)) ? enter_direction:(enter_direction+3)%4;
    md.set(method_type);
}

/* 移動を伴う場合の初期化 Edgeにより始点，目的地を指定 for normal node */
Node::Node(int node_id,int method_type,Edge *enter_edge,MapEnterable *global_map,double remain_second,double node_velocity,std::mt19937& rng, std::uniform_real_distribution<>& dist,double turn_left){
    id=node_id;
    type=NORMAL;
    velocity=node_velocity;
    step=velocity*Dt;
    status=0;
    map=global_map;
    enter_direction=global_map->enterable_points.at(enter_edge->from->id).direction;
    route=enter_edge;
    ++route->counter;
    sendable=false;
    mv.set_movement(enter_edge->from->point,enter_edge->to->point,step,remain_second*velocity);
    table_id=(RND_PROB(turn_left,dist(rng))) ? enter_direction:(enter_direction+3)%4; // Left -> 0:L|D or 3:U|L
    //LOGGER_DATA_PRINT(INFO,"call rng at %s %d",__FILE__,__LINE__);
    md.set(method_type);
    update_goal(rng,dist);
}

/**ノードが現在位置で配信できる情報の選択 */
void Node::check_position(){
    if(type==NORMAL){
        md.update_trust_information();
        if(md.trust_index.size()>0){
            sendable=md.select_broadcast_information(mv.position.x,mv.position.y,consider_all_TA);
        }else{
            sendable=false;
        }
    }else if(type==MALICIOUS){
        if(md.receive_time>0){
            sendable=md.select_broadcast_information(mv.position.x,mv.position.y,1);
        }
    }else if(type==FIXED_SOURCE){
        sendable=true;
    }
}

void Node::update_goal(std::mt19937& rng, std::uniform_real_distribution<>& dist){
    LOGGER_DATA_PRINT(DEBUG,"node %d update_goal remain %.2f",id,mv.remain);
    while(mv.arrive && mv.type==Movement::ACTIVE){
        if(map!=nullptr){
            if(map->map.at(route->to->id).size()==4){ // 4方向に移動できる格子点である場合
                size_t table=md.get_trust_state()*4+table_id;// base_table_index*4 + table_id:shift_table
                route=&map->map.at(route->to->id).at(route->to->routing.get_next_direction(table,route->direction,rng,dist));
                ++route->counter;
                mv.set_new_goal(route->to->point,step);
                LOGGER_DATA_PRINT(DEBUG,"node %d update_goal set_new_goal remain %.2f",id,mv.remain);
            }else{ // そうでなければ，行き止まりで停止
                mv.type=Movement::STAY;
            }
        }else{
            mv.type=Movement::STAY;
        }
    }
}

void Node::update(std::mt19937& rng, std::uniform_real_distribution<>& dist){
    mv.update();
    while(mv.arrive && mv.type==Movement::ACTIVE){
        if(map!=nullptr){
            if(map->map.at(route->to->id).size()==4){ // 4方向に移動できる格子点である場合
                size_t table=md.get_trust_state()*4+table_id;
                EdgeDirection pre_direction=route->direction;
                //LOGGER_DATA_PRINT(INFO,"node %d %d (%f,%f) update",id,type,mv.position.x,mv.position.y);
                route=&map->map.at(route->to->id).at(route->to->routing.get_next_direction(table,route->direction,rng,dist));
                ++route->counter;
                LOGGER_DATA_PRINT(DEBUG,"next direction Node %d  direction update %d -> %d  sub:%d",id,pre_direction,route->direction,(route->direction-pre_direction+4)%4);
                mv.set_new_goal(route->to->point,step);
            }else{ // そうでなければ，行き止まりで停止
                mv.type=Movement::STAY;
            }
        }else{
            mv.type=Movement::STAY;
        }
    }
}

void Node::connection(Checker *checker,Node *node,double current_time){
    checker->total_connection+=1;
    if(type==NORMAL){
        md.receive_information(node->id,id,&node->md,current_time,1);
    }else{
        if(node->md.broadcast_index.at(0)==0){ // 元のTA情報を受信した場合
            if(md.receive_time==0.0){ // 初めて受信した場合
                md.receive_information(node->id,id,&node->md,current_time,0);
                md.receive_time=current_time;
                md.malicious_disguise(node->id,id,&node->md,current_time); // 受信内容に基づく，偽装工作の更新 TODO 1025
            }
        }
    }
}

void Node::describe(){
    printf("id:%d  pos:%.2f,%.2f  unit:%.2f,%.2f  table:%ld\n",id,mv.position.x,mv.position.y,mv.unit.x,mv.unit.y,table_id);
}

#endif