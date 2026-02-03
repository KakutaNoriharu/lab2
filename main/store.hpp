#ifndef STORE_HPP_
#define STORE_HPP_

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <set>
#include "../utility/utility.h"

// データ全体を保存する実体　ほとんどvector
template<typename T>
class GlobalStore{
private:
public:
    std::vector<T> events; // 任意の型のイベントデータを保存するvector
    size_t require_size; // イベントの保存に必要なサイズを共有するための変数
    size_t size; // イベント全体の数
    ~GlobalStore() noexcept = default;
    GlobalStore(const GlobalStore& other) = default;
    GlobalStore(GlobalStore&& other) noexcept = default;
    GlobalStore & operator=(const GlobalStore& other) = default;

    GlobalStore(){
        initialize();
    }
    void initialize(){
        events.clear();
        require_size=0;
        size=0;
    }
    size_t append(T data){
        events.push_back(data); // 予めデータを予約することで高速化できるかもしれない
        return size++;
    }
};

/**
 * ユーザー毎のデータを蓄積する実体
 * Event:保存するイベントのデータ構造
 * EventMap:Eventに基づいて，記録されていくデータ．
 */
template <typename Event,typename EventMap>
class Store{
public:
    GlobalStore<Event> *GS; // イベントの実体が保存されているGSへのアクセスポインタ
    std::vector<int> store; // binary value  1:have 0:not have
    size_t size; // store.size()
    size_t count; // stored event count
    Store(){
        GS=NULL;
        size=0;
        store.clear();
    }
    ~Store() noexcept = default;
    Store(const Store& other) = default;
    Store(Store&& other) noexcept = default;
    Store & operator=(const Store& other) = default;
    
    // イベントインデックスを受け取り，storeを受け取り状態に更新，イベントに対する処理は含まれない
    void add(size_t index){
        if(index+1>=store.size()){
            store.resize(index+1,0); // capacityが不足する場合はpush_backと同様に二倍になる
            size=store.size();
        }
        store.at(index)=1;
    }
    /**イベントのシェア
     * 別のstoreを受け取り，自身のstoreに含まれていない場合，storeに加え，
     * 加えたイベントに基づきEventMapを更新，更新処理は引数の関数を利用*/
    void share(Store *share_store,std::vector<EventMap> *eMap,void (add_process)(Event*,std::vector<EventMap>*)){
        std::vector<int> *share_data=&share_store->store;
        size_t share_size=share_store->size;
        if(share_size>=store.size()){
            store.resize(share_size+1,0);
            size=store.size();
        }
        if(eMap->size()<=GS->require_size){
            eMap->resize(GS->require_size);
        }
        for(size_t i=0;i<share_size;++i){
            if(share_data->at(i)==1 && store.at(i)==0){
                store.at(i)=1;
                add_process(&GS->events.at(i),eMap);
            }
        }
    }
    /**イベントのシェア
     * 別のstoreを受け取り，自身のstoreに含まれていない場合，storeに加え，
     * 加えたイベントに基づきEventMapを更新，更新処理は引数の関数を利用*/
    void share2(Store *share_store,std::vector<EventMap> *eMap,std::vector<int> *node_map,void (add_process)(std::vector<int>*,Event*,std::vector<EventMap>*)){
        std::vector<int> *share_data=&share_store->store;
        size_t share_size=share_store->size;
        if(share_size>=store.size()){
            store.resize(share_size+1,0);
            size=store.size();
        }
        if(eMap->size()<=GS->require_size){
            eMap->resize(GS->require_size);
        }
        for(size_t i=0;i<share_size;++i){
            if(share_data->at(i)==1 && store.at(i)==0){
                store.at(i)=1;
                add_process(node_map,&GS->events.at(i),eMap);
            }
        }
    }

    // share_storeと比較して保持していないイベントデータを追加する
    /**
     * @brief StoreのEventMapの共有
     * @param share_store 共有されたストア
     * @param map データの更新先map
     * @param add_process データの更新関数
     */
    void update(Store *share_store,EventMap *map,void (add_process)(Event*,EventMap*)){
        std::vector<int> *share_data=&share_store->store;
        size_t share_size=share_store->size;
        if(share_size>=store.size()){
            store.resize(share_size+1,0);
            size=store.size();
        }
        for(size_t i=0;i<share_size;++i){
            if(share_data->at(i)==1 && store.at(i)==0){
                store.at(i)=1;
                add_process(&GS->events.at(i),map);
            }
        }
    }
};

#endif