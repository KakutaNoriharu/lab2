#ifndef _LOGGER_
#define _LOGGER_
#include <stdio.h>

/**
 * プログラムの処理を追うためにログを残す機能をまとめたものです
 * 
 * 使い方
 * main関数が記述されるファイルの
 * ログのレベルを指定しながら，ログを出力します．
 * ログのレベルはDEBUG,INFO,WARNING,ERRORの四種類あり，それぞれログの重要度によって振り分けてください．
 * LOGGER_PRINT,LOGGER_DATA_PRINTによって，ログを出力します．２つの違いは変数の内容を出力するかどうかです．
 * この二つの使い方はprintfと同じです．
 * main関数でLOGGER_SETを呼び出してログが出力されるファイル名と，出力するログのレベルを指定します．
 * main関数の最後にLOGGER_CLOSE()を呼び出して，ファイルポインタを閉じます．
 * 
 * 注意
 * LOGGER_SETでファイルが開けない場合のエラーハンドリングは実装されていないので，ログがうまく出力されない可能性があります．
*/

typedef enum LOG_LEVELS{
    DEBUG,      // デバック用の内容
    INFO,       // 基本的な内容
    WARNING,    // 想定外の動作の場合に表示
    ERROR,      // 致命的な動作の場合に表示
    LOG_LEVELS_LENGTH
}LOG_LEVELS;

#ifdef LOGGER_USE
    LOG_LEVELS OUTPUT_LOG_LEVEL;
    FILE *LoggerFilePointer;

    #define LOGGER_DATA_PRINT(level,fmt,...) do{if(LoggerFilePointer!=NULL && level>=OUTPUT_LOG_LEVEL) fprintf(LoggerFilePointer,"%s : %s (%d) %s : " fmt "\n",#level,__FILE__, __LINE__, __func__,__VA_ARGS__);fflush(LoggerFilePointer);}while(0)
    #define LOGGER_PRINT(level,fmt) do{if(LoggerFilePointer!=NULL && level>=OUTPUT_LOG_LEVEL) fprintf(LoggerFilePointer,"%s : %s (%d) %s : " fmt "\n",#level,__FILE__, __LINE__, __func__);fflush(LoggerFilePointer);}while(0)
    #define LOGGER_SET(file_name,level) do{LoggerFilePointer=fopen(file_name,"w");OUTPUT_LOG_LEVEL=level;LOGGER_PRINT(INFO,"LOGGER START");}while(0)
    #define LOGGER_OUT(output,level) do{LoggerFilePointer=output;OUTPUT_LOG_LEVEL=level}while(0)
    #define LOGGER_CLOSE() do{LOGGER_PRINT(INFO,"LOGGER CLOSE");fclose(LoggerFilePointer);}while(0)
#else
    #define LOGGER_DATA_PRINT(level,fmt,...) {};
    #define LOGGER_PRINT(level,fmt) {};
    #define LOGGER_SET(file_name,level) {};
    #define LOGGER_OUT(output,level) {};
    #define LOGGER_CLOSE() {};
#endif

#endif


/**
 * sample.c
#include <stdio.h>
#include <stdlib.h>
#include "logger.h"

int main(){
    //LOGGER_SET("z_main.log",INFO); // ファイルへの出力
    LOGGER_OUT(stdout,INFO); // printfと同じ，標準出力への出力
    for(int i=0;i<10;++i){
        LOGGER_DATA_PRINT(INFO,"test:%d",i);
        if(i>0){
            printf("%f",1.0/(double)i);
        }else{
            LOGGER_DATA_PRINT(ERROR,"division by %d",i);
        }
    }
    LOGGER_PRINT(DEBUG,"complete")
    LOGGER_CLOSE(); //
    return 0;
}
 */