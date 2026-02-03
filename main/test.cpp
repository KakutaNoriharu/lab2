#include "../utility/fileWrapper.h"
#define LOGGER_USE
#include "../utility/logger.h"

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

void run_simulation(FILE *fp,int simulation_seed,int use_gnuplot_setting,bool use_progress_bar,FILE *recorder,Params *params){
	SimulationPlot simulation(simulation_seed,use_gnuplot_setting,use_progress_bar,recorder,params);
	params->describe();
	int information_id=0;
	FloatingInformation true_information(information_id++,0,0);
	FloatingInformation fake_information(information_id++,1,1);
	simulation.set_base_information(&true_information);
	if(distinct_fake==1){
		fake_information.info=0;
		fake_information.fake_id=1;
	}else{
		fake_information.info=1;
		fake_information.fake_id=0;
	}
	for(std::vector<double> t : params->vTA){if(t.size()==4)true_information.add_TA_range(t[0],t[1],t[2],t[3]);}
	for(std::vector<double> t : params->vRA){if(t.size()==4)true_information.add_RA_range(t[0],t[1],t[2],t[3]);}
	for(std::vector<double> t : params->vfTA){if(t.size()==4)fake_information.add_TA_range(t[0],t[1],t[2],t[3]);}
	for(std::vector<double> t : params->vfRA){if(t.size()==4)fake_information.add_RA_range(t[0],t[1],t[2],t[3]);}

	simulation.set_information(&true_information);
	simulation.set_information(&fake_information);

	simulation.skip(50000); // 空回し
	simulation.reset_record();
    simulate_run_with_schedule(simulation,10000,g_event_list);
    simulation.write(fp);
	simulation.close();
}

int main(){
    LOGGER_SET("test-log.log",INFO);
    const char* parameter_filename = "params/test.prm";
    ParameterLoader loader(parameter_filename);
	ParameterManager mgr(0);
	set_parameters(&mgr,parameter_filename);
	add_progress_bar(&mgr.buffer);
	loader.describe();
	load_event_vector(&mgr,loader);
    g_method=5;
    g_Nxy={4,4};
	mgr.print();
	printf("%s\n",mgr.buffer.string_);
    Params params;
    //run_simulation(NULL,1,1,true,NULL,&params);
	// g_event_listが適切に変更されているかdebugする
	for(auto e : g_event_list){
        const BaseEvent *next_event = e.get();
		printf("%s\n",next_event->name.c_str());
	}
	mgr.update();
	printf("update mgr\n");
	mgr.print();
	printf("%s\n",mgr.buffer.string_);
	Params params2;
	for(auto e : g_event_list){
        const BaseEvent *next_event = e.get();
		printf("%s\n",next_event->name.c_str());
	}
	mgr.update();
	printf("update mgr2\n");
	mgr.print();
	printf("%s\n",mgr.buffer.string_);
	Params params3;
	for(auto e : g_event_list){
        const BaseEvent *next_event = e.get();
		printf("%s\n",next_event->name.c_str());
	}
	//run_simulation(NULL,1,1,true,NULL,&params2);
	printf("describe g_event_list\n");

    return 0;
}