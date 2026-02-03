#ifndef _PROGRESS_BAR_HPP_
#define _PROGRESS_BAR_HPP_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <pthread.h>
#include "utility.hpp"

/**
 * プログレスバーの表示と，printf()がプログレスバーと競合しないように補助する機能をまとめたファイル
 */

/**
 * プログレスバーの補助をする構造体
 * プログレスバーの表示と，その他の表示を分ける */
typedef struct ProgressBarManager{
    size_t size_,capacity_;
    StringBuffer **progress_bar_buffer_;
    char buffer_[1024];
    pthread_mutex_t mutex;
    ProgressBarManager() : size_(0),capacity_(0),progress_bar_buffer_(NULL),buffer_("\0") {
        pthread_mutex_init(&mutex,NULL);
    }
    ~ProgressBarManager(){}

    void increase_capacity() {
        capacity_ = (capacity_ == 0) ? 1 : capacity_ * 2;
        progress_bar_buffer_ = (StringBuffer **)realloc(progress_bar_buffer_, capacity_ * sizeof(StringBuffer*));
        if(progress_bar_buffer_==NULL){
            fprintf(stderr,"ReallocError ProgressBarManager increase capacity\n");
            exit(EXIT_FAILURE);
        }
    }

    /* プログレスバーの内容が保存されている文字列ポインタを設定 */
    void set_progress_string(StringBuffer *progress_bar){
        pthread_mutex_lock(&mutex);
        while(size_+1>capacity_){increase_capacity();}
        progress_bar_buffer_[size_]=progress_bar;
        if(size_++){fprintf(stderr, "\n");}
        pthread_mutex_unlock(&mutex);
    }
    void unset_progress_string(StringBuffer *progress_bar){
        pthread_mutex_lock(&mutex);
        for(size_t i=0;i<size_;++i){
            if(progress_bar==progress_bar_buffer_[i]){
                progress_bar_buffer_[i]=progress_bar_buffer_[size_-1];//!
                progress_bar_buffer_[size_-1]=NULL;
                break;
            }
        }
        --size_;
        if(size_>0){fprintf(stderr,"\033[%luA",size_);}
        fprintf(stderr,"\033[2K\r%s complete\n",progress_bar->string_);//行をクリアしてから表示
        fprintf(stderr,"\033[%luB",size_);
        pthread_mutex_unlock(&mutex);
    }

    /* プログレスバーを表示 */
    void print_progress_bar(){
        pthread_mutex_lock(&mutex);
        if(progress_bar_buffer_!=NULL){
            if(size_>0){
                if(size_>1)fprintf(stderr,"\033[%luA",size_-1);// カーソルを左上に
                for(size_t i=0;i<size_-1;++i){
                    if(progress_bar_buffer_[i]->string_!=NULL){
                        fprintf(stderr,"\033[2K\r%s\n",progress_bar_buffer_[i]->string_);//行をクリアしてから表示
                    }else{
                        fprintf(stderr,"skip\n");
                    }
                }
                if(progress_bar_buffer_[size_-1]->string_!=NULL){
                    fprintf(stderr,"\033[2K\r%s",progress_bar_buffer_[size_-1]->string_);//行をクリアしてから表示
                }
            }
        }
        pthread_mutex_unlock(&mutex);
    }

    /* プログレスバー以外の文字列を表示 (printf) */
    void print(const char *format, ...){
        pthread_mutex_lock(&mutex);
        va_list va;
        va_start(va,format);
        if(size_>0)fprintf(stderr,"\033[%luA",size_-1); // カーソルを左上に
        vsnprintf(buffer_,1024,format, va);
        va_end(va);
        if(strcmp(format,"\n")==0){
            fprintf(stdout, "\n");    
        }else{
            if(strchr(buffer_,'\n')!=NULL){
                char *ptr = strtok(buffer_, "\n");
                while(ptr != NULL){
                    fprintf(stderr, "\033[2K\r");
                    fprintf(stdout, "%s\n", ptr);
                    ptr = strtok(NULL, "\n");
                }
            }else{
                fprintf(stdout, "%s", buffer_);
            }
        }
        if(size_>0){
            for(size_t i=0;i<size_;++i){
                fprintf(stderr, "\n");
            }
        }
        pthread_mutex_unlock(&mutex);
    }
}ProgressBarManager;

#if defined(USE_PROGRESS_BAR)
ProgressBarManager __PBM;

inline void print_progress_bar(){
    __PBM.print_progress_bar();
}

inline void add_progress_bar(StringBuffer *string){
    __PBM.set_progress_string(string);
}

inline void pop_progress_bar(StringBuffer *string){
    __PBM.unset_progress_string(string);
}

#define printf(...) __PBM.print(__VA_ARGS__)

#endif

/**
 * total : total loop count
 * step  : step width  ex(0.1):10%,20%,30%,...  ex(0.01):1%,2%,3%,... 
 * 注意
 * 高速に呼び出される場合は，表示処理の方が遅くなるため推奨されない  */
typedef struct ProgressBar{
    TimeManager timer;
    clock_t start_time_;
    int progress_count_,progress_total_;
    double elapsed_,progress_,remain_,speed_;
    double unit_,r_total_;
    double update_stopper_count_,stopper_unit_;
    const char* name_;
    bool updated;
    StringBuffer progress_bar_;
    ProgressBar(int total,double step,const char* name) : start_time_(clock()),progress_count_(0),progress_total_(total),
                    elapsed_(0.0),progress_(0.0),remain_(0.0),speed_(0.0),
                    unit_(1.0/CLOCKS_PER_SEC),r_total_(1.0/(double)total),
                    update_stopper_count_(0),stopper_unit_(step*total),name_(name),updated(false){
    #if defined(USE_PROGRESS_BAR)
    __PBM.set_progress_string(&progress_bar_);
    #endif
    }
    ~ProgressBar(){
    #if defined(USE_PROGRESS_BAR)
    __PBM.unset_progress_string(&progress_bar_);
    #endif
    }
    double get_progress_elapsed(){
        return (double)(clock() - start_time_) * unit_;
    }
    void set_progress_bar(int total,double step,const char* name){
        timer.reset();
        start_time_=clock();
        progress_count_=0;
        progress_total_=total;
        elapsed_=0.0;
        progress_=0.0;
        remain_=0.0;
        speed_=0.0;
        r_total_=(1.0/(double)total);
        update_stopper_count_=0;
        stopper_unit_=(double)total*step;
        name_=name;
    }
    void set_progress_count(int count){
        progress_count_ = count;
    }
    StringBuffer* get_progress_bar(){
        return &progress_bar_;
    }
    void update_progress_bar(){
        progress_count_+=1;
        if(progress_count_>=update_stopper_count_ || progress_count_==progress_total_){
            if(progress_count_<=progress_total_){
                update_stopper_count_+=stopper_unit_;
                double speed_diff = speed_;
                //elapsed_ = get_progress_elapsed();
                elapsed_ = timer.get_elapsed_time_sec();
                progress_ = (double) progress_count_ * r_total_ * 100.0;
                speed_ = (double)progress_count_ / elapsed_;
                remain_ = elapsed_/progress_*(100.0 - progress_);
                progress_bar_.clear();
                if(name_!=NULL)progress_bar_.insert("%8.8s : ",name_);
                progress_bar_.insert("[%4d/%4d] %6.2f%% time:%7.2fs remain:%7.2fs speed:%6.3f (item/s %+.3f) ",
                progress_count_,progress_total_,progress_,elapsed_,remain_,speed_,speed_-speed_diff);
                updated=true;
            }
        }
    }
    void print_progress_bar(){
        if(updated){
            fprintf(stderr, "\r%s",progress_bar_.string_);
            updated=false;
        }
    }
    const char* get_string(){
        return progress_bar_.string_;
    }
}ProgressBar;

#endif


/**
// sample.c
#include <stdio.h>
#include "utility/progress.hpp"

#define LOOP 100000

int main(){
    ProgressBarManager PBM;
    ProgressBar PB(LOOP);
    PBM.set_progress_string(PB.get_progress_bar());
    for(int i=0;i<LOOP;++i){
        if(i%500==0)PBM.print("i= %d\n",i);
        PB.update_progress_bar();
        PBM.print_progress_bar();
    }
    return 0;
} 

 */