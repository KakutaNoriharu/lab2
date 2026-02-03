#ifndef AREA_HPP_
#define AREA_HPP_
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <vector>
#include <set>
#include "point.h"

/**
 * @brief 長方形構造体
 */
typedef struct Area{
    double location_x,location_y; // location of center
    double size_x,size_y; // square edge size
    double left_bottom_x,left_bottom_y; // location of left bottom corner
    double right_top_x,right_top_y; // location of right top corner
}Area;

/**
 * @brief 中心に基づく長方形の作成
 * @param location_x x座標中心位置
 * @param location_y y座標中心位置
 * @param size_x x方向の大きさ
 * @param size_y y方向の大きさ
 */
Area createArea(double location_x, double location_y, double size_x, double size_y){
    Area area;
    area.location_x = location_x;
    area.location_y = location_y;
    area.size_x = size_x;
    area.size_y = size_y;
    area.left_bottom_x = location_x-size_x/2;
    area.left_bottom_y = location_y-size_y/2;
    area.right_top_x = location_x+size_x/2;
    area.right_top_y = location_y+size_y/2;
    return area;
}


/**
 * @brief 端に基づく長方形の作成
 * @param left_bottom_x 左下x座標
 * @param left_bottom_y 左下y座標
 * @param right_top_x 右上x座標
 * @param right_top_x 右上y座標
 */
Area createAreaByCorners(double left_bottom_x, double left_bottom_y, double right_top_x, double right_top_y){
    return createArea((left_bottom_x+right_top_x)/2, (left_bottom_y+right_top_y)/2, right_top_x-left_bottom_x, right_top_y-left_bottom_y);
}

/**
 * @brief 中心に基づく長方形の作成
 * @param location 中心点
 * @param size_x x方向の大きさ
 * @param size_y y方向の大きさ
 */
Area createAreaByLocationPoint(Point location, double size_x, double size_y){
    return createArea(location.x, location.y, size_x, size_y);
}

/* x,yは中心座標，端を含む */
bool onArea(Area area, double x, double y){
    return (area.size_x>0.0 && area.size_y>0.0 && x>=area.left_bottom_x && x<=area.right_top_x && y>=area.left_bottom_y && y<=area.right_top_y);
}

/* x,yは中心座標，端を含まない */
bool inArea(Area area, double x, double y){
    return (area.size_x>0.0 && area.size_y>0.0 && x>area.left_bottom_x && x<area.right_top_x && y>area.left_bottom_y && y<area.right_top_y);
}


/* x,yは中心座標，端を含む */
bool onAreaByPoint(Area area, Point location){
    return (area.size_x>0.0 && area.size_y>0.0 &&location.x>=area.left_bottom_x && location.x<=area.right_top_x && location.y>=area.left_bottom_y && location.y<=area.right_top_y);
}

/* x,yは中心座標，端を含まない */
bool inAreaByPoint(Area area, Point location){
    return (area.size_x>0.0 && area.size_y>0.0 && location.x>area.left_bottom_x && location.x<area.right_top_x && location.y>area.left_bottom_y && location.y<area.right_top_y);
}


/* 直線と交わるか評価 (直線は水平，垂直方向を前提とする)*/
bool isCrossOnArea(Area area, Point s, Point e){
if(onAreaByPoint(area,s) || onAreaByPoint(area,e)){ // 始点と終点のどちらかがエリア内なら交差する
        return true;
    }else{
        if(EQUAL(s.x,e.x)){ // 格子状道路を仮定して，縦方向の経路の場合
            if((IN_BAR(area.left_bottom_y,s.y,e.y) || IN_BAR(area.right_top_y,s.y,e.y)) && (IN_BAR(s.x,area.left_bottom_x,area.right_top_x))){
                return true; // x座標がエリアx座標内でかつ，始点と終点の内側に，yについて境界がある場合
            }
        }else{ // 横方向の経路の場合
            if((IN_BAR(area.left_bottom_x,s.x,e.x) || IN_BAR(area.right_top_x,s.x,e.x)) && (IN_BAR(s.y,area.left_bottom_y,area.right_top_y))){
                return true; // y座標がエリアのy座標内でかつ，始点と終点の内側に，yについて境界がある場合
            }
        }
    }
    return false;
}

/**
 * Areaを重複のない部分Areaに分割して，Funcを適用させる．
 */
template <typename Func>
void iterate_grid_cells(const std::vector<Area>& areas, Func func) {
    std::set<double> xs, ys;
    for (auto& a : areas) {
        xs.insert(a.left_bottom_x);
        xs.insert(a.right_top_x);
        ys.insert(a.left_bottom_y);
        ys.insert(a.right_top_y);
    }
    std::vector<double> x(xs.begin(), xs.end()), y(ys.begin(), ys.end());
    for (size_t i = 0; i + 1 < x.size(); ++i) {
        for (size_t j = 0; j + 1 < y.size(); ++j) {
            double x1 = x[i], x2 = x[i + 1];
            double y1 = y[j], y2 = y[j + 1];
            if (x2 - x1 <= 1e-9 || y2 - y1 <= 1e-9) continue;
            double cx = (x1 + x2) / 2.0, cy = (y1 + y2) / 2.0;
            for (auto& a : areas) {
                if (cx >= a.left_bottom_x && cx < a.right_top_x &&
                    cy >= a.left_bottom_y && cy < a.right_top_y) {
                    func(x1, y1, x2, y2);
                    break;
                }
            }
        }
    }
}

/**
 * @brief Area配列をいくつかの重複のないAreaに変換する
 * * 座標離散化（グリッド分割）アルゴリズムを使用
 * * @param areas Area構造体のベクタ
 * @return std::vector<Area> 重複のないAreaの集合
 */
std::vector<Area> decompose_to_non_overlapping_areas(const std::vector<Area>& areas) {
    std::vector<Area> res;
    iterate_grid_cells(areas, [&](double x1, double y1, double x2, double y2) {
        res.push_back(createAreaByCorners(x1, y1, x2, y2));
    });
    return res;
}

/**
 * @brief 重複を除いたArea配列の合計面積を計算する
 * * 座標離散化（グリッド分割）アルゴリズムを使用
 * * @param areas Area構造体のベクタ
 * @return double 和集合の合計面積
 */
double calculate_union_area_discretized(const std::vector<Area>& areas) {
    double total = 0.0;
    iterate_grid_cells(areas, [&](double x1, double y1, double x2, double y2) {
        total += (x2 - x1) * (y2 - y1);
    });
    return total;
}

double calculate_crossing_length(const std::vector<Area>& areas,double XL,double YL,int Nx,int Ny) {
    if (areas.empty()) {
        return 0.0;
    }
    double road_length = 0.0;
    std::vector<Area> no_covered_areas = decompose_to_non_overlapping_areas(areas);
    if(Nx==0){
        double vx=XL/2.0;
        double sy=0;
        for(const Area &a : no_covered_areas){
            if(a.left_bottom_x<=vx && vx<=a.right_top_x && sy<=a.left_bottom_y){
                road_length+=MIN(YL,a.right_top_y)-MAX(sy,a.left_bottom_y);
                sy=MAX(sy,a.right_top_y);
            }
        }
    }else{
        for(int x=0;x<=Nx;++x){
            double vx=XL/Nx*x;
            double sy=0;
            for(const Area &a : no_covered_areas){
                if(a.left_bottom_x<=vx && vx<=a.right_top_x && sy<=a.left_bottom_y){
                    road_length+=MIN(YL,a.right_top_y)-MAX(sy,a.left_bottom_y);
                    sy=MAX(sy,a.right_top_y);
                }
            }
        }
    }
    if(Ny==0){
        double hy=YL/2.0;
        double sx=0.0;
        for(const Area &a : no_covered_areas){
            if(a.left_bottom_y<=hy && hy<=a.right_top_y && sx<=a.left_bottom_x){
                road_length+=MIN(XL,a.right_top_x)-MAX(sx,a.left_bottom_x);
                sx=MAX(sx,a.right_top_x);
            }
        }
    }else{
        for(int y=0;y<=Ny;++y){
            double hy=YL/Ny*y;
            double sx=0.0;
            for(const Area &a : no_covered_areas){
                if(a.left_bottom_y<=hy && hy<=a.right_top_y && sx<=a.left_bottom_x){
                    road_length+=MIN(XL,a.right_top_x)-MAX(sx,a.left_bottom_x);
                    sx=MAX(sx,a.right_top_x);
                }
            }
        }
    }
    return road_length;
}

void printArea(Area area){
    printf("Area: location_x=%f, location_y=%f, size_x=%f, size_y=%f\n",area.location_x, area.location_y, area.size_x, area.size_y);
}

void printAreaCorners(Area area){
    printf("Area: left_bottom x=%f, y=%f, right_top x=%f, y=%f\n",area.left_bottom_x, area.left_bottom_y, area.right_top_x, area.right_top_y);
}
#endif