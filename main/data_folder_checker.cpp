#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <string.h>
#include "../utility/argumentParser.hpp"

/**
 * 指定したフォルダから
 * そのフォルダ以下に位置するすべてのcsvファイルについて，以下の条件に合わないでファイルを削除する
 * 条件
 * 1. seedから始まるheader行があること ex:seed,val1,val2...
 * 2. header行以降でコンマの数が各行で一致していること
 * 3. seed行があり，要素が1,2,3,4,...であること
 * 
 * 対応済み要素
 * ・先頭行がheader行である場合
 * ・先頭行がパラメータ行であり，それ以降にheader行がある場合
 * */

bool REMOVE_FILE=false;

/**seed列が行頭にある場合を想定して，行頭の値を取得
 * get digit in string
 * ex:
 * "123" -> 123
 * "-123" -> -1
 * "1.2.3" -> 1
 * "12,3" -> 12  */
int get_first_uint(const char *str){
    char buffer[1024];
    int digit_str_itr=0;
    while(str[digit_str_itr]!='\0'){
        if(isdigit(str[digit_str_itr])){
            buffer[digit_str_itr]=str[digit_str_itr];
            digit_str_itr+=1;
        }else{
            break;
        }
    }
    buffer[digit_str_itr]='\0';
    if(digit_str_itr>0){
        return atoi(buffer);
    }
    return -1;
}

/**
 * headerはseed,something1,something2,...の形式を想定
 * 行の文字列が"seed"から始まるかどうかを判定．
 * header行以前の行でseedから文字列が始まることは想定されない．
 */
bool is_header(const char *str){
    if(str[0]=='s' && str[1]=='e' && str[2]=='e' && str[3]=='d'){
        return true;
    }
    return false;
}

// コンマの数を数える
int count_comma(const char* string){
    int comma_counter=0;
    int string_length=strlen(string);
    for(int i=0;i<string_length;++i){
        if(string[i]==',') ++comma_counter;
    }
    return comma_counter;
}

int get_header_comma(FILE *fp){
    char buffer[1024];
    
    while(fgets(buffer,1024,fp)!=NULL){
        // bufferの先頭要素が"seed"の場合
        if(is_header(buffer)){
            return count_comma(buffer);
        }
    }
    return -1;
}
void check_csv_unexpected(const char *folder,const char *filename){
    FILE *fp;
    char buffer[1024];
    int num_of_comma=0;
    snprintf(buffer,1024,"%s/%s",folder,filename);
    if((fp=fopen(buffer,"r"))==NULL){
        fprintf(stderr, "file cant open %s\n",filename);
        return;
    }
    num_of_comma=get_header_comma(fp);
    if(num_of_comma==-1){
        printf("%s/%s is unexpected header not found\n",folder,filename);
        // to remove
        if(REMOVE_FILE){
            snprintf(buffer,1024,"%s/%s",folder,filename);
            remove(buffer);
        }
        return;
    }
    int check_value=1;
    while(fgets(buffer,1024,fp)!=NULL){
        if(check_value++!=get_first_uint(buffer)){
            printf("%s/%s is unexpected seed is not collect\n",folder,filename);
            // to remove
            if(REMOVE_FILE){
                snprintf(buffer,1024,"%s/%s",folder,filename);
                remove(buffer);
            }
            continue;
        }
        if(count_comma(buffer)!=num_of_comma){
            printf("%s/%s is unexpected comma is not same\n",folder,filename);
            // to remove
            if(REMOVE_FILE){
                snprintf(buffer,1024,"%s/%s",folder,filename);
                remove(buffer);
            }
            continue;
        }
    }
    //printf("%s/%s is safe\n",folder,filename);
    fclose(fp);
    return;
}

void check_data_file(const char * folder){
    DIR *dir;
    struct dirent *entry;
    int checked=0;
    // 検索するフォルダのパスを指定
    const char *extension = ".csv";
    // フォルダを開く
    dir = opendir(folder);
    if (dir == NULL) {
        fprintf(stderr, "dir cant open %s\n",folder);
        return;
    }
    // フォルダ内の特定の拡張子を持つファイルを表示
    while ((entry = readdir(dir)) != NULL) {
        // ファイル名の拡張子を取得
        char *fileExtension = strrchr(entry->d_name, '.');
        // 拡張子が一致する場合に表示
        if (fileExtension != NULL && strcmp(fileExtension, extension) == 0) {
            check_csv_unexpected(folder,entry->d_name);
            ++checked;
        }
        if(entry->d_type==DT_DIR && entry->d_name[0]!='.'){
            char buffer[300];
            snprintf(buffer,300,"%s/%s",folder,entry->d_name);
            check_data_file(buffer);
        }
    }
    printf("folder : %s  %d files checked\n",folder,checked);
    // フォルダを閉じる
    closedir(dir);
    return;
}

int main(int argc,char **argv){
    std::string data_folder="data";
    ArgumentParser arg("data folder checker");
    arg.add("-f","--folder",STRING,&data_folder,"data folder");
    arg.add("-r","--remove",FLAG,&REMOVE_FILE,"remove unexpected data file");
    arg.parse(argc,argv);
    printf("data folder : %s  remove : %d\n",data_folder.c_str(),REMOVE_FILE);
    check_data_file(data_folder.c_str());
    printf("check finished\n");
    return 0;
}