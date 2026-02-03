#ifndef FIELD_ACCESSOR_H_
#define FIELD_ACCESSOR_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * 概要
 * 特定小領域に存在するオブジェクトへのポインターを保存する構造体 
 * 注意
 * ・割り当てからアクセスまでの間でポインターが変更されることを想定していない．
 * ・FieldAccessorUnitのコンストラクタで，accessorを確保してはいけない．FieldAccessのresizeでtmpを共有しているため．
 * ・コンストラクタ，デコンストラクタのためにC++で記述
 * */

typedef struct FieldAccessorUnit{
    void **accessor_; // 当領域に存在するオブジェクトへのポインター
    FieldAccessorUnit *surround_[8]; // 周囲八マスへのアクセスのためのポインターを保存する
    size_t capacity_,*size_; // accessor_ の最大容量，利用中のサイズ
    size_t access_counter_; // forループのためのカウンター
    unsigned short surround_counter_; // forループのためのカウンター
    FieldAccessorUnit(size_t *size) : accessor_(NULL),surround_{NULL},capacity_(0),size_(size),access_counter_(0),surround_counter_(0){}
    ~FieldAccessorUnit(){if(accessor_!=NULL)free(accessor_);}
    // ポインタを保存する容量を増加
    void increase_capacity() {
        capacity_ = (capacity_ == 0) ? 1 : capacity_ * 2;
        accessor_ = (void **)realloc(accessor_, capacity_ * sizeof(void *));
        if(accessor_ == NULL){
            fprintf(stderr,"ReallocError FieldAccessorUnit\n");
            exit(EXIT_FAILURE);
        }
    }
    // 単位領域にオブジェクトへのポインタを追加
    inline void append(void *pointer) {
        if(*size_ >= capacity_)increase_capacity();
        accessor_[*size_] = pointer;
        *size_ += 1;
    }
    // オブジェクトへのポインタを取得
    inline void* get_accessor(size_t iterator){
        if(iterator<*size_){
            return accessor_[iterator];
        }
        return NULL;
    }
    // 周辺領域を含めたオブジェクトへのアクセス関数．NULLを返すまで呼ばれることを想定．
    inline void* access() {
        void *accessor = NULL;
        // 周囲のユニットを優先してアクセス
        while(surround_counter_<8){
            if (surround_[surround_counter_] != NULL){
                while (access_counter_ < *surround_[surround_counter_]->size_){
                    accessor = surround_[surround_counter_]->get_accessor(access_counter_);
                    ++access_counter_;
                    if(accessor != NULL){
                        return accessor;
                    }
                }
                access_counter_=0;
            }
            ++surround_counter_;
        }

        // 自ユニットへのアクセス
        while(access_counter_ < *size_){
            accessor = get_accessor(access_counter_);
            ++access_counter_;
            if(accessor != NULL){
                return accessor;
            }
        }

        access_counter_ = 0;
        surround_counter_ = 0;
        return NULL;
    }
    void clear(){
        if(accessor_!=NULL){
            free(accessor_);
        }
    }
}FieldAccessorUnit;

typedef struct FieldAccessor{
    FieldAccessorUnit *unit_;
    size_t capacity_,size_;
    double field_scale_x_,field_scale_y_; // フィールドの大きさ
    double field_separate_unit_x_,field_separate_unit_y_; // フィールドを分割した1単位の長さ
    size_t x_unit_length_,y_unit_length_,unit_length_; // 分割した数
    size_t *size_series_;
    FieldAccessor() = default;
    FieldAccessor(double field_x_length,double field_y_length,double separate_x,double separate_y)
        : unit_(NULL),capacity_(0),size_(0),field_scale_x_(field_x_length),field_scale_y_(field_y_length),
          field_separate_unit_x_(1.0/separate_x),field_separate_unit_y_(1.0/separate_y),size_series_(NULL){
        initialize(field_x_length,field_y_length,separate_x,separate_y);
    }
    ~FieldAccessor(){
        if(unit_!=NULL){
            for(size_t i=0;i<unit_length_;++i){
                unit_[i].clear();
            }
            free(unit_);
        }
        if(size_series_!=NULL){
            free(size_series_);
        }
    }
    void initialize(double field_x_length,double field_y_length,double separate_x,double separate_y){
        unit_=NULL;
        capacity_=0;
        size_=0;
        field_scale_x_=field_x_length;
        field_scale_y_=field_y_length;
        field_separate_unit_x_=1.0/separate_x;
        field_separate_unit_y_=1.0/separate_y;
        size_series_=NULL;
        x_unit_length_=(int)(field_scale_x_ *field_separate_unit_x_);
        y_unit_length_=(int)(field_scale_y_ *field_separate_unit_x_);
        unit_length_=x_unit_length_*y_unit_length_;
        resize(x_unit_length_,y_unit_length_);
    }
    // フィールドを設定された細かさで構成
    void resize(size_t x_length,size_t y_length) {
        x_unit_length_=x_length;
        y_unit_length_=y_length;
        unit_length_=x_unit_length_*y_unit_length_;
        size_series_=(size_t*)malloc(sizeof(size_t)*unit_length_);
        unit_ = (FieldAccessorUnit *)malloc(sizeof(FieldAccessorUnit) *unit_length_);
        for(size_t i=0;i<unit_length_;++i){
            FieldAccessorUnit tmp(&size_series_[i]);
            unit_[i]=tmp;
            unit_[i].increase_capacity();
            unit_[i].increase_capacity();
            unit_[i].increase_capacity();
        }
        set_surround();
    }
    // 周辺エリアへのポインタを設定する．
    void set_surround(){
        for(size_t y=0;y<y_unit_length_;++y){
            for(size_t x=0;x<x_unit_length_;++x){
                FieldAccessorUnit *unit = &unit_[y*x_unit_length_+x];
                if(y>0 && x>0)unit->surround_[0]=&unit_[(y-1)*x_unit_length_+x-1];
                if(y>0)unit->surround_[1]=&unit_[(y-1)*x_unit_length_+x+0];
                if(y>0 && x<x_unit_length_-1)unit->surround_[2]=&unit_[(y-1)*x_unit_length_+x+1];
                if(x>0)unit->surround_[3]=&unit_[y*x_unit_length_+x-1];
                if(x<x_unit_length_-1)unit->surround_[4]=&unit_[y*x_unit_length_+x+1];
                if(y<y_unit_length_-1 && x>0)unit->surround_[5]=&unit_[(y+1)*x_unit_length_+x-1];
                if(y<y_unit_length_-1)unit->surround_[6]=&unit_[(y+1)*x_unit_length_+x+0];
                if(y<y_unit_length_-1 && x<x_unit_length_-1)unit->surround_[7]=&unit_[(y+1)*x_unit_length_+x+1];                
            }
        }
    }
    inline size_t get_itr_x(double x){
        size_t itr=(int)(x *field_separate_unit_x_);
        return (itr<x_unit_length_) ? itr : x_unit_length_-1;
    }
    inline size_t get_itr_y(double y){
        size_t itr=(int)(y *field_separate_unit_y_);
        return (itr<y_unit_length_) ? itr : y_unit_length_-1;
    }
    inline size_t get_itr(double x,double y){return get_itr_x(x)+get_itr_y(y)*x_unit_length_;}

    // アクセッサをリセットする．
    void clear(){
        memset(size_series_, 0, unit_length_ * sizeof(size_t));
    }
    // 隣接エリアへのアクセスを中断した場合にリセットをする必要がある．
    void reset(size_t itr){
        if(itr<unit_length_){
            unit_[itr].access_counter_=0;
            unit_[itr].surround_counter_=0;
        }
    }
    // 隣接エリアのオブジェクトへのアクセス．NULLが得られるまで回されることを想定．
    void* access(size_t itr){
        if(itr<unit_length_){
            return unit_[itr].access();
        }
        return NULL;
    }
    // フィールド内のノードを追加
    void append(double x,double y,void *pointer){
        size_t itr=get_itr(x,y);
        if(itr<unit_length_){
            unit_[itr].append(pointer);
        }
    }
    // 隣接エリア含めたオブジェクト列を作成
}FieldAccessor;

#endif

/*

//sample.c
#include <time.h>
#include <math.h>

#define CONNECT_2D(x1,y1,x2,y2,r) (fabs((x1)-(x2))>(r)) ? false : (fabs((y1)-(y2))>(r) ? false : ((r)*(r)>((x1)-(x2))*((x1)-(x2))+((y1)-(y2))*((y1)-(y2))))

int ID=0;

typedef struct Node{
    int id;
    double x;
    double y;
    Node(){
        id=ID++;
        x=0.0;
        y=0.0;
    }
}Node;

int main(){
    size_t nodes_size=200;
    Node *nodes=(Node*)malloc(sizeof(Node)*nodes_size);
    double g_XL=2000.0;
    double g_YL=2000.0;
    double g_Cr=20.0;
    FieldAccessor FA(2000,2000,20,20);
    for(size_t i=0;i<nodes_size;++i){
        Node new_node;
        nodes[i]=new_node;
    }
    int max_loop=3000;
    int cnt=0;
    {
        srand(1);
        clock_t stime=clock();
        for(int loop=0;loop<max_loop;++loop){
            for(size_t i=0;i<nodes_size;++i){
                Node &node=nodes[i];
                node.x=(double)rand()/RAND_MAX*g_XL;
                node.y=(double)rand()/RAND_MAX*g_YL;
            }

            for(size_t i=0;i<nodes_size;++i){
                Node &node = nodes[i];
                for(size_t j=0;j<nodes_size;++j){
                    if(i==j)continue;
                    Node &to = nodes[j];
                    if(CONNECT_2D(node.x,node.y,to.x,to.y,g_Cr)){
                        cnt++;
                    }
                }
            }
        }
        printf(" %d cnt %d time:%fs\n",0,cnt,(double)(clock()-stime)/CLOCKS_PER_SEC);
    }
    
    {
        srand(1);
        clock_t stime=clock();
        int cnt=0;
        for(int loop=0;loop<max_loop;++loop){
            FA.clear();
            for(size_t i=0;i<nodes_size;++i){
                Node &node=nodes[i];
                node.x=(double)rand()/RAND_MAX*g_XL;
                node.y=(double)rand()/RAND_MAX*g_YL;
                FA.append(node.x,node.y,&node);
            }

            for(size_t i=0;i<nodes_size;++i){
                Node &node = nodes[i];
                size_t itr=FA.get_itr(node.x,node.y);
                for(Node *p=(Node*)FA.access(itr);p!=NULL;p=(Node*)FA.access(itr)){
                    if(&node!=p){
                        if(CONNECT_2D(node.x,node.y,p->x,p->y,g_Cr)){
                            cnt++;
                        }
                    }
                }
            }
        
        }
        printf(" %d cnt %d time:%fs\n",1,cnt,(double)(clock()-stime)/CLOCKS_PER_SEC);
    }
    return 0;
}
*/