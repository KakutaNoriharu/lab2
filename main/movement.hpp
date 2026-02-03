#ifndef _MOVEMENT_HPP_
#define _MOVEMENT_HPP_

#include <stdbool.h>
#include <stdio.h>
#include "point.h"
#include "../utility/utility.h"
#include "../utility/logger.h"

/**
 * note
 * 予めdelta_tを渡したうえで利用する必要がある．
 */

class Movement{
private:
public:
    enum MOVEMENT_TYPE{
        ACTIVE,     // move to goal or stop
        STAY,       // not move
        END         // size of movement type
    };
    static double dt; // 起動されるすべてのシミュレーションで値が変わらないことを想定
    Point position,goal,unit; // 現在位置，目的地，単位ステップあたりの移動距離
    bool arrive;    // true:position==goal false:position!=goal
    double remain;  // 距離[m]
    double stop_time; // 停止時間[s]
    MOVEMENT_TYPE type; // 動作状態　ACTIVE:移動中，STAY:停止中
    Movement();
    Movement(double start_x,double start_y,double goal_x,double goal_y,double step_length);
    Movement(Point arg_start,Point arg_goal,double step_length);
    ~Movement() noexcept = default;
    Movement(const Movement& other) = default;
    Movement(Movement&& other) noexcept = default;
    Movement & operator=(const Movement& other) = default;
    static void set_delta_t(double delta_t);
    void set_movement(Point start,Point trg,double step_length,double init_progress_length);
    void set_new_goal(Point new_goal,double step_length);
    void set_stop_time(double stop);
    void update();
    void describe();
};

double Movement::dt=-1.0;

void Movement::set_delta_t(double delta_t){
    dt=delta_t;
}

/**
 * 空の初期化 */
Movement::Movement(){
    init_position(&position,0.0,0.0);
    init_position(&goal,0.0,0.0);
    init_position(&unit,0.0,0.0);
    remain=0.0;
    stop_time=0.0;
    type=STAY;
    arrive=false;
}

/**
 * Movementの初期化
 * start_x,start_y,goal_x,goal_y 初期位置，目的地のx,y座標
 * step_length：単位ステップあたりの移動距離[m] */
Movement::Movement(double start_x,double start_y,double goal_x,double goal_y,double step_length){
    init_position(&position,start_x,start_y);
    init_position(&goal,goal_x,goal_y);
    cal_move_unit(position,goal,&unit,step_length);
    remain=0.0;
    stop_time=0.0;
    type=ACTIVE;
    arrive=false;
}

/**
 * Movementの初期化
 * arg_start,arg_goal：初期位置，目的地
 * step_length：単位ステップあたりの移動距離[m] */
Movement::Movement(Point arg_start,Point arg_goal,double step_length){
    position=arg_start;
    goal=arg_goal;
    cal_move_unit(position,goal,&unit,step_length);
    remain=0.0;
    type=ACTIVE;
    arrive=false;
}

/**
 * Movementの初期化
 * start,goal：現在位置と目的地
 * step_length：単位ステップあたりの移動距離[m]
 * init_progress_length：初期状態からの移動距離[m] */
void Movement::set_movement(Point start,Point trg,double step_length,double init_progress_length){
    position=start;
    goal=trg;
    cal_move_unit(position,goal,&unit,step_length);
    arrive=false;
    remain=0.0;
    if(init_progress_length<cal_point_distance(start,goal)){
        update_point_to_goal_by_distance(&position,&goal,init_progress_length);
    }else{
        //LOGGER_DATA_PRINT(WARNING,"init_progress_length>edge_distance  : %.2f > %.2f",init_progress_length,cal_point_distance(start,goal));
        arrive=true;
        remain=init_progress_length-cal_point_distance(start,goal);
        position=goal;
    }
    type=ACTIVE;
}

/**
 * 次の目的地を設定する
 * 移動しそびれた距離があればその分だけ移動する
 * new_goal：次の目的地
 * step_length：単位ステップあたりの移動距離[m] */
void Movement::set_new_goal(Point new_goal,double step_length){
    cal_move_unit(goal,new_goal,&unit,step_length);
    if(!isSamePoint(position,goal)){
        //LOGGER_DATA_PRINT(WARNING,"current position is not equal to goal (%.2f,%.2f) (%.2f,%.2f)",position.x,position.y,goal.x,goal.y);
    }
    if(isSamePoint(new_goal,goal)){
        LOGGER_PRINT(DEBUG,"new goal is same as current goal");
    }
    goal=new_goal;
    arrive=false;
    if(remain>0){
        if(remain>=cal_point_distance(position,goal)){
            LOGGER_DATA_PRINT(DEBUG,"notice : distance to new goal(%.2f) is shorter than remain(%.2f)",cal_point_distance(position,goal),remain);
            arrive=true;
            remain-=cal_point_distance(position,goal);
            position=goal;
        }else{
            update_point_to_goal_by_distance(&position,&goal,remain);
        }
    }
}

/**
 * 停止時間を設定
 * 設定した時点の位置で停止する． */
void Movement::set_stop_time(double stop){
    if(dt>0){
        stop_time=stop;
    }else{
        fprintf(stderr, "WARNING Movement::set_sopt_time  dt is not setted.  skip set stop time\n");
    }
}


/**
 * Movementの位置の更新
 * arriveがtrueの場合は次の目的地を設定するまで位置を更新しない
 * さらに，その時間の分の移動距離はremainに加算されない
 * todo: 高速化 */
inline void Movement::update(){
    if(type==ACTIVE && arrive==false){
        if(stop_time>0){
            stop_time-=dt;
            if(stop_time<0){
                Point pre_position=position;
                add_point_with_rate(&position,&unit,-stop_time/dt);
                // if new_position over goal position is goal
                if(!(IN_BAR(position.x,pre_position.x,goal.x) || IN_BAR(position.y,pre_position.y,goal.y))){
                    arrive=true;
                    remain=cal_point_distance(position,goal);
                    position=goal;
                }
                stop_time=0.0;
            }
        }else{
            Point pre_position=position;
            add_point_by(&position,&unit);
            // if new_position over goal position is goal
            if(!(IN_BAR(position.x,pre_position.x,goal.x) || IN_BAR(position.y,pre_position.y,goal.y))){
                arrive=true;
                remain=cal_point_distance(position,goal);
                position=goal;
            }
        }
    }else if(type==STAY){
        // nothing to do
    }
}

/**
 * Movementの変数表示 */
void Movement::describe(){
    printf("Movement type:%d  position x/y:%.2f/%.2f  goal x/y:%.2f/%.2f  arrive:%d  remain:%.2f  stop:%.2f\n",
    type,position.x,position.y,goal.x,goal.y,arrive,remain,stop_time);
}

#endif