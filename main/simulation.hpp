#ifndef SIMULATION_HPP_
#define SIMULATION_HPP_

#define USE_PROGRESS_BAR

#include <stdio.h>
#include <stdlib.h>
#include <random>
#include <vector>

#include "parameters.hpp"
#include "point.h"
#include "map_enterable.hpp"
#include "floatingInformation.hpp"
#include "node.hpp"
#include "../utility/fieldAccessor.hpp"
#include "../utility/logger.h"
#include "../utility/progress.hpp"

std::uniform_real_distribution<> dist(0,1); // to transform randomly generated values into a uniform distribution

enum FLAGS{ // シミュレーションで扱うフラグ
    MALICIOUS_STOP_FLAG, // to manage malicious node use stop
    TOTAL_FLAG_NUM // the number of flags
};

/**
 * @brief IF simulation class
 */
class Simulation{
private:
public:
    std::mt19937 rng;  // Mersenne Twister乱数生成器
    int seed;
    bool progress_ber_on;
    FILE *record_file;
    double current_time; // simulation current time
    double skipped_time; // simulation skiped time :  simulation time = (current_time-skipped time)
    double fixed_lambda;
    Params params; // パラメータ
    Checker checker; // ノードの状態についての記録クラス
    int node_id_counter; // ノードのid指定
    std::vector<Node> nodes; // ノードを保存するベクタ
    size_t nodes_active_size,nodes_size; // ノードの数，ノードの保存容量
    MapEnterable map; // ノードの移動するマップ
    std::vector<FloatingInformation*> informations; // simulation内で扱われている情報へのポインタ．
    FloatingInformation *base_information=nullptr;
    FieldAccessor FA; // field accessor
    std::vector<int> node_counter; // 各状態のnodeの数をカウントする
    std::vector<std::set<int>> node_erase_state_counter; // 退出時のノードの信用状態毎のカウンタ
    
    int malicious_broadcast_counter=0;
    /**
     * simulationで一度しか起きない事象について，発生の有無を確認するためのflags 0:未,1:既
     * 0: 攻撃者が一定時間停止して配信する場合，停止の有無． */
    bool flags[TOTAL_FLAG_NUM]={0};

    size_t RA_entered_nodes_at_Atk_appear=0;
    std::set<int> RA_entered_nodes; // RAに侵入したノードのカウンタ

    std::vector<double> TA_entered_time; // TAに進入したノードの進入時刻リスト
    std::set<int> TA_entered_nodes; // TAに進入したノードのカウンタ

    toString setting_information; // パラメータの他に，記録すべき要素をここに挿入(ex:event)

    std::vector<int> IF_join_nodes; // IFに参加したノードのidを保持;
    GlobalStore<EventDataReceive> GS; // 受信データの蓄積


    Simulation() = default; // FAできない
    Simulation(int sd,bool use_progress_bar,FILE *recorder,Params *parameter_setting);
    ~Simulation() noexcept = default;
    Simulation(const Simulation& other) = default;
    Simulation(Simulation&& other) noexcept = default;
    Simulation & operator=(const Simulation& other) = default;
    void initialize(Params *parameter_setting);
    void set_base_information(FloatingInformation *information);
    void reset(int sd);
    void reset_record();
    void set_information(FloatingInformation *information);
    void add_node(Node new_node);
    void optional_appear(Node::NODE_TYPE type,FloatingInformation *information,double velocity);
    void optional_appear_with_point(Point start,Point goal,Node::NODE_TYPE type,FloatingInformation *information,double velocity);
    void fixed_source_appear(FloatingInformation *information,double position_x,double position_y);
    void node_erase(Node &n); // 退出するノードについての処理
    void appear();
    void remove();
    void move();
    void observe(); // nodeの観測：情報の真偽の記録
    void broadcast();
    void check();
    void update();
    void skip(int max_loop);
    void run(int max_loop);
    void write(FILE *fp);
};

/**
 * @brief シミュレーターの生成
 * @param sd シミュレーションのseed値
 * @param use_progress_bar シミュレーションの進捗状態の表示設定
 * @param recorder シミュレーション内部記録の出力先
 * @param parameter_setting パラメータ設定
 */
Simulation::Simulation(int sd,bool use_progress_bar,FILE *recorder,Params *parameter_setting){
    seed=sd;
    progress_ber_on=use_progress_bar;
    record_file=recorder;
    initialize(parameter_setting);
}

/**
 * @brief シミュレーターの初期化
 * @param パラメータ設定
 */
void Simulation::initialize(Params *parameter_setting){
    rng.seed(seed); // seed設定
    LOGGER_DATA_PRINT(DEBUG,"set seed : %d",seed);
    params=*parameter_setting;
    params.set_params();
    current_time=0.0;
    skipped_time=0.0;
    fixed_lambda=-1.0/params.Lambda;
    for(int i=0;i<TOTAL_FLAG_NUM;++i){
        flags[i]=0;
    }
    Movement::set_delta_t(Dt); // 動作に時間単位設定
    nodes.clear();
    node_id_counter=0;
    nodes_size=0;
    nodes_active_size=0;
    double x_shift=(params.Nxy.first==0) ? params.XL/2.0 : 0.0;
    double y_shift=(params.Nxy.second==0) ? params.YL/2.0 : 0.0;

    // map生成
    map.create_enterable_grid_map(rng,dist,params.Nxy.first,params.Nxy.second,params.XL,params.YL,x_shift,y_shift,params.EL,params.Lambda);
    if(use_diff_lambda){
        map.reset_enter_time(rng,dist,LEFT,params.Lambda_LEFT);
        map.reset_enter_time(rng,dist,RIGHT,params.Lambda_RIGHT);
    }
    map.set_routing(static_cast<RoutingType>(g_route),4,params.Pt,rng,dist);
    // print map data
    /*
    for(EdgePoint ep : map.points){
        for(int i=0;i<4;++i){
            for(size_t j=0;j<ep.routing.table.at(i).size();++j){
                fprintf(stdout,"%s",getString(" ","\n",
                "enter",i,"table",j,"route",
                ep.routing.table.at(i).at(j).at(0),
                ep.routing.table.at(i).at(j).at(1),
                ep.routing.table.at(i).at(j).at(2),
                ep.routing.table.at(i).at(j).at(3)
                ).c_str());
            }
        }
    }
     */
    node_counter.resize(8); // n(=8)個の種類のノードに分類して，保存する
    node_erase_state_counter.resize(8);
    FA.initialize(params.XL+params.EL*0,params.YL+params.EL*0,params.Cr,params.Cr); // field accessorの初期化
    // 内部記録ヘッダー設定
    if(record_file!=NULL){
        fprintf(record_file,"%s\n",params.get_parameter_string().c_str());
        fprintf(record_file,"%s",getString(",","\n","current_time",0,1,2,3,4,5,6,7,"inRAnodesCounter","inRA").c_str());
    }
}

/**
 * @brief 元の情報の設定
 * @param information 元の情報
 */
void Simulation::set_base_information(FloatingInformation *information){
    base_information=information;
}

/**
 * @brief シミュレーションのリセット
 * @param sd seed値
 */
void Simulation::reset(int sd){
    seed=sd;
    srand(seed);
    node_id_counter=0;
    nodes.clear();
    nodes_size=0;
    current_time=0.0;
    nodes_active_size=0;
}

/**
 * @brief シミュレーション内部のカウンタのリセット
 * @details 空回し後に，不要なカウント要素をリセットする
 */
void Simulation::reset_record(){
    // edgeのカウンタをリセット
    for(std::vector<Edge> ve : map.map){
        for(Edge e : ve){
            e.counter=0;
        }
    }
    RA_entered_nodes.clear();
    RA_entered_nodes_at_Atk_appear=0;
    TA_entered_nodes.clear();
    TA_entered_time.clear();
}

/**
 * @brief 情報の追加
 * @param information 追加する情報
 * @details 追加した情報を元に，RAなどマップ情報の更新を行う．
 */
void Simulation::set_information(FloatingInformation *information){
    for(FloatingInformation *p : informations){
        if(p==information){ // 既に保存されている場合は終了
            LOGGER_DATA_PRINT(INFO, "same information(%p) call set_information", (void*)information);
            return;
        }
    }
    // 1211
    information->shareGS = &GS;
    information->IF_join_nodes = &IF_join_nodes;
    
    // 1106
    information->covered_road_length=calculate_crossing_length(information->TA,params.XL,params.YL,params.Nxy.first,params.Nxy.second);
    LOGGER_DATA_PRINT(INFO,"info:%d cvl:%f",information->id,information->covered_road_length);
    int add_size=1<<informations.size();
    informations.push_back(information);
    map.copy_routing();
    if(information->RA.size()>0){
        for(size_t point_i=0,points_size=map.points.size();point_i<points_size;++point_i){
            EdgePoint &p=map.points.at(point_i);
            if(p.connection_counter!=4)continue;
            for(int enter_edge=0;enter_edge<4;++enter_edge){
                for(size_t table=add_size*4,table_size=p.routing.table.at(enter_edge).size();table<table_size;++table){
                    std::vector<double> &route_table=p.routing.table.at(enter_edge).at(table);
                    for(int i=0;i<4;++i){
                        for(Area ra : information->RA){
                            //if(onAreaByPoint(ra,p.connection.at(i)->point)){ // in なら含まない，onなら含む
                            if(isCrossOnArea(ra,p.point,p.connection.at(i)->point)){
                                if(route_table.at(i)>=0){ // もし，行き先がRA内で，選択する可能性があるなら
                                    LOGGER_DATA_PRINT(INFO,"Route table Banned enter:%d table:%ld i:%d pb:%.2f pos %.2f,%.2f edge(id):%d->%d",enter_edge,table,i,route_table.at(i),p.point.x,p.point.y,p.id,p.connection.at(i)->id);
                                    route_table.at(i)=-1; // banned
                                    double rate_sum=0.0;
                                    int banned_count=1;
                                    for(int j=1;j<4;++j){
                                        if(route_table.at((i+j)%4)>0){
                                            rate_sum+=route_table.at((i+j)%4);
                                        }else{
                                            ++banned_count;
                                        }
                                    }
                                    if(rate_sum>0){
                                        for(int j=1;j<4;++j){
                                            if(route_table.at((i+j)%4)>=0){
                                                route_table.at((i+j)%4)=route_table.at((i+j)%4)/rate_sum;
                                            }
                                        }
                                    }else{
                                        LOGGER_DATA_PRINT(INFO,"Route Table pb sum is 0  enter:%d table:%ld i:%d",enter_edge,table,i);
                                        if(0){
                                            for(int j=1;j<4;++j){
                                                if(route_table.at((i+j)%4)>=0){
                                                    route_table.at((i+j)%4)=1.0/(double)(4.0-banned_count);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

/**
 * @brief ノードの追加
 * @param new_node 追加するノード
 */
void Simulation::add_node(Node new_node){
    if(nodes_active_size<nodes_size){
        nodes.at(nodes_active_size)=new_node;
    }else{
        nodes.push_back(new_node);
        ++nodes_size;
    }
    ++nodes_active_size;
}

/**
 * @brief 退出するノードに対する処理
 * @details 最終的に利用していた情報を記録する
 */
void Simulation::node_erase(Node &n){
    node_erase_state_counter.at(n.md.get_trust_state()).insert(n.id);
}

/**
 * @brief ノードの出現
 * @details 各端点について，出現時刻<=現在時刻であればノードを出現させる．
 */
void Simulation::appear(){
    for(size_t i=0,size=map.enterable_points.size();i<size;++i){
        EnterablePoint &enter_point = map.enterable_points.at(i);
        while(enter_point.enterable && enter_point.poisson_enter_time<current_time){
            Edge &edge=map.map.at(enter_point.point_id).at(0);
            double turn_left=0.5; // 右と左の方向について，左を選ぶ確率，　0.5->左:右=1:1
            if(use_diff_lambda){
                if(edge.direction==UP)turn_left=params.Lambda_LEFT/(params.Lambda_LEFT+params.Lambda_RIGHT);
                if(edge.direction==DOWN)turn_left=params.Lambda_RIGHT/(params.Lambda_LEFT+params.Lambda_RIGHT);
            }
            Node new_node(node_id_counter++,params.Method,&edge,&map,current_time-enter_point.poisson_enter_time,CNV_UNIFORM(dist(rng),params.Vm-params.Vw,params.Vm+params.Vw),rng,dist,turn_left);
            //LOGGER_DATA_PRINT(INFO,"call rng at %s %d",__FILE__,__LINE__);

            add_node(new_node);
            enter_point.update(rng,dist); // もし，ノードの出現について修正するならここ
        }
    }
}

void Simulation::remove(){
    for(size_t i=0;i<nodes_active_size;++i){
        Node &node = nodes.at(i);
        if(node.mv.arrive){
            node_erase(node);
            nodes[i]=nodes[nodes_active_size-1];
            --nodes_active_size;
            --i;
        }
    }
}

/**
 * @brief 特殊なノードの出現(最初に情報を持ったノード，攻撃者)
 * @param type ノードのタイプ，NORMAL or MALICIOUS
 * @param information ノードがもつ情報
 * @param velocity ノードの速度
 */
void Simulation::optional_appear(Node::NODE_TYPE type,FloatingInformation *information,double velocity){
    size_t enter_point_itr=0; // y方向のノードが出現する端点の通し番号
    if(params.Nxy.first>0){ // 二次元のマップの場合
        for(size_t i=0,size=map.points.size();i<size;++i){
            EdgePoint &p=map.points.at(i);
            // y方向のエリア中点により近いものを選択．
            if(fabs(p.point.y-params.YL*0.5)<fabs(map.points.at(enter_point_itr).point.y-params.YL*0.5)){
                enter_point_itr=i;
            }
            // x方向の0により近いものを選択．
            if(p.point.x<map.points.at(enter_point_itr).point.x){
                enter_point_itr=i;
            }
        }
    }else{ // 一次元のマップの場合
        for(size_t i=0,size=map.points.size();i<size;++i){
            EdgePoint &p=map.points.at(i);
            if(p.point.y<map.points.at(enter_point_itr).point.y){
                enter_point_itr=i;
            }
        }
    }
    Edge enter_edge=map.map.at(enter_point_itr).at(0);
    Node new_node(node_id_counter++,params.Method,&enter_edge,&map,0.0,velocity,rng,dist,0.0);
    new_node.type=type;
    new_node.md.set_trust_information(information,current_time);
    add_node(new_node);
    //set_information(information);
    if(type==Node::MALICIOUS && RA_entered_nodes_at_Atk_appear==0)RA_entered_nodes_at_Atk_appear=RA_entered_nodes.size();
    LOGGER_DATA_PRINT(INFO,"add information node %d  info id:%d content:%d",new_node.id,information->id,information->info);
}

/**
 * @brief 始点と終点を決めた，特殊なノードの出現
 * @param start ノードの始点
 * @param goal ノードの終点
 * @param type ノードのタイプ NORMAL or MALICIOUS
 * @param information ノードがもつ情報
 * @param velocity ノードの速度
 */
void Simulation::optional_appear_with_point(Point start,Point goal,Node::NODE_TYPE type,FloatingInformation *information,double velocity){
    Node new_node(node_id_counter++,params.Method,start,goal,0.0,velocity,0.0,0.0);
    new_node.type=type;
    new_node.md.set_trust_information(information,current_time);
    add_node(new_node);
    //set_information(information);
    if(type==Node::MALICIOUS && RA_entered_nodes_at_Atk_appear==0)RA_entered_nodes_at_Atk_appear=RA_entered_nodes.size();
    LOGGER_DATA_PRINT(INFO,"add information node %d  info id:%d content:%d",new_node.id,information->id,information->info);
    broadcast();
    check();
}

/**
 * @brief 固定情報源(FS)の設置
 * @param information FSが配信する情報
 * @param position_x FSのx座標
 * @param position_y FSのy座標
 */
void Simulation::fixed_source_appear(FloatingInformation *information,double position_x,double position_y){
	Node new_node(node_id_counter++,params.Method,Node::FIXED_SOURCE,position_x,position_y);
	new_node.md.set_broadcast_information(information,current_time);
	add_node(new_node);
	//set_information(information);
    LOGGER_DATA_PRINT(INFO,"add information node %d  info id:%d content:%d",new_node.id,information->id,information->info);
    broadcast();
    check();
}

/**
 * @brief ノードの位置の更新
 */
void Simulation::move(){
    for(size_t i=0;i<nodes_active_size;++i){
        Node &node = nodes.at(i);
        if(node.type==Node::NORMAL){
            node.update(rng,dist);
        }else{
            // malicious behavior
            //if(flags[MALICIOUS_STOP_FLAG]==0 && (params.Attack==2 || params.Attack==3)){
            if(node.status==0 && (params.Attack==2 || params.Attack==3)){
                if(params.Attack==2){
                    node.update(rng,dist);
                    if(base_information->check_inTA(node.mv.position.x,node.mv.position.y)){
                        node.status=1; // 初めてTAに入った．ATTENTION：位置を補正しない．ただし，出現時刻が整数時刻でTAが整数で与えられれば，問題ないはず？
                        node.mv.set_stop_time(params.Slp);
                    }
                }else if(params.Attack==3){
                    double node_center_TA_distance = cal_point_distance(node.mv.position,{(params.TA_START_X+params.TA_END_X)*0.5,(params.TA_START_Y+params.TA_END_Y)*0.5});
                    Point pre_position = node.mv.position;
                    node.update(rng,dist);
                    double node_center_TA_distance_current = cal_point_distance(node.mv.position,{(params.TA_START_X+params.TA_END_X)*0.5,(params.TA_START_Y+params.TA_END_Y)*0.5});
                    if(node_center_TA_distance<node_center_TA_distance_current){
                        node.status=1;
                        node.mv.set_stop_time(params.Slp-Dt);
                        node.mv.position=pre_position;
                    }
                }else{
                    LOGGER_DATA_PRINT(WARNING,"simulation move unexpected  Attack:%d",params.Attack);
                    double stop_position=(params.Attack==2) ? params.TA_START_X : params.XL*0.5;
                    // 攻撃者がTAを横方向に直線的に移動することを想定
                    if(node.mv.position.x>=stop_position){
                        node.mv.set_stop_time(params.Slp-(node.mv.position.x-stop_position)/node.velocity);
                        node.mv.position.x=stop_position;
                        flags[MALICIOUS_STOP_FLAG]=1;
                    }
                }
            }else{
                node.update(rng,dist);
            }
        }
        if(node.mv.arrive){
            node_erase(node);
            //nodes[i]=nodes[nodes_active_size-1];
            for(size_t j=i;j<nodes_active_size-1;++j){nodes[j]=nodes[j+1];}
            --nodes_active_size;
            --i;
        }
    }
}

void Simulation::observe(){
    for(FloatingInformation* info : informations){
        for(Area ra : info->RA){
            for(size_t i=0;i<nodes_active_size;++i){
                Node *n = &nodes.at(i);
                if(onArea(ra,n->mv.position.x,n->mv.position.y) && (n->type==Node::NODE_TYPE::NORMAL)){
                    n->md.update_observe(n->id,info->id,info->fake_id>0,current_time);
                }
            }
        }
    }
}

/**
 * @brief ノードの情報配信
 */
void Simulation::broadcast(){
    FA.clear();
    for(size_t i=0;i<nodes_active_size;++i){
        Node *node = &nodes.at(i);
        FA.append(node->mv.position.x,node->mv.position.y,node);
        node->md.set_position(&node->mv.position);
        //node->md.user_position=&node->mv.position;
    }
    for(size_t i=0;i<nodes_active_size;++i){
        Node *from = &nodes.at(i);
        //from->md.user_position=&from->mv.position; // 宛先側のpositionを利用する場面に不適応のためコメントアウト
        from->check_position();
        if(from->type==Node::MALICIOUS && from->md.receive_time>0){// 攻撃者かつ情報受信後で改変情報を持つ
            if(malicious_fake_id>0)from->id=node_id_counter++; // idを偽装
            LOGGER_DATA_PRINT(DEBUG,"malicious node sendable:%d  id:%d",from->sendable,from->id);
        }
        if(from->sendable){
            if(from->type==Node::MALICIOUS){
                malicious_broadcast_counter+=1;
            }
            from->md.record_broadcast(from->id,current_time); // 配信時刻について記録
            checker.total_broadcast+=1;
            size_t itr=FA.get_itr(from->mv.position.x,from->mv.position.y);
            for(Node *to=(Node*)FA.access(itr);to!=NULL;to=(Node*)FA.access(itr)){
                if(from==to)continue;
                if(point_in_range(from->mv.position,to->mv.position,params.Cr)){
                    to->connection(&checker,from,current_time);
                }
            }
        }
    }
}

/**
 * @brief ノードの状態の観測
 */
void Simulation::check(){
    int in_RA_nodes=0;
    for(size_t i=0;i<node_counter.size();++i){
        node_counter.at(i)=0;
    }

    for(size_t i=0;i<nodes_active_size;++i){
        Node &n=nodes.at(i);
        node_counter.at(n.md.get_trust_state())+=1;
        if(ON_AREA(n.mv.position.x,n.mv.position.y,params.TA_START_X,params.TA_END_X,params.TA_START_Y,params.TA_END_Y)){
            if(TA_entered_nodes.find(n.id)==TA_entered_nodes.end()){
                TA_entered_nodes.insert(n.id);
                TA_entered_time.push_back(current_time);
            }
        }

        if(n.type==Node::NODE_TYPE::NORMAL && informations.size()>0 && informations.at(0)->RA.size()>0){
            for(Area ra : informations.at(0)->RA){
                //if(point_on_rectangle_area(n.mv.position,ra.first,ra.second)){
                if(onAreaByPoint(ra,n.mv.position)){
                    ++in_RA_nodes;
                    RA_entered_nodes.insert(n.id);
                    break;
                }
            }
        }
    }
    checker.max_true_node_count=MAX(checker.max_true_node_count,node_counter.at(1));
    checker.max_fake_node_count=MAX(checker.max_fake_node_count,node_counter.at(2));
    checker.total_node_in_ra=RA_entered_nodes.size();
    if(record_file!=NULL){
        fprintf(record_file,"%s",getString(",","\n",
            current_time,
            node_counter.at(0),
            node_counter.at(1),
            node_counter.at(2),
            node_counter.at(3),
            node_counter.at(4),
            node_counter.at(5),
            node_counter.at(6),
            node_counter.at(7),
            RA_entered_nodes.size(),
            in_RA_nodes
            ).c_str());
    }
}

/**
 * @brief シミュレーションの1step更新
 */
void Simulation::update(){
    current_time+=Dt;
    move();
    //observe(); // add 0704; RAはノードによる観測で，真偽を判定できないと仮定する．
    appear();
    broadcast();
    check();
}

/**
 * @brief シミュレーションのから回し
 * @param max_loop シミュレーションから回しstep数
 */
void Simulation::skip(int max_loop){
    current_time=Dt*(double)max_loop;
    appear();
    remove();
    skipped_time=current_time;
}

/**
 * @brief シミュレーションの実行
 * @param max_loop シミュレーション実行step数
 */
void Simulation::run(int max_loop){
    if(progress_ber_on){
        ProgressBar PB(max_loop,0.01,NULL);
        for(int loop=0;loop<max_loop;++loop){
            update();
            PB.update_progress_bar();
            PB.print_progress_bar();
        }
    }else{
        for(int loop=0;loop<max_loop;++loop){
            update();
        }
    }
}


/**
 * @brief シミュレーション結果の記録
 * @param fp 結果の記録先
 */
void Simulation::write(FILE* fp){
    checker.first_true_broadcast_time=informations.at(0)->first_broadcast_time;
    checker.first_true_receive_time=informations.at(0)->first_receive_time;
    checker.last_true_broadcast_time=informations.at(0)->last_broadcast_time;
    checker.last_true_receive_time=informations.at(0)->last_receive_time;
    if(informations.size()>1){
        checker.first_fake_broadcast_time=informations.at(1)->first_broadcast_time;
        checker.first_fake_receive_time=informations.at(1)->first_receive_time;
        checker.last_fake_broadcast_time=informations.at(1)->last_broadcast_time;
        checker.last_fake_receive_time=informations.at(1)->last_receive_time;
    }else{
        checker.first_fake_broadcast_time=0.0;
        checker.first_fake_receive_time=0.0;
        checker.last_fake_broadcast_time=0.0;
        checker.last_fake_receive_time=0.0;
    }

    std::vector<double> diff=cal_diff_double_vector(TA_entered_time);
    long int total_true_broadcast = 0;
    long int total_fake_broadcast = 0;
    long int true_trust_node = 0;
    long int fake_trust_node = 0;
    long int true_broadcast_node = 0;
    long int fake_broadcast_node = 0;
    for(FloatingInformation *info : informations){
        if(info->fake_id==0){
            total_true_broadcast += info->broadcast_counter;
            true_trust_node = info->trust_id_set.size();
            true_broadcast_node = info->broadcast_id_set.size();
        }else{
            total_fake_broadcast += info->broadcast_counter;
            fake_trust_node = info->trust_id_set.size(); // 攻撃者はtrustの更新処理を行わないため加算されない
            fake_broadcast_node = info->broadcast_id_set.size() - 1; // 攻撃者を除く
        }
    }
    checker.total_broadcast -= malicious_broadcast_counter;  // 攻撃者による配信回数を除く
    total_fake_broadcast -= malicious_broadcast_counter;

    if(fp!=NULL){
        if(seed==1){
            fprintf(fp,"%s_and_%s\n",params.get_parameter_string().c_str(),setting_information.result().c_str());
            fprintf(fp,"%s",
                getString(",","\n",
                "seed", // simulation seed value
                "Mtype", // method type value to separate
                "time", // simulation params.elapsed time
                "first_true_receive", // true information first receive time
                "first_fake_receive", // fake information first receive time
                "last_true_receive", // latest true information send time
                "last_fake_receive", // latest fake information send time
                "first_true_broadcast", // true information first receive time
                "first_fake_broadcast", // fake information first receive time
                "last_true_broadcast", // latest true information send time
                "last_fake_broadcast", // latest fake information send time
                "none", // number of node without info
                "true", // number of node trust true
                "fake", // number of node trust fake
                "double", // number of node use all
                "trueMax", // node trust true
                "fakeMax", // node trust fake
                "connect", // normal node connect to
                "broadcast", // normal node broadcast
                "malicious_broadcast", // malicious node broadcast
                "true_broadcast", // total true broadcast
                "fake_broadcast", // total fake broadcast
                "true_trust_node",
                "fake_trust_node",
                "true_broadcast_node",
                "fake_broadcast_node",
                "RAin", // number of node in RA
                "RAinWoAtk", // number of node in RA before attacker appear
                "TAin", // number of node in(through) TA
                "TAinTimeMean", //
                "TAinTimeStd" //
            ).c_str());
        }
        
        fprintf(fp,"%s",
            getString(",","\n",
            seed,
            params.Method,
            current_time,
            checker.first_true_receive_time,
            checker.first_fake_receive_time,
            checker.last_true_receive_time,
            checker.last_fake_receive_time,
            checker.first_true_broadcast_time,
            checker.first_fake_broadcast_time,
            checker.last_true_broadcast_time,
            checker.last_fake_broadcast_time,
            node_counter.at(0),
            node_counter.at(1),
            node_counter.at(2),
            node_counter.at(3),
            checker.max_true_node_count,
            checker.max_fake_node_count,
            checker.total_connection,
            checker.total_broadcast,
            malicious_broadcast_counter,
            total_true_broadcast,
            total_fake_broadcast,
            true_trust_node,
            fake_trust_node,
            true_broadcast_node,
            fake_broadcast_node,
            RA_entered_nodes.size(),
            RA_entered_nodes_at_Atk_appear,
            TA_entered_nodes.size(),
            cal_mean_double_vector(diff),
            cal_std_double_vector(diff)
        ).c_str());
        fflush(fp);
    }else{
        fprintf(stderr,"%s_and_%s\n",params.get_parameter_string().c_str(),setting_information.result().c_str());
        fprintf(stderr,"result\n%s",getString("","\n",
            getString("\t : ","\n",SHOW(seed)),
            getString("\t : ","\n",SHOW(params.Method)),
            getString("\t : ","\n",SHOW(params.Start)),
            getString("\t : ","\n",SHOW(params.Attack)),
            getString("\t : ","\n",SHOW(current_time)),
            getString("\t : ","\n",SHOW(checker.first_true_receive_time)),
            getString("\t : ","\n",SHOW(checker.first_fake_receive_time)),
            getString("\t : ","\n",SHOW(checker.last_true_receive_time)),
            getString("\t : ","\n",SHOW(checker.last_fake_receive_time)),
            getString("\t : ","\n",SHOW(checker.first_true_broadcast_time)),
            getString("\t : ","\n",SHOW(checker.first_fake_broadcast_time)),
            getString("\t : ","\n",SHOW(checker.last_true_broadcast_time)),
            getString("\t : ","\n",SHOW(checker.last_fake_broadcast_time)),
            getString("\t : ","\n",SHOW(node_counter.at(0))),
            getString("\t : ","\n",SHOW(node_counter.at(1))),
            getString("\t : ","\n",SHOW(node_counter.at(2))),
            getString("\t : ","\n",SHOW(checker.max_true_node_count)),
            getString("\t : ","\n",SHOW(checker.max_fake_node_count)),
            getString("\t : ","\n",SHOW(checker.total_connection)),
            getString("\t : ","\n",SHOW(checker.total_broadcast)),
            getString("\t : ","\n",SHOW(malicious_broadcast_counter)),
            getString("\t : ","\n",SHOW(checker.total_node_in_ra)),
            getString("\t : ","\n",SHOW(total_true_broadcast)),
            getString("\t : ","\n",SHOW(total_fake_broadcast)),
            getString("\t : ","\n",SHOW(true_trust_node)),
            getString("\t : ","\n",SHOW(fake_trust_node)),
            getString("\t : ","\n",SHOW(true_broadcast_node)),
            getString("\t : ","\n",SHOW(fake_broadcast_node)),
            getString("\t : ","\n",SHOW(node_erase_state_counter.at(0).size())),
            getString("\t : ","\n",SHOW(node_erase_state_counter.at(1).size())),
            getString("\t : ","\n",SHOW(node_erase_state_counter.at(2).size()))
            //getString("\t : ","\n",SHOW(TA_entered_nodes.size()))
        ).c_str());
    }
}

#endif