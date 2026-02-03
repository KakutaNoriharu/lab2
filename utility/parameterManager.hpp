#ifndef _PARAMETER_MANAGER_HPP_
#define _PARAMETER_MANAGER_HPP_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <vector>
#include <numeric>
#include <functional>
#include "utility.hpp"

/**
 * シミュレーションにおける複数のパラメータの組を扱うためのクラス
 * 機能
 * ・任意の型のパラメータ変数へのアドレス，そのパラメータがとる値を指定することで，update()を呼び出すたびに，パラメータを更新する
 * ・mutexによりスレッドセーフ
 * 使い方
 * ・コンパイル時に -pthreadが必要
例
int a=0;
double b=0;
ParameterManager mgr(0);
Parameter<int> a_v(&mgr,&a,{1,2,3});
Parameter<double> b_v(&mgr,&b,{0.1,0.2,0.3});
mgr.update();
printf("%d %f\n",a,b);//1 0.1
mgr.update();
printf("%d %f\n",a,b);//1 0.2
mgr.update();
printf("%d %f\n",a,b);//1 0.3
mgr.update();
printf("%d %f\n",a,b);//2 0.1
...
 */

// ParameterManagerクラスは複数のパラメータを管理し、それらの組み合わせを更新する機能を提供します。
class ParameterManager {
public:
    size_t ParameterID; // 現在のパラメータID
    size_t ParameterProgress; // 現在の進行度
    size_t ParameterTotal; // 全パラメータの組み合わせ数
    size_t clear_size; // 表示した文字列の長さ
    pthread_mutex_t mutex;
    StringBuffer buffer;
    std::string SetName;
    TimeManager timer;
    std::vector<size_t> parameter_itr; // 各パラメータの現在のインデックス
    std::vector<size_t> parameter_size; // 各パラメータの値の数
    std::vector<std::function<void()>> functions; // パラメータ更新関数のリスト

    // コンストラクタ
    ParameterManager(size_t prg) : ParameterID(0), ParameterProgress(prg), ParameterTotal(1) {
        SetName="";
        pthread_mutex_init(&mutex,NULL);
    }
    // 指定された値でパラメータインデックスを設定する関数
    void set() {
        if (parameter_itr.size() > 0) {
            parameter_itr.at(0) = ParameterProgress;
        }
    }
    // 全パラメータの組み合わせを順次更新する関数．全てのパラメータを終えたらfalseを返す．最初の一回は設定するのみ．update->runの順で使う
    bool update(){
        if (ParameterProgress > 0) {
            for (auto& func : functions) {
                func();
            }
        }else{
            ParameterTotal=1;
            for(auto& size : parameter_size){
                ParameterTotal*=size;
            }
        }
        ParameterProgress += 1;
        print();
        if(ParameterProgress <= ParameterTotal){
            return true;
        }
        return false;
    }
    void print() {
        if (ParameterProgress <= ParameterTotal) {
            buffer.clear();
            buffer.insert("%s  Parameters : ",SetName.c_str());
            for (size_t i = parameter_itr.size() - 1; i < parameter_itr.size(); --i) {
                buffer.insert("[%lu/%lu]", parameter_itr.at(i) + 1, parameter_size.at(i));
            }
            buffer.insert("  Total : %u/%u  %.2f%%  ", ParameterProgress, ParameterTotal, (double)ParameterProgress / (double)ParameterTotal * 100.0);
            double elapsed_sec=timer.get_elapsed_time_sec();
            double remain_sec=timer.get_remain_time_sec((double)ParameterProgress / (double)ParameterTotal);
            buffer.insert("  elapsed : %d:%02d:%02d  remain : %d:%02d:%02d ",
                           static_cast<int>(elapsed_sec)/3600,static_cast<int>(elapsed_sec)/60%60,static_cast<int>(elapsed_sec)%60,
                           static_cast<int>(remain_sec)/3600,static_cast<int>(remain_sec)/60%60,static_cast<int>(remain_sec)%60);
        }
    }
    void lock(){
        pthread_mutex_lock(&mutex);
    }
    void unlock(){
        pthread_mutex_unlock(&mutex);
    }
};

// Parameterクラスは特定の型のパラメータを管理し、ParameterManagerを通じてその組み合わせを更新します。
template<typename T>
class Parameter {
public:
    ParameterManager *mgr; // パラメータマネージャのポインタ
    T *ref; // パラメータの参照
    std::vector<T> values; // パラメータの値のリスト
    size_t id; // パラメータのID

    Parameter(ParameterManager *manager, T *reference) : mgr(manager), ref(reference) {
        initialize();
    }

    // コンストラクタ：パラメータの値を直接受け取る場合
    Parameter(ParameterManager *manager, T *reference, const std::vector<T> parameter_values) : mgr(manager), ref(reference), values(parameter_values) {
        initialize();
    }

    // コンストラクタ：パラメータの値を範囲で指定する場合
    Parameter(ParameterManager *manager, T *reference, int start, int end) : mgr(manager), ref(reference) {
        values.resize(end - start + 1);
        std::iota(values.begin(), values.end(), start);
        initialize();
    }

    // パラメータを初期化する関数
    void initialize() {
        id = mgr->ParameterID++;
        mgr->parameter_itr.push_back(0);
        mgr->parameter_size.push_back(values.size());
        *ref = values.at(0);
        if (mgr->functions.empty()) {
            mgr->functions.push_back([this]() { this->update_itr(); });
            if (mgr->ParameterProgress > 0) {
                mgr->parameter_itr.at(0) = mgr->ParameterProgress - 1;
            }
        }
        mgr->functions.push_back([this]() { this->update(); });
    }

    void set(std::vector<T> init_values){
        values=init_values;
        mgr->parameter_size.at(id)=values.size();
    }

    // パラメータインデックスを更新する関数
    void update_itr() {
        mgr->parameter_itr.at(id) += 1;
    }

    // パラメータの値を更新する関数
    void update() {
        while (mgr->parameter_itr.at(id) >= mgr->parameter_size.at(id)) {
            mgr->parameter_itr.at(id) -= mgr->parameter_size.at(id);
            if (id + 1 < mgr->parameter_itr.size()) {
                mgr->parameter_itr.at(id + 1) += 1;
            }
        }
        *ref = values.at(mgr->parameter_itr.at(id));
    }
};

#endif

/*
// sample code
int main(){
    int a=0;
    double b=0;
    ParameterManager mgr(0);
    Parameter<double> b_v(&mgr,&b,{0.1,0.2,0.3});
    Parameter<int> a_v(&mgr,&a,{1,2,3});
    a_v.set({1,2});
    int size=0;
    while (mgr.update()){
        fprintf(stdout,"%d %d %f\n",++size,a,b);
    }
    return 0;
}
*/


// old version
#ifndef _PARAMETER_MANAGER_HPP_
#define _PARAMETER_MANAGER_HPP_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <vector>
#include <numeric>
#include <functional>
#include "utility.hpp"

// ParameterManagerクラスは複数のパラメータを管理し、それらの組み合わせを更新する機能を提供します。
class ParameterManager {
public:
    size_t ParameterID; // 現在のパラメータID
    size_t ParameterProgress; // 現在の進行度
    size_t ParameterTotal; // 全パラメータの組み合わせ数
    size_t clear_size; // 表示した文字列の長さ
    pthread_mutex_t mutex;
    StringBuffer buffer;
    std::string SetName;
    TimeManager timer;
    std::vector<size_t> parameter_itr; // 各パラメータの現在のインデックス
    std::vector<size_t> parameter_size; // 各パラメータの値の数
    std::vector<std::function<void()>> functions; // パラメータ更新関数のリスト

    // コンストラクタ
    ParameterManager(size_t prg) : ParameterID(0), ParameterProgress(prg), ParameterTotal(1) {
        SetName="";
        pthread_mutex_init(&mutex,NULL);
    }
    // 指定された値でパラメータインデックスを設定する関数
    void set() {
        if (parameter_itr.size() > 0) {
            parameter_itr.at(0) = ParameterProgress;
        }
    }
    // 全パラメータの組み合わせを順次更新する関数．全てのパラメータを終えたらfalseを返す．最初の一回は設定するのみ．
    bool update(){
        if (ParameterProgress > 0) {
            for (auto& func : functions) {
                func();
            }
        }
        ParameterProgress += 1;
        print();
        if(ParameterProgress <= ParameterTotal){
            return true;
        }
        return false;
    }
    void print() {
        if (ParameterProgress <= ParameterTotal) {
            buffer.clear();
            buffer.insert("%s  Parameters : ",SetName.c_str());
            for (size_t i = parameter_itr.size() - 1; i < parameter_itr.size(); --i) {
                buffer.insert("[%lu/%lu]", parameter_itr.at(i) + 1, parameter_size.at(i));
            }
            buffer.insert("  Total : %u/%u  %.2f%%  ", ParameterProgress, ParameterTotal, (double)ParameterProgress / ParameterTotal * 100.0);
            double elapsed_sec=timer.get_elapsed_time_sec();
            double remain_sec=timer.get_remain_time_sec((double)ParameterProgress / ParameterTotal);
            buffer.insert("  elapsed : %d:%02d:%02d  remain : %d:%02d:%02d ",
                           static_cast<int>(elapsed_sec)/3600,static_cast<int>(elapsed_sec)/60%60,static_cast<int>(elapsed_sec)%60,
                           static_cast<int>(remain_sec)/3600,static_cast<int>(remain_sec)/60%60,static_cast<int>(remain_sec)%60);
        }
    }
    void lock(){
        pthread_mutex_lock(&mutex);
    }
    void unlock(){
        pthread_mutex_unlock(&mutex);
    }
};

// Parameterクラスは特定の型のパラメータを管理し、ParameterManagerを通じてその組み合わせを更新します。
template<typename T>
class Parameter {
public:
    ParameterManager *mgr; // パラメータマネージャのポインタ
    T *ref; // パラメータの参照
    std::vector<T> values; // パラメータの値のリスト
    size_t id; // パラメータのID

    Parameter(ParameterManager *manager, T *ref) : mgr(manager), ref(ref) {}

    // コンストラクタ：パラメータの値を直接受け取る場合
    Parameter(ParameterManager *manager, T *ref, const std::vector<T> values) : mgr(manager), ref(ref), values(values) {
        initialize();
    }

    // コンストラクタ：パラメータの値を範囲で指定する場合
    Parameter(ParameterManager *manager, T *ref, int start, int end) : mgr(manager), ref(ref) {
        values.resize(end - start + 1);
        std::iota(values.begin(), values.end(), start);
        initialize();
    }

    // パラメータを初期化する関数
    void initialize() {
        //id = mgr->parameter_itr.size();
        id = mgr->ParameterID++;
        mgr->parameter_itr.push_back(0);
        mgr->parameter_size.push_back(values.size());
        *ref = values.at(0);
        if (mgr->functions.empty()) {
            mgr->functions.push_back([this]() { this->update_itr(); });
            if (mgr->ParameterProgress > 0) {
                mgr->parameter_itr.at(0) = mgr->ParameterProgress - 1;
            }
        }
        mgr->functions.push_back([this]() { this->update(); });
        mgr->ParameterTotal *= values.size();
    }

    void set(const std::vector<T> init_values){
        values=init_values;
        initialize();
    }

    // パラメータインデックスを更新する関数
    void update_itr() {
        mgr->parameter_itr.at(id) += 1;
    }

    // パラメータの値を更新する関数
    void update() {
        while (mgr->parameter_itr.at(id) >= mgr->parameter_size.at(id)) {
            mgr->parameter_itr.at(id) -= mgr->parameter_size.at(id);
            if (id + 1 < mgr->parameter_itr.size()) {
                mgr->parameter_itr.at(id + 1) += 1;
            }
        }
        *ref = values.at(mgr->parameter_itr.at(id));
    }
};

#endif