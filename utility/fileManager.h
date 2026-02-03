#ifndef FILE_MANAGER_H_
#define FILE_MANAGER_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

/**
 * ファイル操作関係の機能をまとめたファイル
 * ディレクトリ作成，ファイルの存在確認，ファイル名の変更，ファイル名の重複確認
 */


/**
 * ファイルの存在確認
 * 存在すればTrueを返す
 */
bool file_is_exist(const char *file_name){
    struct stat st;
    return (stat(file_name,&st)==0);
}

/**
 * split string by splitter
 * if splitter in string this character to '\0' */
size_t string_split(char *string,const char splitter){
    size_t count=1;
    size_t len=strlen(string);
    for(size_t i=0;i<len;++i){
        if(strncmp(&string[i],&splitter,1)==0){
            string[i]='\0';
            count+=1;
        }
    }
    return count;
}

/**make filename
 * argument base file name
 * return fixed file name
 * if file is exist
 * add number to file name  ex "abc.txt" -> "abc(1).txt" */
void make_new_file_name(const char *base_file_name, char *new_file_name, size_t size) {
    char buffer[235];
    char file_extension[10] = "";  // 初期化しておく
    short int file_number = 1;
    
    strncpy(buffer, base_file_name, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    // 最後の '.' を探す
    char *last_dot = strrchr(buffer, '.');
    if (last_dot != NULL) {
        // 拡張子を取得
        strncpy(file_extension, last_dot + 1, sizeof(file_extension) - 1);
        file_extension[sizeof(file_extension) - 1] = '\0';
        
        // `.` を区切りとして、ファイル名部分を切り出す
        *last_dot = '\0';  // `.` を終端文字に置き換える
    }

    // ファイルが存在する場合、番号を付けて新しい名前を作成
    strncpy(new_file_name, base_file_name, size);
    new_file_name[size - 1] = '\0';

    while (file_is_exist(new_file_name)) {
        if (*file_extension) {
            snprintf(new_file_name, size, "%s(%d).%s", buffer, file_number++, file_extension);
        } else {
            snprintf(new_file_name, size, "%s(%d)", buffer, file_number++);
        }
    }
}



/**
 * if receive file name "abc/def/test.txt"
 * make directory abc/def/ */
void make_file_directory(const char *directory_string){
    char buffer[255];
    strcpy(buffer,directory_string);
    size_t len=strlen(buffer);
    string_split(buffer,'/');
    size_t cnt=strlen(buffer);
    while(cnt<len){
        struct stat st;
        if(stat(buffer,&st)!=0){
            int result=0;
            #if defined(_WIN32)
            result=mkdir(buffer);
            #else 
            result=mkdir(buffer, 0777);
            #endif
            if(result!=0){// 0777: all allow
                printf("mkdir %s fail\n",buffer);
            }
        }
        buffer[cnt]='/';
        cnt=strlen(buffer);
    }
}

void file_rename(const char* old_file_name,const char* new_file_name){
    make_file_directory(new_file_name);    
    if(rename(old_file_name,new_file_name)!=0){
        fprintf(stderr, "file rename fail %s -> %s\n",old_file_name,new_file_name);
    }
}

#endif