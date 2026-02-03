#ifndef MAP_
#define MAP_
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <random>
#include "../utility/utility.h"
#include "edge.hpp"

/** NOTIFICATION
 * ポインターによって格子点を指定しているため，vectorのresizeによる更新が絡むと壊れる可能性がある
 * 対策として，予めresize,reserveすることで，メモリ割り当ての更新がないようにする必要がある．
*/

class Map{
private:
    int point_id_;
    int edge_id_;
public:
    std::vector<std::vector<Edge>> map; // 格子状道路
    std::vector<EdgePoint> points; // 格子点
    int grid_num_x;
    int grid_num_y;
    double scale_x;
    double scale_y;
    double p_turn;
    
    Map() = default;
    Map(int x_grid_num,int y_grid_num,double x_distance,double y_distance,double x_shift,double y_shift);
    ~Map() noexcept = default;
    Map(const Map& other) = default;
    Map(Map&& other) noexcept = default;
    Map & operator=(const Map& other) = default;
    void initialize();
    void create_grid_map(int x_grid_num,int y_grid_num,double x_distance,double y_distance,double x_shift,double y_shift);
    void add_enter_route(double distance);
    void sort_edges();
    void set_routing(RoutingType routing_type,int table_size,double p_turn,std::mt19937& rng,std::uniform_real_distribution<>& dist);
    void load_routing(const char load_filename[],int table_size);
    void copy_routing();
    void describe_points();
    void describe_map();
};

/**
 * @brief Mapの生成
 * @param x_grid_num x方向格子数
 * @param y_grid_num y方向格子数
 * @param x_distance x方向エリア長
 * @param y_distance y方向エリア長
 * @param x_shift x方向格子移動長
 * @param y_shift y方向格子移動長
 * @details shiftは座標をshift分だけずらす．仮想道路などで利用
 */
Map::Map(int x_grid_num,int y_grid_num,double x_distance,double y_distance,double x_shift,double y_shift){
    point_id_=0;
    edge_id_=0;
    grid_num_x=x_grid_num;
    grid_num_y=y_grid_num;
    scale_x=x_distance;
    scale_y=y_distance;
    create_grid_map(x_grid_num,y_grid_num,x_distance,y_distance,x_shift,y_shift);
}

/**
 * @brief 要素の初期化
 */
void Map::initialize(){
    point_id_=0;
    edge_id_=0;
    map.clear();
    points.clear();
}

/**
 * @brief 格子状マップの作成
 * @param x_grid_num x方向格子数
 * @param y_grid_num y方向格子数
 * @param x_distance x方向エリア長
 * @param y_distance y方向エリア長
 * @param x_shift x方向格子移動長
 * @param y_shift y方向格子移動長
 * @details shiftは座標をshift分だけずらす．仮想道路などで利用
 */
void Map::create_grid_map(int x_grid_num,int y_grid_num,double x_distance,double y_distance,double x_shift,double y_shift){
    initialize();
    grid_num_x=x_grid_num;
    grid_num_y=y_grid_num;
    scale_x=x_distance;
    scale_y=y_distance;
    // set base points
    double x_point_distance=(x_grid_num>0) ? x_distance/x_grid_num : 0.0;
    double y_point_distance=(y_grid_num>0) ? y_distance/y_grid_num : 0.0;
    ++x_grid_num;
    ++y_grid_num;
    points.reserve(x_grid_num*y_grid_num+2*x_grid_num+2*y_grid_num);
    for(int i=0;i<y_grid_num;i++){
        for(int j=0;j<x_grid_num;j++){
            EdgePoint point(point_id_++,(double)j*x_point_distance+x_shift,(double)i*y_point_distance+y_shift);
            points.push_back(point);
        }
    }
    map.resize(x_grid_num*y_grid_num+2*x_grid_num+2*y_grid_num);// 縦×横+上下，左右の延長
    
    for(int i=0;i<y_grid_num;i++){
        for(int j=0;j<x_grid_num;j++){
            int id=i*x_grid_num+j;
            if(0<j){
                Edge edge(edge_id_++,&points[id],&points[id-1]);
                map[id].push_back(edge);
            }
            if(0<i){
                Edge edge(edge_id_++,&points[id],&points[id-x_grid_num]);
                map[id].push_back(edge);
            }
            if(j<x_grid_num-1){
                Edge edge(edge_id_++,&points[id],&points[id+1]);
                map[id].push_back(edge);
            }
            if(i<y_grid_num-1){
                Edge edge(edge_id_++,&points[id],&points[id+x_grid_num]);
                map[id].push_back(edge);
            }
        }
    }
}

/**
 * @brief 格子状道路の端に進入経路を追加する．口->井
 * @param distance 進入経路の長さ
 */
void Map::add_enter_route(double distance){
    size_t pre_points_size=points.size();
    for(size_t map_start_i=0;map_start_i<pre_points_size;++map_start_i){
        for(size_t point_i=0;point_i<4;++point_i){
            if(points.at(map_start_i).connection.at(point_i)==NULL){
                double point_x=points.at(map_start_i).point.x;
                double point_y=points.at(map_start_i).point.y;
                if(point_i==LEFT){
                    point_x+=distance;
                }else if(point_i==RIGHT){
                    point_x-=distance;
                }else if(point_i==UP){
                    point_y+=distance;
                }else if(point_i==DOWN){
                    point_y-=distance;
                }
                EdgePoint add_point(point_id_++,point_x,point_y);
                points.push_back(add_point);
                Edge add_edge(edge_id_++,&points.at(points.size()-1),&points.at(map_start_i),static_cast<EdgeDirection>((point_i+2)%4));
                map.at(add_point.id).push_back(add_edge);
                Edge add_edge_rev(edge_id_++,&points.at(map_start_i),&points.at(points.size()-1),static_cast<EdgeDirection>((point_i)));
                map.at(map_start_i).push_back(add_edge_rev);
            }
        }
    }
}

/**
 * @brief マップのエッジの順番を方向順にソートする
 */
void Map::sort_edges(){
    for(size_t i=0,size=map.size();i<size;++i){
        if(map.at(i).size()==4){
            for(size_t j=0;j<map.at(i).size();++j){
                Edge &edge=map.at(i).at(j);
                std::swap(map.at(i).at(j),map.at(i).at(edge.direction));
            }
        }
    }
}

/**
 * @brief ルーティングテーブルの作成
 * @param routing_type ルーティングテーブルの設定
 * @param table_size 設けるテーブルの量
 * @param p_turn 曲がる確率 [0:1]
 * @param rng 乱数生成器
 * @param dist 乱数分布
 */
void Map::set_routing(RoutingType routing_type,int table_size,double prob_turn,std::mt19937& rng,std::uniform_real_distribution<>& dist){
    for(size_t i=0,size=points.size();i<size;++i){
        EdgePoint &p=points.at(i);
        p.routing.initialize(table_size,4);
        if(p.connection_counter!=4)continue;
        if(routing_type==RoutingType::NORMAL){
            for(int enter=0;enter<4;++enter){
                for(int table=0;table<table_size;++table){ // table_sizeは4，left|down, down|right, right|up, up|left
                    p.routing.table.at(enter).at(table).at(table)=1-prob_turn; // 直進
                    p.routing.table.at(enter).at(table).at((table+1)%4)=prob_turn; // 曲がる
                }
            }
        }else if(routing_type==RoutingType::UNIFORM){
            for(int enter=0;enter<4;++enter){
                for(int table=0;table<table_size;++table){
                    p.routing.table.at(enter).at(table).at(0)=0.25;
                    p.routing.table.at(enter).at(table).at(1)=0.25;
                    p.routing.table.at(enter).at(table).at(2)=0.25;
                    p.routing.table.at(enter).at(table).at(3)=0.25;
                }
            }
        }else if(routing_type==RoutingType::NO_U_TURN){
            for(int enter=0;enter<4;++enter){
                for(int table=0;table<table_size;++table){
                    for(int edge_itr=0;edge_itr<4;++edge_itr){
                        if((enter+2)%4!=edge_itr){
                            p.routing.table.at(enter).at(table).at(edge_itr)=1.0/3.0;
                        }else{
                            p.routing.table.at(enter).at(table).at(edge_itr)=0.0;
                        }
                    }
                }
            }
        }else if(routing_type==RoutingType::TURN){
            for(int enter=0;enter<4;++enter){
                for(int table=0;table<table_size;++table){
                    for(int edge_itr=0;edge_itr<4;++edge_itr){
                        if((enter+edge_itr)%2==1){
                            p.routing.table.at(enter).at(table).at(edge_itr)=0.5;
                        }else{
                            p.routing.table.at(enter).at(table).at(edge_itr)=0.0;
                        }
                    }
                }
            }
        }else if(routing_type==RoutingType::RANDOM){
            for(int enter=0;enter<4;++enter){
                for(int table=0;table<table_size;++table){
                    double rate=dist(rng);
                    p.routing.table.at(enter).at(table).at(0)=dist(rng)*rate;
                    p.routing.table.at(enter).at(table).at(1)=rate-p.routing.table.at(enter).at(table).at(0);
                    p.routing.table.at(enter).at(table).at(2)=(1-rate)*dist(rng);
                    p.routing.table.at(enter).at(table).at(3)=(1-rate)-p.routing.table.at(enter).at(table).at(2);
                    //LOGGER_DATA_PRINT(INFO,"call rng at %s %d",__FILE__,__LINE__);
                }
            }
        }else if(routing_type==RoutingType::RANDOM_WO_U_TURN){
            for(int enter=0;enter<4;++enter){
                for(int table=0;table<table_size;++table){
                    double rate[3]={dist(rng),dist(rng),0}; // 0---a---b---1; -> 0-rate[0]-a-rate[1]-b-rate[2]-1
                    int rate_itr=0;
                    if(rate[1]<rate[0]){
                        rate[2]=rate[0];
                        rate[0]=rate[1];
                        rate[1]=rate[2];
                    }
                    rate[2]=1-rate[1];
                    rate[1]=rate[1]-rate[0];
                    for(int edge_itr=0;edge_itr<4;++edge_itr){
                        if(edge_itr!=(enter+2)%4){
                            p.routing.table.at(enter).at(table).at(edge_itr)=rate[rate_itr++];
                        }else{
                            p.routing.table.at(enter).at(table).at(edge_itr)=0.0;
                        }
                    }
                    //LOGGER_DATA_PRINT(INFO,"call rng at %s %d",__FILE__,__LINE__);
                }
            }
        }else if(routing_type==RoutingType::RANDOM_TURN){ // ランダムな曲がる確率を与える
            for(int enter=0;enter<4;++enter){
                for(int table=0;table<table_size;++table){ // table_sizeは4，left|down, down|right, right|up, up|left
                    double r=dist(rng);

                    p.routing.table.at(enter).at(table).at(table)=1-r; // 直進
                    p.routing.table.at(enter).at(table).at((table+1)%4)=r; // 曲がる
                }
            }
        }
    }
}

void Map::load_routing(const char load_filename[],int table_size){
    FILE *fp=NULL;
    char line[255];
    double value=0.0;
    fp=fopen(load_filename,"r");
    if(fp==NULL){
        fprintf(stderr,"fail load fail: Map::load_routing  filename:%s\n",load_filename);
    }else{
        for(size_t i=0,size=points.size();i<size;++i){
            EdgePoint &p=points.at(i);
            p.routing.initialize(table_size,4);
            if(p.connection_counter!=4)continue;
            for(int enter=0;enter<4;++enter){
                for(int table=0;table<table_size;++table){ // table_sizeは4，left|down, down|right, right|up, up|left
                    for(int table_i=0;table_i<4;++table_i){
                        if(fgets(line, 255, fp) != NULL){
                            sscanf(line,"%lf",&value);
                            p.routing.table.at(enter).at(table).at(table_i)=value;
                        }else{
                            fprintf(stderr,"fail read fail: Map::load_routing  filename:%s\n",load_filename);
                            p.routing.table.at(enter).at(table).at(table_i)=0.0;
                        }
                    }
                }
            }
        }
    }
}

/**
 * @brief 全ての交差点のテーブルを倍に拡張する
 */
void Map::copy_routing(){
    for(size_t i=0,size=points.size();i<size;++i){
        EdgePoint &p=points.at(i);
        if(p.connection_counter!=4)continue;
        p.routing.copy_table();
    }
}

/**
 * @brief 交差点の情報を表示
 */
void Map::describe_points(){
    printf("describe map points size:%lu\n",points.size());
    for(size_t point_itr=0,size=points.size();point_itr<size;++point_itr){
        printf("%d  %p  ",points.at(point_itr).id,(void*)&points.at(point_itr));
        print_point(points.at(point_itr).point);
    }
}

/**
 * @brief マップの情報を表示
 */
void Map::describe_map(){
    printf("describe map size:%lu\n",points.size());
    for(size_t from_itr=0,from_size=points.size();from_itr<from_size;++from_itr){
        printf("edge from %2ld %p  size : %lu  to :",from_itr,(void*)&points.at(from_itr),map.at(from_itr).size());
        for(size_t to_itr=0,to_size=map.at(from_itr).size();to_itr<to_size;++to_itr){
            Edge &edge=map.at(from_itr).at(to_itr);
            printf(" id:%u c:%lu %u %d > %p ",edge.id,edge.counter,edge.to->id,edge.direction,(void*)edge.to);
        }
        printf("\n");
    }
}

#endif