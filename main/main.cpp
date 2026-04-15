#include "fileWrapper.h"
#define LOGGER_USE // LOGGERを利用する場合に予め定義する
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <unistd.h>
#include <pthread.h>

#include "../utility/parameterManager.hpp"
#include "../utility/argumentParser.hpp"
#include "../utility/utility.h"
#include "../utility/utility.hpp"
#include "parameters.hpp"
#include "simulation_event.hpp"

#define MAX_SEED 10

int thread_num = 4; // number of thread if too many,  process will be killed
int use_gnuplot = 0; // 0:no use 1:use make gif 2:use make script 
int seed = 3; // simulation seed value
bool send_notify = false; // if true send simulation progress to discord

void run_simulation(FILE *fp,int simulation_seed,int use_gnuplot_setting,bool use_progress_bar,FILE *recorder,Params *params,EventList *event_list){
	SimulationPlot simulation(simulation_seed,use_gnuplot_setting,use_progress_bar,recorder,params);
	if(road_prob_data_file_path.size()>0){
		simulation.map.load_routing(road_prob_data_file_path.c_str(),4);
	}

	int information_id=0;
	FloatingInformation true_information(information_id++,0,0); // information_idを0番として現在は使用し、以降1,2,3...
	FloatingInformation fake_information(information_id++,1,1);
	simulation.set_base_information(&true_information);
	if(distinct_fake==1){ // 偽装された情報を検出可能とするかどうか
		fake_information.info=0;
		fake_information.fake_id=1;
	}else{
		fake_information.info=1;
		fake_information.fake_id=0;
	}
	for(std::vector<double> t : params->vTA){if(t.size()==4)true_information.add_TA_range(t[0],t[1],t[2],t[3]);} // paramsのTA設定を追加
	for(std::vector<double> t : params->vRA){if(t.size()==4)true_information.add_RA_range(t[0],t[1],t[2],t[3]);}
	for(std::vector<double> t : params->vfTA){if(t.size()==4)fake_information.add_TA_range(t[0],t[1],t[2],t[3]);}
	for(std::vector<double> t : params->vfRA){if(t.size()==4)fake_information.add_RA_range(t[0],t[1],t[2],t[3]);}

	simulation.set_information(&true_information);
	simulation.set_information(&fake_information);

	simulation.skip(50000); // 空回し
	simulation.reset_record();
	simulate_run_with_schedule(simulation,simulation_max_loop,*event_list);
	simulation.write(fp);	
	
	if(g_route==RoutingType::RANDOM_TURN){
		FILE *road=NULL;
		toString road_filename;
		road_filename.append("road_data/");
		road_filename.append("Nxy_",params->Nxy.first,"_",params->Nxy.second,"/");
		road_filename.append("seed_",simulation.seed);
		road_filename.append(".csv");
		if(file_is_exist(road_filename.result().c_str())==false){
			make_file_directory(road_filename.result().c_str());
			road=fopen(road_filename.result().c_str(),"a");
			fprintf(road,"%s\n",getString(",","","from_id","to_id","from_point_x","from_point_y","to_point_x","to_point_y","counter","direction").c_str());
			for(size_t from_itr=0,from_size=simulation.map.points.size();from_itr<from_size;++from_itr){
				for(size_t to_itr=0,to_size=simulation.map.map.at(from_itr).size();to_itr<to_size;++to_itr){
					Edge &edge=simulation.map.map.at(from_itr).at(to_itr);
					fprintf(road,"%s\n",getString(",","",edge.from->id,edge.to->id,edge.from->point.x,edge.from->point.y,edge.to->point.x,edge.to->point.y,edge.counter,edge.direction).c_str());
				}
			}
			fclose(road);
		}

		FILE *prob=NULL;
		toString prob_filename;
		prob_filename.append("prob_data/");
		prob_filename.append("Nxy_",params->Nxy.first,"_",params->Nxy.second,"/");
		prob_filename.append("seed_",simulation.seed,"_table_",4);
		prob_filename.append(".csv");
		if(file_is_exist(prob_filename.result().c_str())==false){
			make_file_directory(prob_filename.result().c_str());
			prob=fopen(prob_filename.result().c_str(),"a");
			for(size_t i=0,size=simulation.map.points.size();i<size;++i){
				EdgePoint &p=simulation.map.points.at(i);
				for(int enter=0;enter<4;++enter){
					for(int table=0;table<4;++table){
						for(int edge_itr=0;edge_itr<4;++edge_itr){
							fprintf(prob,"%f\n",p.routing.table.at(enter).at(table).at(edge_itr));
						}
					}
				}
			}
			fclose(prob);
		}
	}
	if(1){
		for(size_t i=0,size=simulation.GS.size;i<size;++i){
			EventDataReceive &ed = simulation.GS.events.at(i); 
			//printf("%lu,%f,%lu,%lu,%d\n",i,ed.timestamp,ed.from_id,ed.to_id,ed.information_id);
		}
	}
	simulation.close();
}

void repeat_simulation(size_t simulation_progress_id,Params *params,EventList *event_list){
	bool use_progress_bar = false;
	std::string filename = "data/"+simulation_result_file_path+"/sim_"+params->get_parameter_string_filename()+".csv";
	int csv_seed_progress=1;
	if(file_is_exist(filename.c_str())){
		csv_seed_progress=countCSVRows(filename);
	}
	FILE* fp;
	make_file_directory(filename.c_str());
	if(csv_seed_progress<=MAX_SEED){
		fp=fopen(filename.c_str(),"a");
		ProgressBar PB(MAX_SEED-csv_seed_progress+1,0.001,getString(" ","",simulation_progress_id).c_str());
		for(int i=csv_seed_progress;i<=MAX_SEED;++i){
			run_simulation(fp,i,use_gnuplot,use_progress_bar,NULL,params,event_list);
			PB.update_progress_bar();
		}
		fclose(fp);
	}
}

void *progress(void *ptr){ // シミュレーションの進捗を表示する
	ParameterManager *mgr = (ParameterManager*) ptr;
	size_t progress_state=0;
	while(true){
		sleep(1);
		mgr->print();
		print_progress_bar();
		if(mgr->ParameterProgress!=progress_state && send_notify){
			progress_state=mgr->ParameterProgress;
			if(system(getString("","","python scripts/DiscordNotify.py ",simulation_result_file_path.c_str()," progress=",mgr->ParameterProgress-thread_num,"/",mgr->ParameterTotal).c_str())==-1){
				fprintf(stderr, "fail notify\n");
			}
		}
	}
	return 0;
}

void *thread(void *ptr){ // シミュレーションを行うスレッド
	ParameterManager *mgr = (ParameterManager*) ptr;
	bool remain=true;
	size_t simulation_progress_id;
	while(remain){
		mgr->lock();
		remain=mgr->update();
		// gXL,gXYについてNxyに基づいて変更させるのかどうか
		if(fix_field){
			g_XL=(g_Nxy.first>0) ? 2000.0*g_Nxy.first : 2000.0;
			g_YL=(g_Nxy.second>0) ? 2000.0*g_Nxy.second : 2000.0;
		}
		Params params(mgr);
		EventList event_list;
		bool event_is_not_setted = true;
		add_event_schedule(event_list,{"0","ArrivalEvent","0","0"});
	    for(std::pair<std::string,int> p : parameter_id_map){
			if(p.first=="EVENT"){
				event_is_not_setted = false;
				break;
			}
		}
		if(event_is_not_setted)event_list=set_malicious_event(&params,event_list);
		simulation_progress_id=mgr->ParameterProgress;
		mgr->unlock();
		if(remain==false)break;
		if(use_gnuplot==0){
			repeat_simulation(simulation_progress_id,&params,&event_list);
		}else{
			run_simulation(NULL,seed,use_gnuplot,false,NULL,&params,&event_list);
		}
	}
	return 0;
}

int main(/*int argc, char *argv[]*/){
    TimeManager timer;
	LOGGER_SET(getString("",".log","simulation-log2").c_str(),INFO);
	
    char *parameter_filename=NULL;
    bool use_progress_bar = true;

	g_XL=4000.0;
	g_YL=4000.0;
	g_Nxy={4,4};
	g_vTA={{g_XL*0.5,g_YL*0.5,1200.0,1200.0}};
	g_vRA={};
	//g_vfTA={{g_XL*0.5,g_YL*0.5,400.0,400.0}};
	g_vfTA={};
	g_vfTA={{g_XL*0.5,g_YL*0.5,1200.0,1200.0}};
	g_attack=1;
	g_method=9;
	distinct_fake=1;
	g_Lambda=0.002;
	g_fVm=1.0;
    consider_all_TA=1;
    ArgumentParser arg("Information Floating Simulation ver.2025-11-08+");
    arg.add("-p","--param",STRING_C,&parameter_filename,"simulation parameter list file name");
    arg.add("-g","--gnuplot",INT,&use_gnuplot,"show simulation result  0:no use 1:use make gif 2:use make script");
    arg.add("-pb","--progressbar",FLAG,&use_progress_bar,"show simulation progress");
    arg.add("-s","--seed",INT,&seed,"simulation random seed");

    arg.add("-1","--mdv1",INT,&md_v1,"method parameter value1");
    arg.add("-2","--mdv2",INT,&md_v2,"method parameter value2");
    arg.add("-3","--mdv3",INT,&md_v3,"method parameter value3");
    arg.add("-4","--mdv4",INT,&md_v4,"method parameter value4");


    add_arguments(&arg); // add simulation parameter args
    arg.parse(argc,argv); // set args
    if(parameter_filename==NULL){ // no use parameter file
		Params params;
		if(0){
			g_event_list.clear();
			add_event_schedule(g_event_list,{"0","ArrivalEvent","0","0"}); // {"event time step","event name","node type 0:NORMAL 1:MALICIOUS","information id 0:true, 1:fake"}
			add_event_schedule(g_event_list,{getString("","",g_malicious_appear_time).c_str(),"ArrivalEvent","1","1"});
			run_simulation(NULL,seed,use_gnuplot,use_progress_bar,NULL,&params,&g_event_list);
		}
		EventList event_list;
		bool event_is_not_setted = true;
		add_event_schedule(event_list,{"0","ArrivalEvent","0","0"});
	    for(std::pair<std::string,int> p : parameter_id_map){
			if(p.first=="EVENT"){
				event_is_not_setted = false;
				break;
			}
		}
		if(event_is_not_setted)event_list=set_malicious_event(&params,event_list);
		
		run_simulation(NULL,seed,use_gnuplot,use_progress_bar,NULL,&params,&event_list);

	
	}else if(parameter_filename!=NULL){ // use parameter file
		if(use_gnuplot!=0)thread_num=1; // gif生成する場合はスレッド上限を1に
	    ParameterLoader loader(parameter_filename);
        ParameterManager mgr(0);
        set_parameters(&mgr,parameter_filename);
		load_event_vector(&mgr,loader);
        add_progress_bar(&mgr.buffer);
        std::vector<pthread_t> threads(thread_num);
        for(size_t i=0;i<threads.size();++i){
            pthread_t &th=threads.at(i);
            pthread_create(&th, NULL, *thread, (void *) &mgr);
        }
        pthread_t prgW;
        pthread_create(&prgW,NULL,*progress,(void*)&mgr);
        for(auto th : threads){
            pthread_join(th,NULL);
        }
        pthread_cancel(prgW);
        pop_progress_bar(&mgr.buffer);
        printf("complete %s\n",simulation_result_file_path.c_str());
        LOGGER_DATA_PRINT(INFO,"complete %s  elapsed : %f sec",simulation_result_file_path.c_str(),timer.get_elapsed_time_sec());
    }
    fprintf(stderr,"thread %d complete elapsed : %f sec\n",thread_num,timer.get_elapsed_time_sec());
    return 0;
}
