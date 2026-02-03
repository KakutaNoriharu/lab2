#ifndef ARGUMENT_WRAPPER_H_
#define ARGUMENT_WRAPPER_H_

#include <stdio.h>

/**
 * main関数wrapperのためのwrapper
 * 機能
 * ・wrapperでmain関数の引数を固定してしまうので，ここで予めグローバル変数として，引数を保存している．
 * 
 * 使い方
 * ・wrapperを定義したヘッダファイルなどにインクルードする．
 */

int __wrapped_main__();

int argc=0; // global 引数カウンタ
char** argv=NULL; // global 引数リスト

int main(int argc_,char** argv_){
    argc=argc_;
    argv=argv_;
    __wrapped_main__();
    return 0;
}

#define main __wrapped_main__
#endif