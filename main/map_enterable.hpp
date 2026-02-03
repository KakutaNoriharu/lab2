#ifndef MAP_ENTERABLE_H_
#define MAP_ENTERABLE_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <vector>
#include <random>
#include "map.hpp"

/**
 * @brief 進入点のクラス
 */
class EnterablePoint{
private:
public:
    int point_id; // 点のid
    double enter_lambda; // 点での進入密度
    double poisson_enter_time; // 次回進入時刻
    bool enterable; // 進入可能フラグ
    EdgeDirection direction; // 進入方向
    EnterablePoint();
    EnterablePoint(int id,EdgeDirection edge_direction);
    ~EnterablePoint() noexcept = default;
    EnterablePoint(const EnterablePoint& other) = default;
    EnterablePoint(EnterablePoint&& other) noexcept = default;
    EnterablePoint & operator=(const EnterablePoint& other) = default;
    void set(int id,std::mt19937& rng,std::uniform_real_distribution<>& dist,EdgeDirection edge_direction,double lambda);
    void update(std::mt19937& rng,std::uniform_real_distribution<>& dist);
};

EnterablePoint::EnterablePoint(){
    point_id=-1;
    poisson_enter_time=0.0;
    direction=EdgeDirection::NONE;
    enterable=false;
}
EnterablePoint::EnterablePoint(int id,EdgeDirection edge_direction){
    point_id=id;
    poisson_enter_time=0.0;
    direction=edge_direction;
    enterable=true;
}

/**
 * @brief 進入点の設定
 * @param id 点のid
 * @param rng 乱数生成器
 * @param dist 乱数分布
 * @param edge_direction 進入方向
 * @param lambda 進入密度
 */
void EnterablePoint::set(int id,std::mt19937& rng,std::uniform_real_distribution<>& dist,EdgeDirection edge_direction,double lambda){
    point_id=id;
    direction=edge_direction;
    enter_lambda=lambda;
    if(lambda>0){
        poisson_enter_time=CNV_EXP(dist(rng),enter_lambda);
        //LOGGER_DATA_PRINT(INFO,"call rng at %s %d",__FILE__,__LINE__);
        enterable=true;
    }else{
        poisson_enter_time=-1.0;
        enterable=false;
    }
}

/**
 * @brief 進入時刻の更新
 * @param rng 乱数生成器
 * @param dist 乱数分布
 */
void EnterablePoint::update(std::mt19937& rng,std::uniform_real_distribution<>& dist){
    if(enterable){
        poisson_enter_time+=CNV_EXP(dist(rng),enter_lambda);
        //LOGGER_DATA_PRINT(INFO,"call rng at %s %d",__FILE__,__LINE__);
    }
}

class MapEnterable : public Map{
private:
public:
    std::vector<EnterablePoint> enterable_points;
    MapEnterable() = default;
    MapEnterable(std::mt19937& rng, std::uniform_real_distribution<>& dist,int x_grid_num,int y_grid_num,double x__distance,double y_distance,double x_shift,double y_shift,double enter_route_distance,double lambda);
    ~MapEnterable() noexcept = default;
    MapEnterable(const MapEnterable& other) = default;
    MapEnterable(MapEnterable&& other) noexcept = default;
    MapEnterable & operator=(const MapEnterable& other) = default;
    void create_enterable_grid_map(std::mt19937& rng, std::uniform_real_distribution<>& dist,int x_grid_num,int y_grid_num,double x_distance,double y_distance,double x_shift,double y_shift,double enter_route_distance,double lambda);
    void reset_enter_time(std::mt19937& rng, std::uniform_real_distribution<>& dist, EdgeDirection direction, double lambda);
    void describe_enterable_point();
};

MapEnterable::MapEnterable(std::mt19937& rng, std::uniform_real_distribution<>& dist,int x_grid_num,int y_grid_num,double x_distance,double y_distance,double x_shift,double y_shift,double enter_route_distance,double lambda){
    create_enterable_grid_map(rng,dist,x_grid_num,y_grid_num,x_distance,y_distance,x_shift,y_shift,enter_route_distance,lambda);
}

/**
 * make grid map 
 * arguments
 * int x_grid_num number of point on x axis
 * int y_grid_num number of point on y axis
 * double x_distance distance of point to point for x 
 * double y_distance distance of point to point for y 
 * double x_shift points x position add by x_shift 
 * double x_shift points y position add by y_shift 
 * double lambda poisson enter lambda */
void MapEnterable::create_enterable_grid_map(std::mt19937& rng, std::uniform_real_distribution<>& dist,int x_grid_num,int y_grid_num,double x_distance,double y_distance,double x_shift,double y_shift,double enter_route_distance,double lambda){
    create_grid_map(x_grid_num,y_grid_num,x_distance,y_distance,x_shift,y_shift);
    add_enter_route(enter_route_distance);
    enterable_points.resize(points.size());
    for(size_t i=0,size=map.size();i<size;++i){
        if(map.at(i).size()==1){
            Edge &edge=map.at(i).at(0);
            enterable_points.at(i).set(edge.from->id,rng,dist,edge.direction,lambda);
        }
    }
    sort_edges();
}

void MapEnterable::reset_enter_time(std::mt19937& rng, std::uniform_real_distribution<>& dist, EdgeDirection direction, double lambda){
    for(size_t i=0,size=map.size();i<size;++i){
        if(map.at(i).size()==1){
            Edge &edge=map.at(i).at(0);
            if(edge.direction==direction){
                enterable_points.at(i).set(edge.from->id,rng,dist,edge.direction,lambda);
            }
        }
    }
}

void MapEnterable::describe_enterable_point(){
    printf("describe map enterable points size:%lu\n",points.size());
    for(size_t i=0;i<enterable_points.size();++i){
        EnterablePoint &ep=enterable_points.at(i);
        printf("%lu %d %d %d\n",i,ep.point_id,ep.direction,ep.enterable);
    }
}

#endif