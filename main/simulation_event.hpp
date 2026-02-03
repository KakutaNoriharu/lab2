#ifndef EVENT_HPP_
#define EVENT_HPP_

#include <string>
#include <vector>
#include <functional>
#include <memory> // std::shared_ptr, std::make_unique
#include <algorithm> // std::sort のために必要
#include <iostream>

#include "simulation_plot.hpp"

/**
 * シミュレーションのループ内でのイベントを管理する
 * 
 * 構成
 * ・イベント時刻,int ex: time==loop_time;
 * ・イベント内容,str ex:"APPEAR_MALICIOUS";
 * ・イベントパラメータ,vector<str> ex:{"1","left","3000.0","300.0"};
 */

/**
 * 離散ステップシミュレーションにおけるイベントの基底クラス (BaseEvent)
 * time_step (int) に基づきイベントを発火させる
 */
class BaseEvent {
public:
    const int time_step; // イベントが発火するシミュレーションステップ数
    const std::string name;

    // コンストラクタ
    BaseEvent(int step, const std::string& event_name) 
        : time_step(step), name(event_name) {}

    // 派生クラスで必ず実装されるイベント処理ロジック
    // Simulationの参照を受け取り、シミュレーションの状態を更新する
    virtual void process(Simulation& sim) = 0;
    virtual std::string describe() = 0;
    virtual ~BaseEvent() = default;
};

/**
 * 具体的なイベントクラス例: マニュアル到着イベント (ArrivalEvent)
 * このイベントに固有のパラメータ (speed, 座標など) を持つ
 */
class ArrivalEventManual : public BaseEvent {
public:
    int information_id;
    Node::NODE_TYPE node_type;
    double speed;
    double start_x, start_y;
    double end_x, end_y;

    ArrivalEventManual(int step, int info_id, Node::NODE_TYPE n_type, double s, double sx, double sy, double ex, double ey)
        : BaseEvent(step, "ArrivalEventManual"), 
          information_id(info_id), node_type(n_type), speed(s), start_x(sx), start_y(sy), end_x(ex), end_y(ey) {}

    void process(Simulation& sim) override {
        FloatingInformation *info = nullptr;
        for(FloatingInformation *i : sim.informations){
            if(i->id==information_id){
                info=i;
                break;
            }
        }
        if(info==nullptr){
            printf("ArrivalEventManual Fail : information_id(%d) is not found",information_id);
            LOGGER_DATA_PRINT(ERROR,"ArrivalEventManual Fail : information_id(%d) is not found",information_id);
            exit(EXIT_FAILURE);
        }else{
            sim.optional_appear_with_point(new_point(start_x,start_y),new_point(end_x,end_y),node_type,info,speed);
        }
    }

    std::string describe() override {
        return getString(",","","[",name,time_step,information_id,node_type,speed,start_x,start_y,end_x,end_y,"]");
    }
};

/**
 * 具体的なイベントクラス例: マニュアル到着イベント (ArrivalEvent)
 * このイベントに固有のパラメータ (speed, 座標など) を持つ
 */
class ArrivalEvent : public BaseEvent {
public:
    int information_id;
    Node::NODE_TYPE node_type;

    ArrivalEvent(int step, int info_id, Node::NODE_TYPE n_type)
        : BaseEvent(step, "ArrivalEvent"), 
          information_id(info_id), node_type(n_type) {}

    void process(Simulation& sim) override {
    	double optional_appear_y=(sim.params.Nxy.second==0) ? sim.params.YL*0.5: sim.params.YL/(sim.params.Nxy.second)*(sim.params.Nxy.second/2);
        FloatingInformation *info = nullptr;
        for(FloatingInformation *i : sim.informations){
            if(i->id==information_id){
                info=i;
                break;
            }
        }
        if(info==nullptr){
            printf("ArrivalEvent Fail : information_id(%d) is not found",information_id);
            LOGGER_DATA_PRINT(ERROR,"ArrivalEvent Fail : information_id(%d) is not found",information_id);
            exit(EXIT_FAILURE);
        }else{
            sim.optional_appear_with_point(new_point(sim.params.TA_START_X-(node_type==Node::NORMAL ? 0 : sim.params.Cr),optional_appear_y),new_point(sim.params.XL,optional_appear_y),node_type,info,sim.params.fVm);
        }
    }
    std::string describe() override {
        return getString(",","","[",name,time_step,information_id,node_type,"]");
    }
};

/**
 * イベントの比較構造体 (EventComparator)
 * std::shared_ptr<BaseEvent> の中身 (time_step) を比較し、
 * time_stepが小さいイベントを優先（min-heapとして動作）させる
 */
struct EventComparator {
    bool operator()(const std::shared_ptr<BaseEvent>& a, const std::shared_ptr<BaseEvent>& b) const {
        // time_stepが小さい方が先に来るように < で比較 (昇順ソート用)
        return a->time_step < b->time_step; 
    }
};

// イベントスケジューラとして使用するイベント配列の型
using EventList = std::vector<std::shared_ptr<BaseEvent>>; 

EventList g_event_list;

/**
 * イベントをリストにスケジュールするヘルパー関数 (ソートは後で行う)
 */
void schedule_event(EventList& list, std::shared_ptr<BaseEvent> event) {
    if (event->time_step < 0) {
        std::cerr << "Error: Attempted to schedule event with negative time_step." << std::endl;
        return;
    }
    //std::cout << "[SETUP] Scheduled(" << list.size() << "): " << event->name << " at step " << event->time_step << std::endl;
    list.push_back(std::move(event));
}

/**
 * event_strの文字列を参照してイベントをキューにスケジュールする
 * event_strの形式: {time_step(int), EventName(string), Param1(str), Param2(str), ...}
 * 例: {"1", "ArrivalEvent", "10.0", "0.0", "0.0", "100.0", "50.0"}
 * -> time_step=1, name="ArrivalEvent", speed=10.0, start_x=0.0, ...
 */
void add_event_schedule(EventList& list, const std::vector<std::string>& event_str){
    if (event_str.size() < 2) {
        LOGGER_PRINT(WARNING,"[ERROR] Event string vector is too short (min 2 elements required: time, name).");
        return;
    }

    try {
        int event_str_itr = 0;
        int time_step = std::stoi(event_str[event_str_itr++]); // 0
        const std::string& event_name = event_str[event_str_itr++]; // 1
        std::shared_ptr<BaseEvent> new_event = nullptr;

        // 3. イベント名に応じた派生クラスの生成 (ファクトリロジック)
        if (event_name == "ArrivalEventManual"){
            if (event_str.size()!=9) {
                fprintf(stderr,"[ERROR] ArrivalEvent requires 7 parameters (total 9 elements) event_str size=%lu\n",event_str.size());
                LOGGER_DATA_PRINT(ERROR,"[ERROR] ArrivalEvent requires 7 parameters (total 9 elements) event_str size=%lu\n",event_str.size());
                exit(EXIT_FAILURE);
            }
            int info_id = std::stoi(event_str[event_str_itr++]);
            Node::NODE_TYPE node_type = static_cast<Node::NODE_TYPE>(std::stoi(event_str[event_str_itr++]));
            double speed = std::stod(event_str[event_str_itr++]);
            double start_x = std::stod(event_str[event_str_itr++]);
            double start_y = std::stod(event_str[event_str_itr++]);
            double end_x = std::stod(event_str[event_str_itr++]);
            double end_y = std::stod(event_str[event_str_itr++]);
            new_event = std::make_unique<ArrivalEventManual>(time_step, info_id, node_type, speed, start_x, start_y, end_x, end_y);
        
        }else if(event_name == "ArrivalEvent"){
            if (event_str.size()!=4) {
                fprintf(stderr,"[ERROR] ArrivalEvent requires 2 parameters (total 4 elements) event_str size=%lu\n",event_str.size());
                LOGGER_DATA_PRINT(ERROR,"[ERROR] ArrivalEvent requires 2 parameters (total 4 elements) event_str size=%lu\n",event_str.size());
                exit(EXIT_FAILURE);
            }
            int info_id = std::stoi(event_str[event_str_itr++]);
            Node::NODE_TYPE node_type = static_cast<Node::NODE_TYPE>(std::stoi(event_str[event_str_itr++]));
            new_event = std::make_unique<ArrivalEvent>(time_step, info_id, node_type);
        }else{
            fprintf(stderr, "[ERROR] Unknown event type: %s\n", event_name.c_str());
            LOGGER_DATA_PRINT(WARNING, "[ERROR] Unknown event type: %s", event_name.c_str());
            return;
        }
        if (new_event) { // キューへのスケジュール
            schedule_event(list, std::move(new_event));
        }

    } catch (const std::invalid_argument& e) { // std::stoi/std::stodが失敗した場合のエラー処理
        fprintf(stderr, "[ERROR] Invalid numeric format in event string: %s\n", e.what());
        LOGGER_DATA_PRINT(WARNING, "[ERROR] Invalid numeric format in event string: %s", e.what());
    } catch (const std::out_of_range& e) { // 変換後の値が範囲外であった場合のエラー処理
        fprintf(stderr, "[ERROR] Value out of range in event string: %s\n", e.what());
        LOGGER_DATA_PRINT(WARNING, "[ERROR] Value out of range in event string: %s", e.what());
    } catch (const std::exception& e) { // その他一般的なエラー
        fprintf(stderr, "[ERROR] An unexpected error occurred: %s\n", e.what());
        LOGGER_DATA_PRINT(WARNING, "[ERROR] An unexpected error occurred: %s", e.what());
    }
}

/**
 * イベントをロードし、EventListとして返却し、time_stepでソートする
 */
EventList load_event(ParameterLoader &loader){
    EventList event_list;
    std::vector<std::string> keys = loader.keys();
    if(std::find(keys.begin(),keys.end(),"EVENT")!=keys.end()){
        for(std::vector<std::vector<std::string>> str_vector : loader.getVectorOfVectors("EVENT")){
            for(std::vector<std::string> event_str : str_vector){
                add_event_schedule(event_list, event_str);
            }
        }
    }
    std::sort(event_list.begin(), event_list.end(), EventComparator());
    return event_list;
}

/**
 * 複数のイベントリストをロードし、g_event_queueを初期化する
 */
 void load_event_vector(ParameterManager *mgr, ParameterLoader &loader){
    std::vector<std::string> keys = loader.keys();
    if(std::find(keys.begin(),keys.end(),"EVENT")!=keys.end()){
        // shared_ptrのコンテナのコピーを防ぐため、ムーブセマンティクスを利用
        std::vector<EventList> pms;
        for(std::vector<std::vector<std::string>> str_vector : loader.getVectorOfVectors("EVENT")){
            EventList p;
            for(std::vector<std::string> event_str : str_vector){
                add_event_schedule(p, event_str);
            }
            std::sort(p.begin(), p.end(), EventComparator());
            pms.push_back(std::move(p)); 
        }
        if(pms.size()>1){
            parameter_id_map.push_back(std::pair<std::string,int>({"EVENT",mgr->ParameterID}));
            static Parameter<EventList> p_event_list(mgr,&g_event_list,pms);
        }else if(pms.size()==1){
            g_event_list = std::move(pms.at(0));
        }
    }
}

EventList make_base_event_list(){
    EventList event_list;
    add_event_schedule(event_list,{"0","ArrivalEvent","0","0"}); // first information carry node appear event
    return event_list;
}

EventList set_malicious_event(Params *params,EventList &event_list){
    if(params->malicious_appear_type==0){
        // malicious node not appear
    }else if(params->malicious_appear_type==1){ // appear normal;
    	double optional_appear_y=(params->Nxy.second==0) ? params->YL*0.5: params->YL/(params->Nxy.second)*(params->Nxy.second/2);
        for(int i=0;i<params->malicious_appear_num;++i){
            std::shared_ptr<BaseEvent> new_event = nullptr;
            new_event = std::make_unique<ArrivalEventManual>(
                params->malicious_appear_time+params->malicious_appear_interval*i, 1, Node::MALICIOUS, 
                params->fVm, params->TA_START_X-params->Cr, optional_appear_y, params->XL, optional_appear_y);
            schedule_event(event_list, std::move(new_event));
        }
    }else if(params->malicious_appear_type==2){ // 格子道路の全ての端点から出現 , 一次元の場合に未対応, unverified
        for(int i=0;i<params->malicious_appear_num;++i){
            if(params->Nxy.first==0){
                double x=params->XL/2;
                std::shared_ptr<BaseEvent> new_event_left = nullptr;
                new_event_left = std::make_unique<ArrivalEventManual>(
                    params->malicious_appear_time+params->malicious_appear_interval*i, 1, Node::MALICIOUS, 
                    params->fVm, x, 0.0, x, params->YL);
                schedule_event(event_list, std::move(new_event_left));
                std::shared_ptr<BaseEvent> new_event_right = nullptr;
                new_event_right = std::make_unique<ArrivalEventManual>(
                    params->malicious_appear_time+params->malicious_appear_interval*i, 1, Node::MALICIOUS, 
                    params->fVm, x, params->YL, x, 0.0);
                schedule_event(event_list, std::move(new_event_right));
            }else if(params->Nxy.second==0){
                double y=params->YL/2;
                std::shared_ptr<BaseEvent> new_event_bottom = nullptr;
                new_event_bottom = std::make_unique<ArrivalEventManual>(
                    params->malicious_appear_time+params->malicious_appear_interval*i, 1, Node::MALICIOUS, 
                    params->fVm, 0.0, y, params->XL, y);
                schedule_event(event_list, std::move(new_event_bottom));
                std::shared_ptr<BaseEvent> new_event_top = nullptr;
                new_event_top = std::make_unique<ArrivalEventManual>(
                    params->malicious_appear_time+params->malicious_appear_interval*i, 1, Node::MALICIOUS, 
                    params->fVm, params->XL, y, 0.0, y);
                schedule_event(event_list, std::move(new_event_top));
            }else{
                for(int xp=0;xp<=params->Nxy.first;++xp){ // x->  <-x
                    double x=params->XL/(params->Nxy.first)*(xp);
                    std::shared_ptr<BaseEvent> new_event_left = nullptr;
                    new_event_left = std::make_unique<ArrivalEventManual>(
                        params->malicious_appear_time+params->malicious_appear_interval*i, 1, Node::MALICIOUS, 
                        params->fVm, x, 0.0, x, params->YL);
                    schedule_event(event_list, std::move(new_event_left));
                    std::shared_ptr<BaseEvent> new_event_right = nullptr;
                    new_event_right = std::make_unique<ArrivalEventManual>(
                        params->malicious_appear_time+params->malicious_appear_interval*i, 1, Node::MALICIOUS, 
                        params->fVm, x, params->YL, x, 0.0);
                    schedule_event(event_list, std::move(new_event_right));
                }
                for(int yp=0;yp<=params->Nxy.second;++yp){ // x->  <-x
                    double y=params->YL/(params->Nxy.second)*(yp);
                    std::shared_ptr<BaseEvent> new_event_bottom = nullptr;
                    new_event_bottom = std::make_unique<ArrivalEventManual>(
                        params->malicious_appear_time+params->malicious_appear_interval*i, 1, Node::MALICIOUS, 
                        params->fVm, 0.0, y, params->XL, y);
                    schedule_event(event_list, std::move(new_event_bottom));
                    std::shared_ptr<BaseEvent> new_event_top = nullptr;
                    new_event_top = std::make_unique<ArrivalEventManual>(
                        params->malicious_appear_time+params->malicious_appear_interval*i, 1, Node::MALICIOUS, 
                        params->fVm, params->XL, y, 0.0, y);
                    schedule_event(event_list, std::move(new_event_top));
                }
            }
        }

        // future work
    }
    std::sort(event_list.begin(), event_list.end(), EventComparator());
    return event_list;
}

void call_event(SimulationPlot &sim,int current_loop,const EventList &event_list,size_t &current_event_index){
    while(current_event_index < event_list.size() && event_list[current_event_index].get()->time_step <= current_loop){    
        // shared_ptrの指すBaseEventオブジェクトへのconstポインタを取得
        const BaseEvent *next_event = event_list[current_event_index].get();
        LOGGER_DATA_PRINT(INFO,"  [EVENT] Time=%d(loop=%d), Name: %s",next_event->time_step,current_loop,next_event->name.c_str());
        //printf("[EVENT] Time=%d(loop=%d), Name: %s\n",next_event->time_step,current_loop,next_event->name.c_str());
        // processはsimの状態を変更するため、const_castで一時的にconst性を解除して呼び出す
        const_cast<BaseEvent*>(next_event)->process(sim);
        current_event_index++;
    }
}

void simulate_run_with_schedule(SimulationPlot &sim,int max_loop,const EventList &event_list){
    size_t current_event_index = 0;
    sim.setting_information.append("{EventList:[");
    for(auto e : event_list){
        sim.setting_information.append(e.get()->describe().c_str());
    }
    sim.setting_information.append("]}");
    if(sim.progress_ber_on){
        ProgressBar PB(max_loop,0.0001,NULL);
        for(int loop=0;loop<max_loop;++loop){
            call_event(sim,loop,event_list,current_event_index);
            sim.update();
            if((sim.gnuplot_use_type>0) && loop%20==0 && sim.current_time>10000.0)sim.plot_data(loop);
            PB.update_progress_bar();
            PB.print_progress_bar();
        }
    }else{
        for(int loop=0;loop<max_loop;++loop){
            call_event(sim,loop,event_list,current_event_index);
            sim.update();
            if((sim.gnuplot_use_type>0) && loop%20==0 && sim.current_time>10000.0)sim.plot_data(loop);
        }
    }
    //LOGGER_DATA_PRINT(INFO,"ct:%f rv:%f",sim.current_time,dist(sim.rng));
}

#endif