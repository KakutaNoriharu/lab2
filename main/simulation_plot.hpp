#ifndef SIMULATION_PLOT_HPP_
#define SIMULATION_PLOT_HPP_

#include "../utility/gnuplot.h"
#include "simulation.hpp"

/**
 * @brief IFのシミュレーションに，gnuplotによるプロット機能を加えたもの
 */
class SimulationPlot : public Simulation{
private:
public:
    int gnuplot_use_type; // 0 : no gnuplot, 1 : make gif, 2 : make script, 3 : make frame png 
    double plot_length_x,plot_length_y;
    std::string frame_folder;
    FILE *fp;
    SimulationPlot(int sd,int plot_type,bool use_progress_bar,FILE *recorder,Params *parameter_setting);
    ~SimulationPlot() noexcept = default;
    SimulationPlot(const SimulationPlot& other) = default;
    SimulationPlot(SimulationPlot&& other) noexcept = default;
    SimulationPlot & operator=(const SimulationPlot& other) = default;
    void initial_plot();
    void plot_data(int loop);
    void run(int max_loop);
    void close();
};

/**
 * @brief シミュレーションの生成
 * @param sd seed値
 * @param plot_type gnuplotの出力設定 0:何もしない 1:gnuplotに出力 2:gnuplotスクリプトの出力
 * @param use_progress_bar 進捗状況の表示有無
 * @param recorder 内部記録の保存先
 * @param parameter_setting パラメータ設定
 */
SimulationPlot::SimulationPlot(int sd,int plot_type,bool use_progress_bar,FILE *recorder,Params *parameter_setting){
    seed=sd;
    progress_ber_on=use_progress_bar;
    record_file=recorder;
    initialize(parameter_setting);
    gnuplot_use_type=plot_type;
    if(plot_type>0){
        plotInit(plot_type%2);
    }
    if(gnuplot_use_type>0){
        fp=fopen("gnuplot_tmp_data_file.txt","w");
        double max_point_x=-DBL_MAX;
        double max_point_y=-DBL_MAX;
        double min_point_x=DBL_MAX;
        double min_point_y=DBL_MAX;
        for(size_t i=0,size=map.points.size();i<size;++i){
            EdgePoint &point=map.points.at(i);
            if(point.point.x>max_point_x)max_point_x=point.point.x;
            if(point.point.y>max_point_y)max_point_y=point.point.y;
            if(point.point.x<min_point_x)min_point_x=point.point.x;
            if(point.point.y<min_point_y)min_point_y=point.point.y;
        }
        plot_length_x=(max_point_x+params.XL*0.2)-(min_point_x-params.XL*0.2);
        plot_length_y=(max_point_y+params.YL*0.2)-(min_point_y-params.YL*0.2);
        double TAsize_x = (params.vTA.size()>0) ? params.vTA.at(0).at(2) : 0.0;
        double TAsize_y = (params.vTA.size()>0) ? params.vTA.at(0).at(3) : 0.0;
        double fTAsize_x = (params.vfTA.size()>0) ? params.vfTA.at(0).at(2) : 0.0;
        double fTAsize_y = (params.vfTA.size()>0) ? params.vfTA.at(0).at(3) : 0.0;

        if(gnuplot_use_type==1){
            plotSetOutputGif(5,800,1000);
            std::string gnuplot_output=getString("_",".gif","z_gif/gif",sd,params.Nxy.first,params.Nxy.second,params.Lambda,params.Vm,params.Vw,params.fVm,params.Slp,params.Cr,TAsize_x,TAsize_y,fTAsize_x,fTAsize_y,params.Method);
            char gnuplot_output_file_name[255];
            make_new_file_name(gnuplot_output.c_str(),gnuplot_output_file_name,255);
            fprintf(stderr,"make gif name:%s\n",gnuplot_output_file_name);
            make_file_directory(gnuplot_output_file_name);
            PLOT("set output '%s'\n",gnuplot_output_file_name);
        }else if(gnuplot_use_type==3){ // make frame png  この仕様は将来的に使わなくなると思う．現状で動くかも未検証 unverified
            PLOT("set terminal pngcairo size 800, 1000 enhanced\n");
            frame_folder=getString("_","","plots/",sd,params.Nxy.first,params.Nxy.second,params.Lambda,params.Vm,params.Vw,params.fVm,params.Slp,params.Cr,TAsize_x,TAsize_y,fTAsize_x,fTAsize_y,params.Method,"/");
            fprintf(stderr,"make gif name:%s\n",frame_folder.c_str());
            make_file_directory(frame_folder.c_str());
        }

        plotAddFig(0.0,0.0,1.0,0.8); // モデルプロットスペース
        plotInitSetRange(-params.EL,params.XL+params.EL,-params.EL,params.YL+params.EL);
        //plotSetRange(min_point_x-XL*0.2,max_point_x+XL*0.2,min_point_y-YL*0.2,max_point_y+YL*0.2);
        initial_plot();

        plotAddFig(0.0,0.8,1.0,0.2); // ノード数推移描画スペース
        plotInitSetTics("","");
    }
}

void SimulationPlot::initial_plot(){
    plotCallFig(0);
    for(auto edge_from : map.map){
        for(auto edge : edge_from){
            plotInitArrow("nohead",edge.from->point.x,edge.from->point.y,edge.to->point.x,edge.to->point.y,"black",1);
            //plotLabelConst(getString("","",edge.direction).c_str(),(edge.from->point.x*1.1+edge.to->point.x*0.9)*0.5,(edge.from->point.y*1.1+edge.to->point.y*0.9)*0.5,0);
        }
    }
    for(auto point : map.points){
        //plotCircle(point.point.x,point.point.y,0.1,0.1);
        //plotInitLabelConst(getString("","",point.id).c_str(),point.point.x+plot_length_x*0.02,point.point.y-plot_length_y*0.02,0);
    }
    // if plot always TA or RA, use plotInitXXX here
}

void SimulationPlot::plot_data(int loop){
    // PNG出力設定
    if(gnuplot_use_type==3){
        PLOT("set output '%s/%08d.png'\n", frame_folder.c_str(), loop);
    }
    std::vector<int> hist;
    std::vector<int> cnt;
    cnt.resize(20,0);
    for(unsigned int i=0;i<nodes_active_size;++i){
        Node &n=nodes.at(i);
        cnt.at(n.md.get_trust_state())+=1;
    }

    fprintf(fp,"%s",
        getString(", ","\n",
            current_time,
            cnt.at(0),
            cnt.at(1),
            cnt.at(2),
            cnt.at(3),
            cnt.at(4),
            cnt.at(5),
            cnt.at(6),
            cnt.at(7),
            checker.total_broadcast,
            checker.total_connection
        ).c_str()
    );
    fflush(fp);
    
    plotFrame(); // フレームの開始
    plotSetFig(0); // figure 1
    PLOT("set title 'frame:%d time:%.2f %ld (%d+%d+%d+%d+%d+%d+%d+%d) '\n",
        loop,current_time,nodes_active_size,cnt[0],cnt[1],cnt[2],cnt[3],cnt[4],cnt[5],cnt[6],cnt[7]
    );
    // edgeのラベルの描画
    if(0){
        long max_edge_counter = 0;
        long min_edge_counter = 0;
        for(size_t from_itr=0,from_size=map.points.size();from_itr<from_size;++from_itr){
            for(size_t to_itr=0,to_size=map.map.at(from_itr).size();to_itr<to_size;++to_itr){
                Edge &edge=map.map.at(from_itr).at(to_itr);
                double fix = ((edge.to->point.x-edge.from->point.x) - (edge.to->point.y-edge.from->point.y))*0.1;
                LABEL(edge.from->point.x*0.5+edge.to->point.x*0.5+fix,edge.from->point.y*0.5+edge.to->point.y*0.5+fix,
                    "center","%s",getString(",","",edge.counter).c_str(),0);
                if(edge.counter>max_edge_counter){max_edge_counter=edge.counter;}
                if(edge.counter<min_edge_counter || min_edge_counter==0){min_edge_counter=edge.counter;}
            }
        }

        char color[64]="";
        for(size_t from_itr=0,from_size=map.points.size();from_itr<from_size;++from_itr){
            for(size_t to_itr=0,to_size=map.map.at(from_itr).size();to_itr<to_size;++to_itr){
                Edge &edge=map.map.at(from_itr).at(to_itr);
                get_heatmap_color((double)(edge.counter-min_edge_counter)/(double)(max_edge_counter-min_edge_counter),color);
                plotArrow("nohead",edge.from->point.x,edge.from->point.y,edge.to->point.x,edge.to->point.y,color,20,1);
            }
        }
    }
        
    // nodeのラベル描画
    for(unsigned int i=0;i<nodes_active_size;++i){
        Node &node=nodes.at(i);
        //LABEL(node.mv.position.x+plot_length_x*0.02,node.mv.position.y+plot_length_y*0.02,"center","%lu",node.accumulator.at(0).store.collection_size,0)
        if(node.type==Node::NORMAL){
            if(node.route!=nullptr && map.map.at(node.route->to->id).size()==4){ // 4方向に移動できる格子点である場合
                //int table=node.table_id+node.md.get_trust_state()*4;
                double label_position_x=node.mv.position.x;
                double label_position_y=node.mv.position.y;
                int rotate=0;
                if(0){ // node position move label
                    if(abs(node.mv.unit.x)>0){
                        label_position_y+=plot_length_y*0.02;
                        rotate=90;
                    }else{
                        label_position_x+=plot_length_x*0.02;
                    }
                    toString sb("-","");
                    //std::vector<std::string> sv={"←","↓","→","↑"};
                    sb.append(node.mv.arrive);
                    sb.append(node.mv.type);
                    sb.append(node.route->from->id,node.route->to->id);
                    LABEL(label_position_x,label_position_y,"left", // center
                        "%s",sb.result().c_str(),rotate
                    );
                }
                if(0){ // node information label
                    if(abs(node.mv.unit.x)>0){
                        label_position_y+=plot_length_y*0.02;
                        rotate=90;
                    }else{
                        label_position_x+=plot_length_x*0.02;
                    }
                    LABEL(label_position_x,label_position_y,"left", // center
                        "%s",getString(",","",
                        node.md.informations.at(1).reply_summary.latest_isFake,
                        node.md.informations.at(1).reply_summary.latest_timestamp>0,
                        //node.md.informations.at(0).information!=NULL,
                        node.md.informations.at(1).information!=NULL
                        //node.md.get_trust_state(),
                        //node.enter_direction,
                        //table,
                        ).c_str(),rotate
                    );
                }
                if(0){ // route select label
                    if(abs(node.mv.unit.x)>0){
                        label_position_y+=plot_length_y*0.02;
                        rotate=90;
                    }else{
                        label_position_x+=plot_length_x*0.02;
                    }
                    size_t table=node.md.get_trust_state()*4+node.table_id;
                    toString sb("-","");
                    //std::vector<std::string> sv={"←","↓","→","↑"};
                    for(int next_route_i=0;next_route_i<4;++next_route_i){
                        double ratio=node.route->to->routing.table.at(node.route->direction).at(table).at(next_route_i);
                        if(ratio>0){
                            sb.append(next_route_i);
                        }
                    }
                    LABEL(label_position_x,label_position_y,"left", // center
                        "%s",sb.result().c_str(),rotate
                    );
                }
            }
        }else{
            LABEL(node.mv.position.x+plot_length_x*0.02,node.mv.position.y-plot_length_y*0.02,"center",
                "%s",getString(",","",
                node.md.get_trust_state()
                ).c_str(),0);
        }
        if(node.md.trust_index.at(0)>=0 && node.route!=NULL){
            //LABEL(node.mv.position.x+plot_length_x*0.02,node.mv.position.y+plot_length_y*0.02,"center",
            //    "%d",node.md.get_trust_state(),0);
                //"%d%d%d",node.route->direction,node.directions[0],node.directions[1],0);
        }
    }
    // TAの描画
    char colors[][6]={"blue","red","green"};
    char colors2[][12]={"cyan","magenta","dark-green"};
    for(size_t i=0;i<informations.size();++i){
        FloatingInformation *info = informations.at(i);
        for(Area t : info->TA){
            if(gnuplot_use_type==1){
                plotArrow("nohead",t.left_bottom_x,t.left_bottom_y,t.left_bottom_x,t.right_top_y,colors[i],5,1); // 左の辺
                plotArrow("nohead",t.left_bottom_x,t.left_bottom_y,t.right_top_x,t.left_bottom_y,colors[i],5,1); // 下の辺
                plotArrow("nohead",t.right_top_x,t.left_bottom_y,t.right_top_x,t.right_top_y,colors[i],5,1); // 右の辺
                plotArrow("nohead",t.left_bottom_x,t.right_top_y,t.right_top_x,t.right_top_y,colors[i],5,1); // 上の辺
                if(info->replies.size>0){
                    LABEL(t.right_top_x+plot_length_x*0.02,t.left_bottom_y-plot_length_y*0.02,"center",
                    "%s",getString(",","","TA",info->id,info->replies.size
                    ).c_str(),0);
                }
            }else{
                PLOT("set object rect from  %f, %f to %f, %f fc rgb '%s' fillstyle transparent solid 0.2 noborder\n",t.left_bottom_x,t.left_bottom_y,t.right_top_x,t.right_top_y,colors[i]);
            }
        }
        for(Area r : info->RA){
            if(gnuplot_use_type==1){
                plotArrow("nohead",r.left_bottom_x,r.left_bottom_y,r.left_bottom_x,r.right_top_y,colors2[i],5,2); // 左の辺
                plotArrow("nohead",r.left_bottom_x,r.left_bottom_y,r.right_top_x,r.left_bottom_y,colors2[i],5,2); // 下の辺
                plotArrow("nohead",r.right_top_x,r.left_bottom_y,r.right_top_x,r.right_top_y,colors2[i],5,2); // 右の辺
                plotArrow("nohead",r.left_bottom_x,r.right_top_y,r.right_top_x,r.right_top_y,colors2[i],5,2); // 上の辺
                LABEL(r.right_top_x+plot_length_x*0.02,r.left_bottom_y-plot_length_y*0.02,"center",
                "%s",getString(",","","RA",info->id,info->replies.size
                ).c_str(),0);
            }else{
                PLOT("set object rect from  %f, %f to %f, %f fc rgb '%s' fillstyle transparent solid 0.2\n",r.left_bottom_x,r.left_bottom_y,r.right_top_x,r.right_top_y,colors2[i]);
            }
        }
    }
    // 点の描画
    plotGraph(8,
            "'-' pt 7 ps 1 lc 'blue' title 'none 0'",
            "'-' pt 7 ps 1 lc 'green' title 'true 1'",
            "'-' pt 7 ps 1 lc 'red' title 'fake 2'",
            "'-' pt 7 ps 1 title 'double 3'",
            "'-' pt 7 ps 1 title '4'",
            "'-' pt 7 ps 1 title '5'",
            "'-' pt 7 ps 1 title '6'",
            "'-' pt 7 ps 1 title '7'"
            );
    for(unsigned int i=0;i<nodes_active_size;++i){
        Node &node=nodes.at(i);
        PLOT_DATA(node.mv.position.x,node.mv.position.y);
    }
    PLOT("e\n");
    for(unsigned int i=0;i<nodes_active_size;++i){
        Node &node=nodes.at(i);
        if(node.md.get_trust_state()==1)PLOT_DATA(node.mv.position.x,node.mv.position.y);
    }
    PLOT("e\n");
    for(unsigned int i=0;i<nodes_active_size;++i){
        Node &node=nodes.at(i);
        if(node.md.get_trust_state()==2)PLOT_DATA(node.mv.position.x,node.mv.position.y);
    }
    PLOT("e\n");
    for(unsigned int i=0;i<nodes_active_size;++i){
        Node &node=nodes.at(i);
        if(node.md.get_trust_state()==3)PLOT_DATA(node.mv.position.x,node.mv.position.y);
    }
    PLOT("e\n");    
    for(unsigned int i=0;i<nodes_active_size;++i){
        Node &node=nodes.at(i);
        if(node.md.get_trust_state()==4)PLOT_DATA(node.mv.position.x,node.mv.position.y);
    }
    PLOT("e\n");    
    for(unsigned int i=0;i<nodes_active_size;++i){
        Node &node=nodes.at(i);
        if(node.md.get_trust_state()==5)PLOT_DATA(node.mv.position.x,node.mv.position.y);
    }
    PLOT("e\n");    
    for(unsigned int i=0;i<nodes_active_size;++i){
        Node &node=nodes.at(i);
        if(node.md.get_trust_state()==6)PLOT_DATA(node.mv.position.x,node.mv.position.y);
    }
    PLOT("e\n");
    for(unsigned int i=0;i<nodes_active_size;++i){
        Node &node=nodes.at(i);
        if(node.md.get_trust_state()==7)PLOT_DATA(node.mv.position.x,node.mv.position.y);
    }
    PLOT("e\n");    
    plotSetFig(1);
    plotGraph(8,
              "'gnuplot_tmp_data_file.txt' using 1:2 with lines lc rgb 'blue' title 'none'",
              "'gnuplot_tmp_data_file.txt' using 1:3 with lines lc rgb 'green' title 'only true'",
              "'gnuplot_tmp_data_file.txt' using 1:4 with lines lc rgb 'red' title 'only fake'",
              "'gnuplot_tmp_data_file.txt' using 1:5 with lines title 'double'",
              "'gnuplot_tmp_data_file.txt' using 1:6 with lines title '4'",
              "'gnuplot_tmp_data_file.txt' using 1:7 with lines title '5'",
              "'gnuplot_tmp_data_file.txt' using 1:8 with lines title '6'",
              "'gnuplot_tmp_data_file.txt' using 1:9 with lines title '7'"
              );

    /*  // hist
    plotResetRange(hist_x_start,hist_x_end,0,max_hist_high);
    plotGraph(1,"'-' with boxes notitle");
    for(int i=0;i<max_hist_size;++i){
        PLOT("%f, %d\n",i+0.5,hist.at(i));
    }
    PLOT("e\n");*/
    plotEndFrame();
}

void SimulationPlot::run(int max_loop){
    if(progress_ber_on){
        ProgressBar PB(max_loop,0.0001,NULL);
        for(int loop=0;loop<max_loop;++loop){
            update();
            if((gnuplot_use_type>0) && loop%20==0 && current_time>10000.0)plot_data(loop);
            PB.update_progress_bar();
            PB.print_progress_bar();
        }
    }else{
        for(int loop=0;loop<max_loop;++loop){
            update();
            if((gnuplot_use_type>0) && loop%20==0 && current_time>10000.0)plot_data(loop);
        }
    }
}

void SimulationPlot::close(){
    if(gnuplot_use_type>0){
        make_file_directory("logs/gnuplot/gnuplot_data_file");
        file_rename("gnuplot_tmp_data_file.txt",getString("_",".txt","logs/gnuplot/gnuplot_data_file",params.get_parameter_string_filename().c_str(),seed).c_str());
        fclose(fp);
        plotClose();
    }
}
#endif