#ifndef LEAK_CHECKER_H_
#define LEAK_CHECKER_H_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "argument.h"

/**
 * malloc,realloc,freeをラップして，メモリリークを検知するmain関数ラッパ
 * 機能
 * ・malloc,realloc,freeをラップし，それが呼ばれた関数などを記録
 * ・main関数終了時に，未開放のメモリがあれば，これをメモリを確保した関数名などと共に表示する
 * 　・修正することで，その関数をループすることでのメモリリークを防ぎ，これによるプログラムの強制終了を防ぐ．
 * ・mutexによるスレッドセーフ
 * 
 * 使い方
 * ・main関数の記述されたファイルの一番上にインクルードする．
 */

typedef struct __Allocator{
    void *pointer_;
    size_t size_;
    const char *file_;
    unsigned int line_;
}__Allocator;

typedef struct __LeakChecker{
    size_t size_,capacity_;
    __Allocator *memories_;
    pthread_mutex_t *mutex;
}__LeakChecker;

__LeakChecker __LCer={0,0,NULL,NULL}; // global メモリリークチェッカー

void __LeakChecker_init(){
    __LCer.mutex=(pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    if(__LCer.mutex==NULL){
        fprintf(stderr,"MallocError LeakChecker init mutex\n");
        exit(EXIT_FAILURE);
    }
    pthread_mutex_init(__LCer.mutex,NULL);
}

void __LeakChecker_lock(){
    if(__LCer.mutex==NULL)__LeakChecker_init();
    pthread_mutex_lock(__LCer.mutex);
}
void __LeakChecker_unlock(){
    pthread_mutex_unlock(__LCer.mutex);
}

void __LeakChecker_describe(){
    fprintf(stderr, "Active allocated memories  size : %lu\n",__LCer.size_);
    for(size_t i=0;i<__LCer.size_;++i){
        __Allocator *memory=&__LCer.memories_[i];
        fprintf(stderr, "%lu : %s line:%u size:%lu %p\n",i+1,memory->file_,memory->line_,memory->size_,memory->pointer_);
    }
}

void __LeakChecker_increase_capacity() {
    __LCer.capacity_ = (__LCer.capacity_ == 0) ? 1 : __LCer.capacity_ * 2;
    __LCer.memories_ = (__Allocator *)realloc(__LCer.memories_, __LCer.capacity_ * sizeof(__Allocator));
    if(__LCer.memories_==NULL){
        fprintf(stderr,"ReallocError LeakChecker increase capacity\n");
        exit(EXIT_FAILURE);
    }
}

int __LeakChecker_find(void *p){
    for(size_t i=0;i<__LCer.size_;++i){
        if(__LCer.memories_[i].pointer_==p){
            return i;
        }
    }
    return -1;
}

void * __LeakChecker_malloc(size_t size,const char* file,unsigned int line){
    void *p=malloc(size);
    if(p==NULL)return NULL;
    __LeakChecker_lock();
    if(__LCer.size_+1>__LCer.capacity_){
        __LeakChecker_increase_capacity();
    }
    __Allocator *memory=&__LCer.memories_[__LCer.size_++];
    memory->file_=file;
    memory->line_=line;
    memory->pointer_=p;
    memory->size_=size;
    __LeakChecker_unlock();
    return p;
}

void * __LeakChecker_realloc(void *pointer,size_t size,const char* file,unsigned int line){
    if(pointer==NULL){
        pointer=__LeakChecker_malloc(size,file,line);
        return pointer;
    }
    int index=__LeakChecker_find(pointer);
    void *p=realloc(pointer,size);
    if(p==NULL)return NULL;
    __LeakChecker_lock();
    __Allocator *memory;
    if(index!=-1){
        memory=&__LCer.memories_[index];
    }else{
        if(__LCer.size_+1>__LCer.capacity_){
            __LeakChecker_increase_capacity();
        }
        memory=&__LCer.memories_[__LCer.size_++];
    }
    memory->file_=file;
    memory->line_=line;
    memory->pointer_=p;
    memory->size_=size;
    __LeakChecker_unlock();
    return p;
}

void __LeakChecker_free(void *pointer,const char* file,unsigned int line){
    __LeakChecker_lock();
    int index=__LeakChecker_find(pointer);
    if(index!=-1){
        __LCer.memories_[index]=__LCer.memories_[--__LCer.size_];
        free(pointer);
        pointer=NULL;
    }else{
        if(pointer!=NULL){
            fprintf(stderr, "LeakChecker free pointer is not registered : %p  %s:%u\n",pointer,file,line);
            //exit(EXIT_FAILURE);
        }
    }
    __LeakChecker_unlock();
}

void __LeakChecker_close(){
    if(__LCer.size_>0){
        fprintf(stderr, "Close LeakChecker  unlock memories : %lu\n",__LCer.size_);
        __LeakChecker_describe();
    }
    if(__LCer.memories_!=NULL){
        free(__LCer.memories_);
    }
    if(__LCer.mutex!=NULL){
        free(__LCer.mutex);
    }
}

#define malloc(x) __LeakChecker_malloc(x,__FILE__,__LINE__)
#define realloc(x,size) __LeakChecker_realloc(x,size,__FILE__,__LINE__)
#define free(x) __LeakChecker_free(x,__FILE__,__LINE__)

int __main__LeakCheckerWrapped();

int main(){
    __LeakChecker_init();
    __main__LeakCheckerWrapped();
    __LeakChecker_close();
    return 0;
}

#undef main
#define main __main__LeakCheckerWrapped

#endif

