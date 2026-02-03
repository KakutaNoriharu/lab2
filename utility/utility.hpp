#ifndef UTILITY_HPP_
#define UTILITY_HPP_

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>

#include <string.h>
#include <stdarg.h>

/* 動的に文字列を保存する文字列バッファ */
typedef struct StringBuffer{
    char *string_;
    char buffer_[255];
    size_t string_size_;
    size_t capacity_;
    StringBuffer() : string_(NULL),string_size_(0),capacity_(0){}
    ~StringBuffer(){
        if(string_!=NULL)free(string_);
    }

    void increase_capacity(){
        capacity_ = (capacity_ == 0) ? 1 : capacity_ * 2;
        string_=(char *)realloc(string_, capacity_ * sizeof(char));
        if(string_==NULL){
            fprintf(stderr,"ReallocError StringBuffer\n");
            exit(0);
        }
    }
    /* 文字列バッファに文字列を追加 */
    void concat(char *string){
        while(string_size_+strlen(string)>capacity_){
            increase_capacity();
        }
        //char string_buffer[string_size_+strlen(string)];
        //snprintf(string_buffer,string_size_+strlen(string),"%s%s",string_,string);
        strcat(string_,string);
        string_size_=strlen(string_);
    }
    /* 文字列バッファに文字列をprintf形式で挿入 */
    void insert(const char *format,...){
        va_list va;
        va_start(va,format);
        vsnprintf(buffer_,255,format,va);
        while(string_size_+strlen(buffer_)+1>capacity_){
            increase_capacity();
        }
        //strcat(string_,buffer_);

        strcpy(string_+string_size_,buffer_);
        string_size_=strlen(string_);
        va_end(va);
    }
    /* 文字列バッファをクリア(バッファサイズはそのまま) */
    void clear(){
        if(capacity_>0)string_[0]='\0';
        string_size_=0;
    }
}StringBuffer;

// 可変長引数を扱うための再帰的な関数テンプレート
template<typename T>
void appendToStringStream(const char *,const char *end,std::ostringstream& oss, T value) {
    oss << value << end;
}
template<typename T, typename... Args>
void appendToStringStream(const char *splitter,const char *end,std::ostringstream& oss, T value, Args... args) {
    oss << value << splitter;
    appendToStringStream(splitter, end, oss, args...);
}
// 可変長引数を受け取って文字列を生成する関数 ex:{arg1}{splitter}{srg2}{splitter}{arg3}{end};
template<typename... Args>
std::string getString(const char *splitter,const char *end,Args... args) {
    std::ostringstream oss;
    appendToStringStream(splitter, end, oss, args...);
    return oss.str();
}

class toString { // unverified
    private:
        std::string splitter;
        std::string end;
        std::stringstream ss;
        bool first;
    
    public:
        toString(const std::string& splitter_ = "", const std::string& end_ = "")
            : splitter(splitter_), end(end_), first(true) {}
    
        // 可変引数テンプレート再帰の終了条件
        void append_impl() {
            ss << end;
        }
    
        // 最初の引数を処理し、残りを再帰的に処理
        template<typename T, typename... Args>
        void append_impl(T first_arg, Args... rest) {
            if (!this->first) {
                ss << splitter;
            }
            ss << first_arg;
            this->first = false;
            append_impl(rest...);
        }
    
        // 公開インターフェース
        template<typename... Args>
        void append(Args... args) {
            append_impl(args...);
        }
    
        // 結果を文字列として取得
        std::string result() const {
            return ss.str();
        }
    
        // バッファをクリア
        void clear() {
            ss.str("");
            ss.clear();
            first = true;
        }
    };

std::string createIndexedString(const std::string& str, int num, const std::string& separator) {
    std::ostringstream result;
    for (int i = 0; i < num; ++i) {
        result << str << i; // 基本文字列と修飾数を結合
        if (i < num - 1) {  // 最後の要素以外には区切り文字を追加
            result << separator;
        }
    }
    return result.str();
}

#include <chrono>
#include <time.h>

// elapsed time information
inline std::string get_sec_to_hms_str(double seconds){
    std::ostringstream oss;
    double minutes = seconds / 60;
    double hours = minutes / 60;
    oss<<static_cast<int>(hours)<<":"<<static_cast<int>(minutes)%60<<":"<<static_cast<int>(seconds)%60;
    return oss.str(); 
}

typedef struct TimeManager{
    std::chrono::system_clock::time_point start_time;
    TimeManager() : start_time(std::chrono::system_clock::now()){};
    void reset(){
        start_time=std::chrono::system_clock::now();
    }
    // elapsed time [s]
    double get_elapsed_time_sec(){
        std::chrono::duration<double> elapsed = std::chrono::system_clock::now()-start_time;
        return elapsed.count();
    }
    // remain time [s]
    double get_remain_time_sec(double progress_rate){
        return get_elapsed_time_sec()/progress_rate*(1-progress_rate);
    }
    // elapsed time information
    inline std::string get_elapsed_time_str(){
        return get_sec_to_hms_str(get_elapsed_time_sec());
    }
    // remain time information
    inline std::string get_remain_time_str(double progress_rate){
        return get_sec_to_hms_str(get_remain_time_sec(progress_rate));
    }
}TimeManager;


#include <cstring>

template <typename T>
bool SearchParameterArg(int arg_c, char *arg_v[], const char *arg, T *value) {
    for (int i = 1; i < arg_c; ++i) {
        if (strcmp(arg_v[i], arg) == 0 && (i + 1) < arg_c){
            *value = (T)atof(arg_v[i + 1]);
            return false;
        }
    }
    return true;
}


/**関数: 可変長引数を受け取り、適用状態を整数として返す
 * 総数とそれぞれの適用状態(bool or 0,1) */
int calculate_state(int num_ids, ...) {
    int state = 0; // 適用状態を表す整数
    // 可変長引数リストを初期化
    va_list args;
    va_start(args, num_ids);
    for (int i = 0; i < num_ids; i++) {
        // 各項目の適用状態（0または1）を取得
        int is_applied = va_arg(args, int);
        // 適用状態を整数として構築
        if (is_applied>=0)
        if (is_applied) {
            state |= (1 << i); // i番目のビットを1にセット
        }
    }
    // 可変長引数リストを終了
    va_end(args);
    return state;
}


int countCSVRows(const std::string& filename){
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file (countCSVRows): " << filename << std::endl;
        return -1;
    }

    int rowCount = 0;
    std::string line;
    std::string pre_line;
    while (std::getline(file, line)) {
        rowCount++;
        pre_line=line;
    }
    file.close();
    size_t pos = pre_line.find_first_of("0123456789");
    if (pos != std::string::npos) {
        std::string num_str = pre_line.substr(pos, pre_line.find_first_not_of("0123456789", pos) - pos);
        int num = std::stoi(num_str)+1;
        return num;
    }
    return rowCount;
}

// 配列計算
#include <math.h>
double cal_mean_double_vector(std::vector<double> v){
    double s = 0.0;
    for(double x : v){
        s += x;
    }
    return s/(double)v.size();
}

double cal_std_double_vector(std::vector<double> v){
    double mean = cal_mean_double_vector(v);
    double s = 0.0; //diff squared sum
    for(double x :v){
        s += (x-mean)*(x-mean);
    }
    return sqrt(s/(double)v.size());
}

std::vector<double> cal_diff_double_vector(std::vector<double> v){
    std::vector<double> diff;
    if(v.size()>0){
        double p = v.at(0);
        for(size_t i=1,size=v.size();i<size;++i){
            diff.push_back(v.at(i)-p);
            p = v.at(i);
        }
    }
    return diff;
}
#endif