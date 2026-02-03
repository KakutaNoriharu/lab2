#ifndef EDGE_HPP_
#define EDGE_HPP_
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <vector>
#include <random>

#include "point.h"
#include "../utility/logger.h"

/**
 * エッジ，辺を扱う構造体
 * 始点と終点により定義される．
 * 一方向のみ扱える．双方向に移動する場合，別に始点と終点を入れ替えたエッジを宣言する必要がある．
 * 格子状道路を前提としている，全てのエッジは上下左右方向のみにつながっていると考えている． */

/* エッジの始点から終点への向き */
typedef enum EdgeDirection{
    LEFT,
    DOWN,
    RIGHT,
    UP,
    NONE,
    NUM_OF_EDGE_DIRECTION
}EdgeDirection;

typedef enum RoutingType{
    NORMAL, // two direction can use types from(L,D,R,U)E tables:[L,D],[D,R],[R,U],[U,R] = [1-p_turn,p_turn,0,0],[0,1-p_turn,p_turn,0]... ex:[0.5,0.5,0,0],[0,0.5,0.5,0]
    UNIFORM, // node can select route random [L,R,U,D] tables[0] = [0.25,0.25,0.25,0.25] x4
    TURN, // node must turn on point from(L,D,R,U)E L:[0.0,0.5,0.0,0.5] U:[0.5,0,0.5,0] ...
    NO_U_TURN, // node can select route without U turn from(L,D,R,U)E [L]:[0.33,0.33,0.0,0.33],[D]:[0.33,0.33,0.33,0.0]... 
    RANDOM, // any:[a,b,c,1-a-b-c]
    RANDOM_WO_U_TURN, // enter:L [0,a,b,1-a-b]  
    RANDOM_TURN, // NORMAL p_turn is rundom
    RANDOM_TURN_LOAD, // LOAD 
}RoutingType;

/**
 * Routing Table
 * table [enter direction][table id][routing probabilities]
 * if routing probabilities is negative value the route is banned
 * if all route banned route is selected randomly
 */
class EdgeRouting{
private:
public:
    RoutingType routing_type;
    std::vector<std::vector<std::vector<double>>> table; // [enter_direction(l,d,r,u)][table_id(所持情報に依存するid)][tables(l,d,r,u:進行方向の確率)]
    EdgeRouting() = default;
    EdgeRouting(int table_size,int edge_size);
    ~EdgeRouting() noexcept = default;
    EdgeRouting(const EdgeRouting& other) = default;
    EdgeRouting(EdgeRouting&& other) noexcept = default;
    EdgeRouting & operator=(const EdgeRouting& other) = default;
    void initialize(int table_size,int edge_size);
    void copy_table();
    EdgeDirection get_next_direction(size_t table_id,EdgeDirection enter_direction,std::mt19937& rng,std::uniform_real_distribution<>& dist);
};

EdgeRouting::EdgeRouting(int table_size,int edge_size){
    initialize(table_size,edge_size);
}

void EdgeRouting::initialize(int table_size,int edge_size){
    table.resize(edge_size);// 進入経路数
    for(int i=0;i<edge_size;++i){ 
        table.at(i).resize(table_size); // テーブルの数
        for(int j=0;j<table_size;++j){
            table.at(i).at(j).resize(edge_size,0); // 退出経路数
        }
    }
}
/**
 * @brief tableを2倍に複製拡張する
 * @details table_sizeを複製し，2倍にする．表現可能なtableを倍に増やす
 */
void EdgeRouting::copy_table(){
    size_t initial_table_size=table.at(0).size();
    for(size_t enter_edge=0,enter_edge_size=table.size();enter_edge<enter_edge_size;++enter_edge){
        size_t base_table_size=table.at(enter_edge).size();
        table.at(enter_edge).resize(table.at(enter_edge).size()*2);
        for(size_t i=initial_table_size,size=table.at(enter_edge).size();i<size;++i){
            table.at(enter_edge).at(i)=table.at(enter_edge).at(i%base_table_size);
        }
    }
}

EdgeDirection EdgeRouting::get_next_direction(size_t table_id,EdgeDirection enter_direction,std::mt19937& rng,std::uniform_real_distribution<>& dist){
    std::vector<int> selectable_directions;
    double value=dist(rng); // 経路選択確率
    //LOGGER_DATA_PRINT(INFO,"call rng at %s %d",__FILE__,__LINE__);
    double cum_sum=0; // 選択確率の累積和
    int banned=0;
    for(int i=0;i<4;++i){
        if(table.at(enter_direction).at(table_id).at(i)<0.0){
            ++banned;
        }else{
            selectable_directions.push_back(i);
            cum_sum+=table.at(enter_direction).at(table_id).at(i); //経路選択テーブル和が1になる
        }
        if(cum_sum>=value){
            LOGGER_DATA_PRINT(DEBUG,"get next direction i:%d cum_sum:%.2f(last+%.2f) value:%.2f",i,cum_sum,table.at(enter_direction).at(table_id).at(i),value);
            return static_cast<EdgeDirection>(i);
        }
    }
    if(banned==4){
        if(1){
            return enter_direction; // とりあえず直進
        }else{
            int random_route=(int)(value/0.25);
            LOGGER_DATA_PRINT(DEBUG,"get_next_direction random call  %d",random_route);
            return static_cast<EdgeDirection>(random_route); // ランダムに選択
        }
    }else{ // 選択できる経路からランダムに決定
        LOGGER_DATA_PRINT(DEBUG,"select random route : selectable route:%ld",selectable_directions.size());
        return static_cast<EdgeDirection>(selectable_directions.at((value>=1.0) ? selectable_directions.size()-1:static_cast<int>((double)(selectable_directions.size())*value)));
    }
    LOGGER_PRINT(WARNING,"no match next direction");
    return EdgeDirection::LEFT;
}

class EdgePoint{
private:
public:
    int id; // -1:unset other:setted  from 0 for array access
    Point point; // position
    int connection_counter; // Number of connected edge
    std::vector<EdgePoint *> connection;
    EdgeRouting routing;
    EdgePoint();
    EdgePoint(int point_id,double x,double y);
    ~EdgePoint() noexcept = default;
    EdgePoint(const EdgePoint& other) = default;
    EdgePoint(EdgePoint&& other) noexcept = default;
    EdgePoint & operator=(const EdgePoint& other) = default;
    void describe();
};

EdgePoint::EdgePoint(){
    id=-1;
    point=new_point(0.0,0.0);
    connection_counter=0;
    connection.resize(EdgeDirection::NUM_OF_EDGE_DIRECTION,NULL);
}

EdgePoint::EdgePoint(int point_id,double x,double y){
    id=point_id;
    point=new_point(x,y);
    connection_counter=0;
    connection.resize(EdgeDirection::NUM_OF_EDGE_DIRECTION,NULL);
}

void EdgePoint::describe(){
    printf("Point:%d  %.2f / %.2f\n",id,point.x,point.y);
}

/* 始点と終点を扱う辺 */
class Edge{
public:
    int id; // -1:unset other:setted edge  from 0 for array access
    EdgePoint *from,*to; // star,end position
    EdgeDirection direction;
    double distance; // length of edge
    long int counter; // ノードが移動した総回数．Edgeが選択された場合に加算．
    Edge();
    Edge(int edge_id,EdgePoint *start,EdgePoint *goal);
    Edge(int edge_id,EdgePoint *start,EdgePoint *goal,EdgeDirection direction_setting);
    ~Edge() noexcept = default;
    Edge(const Edge& other) = default;
    Edge(Edge&& other) noexcept = default;
    Edge & operator=(const Edge& other) = default;
    void describe();
};

Edge::Edge(){
    id=-1;
    counter=0;
}

Edge::Edge(int edge_id,EdgePoint *start,EdgePoint *goal){
    id=edge_id;
    counter=0;
    from=start;
    to=goal;
    distance=cal_point_distance(start->point,goal->point);
    double angle=cal_point_angle(start->point,goal->point)+1.0;
    if(angle>1.75){
        direction=RIGHT;
    }else if(angle>1.25){
        direction=UP;
    }else if(angle>0.75){
        direction=LEFT;
    }else if(angle>0.25){
        direction=DOWN;
    }else{
        direction=RIGHT;
    }
    if(start->connection.at(direction)==NULL){
        start->connection.at(direction)=goal;
        ++start->connection_counter;
    }
}

Edge::Edge(int edge_id,EdgePoint *start,EdgePoint *goal,EdgeDirection direction_setting){
    id=edge_id;
    counter=0;
    from=start;
    to=goal;
    distance=cal_point_distance(start->point,goal->point);
    direction=direction_setting;
        if(start->connection.at(direction)==NULL){
        start->connection.at(direction)=goal;
        ++start->connection_counter;
    }
}

void Edge::describe(){
    double re_cal_dis = cal_point_distance(from->point,to->point);
    printf("Edge:%d  %d -> %d distance:%.2f  %.2f/%.2f -> %.2f/%.2f\n",id,from->id,to->id,distance,from->point.x,from->point.y,to->point.x,to->point.y);
    printf("re %.2f  %.2f  if not equal check code\n",re_cal_dis,distance);
    from->describe();
    to->describe();
}

#endif