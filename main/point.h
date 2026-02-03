#ifndef POINT_H_
#define POINT_H_

#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <float.h> // DBL_EPSILON
#include "../utility/utility.h"

#ifndef M_PI
#define M_PI (3.14159265358979323846) //3.14159265358979323846
#endif

/**
 * @brief 2つのdouble変数を扱う構造体
 * @param x x座標
 * @param y y座標*/
typedef struct Point{
    double x,y;
}Point;

/**
 * @brief Point構造体の座標を初期化する
 *
 * @param p 初期化対象のPoint構造体へのポインタ
 * @param x 設定するx座標の値
 * @param y 設定するy座標の値
 */
void init_position(Point *p,double x,double y){
    p->x = x;
    p->y = y;
}

/**
 * @brief Pointを作る関数
 * @param x,y 各座標の値 */
Point new_point(double x,double y){
    Point p = {x,y};
    return p;
}
/**
 * @brief 2つのPoint a,b が同じか判定する関数
 * @param a,b 比較するPoint*/
bool isSamePoint(Point a,Point b){
    return (EQUAL(a.x,b.x) && EQUAL(a.y,b.y));
}
/**
 * @brief Pointの内容を表示する */
void print_point(Point p){
    printf("x:%.2f y:%.2f\n",p.x,p.y);
}

#define PointCalculateNew(point1,op,point2) new_point(point1.x op point2.x, point1.y op point2.y) // point1 op(+,-,*,/) point2

/**
 * @brief Pointを加算し，結果のPointを得る
 * @param base,add 加算要素 
 * 
 * return base + add*/
Point add_point(Point base,Point add){
    Point p = {base.x+add.x,base.y+add.y};
    return p;
}

/**
 * @brief Pointを減算し，結果のPointを得る
 * @param a 引かれるPoint
 * @param b 引くPoint
 * 
 * return a-b */
Point sub_point(Point a,Point b){
    Point p = {a.x-b.x,a.y-b.y};
    return p;
}

/**
 * @brief ポイントを加算する
 * @param p 加算対象
 * @param b 加算量
 */
void add_point_by(Point *p,Point *b){
    p->x = p->x + b->x;
    p->y = p->y + b->y;
}

/**
 * @brief 2点の中点を求める
 * @param a,b 中点を求めるための点
 */
Point middle_point(Point a,Point b){
    Point middle;
    if(a.x==b.x){
        middle.x=a.x;
    }else{
        middle.x=(a.x+b.x)*0.5;
    }
    if(a.y==b.y){
        middle.y=a.y;
    }else{
        middle.y=(a.y+b.y)*0.5;
    }
    return middle;
}

/**
 * @brief 位置を進行量の分だけ進める
 * @param p 現在位置
 * @param b 進行量，または目的地
 * @param rate bに対する比率 1.0ならbだけ進む
 */
void add_point_with_rate(Point *p,Point *b,double rate){
    p->x = p->x + b->x * rate;
    p->y = p->y + b->y * rate;
}
/**
 * @brief 点を目的地へ進める
 * @param start 現在位置
 * @param goal 目的地
 * @param distance 進行距離
 * 
 * startの値をdistanceだけgoalに近づける
 * 超過する可能性がある 
 * */
void update_point_to_goal_by_distance(Point *start,Point *goal,double distance){
    if((fabs(start->y - goal->y) < DBL_EPSILON)){
        if(start->x<goal->x){
            start->x+=distance;
        }else{
            start->x-=distance;
        }
    }else if((fabs(start->x - goal->x) < DBL_EPSILON)){
        if(start->y<goal->y){
            start->y+=distance;
        }else{
            start->y-=distance;
        }
    }else{
    	double arg = atan2(goal->y-start->y,goal->x-start->x);
    	start->x+=cos(arg)*distance;
	    start->y+=sin(arg)*distance;
    }
}

/**
 * @brief 2点間の距離を計算する 
 * @param a,b 計算する2点*/
double cal_point_distance(Point a,Point b){
    if(fabs(a.y - b.y) < DBL_EPSILON){
        return fabs(a.x-b.x);
    }else if(fabs(a.x - b.x) < DBL_EPSILON){
        return fabs(a.y-b.y);
    }else{
        return sqrt(pow(a.x-b.x,2.0)+pow(a.y-b.y,2.0));
    }
}

/**
 * @brief 2つのベクトルのなす角を返す [-1~1]
 * @param start 始点ベクトル
 * @param goal 終点ベクトル
 * 
 * aとbからなるベクトルの角度を返す　値域[-1(-180):1(180)]*/
double cal_point_angle(Point start,Point goal){
    return atan2(goal.y - start.y,goal.x - start.x)/M_PI;
}

/**
 * @brief 点がエリアに入っているか評価(境界を含まない)
 * @param a 評価対象の点
 * @param x_start x方向のエリアの始点
 * @param y_start y方向のエリアの始点
 * @param x_end x方向のエリアの終点
 * @param y_end y方向のエリアの終点
 */
bool point_in_area(Point a,double x_start,double y_start,double x_end,double y_end){
    return IN_AREA(a.x,a.y,x_start,x_end,y_start,y_end);
}

/**
 * @brief 点がエリアに入っているか評価(境界を含む)
 * @param a 評価対象の点
 * @param x_start x方向のエリアの始点
 * @param y_start y方向のエリアの始点
 * @param x_end x方向のエリアの終点
 * @param y_end y方向のエリアの終点 */
bool point_on_area(Point a,double x_start,double y_start,double x_end,double y_end){
    return ON_AREA(a.x,a.y,x_start,x_end,y_start,y_end);
}

/**
 * @brief 点が矩形エリアに入っているか評価(境界を含まない)
 * @param a 評価対象の点
 * @param start 矩形エリア始点
 * @param end 矩形エリア終点 */
bool point_in_rectangle_area(Point a,Point start,Point end){
    return (IN_AREA(a.x,a.y,start.x,end.x,start.y,end.y));
}
/**
 * @brief 点が矩形エリアに入っているか評価(境界を含む)
 * @param a 評価対象の点
 * @param start 矩形エリア始点
 * @param end 矩形エリア終点 */
bool point_on_rectangle_area(Point a,Point start,Point end){
    return (ON_AREA(a.x,a.y,start.x,end.x,start.y,end.y));
}

/**
 * @brief 二点間の距離がrange以下か評価する
 * @param a,b 距離を評価する二点
 * @param range 比較する値
 * 
 * aとbの距離がrange以下であるか判定 */
bool point_in_range(Point a,Point b,double range){
    if(fabs(a.x-b.x)>range || fabs(a.y-b.y)>range){
        return false;
    }else{
        return (range*range>=(a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y));
    }
}

/**
 * @brief 始点から終点への移動単位を取得
 * @param start 始点
 * @param goal 終点
 * @param unit 移動単位の保存先
 * @param move_length 単位移動距離
 * 
 * startからgoalに向けてmove_lengthだけ動く*unitを設定する */
void cal_move_unit(Point start,Point goal,Point *unit,double move_length){
    if((fabs(start.y - goal.y) < DBL_EPSILON)){
        if(start.x<goal.x){
            unit->x=move_length;
        }else{
            unit->x=-move_length;
        }
        unit->y=0.0;
    }else if((fabs(start.x - goal.x) < DBL_EPSILON)){
        unit->x=0.0;
        if(start.y<goal.y){
            unit->y=move_length;
        }else{
            unit->y=-move_length;
        }
    }else{
    	double arg = atan2(goal.y-start.y,goal.x-start.x);
    	unit->x=cos(arg)*move_length;
	    unit->y=sin(arg)*move_length;
    }
}

#endif