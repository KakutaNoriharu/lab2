#ifndef FILE_WRAPPER_H_
#define FILE_WRAPPER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include "argument.h"
#include "fileManager.h"
/**
 * fopen,fcloseについてのwrapper
 * 機能
 * ・同じファイルをfopenしないようにする．
 * ・ファイル名にディレクトリが含まれる場合，そのディレクトリがなければ作成し，ファイルが生成されないのを防ぐ
 * ・main関数終了時に，閉じていないファイルポインタが，どこで開かれたファイルポインタか表示する．
 * 　・修正することで，プログラムをループして扱う場合のメモリリークを防げる．
 * ・mutexによりスレッドセーフ．コンパイル時に"-pthread"が必要
 */

/**
 * ファイルアクセスハンドラ
 * ファイル名とファイルポインタを保存する
 * ファイル名はfopenでの検索に，ファイルポインタはfcloseでの検索に利用 */
typedef struct __FileAccessHandler{
    char *file_name_;
    FILE *file_pointer_;
}__FileAccessHandler;

/**
 * ファイルアクセスマネージャー
 * ファイルアクセスハンドラを管理
 * mutexによりスレッドセーフ */
typedef struct __FileAccessManager{
    unsigned int size_,capacity_;
    __FileAccessHandler *open_file_handlers_;
    pthread_mutex_t *mutex;
}__FileAccessManager;


__FileAccessManager __FAM={0,0,NULL,NULL}; // global ファイルアクセスマネージャー

void __FileAccessManager_init(){
    __FAM.mutex=(pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    if(__FAM.mutex==NULL){
        fprintf(stderr,"MallocError FileAccessManager init mutex\n");
        exit(EXIT_FAILURE);
    }
    pthread_mutex_init(__FAM.mutex,NULL);
}

void __FileAccessManager_increase_capacity() {
    __FAM.capacity_ = (__FAM.capacity_ == 0) ? 1 : __FAM.capacity_ * 2;
    __FAM.open_file_handlers_ = (__FileAccessHandler *)realloc(__FAM.open_file_handlers_, __FAM.capacity_ * sizeof(__FileAccessHandler));
    if(__FAM.open_file_handlers_==NULL){
        fprintf(stderr,"ReallocError FileAccessManager increase capacity\n");
        exit(EXIT_FAILURE);
    }
}

/* search by filename if exist return index else -1  */
int __FileAccessManager_find_filename(const char* filename){
    for(unsigned int i=0;i<__FAM.size_;++i){
        if(strcmp(__FAM.open_file_handlers_[i].file_name_,filename)==0){
            return i;
        }
    }
    return -1;
}

/* search by fp if exist return index else -1  */
int __FileAccessManager_find_file_pointer(FILE* fp){
    for(unsigned int i=0;i<__FAM.size_;++i){
        if(fp==__FAM.open_file_handlers_[i].file_pointer_){
            return i;
        }
    }
    return -1;
}

// fopen wrapper
FILE* __FileAccessManager_fopen(const char* filename,const char* open_mode){
    pthread_mutex_lock(__FAM.mutex);
    if(__FileAccessManager_find_filename(filename)==-1){
        if(__FAM.size_+1>__FAM.capacity_){
            __FileAccessManager_increase_capacity();
        }
        __FAM.open_file_handlers_[__FAM.size_].file_name_=(char*)malloc(sizeof(char)*(strlen(filename)+1));
        if(__FAM.open_file_handlers_[__FAM.size_].file_name_==NULL){
            fprintf(stderr,"MallocError FileAccessManager append filename\n");
            exit(EXIT_FAILURE);
        }
        strcpy(__FAM.open_file_handlers_[__FAM.size_].file_name_,filename);
        make_file_directory(filename);
        if((__FAM.open_file_handlers_[__FAM.size_].file_pointer_=fopen(filename,open_mode))==NULL){
            fprintf(stderr,"File open Error FileAccessManager\n");
            exit(EXIT_FAILURE);
        }
    }else{
        fprintf(stderr,"File open Error FileAccessManager  file is already open : %s\n",filename);
        exit(EXIT_FAILURE);
    }
    pthread_mutex_unlock(__FAM.mutex);
    return __FAM.open_file_handlers_[__FAM.size_++].file_pointer_;
}

int __FileAccessManager_fclose(FILE *fp){
    pthread_mutex_lock(__FAM.mutex);
    int result=EOF;
    int index=__FileAccessManager_find_file_pointer(fp);
    if(index!=-1){
        result=fclose(__FAM.open_file_handlers_[index].file_pointer_);
        if(result==0){
            free(__FAM.open_file_handlers_[index].file_name_);
            __FAM.open_file_handlers_[index]=__FAM.open_file_handlers_[--__FAM.size_];
        }else{
            fprintf(stderr,"File close Error FileAccessManager fail fclose\n");
            exit(EXIT_FAILURE);
        }
    }else{
        fprintf(stderr,"File close Error FileAccessManager file is not registered\n");
        exit(EXIT_FAILURE);
    }
    pthread_mutex_unlock(__FAM.mutex);
    return result;
}

void __FileAccessManager_describe(){
    fprintf(stderr, "Active file pointers  size : %u\n",__FAM.size_);
    for(unsigned int i=0;i<__FAM.size_;++i){
        printf("%d : %s %p\n",i+1,__FAM.open_file_handlers_[i].file_name_,(void*)__FAM.open_file_handlers_[i].file_pointer_);
    }
}

// fclose wrapper
void __FileAccessManager_close(){
    if(__FAM.size_>0){
        fprintf(stderr, "\nClose FileAccessManager  Unclosed file is exist\n");
        __FileAccessManager_describe();
        int result;
        for(unsigned int i=0;i<__FAM.size_;++i){
            if((result=fclose(__FAM.open_file_handlers_[i].file_pointer_))==EOF){
                fprintf(stderr,"File close Error FileAccessManager close fail fclose\n");
                exit(EXIT_FAILURE);
            }
            free(__FAM.open_file_handlers_[i].file_name_);
        }
    }
    free(__FAM.open_file_handlers_);
    free(__FAM.mutex);
}


#define fopen __FileAccessManager_fopen // macro fileWrapper.h
#define fclose __FileAccessManager_fclose // macro fileWrapper.h


// main関数ラッパー
int __main__FileWrapped();

int main(){
    __FileAccessManager_init();
    __main__FileWrapped();
    __FileAccessManager_close();
    return 0;
}

#undef main
#define main __main__FileWrapped

#endif

/**
 * //sample.cpp
#include "leakChecker.h"
int main(){
    FILE *fp1=fopen("tx1.txt","w");
    FILE *fp2=fopen("tx2.txt","w");
    fclose(fp1);
    fclose(fp2);
    __FileAccessManager_close();
    LeakChecker_close();
    return 0;
}*/