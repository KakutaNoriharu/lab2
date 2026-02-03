#ifndef PROFILER_H_
#define PROFILER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "argument.h"

/**
 * 関数や処理の一部に対して，実行にかかった時間を算出する機能を提供する
 * 
 * 関数に対する利用
 * 任意の関数に対して実行時間を計測する
 * ex: PROFILE(func(any)) ;
 * 
 * プログラムの一部に対する利用
 * プログラムの一部分に対して実行時間を計測
 * PROFILER_OPEN;
 * // your code
 * for(int i=0;i<10;++i){
 *  print("%d\n",i);
 * }
 * PROFILER_CLOSE; 
 * 
 * 実行時間の割合を表示する．
 * global 全体に占める割合
 * partial 任意にプロファイルしたものに対して占める割合
 */

typedef struct __ProfileManageContent__{
    char name[256];
    int call_count;
    clock_t total_clock;
}__ProfileManageContent__;

typedef struct __ProfileManager__{
    __ProfileManageContent__* content;
    unsigned int size;
    clock_t total_clock;
    clock_t global_clock;
    pthread_mutex_t mutex;
}__ProfileManager__;

typedef struct __Profiler__{
    clock_t constructed_time_;
    char name[256];
}__Profiler__;

static __ProfileManager__ __profile_manager__;

void __init_profiler_manager__(){
    __profile_manager__.content=NULL;
    __profile_manager__.global_clock=0;
    __profile_manager__.total_clock=0;
    __profile_manager__.size=0;
    pthread_mutex_init(&__profile_manager__.mutex,NULL);
}

int __find_profiler__(const char *name){
    for(unsigned int i=0;i<__profile_manager__.size;++i){
        if(strncmp(__profile_manager__.content[i].name,name,255)==0){
            return i;
        }
    }
    return -1;
}

void __add_new_profiler__(__Profiler__ *p){
    __ProfileManageContent__ new_content;
    strncpy(new_content.name,p->name,255);
    new_content.call_count=1;
    new_content.total_clock=p->constructed_time_;
    __profile_manager__.content=(__ProfileManageContent__*)realloc(__profile_manager__.content,sizeof(__ProfileManageContent__)*(__profile_manager__.size+1));
    if(__profile_manager__.content==NULL){
        fprintf(stderr, "Profiler Manager realloc fail\n");
        exit(EXIT_FAILURE);
    }
    __profile_manager__.content[__profile_manager__.size++]=new_content;
}

__ProfileManager__* __manage_profiler__(__Profiler__ *p){
    if(p!=NULL){
        int index=__find_profiler__(p->name);
        if(index==-1){
            __add_new_profiler__(p);
        }else{
            ++__profile_manager__.content[index].call_count;
            __profile_manager__.content[index].total_clock+=p->constructed_time_;
        }
        __profile_manager__.total_clock+=p->constructed_time_;
    }
    return &__profile_manager__;
}

void __describe_profile_result__(){
    __profile_manager__.global_clock=clock()-__profile_manager__.global_clock;
    fprintf(stderr,"profile result  Clock global:%lu total:%lu\n",__profile_manager__.global_clock,__profile_manager__.total_clock);
    for(unsigned int i=0;i<__profile_manager__.size;++i){
        fprintf(stderr, "global : %7.3f %%  partial : %7.3f %%  call:%5d  clock:%10lu  %s\n",
        (double)__profile_manager__.content[i].total_clock/__profile_manager__.global_clock*100.0,
        (double)__profile_manager__.content[i].total_clock/__profile_manager__.total_clock*100.0,
        __profile_manager__.content[i].call_count,
        __profile_manager__.content[i].total_clock,
        __profile_manager__.content[i].name);
    }
}

void __close_profiler_manager__(){
    __describe_profile_result__();
    if(__profile_manager__.content!=NULL){
        free(__profile_manager__.content);
    }
}

__Profiler__ __get__Profiler(const char* file,unsigned int line,const char* func){
    __Profiler__ p;
    snprintf(p.name,255,"%s:%u %s",file,line,func);
    p.constructed_time_=clock();
    return p;
}

void __close_Profiler(__Profiler__ *p){
    p->constructed_time_=clock()-p->constructed_time_;
    __manage_profiler__(p);
}

#define PROFILER_OPEN __Profiler__ __profiler__ = __get__Profiler(__FILE__,__LINE__,__func__)
#define PROFILER_CLOSE __close_Profiler(&__profiler__)
#define __PROFILER_OPEN__(x) __Profiler__ __profiler__ = __get__Profiler(__FILE__,__LINE__,x)
#define PROFILE(x) do{__PROFILER_OPEN__(#x);(x);PROFILER_CLOSE;}while(0)

int __profile_wrapped_main__();

int main(){
    __init_profiler_manager__();
    PROFILE(__profile_wrapped_main__());
    __close_profiler_manager__();
    return 0;
}

#undef main
#define main __profile_wrapped_main__

#endif
/**
// sample.c
#include <stdio.h>
#include <stdlib.h>
#include "profiler.h"

inline void func(int x){
    PROFILER_OPEN;
    for(int i=0;i<x*x*x*x;++i){
        if(x%7==0)printf("%d",__COUNTER__);
        fflush(stdout);
    }
    printf(" func end\n");
    PROFILER_CLOSE;
}

int main(){
    PROFILE(func(15));
    PROFILE(func(25));
    PROFILE(func(35));
    return 0;
}
 */