#ifndef PARAMETERS_HPP_
#define PARAMETERS_HPP_

#include "../utility/utility.h"
#include "../utility/utility.hpp"
#include "../utility/parameterManager.hpp"
#include "../utility/parameterLoader.hpp"
#include "../utility/argumentParser.hpp"

/**
 * 2025-02-18
 * 更新途中
 * parameterLoaderを使えるように更新する
 * TAのパラメータについて柔軟性を追加する
 */

// add 1208
std::string road_prob_data_file_path=""; 
int use_scheme_3=0;

std::string simulation_result_file_path="notitle"; // シミュレーション結果を保存するファイルの名前
double Dt = 1.0; //Delta_T [t]

double g_Lambda = 0.01; //Poisson_Enter_Lambda_Second [s^-1]
double g_Lambda_LEFT = 0.01; //From LEFT edge of Area : Poisson_Enter_Lambda_Second [s^-1]
double g_Lambda_RIGHT = 0.01; //From RIGHT edge of Area : Poisson_Enter_Lambda_Second [s^-1]
double g_Vm = 1.0; //Node_Velocity_mean [m/s]
double g_Vw = 0.0; //Node_Velocity_width [m/s]
double g_fVm = 1.0; //Malicious_Node_Velocity_mean [m/s]
double g_Slp = 0.0; //Malicious_Node_Sleep_Second [s]　ATTENTION：EVENTに非対応，うまく動作しないはず．

double g_Cr = 20.0; //Connectable_Range [m]
double g_Pt = 0.5; // Probability turn

double g_XL = 4000.0; // Field_X_Length [m]
double g_YL = 4000.0; // Field_Y_Length [m]
double g_EL = 200.0; // Field_Enter_Route_Length [m]
std::pair<int,int> g_Nxy = {4,4}; // Grid X*Y

/**
 * TAの設定について，柔軟に指定したい
 * 要件
 * .prmで設定可能なこと
 * 長方形を組み合わせた形状であること
 *
 * 課題
 * .prmでは，TAの設定は複数扱える状態であってほしいが，今は記述単位が[x_size y_size]であり，単一の長方形しか扱えない
 * また，TAの位置がサービスエリアの中心に固定されている．
 *
 * 方針
 * TAの長方形を複数指定する方法を考える
 * 扱う変数
 * ・TAで必要になる長方形の数：TA_parts_num (int)
 * ・長方形の位置：TA_parts_pos (std::vector<std::pair<double,double> >)
 * ・長方形の大きさ：TA_parts_size (std::vector<std::pair<double,double> >)
 *
 * .prmでの記述
 * TA,
 */

std::vector<std::vector<double>> g_vTA = {{g_XL*0.5,g_YL*0.5,1000.0,1000.0}};
std::vector<std::vector<double>> g_vfTA = {{g_XL*0.5,g_YL*0.5,1000.0,1000.0}};
std::vector<std::vector<double>> g_vRA = {};
std::vector<std::vector<double>> g_vfRA = {};

double g_TA_START_X,g_TA_START_Y,g_TA_END_X,g_TA_END_Y;

int g_malicious_appear_time=1000; // malicious node appear time
int g_malicious_appear_interval=0; // malicious node appear interval
int g_malicious_appear_num=1; // malicious node appear num
int g_malicious_appear_type=1; // malicious node appear type, 0:not appear, 1:normal, 2:not defined

/**受信した情報の処理
 * 0 : 未定義
 * 1 : 新しく受信した情報で更新
 * 2 : 未定義
 * 3 : 初めて受け取った情報更新，以降更新しない
 * 4 : 偽装された情報を受信していればそちらを信用
 * 5 : 受信回数の多い方を利用 */
int g_method=1;

/**IFの開始方法
 * 0 : 移動ノードによりIFを開始する
 * 1 : FSによりIFを開始する */
int g_start=0;

/**攻撃者の行動 ATTENTION：EVENTに非対応，単一のノードにのみ効果を持つので注意
 * 0 : 悪意のあるノードが出現しない
 * 1 : TAを真っ直ぐに通過する
 * 2 : TAの端で停止したのち，移動する
 * 3 : TAの中心で停止したのち，移動する */ //形骸化
int g_attack=1;

/**
 * 0 : NORMAL いつもの，経路は2つから選択，p_turnで曲がる
 * 1 : UNIFORM 全ての点で，進行方向によらず，1/4のランダムに選択する
 * 2 : TURN 全ての点で，必ず左右のどちらかに曲がる，曲がる方向は1/2のランダム
 * 3 : NO_U_TURN Uターンを除いた3つの経路から確率1/3でランダムに選択
 * 4 : RANDOM 全ての経路に対してランダムに曲がる確率が割り振られる
 * 5 : RANDOM_WO_U_TURN Uターンを除く3つの経路に対して選択する確率をランダムに割り振る */
int g_route=0;

int malicious_fake_id=0; // 攻撃者が時間に応じてidを偽装するかどうか
int distinct_fake=1; // 偽装された情報を区別できるか 0:区別しない，1:区別する(信用する1つを配信)
int consider_all_TA=1; // ノードがいくつかのTA情報を保持する場合に，利用する情報だけでなく，すべてのTA情報を考慮して配信するかどうか，1:配信する情報のTA内で受信せずとも許可，2:受信が必要
int share_replys=0; // TODO:ノードが情報にreplyを付加し，これを交換するかどうか
int fix_field=0; // パラメータNxyに応じてフィールドの大きさを制御するかどうか，制御する場合はNx=2000*(Nx) if Nx>0 else 2000
int use_diff_lambda=0; // SAの両端について，到着率を差を作るかどうか
int simulation_max_loop=50000; // シミュレーション時間

int g_tmp=0; // テストパラメータ，任意の位置に使用可能

void add_arguments(ArgumentParser *parser){
    parser->add("-l","--Lambda",DOUBLE,&g_Lambda,"Poisson_Enter_Lambda_Second [s^-1]");
    parser->add("-L","--Lambda_LEFT",DOUBLE,&g_Lambda_LEFT,"From LEFT edge of Area : Poisson_Enter_Lambda_Second [s^-1]");
    parser->add("-R","--Lambda_RIGHT",DOUBLE,&g_Lambda_RIGHT,"From RIGHT edge of Area : Poisson_Enter_Lambda_Second [s^-1]");
    parser->add("-v","--Vm",DOUBLE,&g_Vm,"Node_Velocity_mean [m/s]");
    parser->add("-fv","--fVm",DOUBLE,&g_fVm,"Malicious_Node_Velocity_mean [m/s]");
    parser->add("-slp","--AttackerSleepTime",DOUBLE,&g_Slp,"Malicious_Node_Sleep_Second [s]");
    parser->add("-vw","--Vw",DOUBLE,&g_Vw,"Node_Velocity_width [m/s]");
    parser->add("-cr","--Cr",DOUBLE,&g_Cr,"Connectable_Range [m]");
    parser->add("-trun","--P_turn",DOUBLE,&g_Pt,"Probability turn");
    parser->add("-x","--XL",DOUBLE,&g_XL,"Field_X_Length [m]");
    parser->add("-y","--YL",DOUBLE,&g_YL,"Field_Y_Length [m]");
    parser->add("-e","--EL",DOUBLE,&g_EL,"Field_Enter_Route_Length [m]");
    parser->add("-n","--Nxy",INT_PAIR,&g_Nxy,"Grid X*Y ex:[-n {Nx} {Ny}]");
    parser->add("-TA","--vTA",DOUBLE_VECTOR_VECTOR,&g_vTA,"TA setting ex:[-TA loc.x loc.y size.x size.y]",4);
    parser->add("-fTA","--vfTA",DOUBLE_VECTOR_VECTOR,&g_vfTA,"fake TA setting ex:[-fTA loc.x loc.y size.x size.y]",4);
    parser->add("-RA","--vRA",DOUBLE_VECTOR_VECTOR,&g_vRA,"RA setting ex:[-RA loc.x loc.y size.x size.y]",4);
    parser->add("-fRA","--vfRA",DOUBLE_VECTOR_VECTOR,&g_vfRA,"fake RA setting ex:[-fRA loc.x loc.y size.x size.y]",4);
    
    parser->add("-matime","--malicious_appear_time",TypeName::INT,&g_malicious_appear_time,"malicious node appear time ex:[-matime {int}]");
    parser->add("-manum","--malicious_appear_num",TypeName::INT,&g_malicious_appear_num,"malicious node appear num ex:[-manum {int}]");
    parser->add("-mainter","--malicious_appear_interval",TypeName::INT,&g_malicious_appear_interval,"malicious node appear time ex:[-mainter {int}]");
    parser->add("-matype","--malicious_appear_type",TypeName::INT,&g_malicious_appear_type,"malicious node appear time ex:[-matype {int}]");

    parser->add("-md","--method",INT,&g_method,"Method");
    parser->add("-start","--start",INT,&g_start,"Start");
    parser->add("-atk","--attack",INT,&g_attack,"Attack");
    parser->add("-route","--route",INT,&g_route,"Route");
    parser->add("-fakeId","--malicious_fake_id",INT,&malicious_fake_id,"malicious_fake_id");
    parser->add("-fake","--distinct_fake",INT,&distinct_fake,"distinct_fake");
    parser->add("-cata","--consider_all_TA",INT,&consider_all_TA,"consider_all_TA");
    parser->add("-fix_field","--fix_field",INT,&fix_field,"fix_field");
    parser->add("-dif_lambda","--use_diff_lambda",INT,&use_diff_lambda,"use_diff_lambda");
    parser->add("-reply","--reply_share",INT,&share_replys,"post reply and share");
    parser->add("-loop","--simulation_max_loop",INT,&simulation_max_loop,"simulation_max_loop");
    parser->add("-tmp","--template",INT,&g_tmp,"template parameter");
}

std::vector<std::pair<std::string,int>> parameter_id_map;

typedef struct Params{
    double Lambda = 0.001; //Poisson_Enter_Lambda_Second [s^-1]
    double Lambda_LEFT = 0.001; //From LEFT edge of Area : Poisson_Enter_Lambda_Second [s^-1]
    double Lambda_RIGHT = 0.001; //From RIGHT edge of Area : Poisson_Enter_Lambda_Second [s^-1]
    double Vm = 1.0; //Node_Velocity_mean [m/s]
    double Vw = 0.0; //Node_Velocity_width [m/s]
    double fVm = 1.0; //Malicious_Node_Velocity_mean [m/s]
    double Slp = 3000.0; //Malicious_Node_Sleep_Second [s]
    double Cr = 20.0; //Connectable_Range [m]
    double Pt = 0.5; // Probability turn

    double XL = 2000.0; // Field_X_Length [m]
    double YL = 2000.0; // Field_Y_Length [m]
    double EL = 200.0; // Field_Enter_Route_Length [m]
    std::pair<int,int> Nxy = {5,5};

    /*
    std::pair<double,double> LTA = {1000.0,1000.0};
    std::pair<double,double> LRA = {0.0,0.0};
    std::pair<double,double> LfTA = {1000.0,1000.0};
    std::pair<double,double> LfRA = {0.0,0.0};
    */

    std::vector<std::vector<double>> vTA = {{XL*0.5,YL*0.5,1000.0,1000.0}}; // {center_x, center_y, size_x, size_y}
    std::vector<std::vector<double>> vfTA = {{XL*0.5,YL*0.5,1000.0,1000.0}}; // {center_x, center_y, size_x, size_y}
    std::vector<std::vector<double>> vRA = {}; // {center_x, center_y, size_x, size_y}
    std::vector<std::vector<double>> vfRA = {}; // {center_x, center_y, size_x, size_y}

    double TA_START_X = XL;
    double TA_START_Y = YL;
    double TA_END_X = 0.0;
    double TA_END_Y = 0.0;

    int malicious_appear_time=5000;
    int malicious_appear_interval=0;
    int malicious_appear_num=1;
    int malicious_appear_type=1;
    
    /**
     * 0 : 未定義
     * 1 : 新しく受信した情報で更新
     * 2 : 未定義
     * 3 : 初めて受け取った情報更新，以降更新しない
     * 4 : 偽装された情報を受信していればそちらを信用
     * 5 : 受信回数の多い方を利用 */
    int Method=1;

    /**IFの開始方法
     * 0 : 移動ノードによりIFを開始する
     * 1 : FSによりIFを開始する */
    int Start=0;

    /**
     * 0 : 悪意のあるノードが出現しない
     * 1 : TAを真っ直ぐに通過する
     * 2 : TAの端で停止したのち，移動する
     * 3 : TAの中心で停止したのち，移動する */
    int Attack=1;

    std::unordered_map<std::string,int> parameter_setting_itr;
    // initialize
    Params(){
        this->Lambda=g_Lambda;
        this->Lambda_LEFT=g_Lambda_LEFT;
        this->Lambda_RIGHT=g_Lambda_RIGHT;
        this->Vm=g_Vm;
        this->Vw=g_Vw;
        this->fVm=g_fVm;
        this->Slp=g_Slp;
        this->Cr=g_Cr;
        this->Pt=g_Pt;
        this->XL=g_XL;
        this->YL=g_YL;
        this->EL=g_EL;
        this->malicious_appear_time=g_malicious_appear_time;
        this->malicious_appear_interval=g_malicious_appear_interval;
        this->malicious_appear_num=g_malicious_appear_num;
        this->malicious_appear_type=g_malicious_appear_type;
        this->Nxy=g_Nxy;
        this->vTA=g_vTA;
        this->vRA=g_vRA;
        this->vfTA=g_vfTA;
        this->vfRA=g_vfRA;
        this->Method=g_method;
        this->Start=g_start;
        this->Attack=g_attack;
        this->set_params();
    }
    Params(ParameterManager *parameter_manager){
        this->Lambda=g_Lambda;
        this->Lambda_LEFT=g_Lambda_LEFT;
        this->Lambda_RIGHT=g_Lambda_RIGHT;
        this->Vm=g_Vm;
        this->Vw=g_Vw;
        this->fVm=g_fVm;
        this->Slp=g_Slp;
        this->Cr=g_Cr;
        this->Pt=g_Pt;
        this->XL=g_XL;
        this->YL=g_YL;
        this->EL=g_EL;
        this->malicious_appear_time=g_malicious_appear_time;
        this->malicious_appear_interval=g_malicious_appear_interval;
        this->malicious_appear_num=g_malicious_appear_num;
        this->malicious_appear_type=g_malicious_appear_type;
        this->Nxy=g_Nxy;
        this->vTA=g_vTA;
        this->vRA=g_vRA;
        this->vfTA=g_vfTA;
        this->vfRA=g_vfRA;
        this->Method=g_method;
        this->Start=g_start;
        this->Attack=g_attack;
        this->set_params();
        this->set_itr(parameter_manager);
    }
    Params(
        double Lambda_, double Lambda_LEFT_, double Lambda_RIGHT_,
        double Vm_, double Vw_, double fVm_, double Slp_,
        double Cr_, double Pt_,
        double XL_, double YL_, double EL_,
        int malicious_appear_time_,
        int malicious_appear_interval_,
        int malicious_appear_num_,
        int malicious_appear_type_,
        std::pair<int,int> Nxy_,
        std::vector<std::vector<double>> vTA_, std::vector<std::vector<double>> vRA_,
        std::vector<std::vector<double>> vfTA_, std::vector<std::vector<double>> vfRA_,
        int Method_, int Start_, int Attack_
    ){
        this->Lambda = Lambda_;
        this->Lambda_LEFT = Lambda_LEFT_;
        this->Lambda_RIGHT = Lambda_RIGHT_;
        this->Vm = Vm_;
        this->Vw = Vw_;
        this->fVm = fVm_;
        this->Slp = Slp_;
        this->Cr = Cr_;
        this->Pt = Pt_;
        this->XL = XL_;
        this->YL = YL_;
        this->EL = EL_;
        this->malicious_appear_time = malicious_appear_time_;
        this->malicious_appear_interval = malicious_appear_interval_;
        this->malicious_appear_num = malicious_appear_num_;
        this->malicious_appear_type = malicious_appear_type_;
        this->Nxy = Nxy_;
        this->vTA = vTA_;
        this->vRA = vRA_;
        this->vfTA = vfTA_;
        this->vfRA = vfRA_;
        this->Method = Method_;
        this->Start = Start_;
        this->Attack = Attack_;
        set_params();
    }

    void set_params(){ // 最大範囲でくくる unverified
        TA_START_X=XL*0.5;
        TA_START_Y=YL*0.5;
        TA_END_X=XL*0.5;
        TA_END_Y=YL*0.5;

        for(size_t i=0;i<vTA.size();i++){
            TA_START_X = MIN(TA_START_X, vTA[i][0]-vTA[i][2]*0.5);
            TA_END_X = MAX(TA_END_X, vTA[i][0]+vTA[i][2]*0.5);
            TA_START_Y = MIN(TA_START_Y, vTA[i][0+1]-vTA[i][2+1]*0.5);
            TA_END_Y = MAX(TA_END_Y, vTA[i][0+1]+vTA[i][2+1]*0.5);
        }
    }

    void set_itr(ParameterManager *parameter_manager){
        for(std::pair<std::string, int>& param_pair : parameter_id_map){
            parameter_setting_itr[param_pair.first] = (int)parameter_manager->parameter_itr[param_pair.second];
        }
    }

    void describe(){
        printf("Delta T              : %.2f [s]\n",Dt);
        printf("Field range          : %.1f, %.1f [m]\n",XL,YL);
        printf("Enter route range    : %.1f [m]\n",EL);
        printf("Grid map point       : %d, %d\n",Nxy.first,Nxy.second);
        printf("TA size S/E  X       : %.2f / %.2f [m]\n",TA_START_X,TA_END_X);
        printf("             Y       : %.2f / %.2f [m]\n",TA_START_Y,TA_END_Y);
        printf("Poisson Lambda       : %.5f [1/s]\n",Lambda);
        if(use_diff_lambda)printf("Poisson Lambda L/R   : %.5f / %.5f [1/s]\n",Lambda_LEFT,Lambda_RIGHT);
        printf("Velocity mean/width  : %.2f / %.2f [m/s]\n",Vm,Vw);
        printf("Malicious mean/sleep : %.2f / %.2f [m/s]\n",fVm,Slp);
        printf("Connectable range    : %.2f [m]\n",Cr);
        printf("Probability of turn  : %.2f [m]\n",Pt);
    }

    std::string get_parameter_string(){
        toString str("_","");
        str.append(
            SHOW(Lambda),
            SHOW(Vm),
            SHOW(Vw),
            SHOW(fVm),
            SHOW(Slp),
            SHOW(Cr),
            SHOW(Pt),
            "Nx",Nxy.first,
            "Ny",Nxy.second,
            SHOW(XL),
            SHOW(YL),
            SHOW(malicious_appear_time),
            SHOW(malicious_appear_interval),
            SHOW(malicious_appear_num),
            SHOW(malicious_appear_type),
            SHOW(Method),
            SHOW(Start),
            SHOW(Attack),
            SHOW(malicious_fake_id),
            SHOW(distinct_fake),
            SHOW(consider_all_TA),
            SHOW(fix_field),
            SHOW(use_diff_lambda),
            SHOW(simulation_max_loop)
        );
        toString s_vTA("~","");
        s_vTA.append("TA");
        for(std::vector<double> v : vTA){
            s_vTA.append(v[0]);
            s_vTA.append(v[1]);
            s_vTA.append(v[2]);
            s_vTA.append(v[3]);
            s_vTA.append("e");
        }
        str.append(s_vTA.result());
        toString s_vRA("~","");
        s_vRA.append("RA");
        for(std::vector<double> v : vRA){
            s_vRA.append(v[0]);
            s_vRA.append(v[1]);
            s_vRA.append(v[2]);
            s_vRA.append(v[3]);
            s_vRA.append("e");
        }
        str.append(s_vRA.result());
        toString s_vfTA("~","");
        s_vfTA.append("fTA");
        for(std::vector<double> v : vfTA){
            s_vfTA.append(v[0]);
            s_vfTA.append(v[1]);
            s_vfTA.append(v[2]);
            s_vfTA.append(v[3]);
            s_vfTA.append("e");
        }
        str.append(s_vfTA.result());
        toString s_vfRA("~","");
        s_vfRA.append("fRA");
        for(std::vector<double> v : vfRA){
            s_vfRA.append(v[0]);
            s_vfRA.append(v[1]);
            s_vfRA.append(v[2]);
            s_vfRA.append(v[3]);
            s_vfRA.append("e");
        }
        str.append(s_vfRA.result());
        return str.result();
    }

    std::string get_parameter_string_filename(){
        toString str("_","");
        if(parameter_id_map.size()>0){
            for(std::pair<std::string,int> p : parameter_id_map){
                str.append(p.first,parameter_setting_itr[p.first]);
            }
        }else{
            str.append("default"); // todo 区別できるようにする
        }
        return str.result();
    }
}Params;

// 任意の文字列stringについて文字startから文字endまで，startとendを含まない形で抜き出した文字列をbufferに保存する関数
void string_extractor(const char* string, const char* start,const char* end,char *buffer){
    size_t i=0;
    while(string[i]!='\0'){
        if(string[i]==*start || string[i]=='\n'){
            ++i;
            break;
        }
        ++i;
    }
    size_t j=i;
    while(string[j]!='\0'){
        if(string[j]==*end || string[j]=='\n')break;
        ++j;
    }
    if(j<strlen(string)){
        strncpy(buffer,&string[i],j-i);
        buffer[j-i]='\0';
    }else{
        sprintf(buffer,"%s",string);
    }
}

// パラメータファイル名を受け取り，そのパラメータをパラメータマネージャーに登録する
void set_parameters(ParameterManager *mgr,const char *parameter_filename){
    ParameterLoader loader(parameter_filename);
    std::vector<std::string> keys = loader.keys();

    char buffer[256] = {0};
    string_extractor(parameter_filename,"/",".",buffer);
    simulation_result_file_path = buffer;
    printf("set output %s\n",simulation_result_file_path.c_str());

    // add 1208
    if(std::find(keys.begin(),keys.end(),"road_prob_file_name")!=keys.end()){
        road_prob_data_file_path=loader.get("road_prob_file_name").at(0);
    }
    if(std::find(keys.begin(),keys.end(),"use_scheme_3")!=keys.end()){
        use_scheme_3=loader.set("use_scheme_3");
    }

    if(std::find(keys.begin(),keys.end(),"malicious_fake_id")!=keys.end()){
        malicious_fake_id=loader.set("malicious_fake_id");
    }
    if(std::find(keys.begin(),keys.end(),"consider_all_TA")!=keys.end()){
        consider_all_TA=loader.set("consider_all_TA");
    }
    if(std::find(keys.begin(),keys.end(),"distinct_fake")!=keys.end()){
        distinct_fake=loader.set("distinct_fake");
    }
    if(std::find(keys.begin(),keys.end(),"fix_field")!=keys.end()){
        fix_field=loader.set("fix_field");
    }
    if(std::find(keys.begin(),keys.end(),"use_diff_lambda")!=keys.end()){
        use_diff_lambda=loader.set("use_diff_lambda");
    }
    if(std::find(keys.begin(),keys.end(),"simulation_loop")!=keys.end()){
        simulation_max_loop=loader.set("simulation_loop");
    }
    if(std::find(keys.begin(),keys.end(),"route")!=keys.end()){
        g_route=loader.set("route");
    }
    if(std::find(keys.begin(),keys.end(),"reply")!=keys.end()){
        share_replys=loader.set("reply");
    }
    if(std::find(keys.begin(),keys.end(),"field_length")!=keys.end()){
        std::vector<std::vector<std::string>> field_length_vec = loader.getVector("field_length");
        if(!field_length_vec.empty() && !field_length_vec[0].empty()){
            g_XL = atof(field_length_vec[0][0].c_str());
            if(field_length_vec[0].size()>1){
                g_YL = atof(field_length_vec[0][1].c_str());
            }else{
                g_YL = g_XL;
            }
        }
    }
    if(std::find(keys.begin(),keys.end(),"Nxy")!=keys.end()){
        std::vector<std::pair<int,int>> pms;
        for(const auto& tuple_vec : loader.getVector("Nxy")){
            if(tuple_vec.size()>=2){
                int a = atoi(tuple_vec[0].c_str());
                int b = atoi(tuple_vec[1].c_str());
                pms.push_back(std::pair<int,int>(a,b));
            }else{
                LOGGER_DATA_PRINT(WARNING,"Nxy parameter tuple vector size: %lu < 2 ",tuple_vec.size());
            }
        }
        if(pms.size()>1){
            parameter_id_map.push_back(std::pair<std::string,int>({"Nxy",mgr->ParameterID}));
            static Parameter<std::pair<int,int>> p_Nxy(mgr,&g_Nxy,pms);
        }else if(pms.size()==1){
            g_Nxy=pms.at(0);
        }
    }

    // simulation parameter values
    if(std::find(keys.begin(),keys.end(),"lambda")!=keys.end()){
        std::vector<double> pms;
        for(std::string str : loader.get("lambda")){
            pms.push_back(atof(str.c_str()));
        }
        if(pms.size()>1){
            parameter_id_map.push_back(std::pair<std::string,int>({"lambda",mgr->ParameterID}));
            static Parameter<double> p_lambda(mgr,&g_Lambda,pms);
        }else if(pms.size()==1){
            g_Lambda=pms.at(0);
        }
    }
    if(std::find(keys.begin(),keys.end(),"Vm")!=keys.end()){
        std::vector<double> pms;
        for(std::string str : loader.get("Vm")){
            pms.push_back(atof(str.c_str()));
        }
        if(pms.size()>1){
            parameter_id_map.push_back(std::pair<std::string,int>({"Vm",mgr->ParameterID}));
            static Parameter<double> p_Vm(mgr,&g_Vm,pms);
        }else if(pms.size()==1){
            g_Vm=pms.at(0);
        }
    }
    if(std::find(keys.begin(),keys.end(),"fVm")!=keys.end()){
        std::vector<double> pms;
        for(std::string str : loader.get("fVm")){
            pms.push_back(atof(str.c_str()));
        }
        if(pms.size()>1){
            parameter_id_map.push_back(std::pair<std::string,int>({"fVm",mgr->ParameterID}));
            static Parameter<double> p_fVm(mgr,&g_fVm,pms);
        }else if(pms.size()==1){
            g_fVm=pms.at(0);
        }
    }
    if(std::find(keys.begin(),keys.end(),"Slp")!=keys.end()){
        std::vector<double> pms;
        for(std::string str : loader.get("Slp")){
            pms.push_back(atof(str.c_str()));
        }
        if(pms.size()>1){
            parameter_id_map.push_back(std::pair<std::string,int>({"Slp",mgr->ParameterID}));
            static Parameter<double> p_Slp(mgr,&g_Slp,pms);
        }else if(pms.size()==1){
            g_Slp=pms.at(0);
        }
    }
    if(std::find(keys.begin(),keys.end(),"Vw")!=keys.end()){
        std::vector<double> pms;
        for(std::string str : loader.get("Vw")){
            pms.push_back(atof(str.c_str()));
        }
        if(pms.size()>1){
            parameter_id_map.push_back(std::pair<std::string,int>({"Vw",mgr->ParameterID}));
            static Parameter<double> p_Vw(mgr,&g_Vw,pms);
        }else if(pms.size()==1){
            g_Vw=pms.at(0);
        }
    }
    if(std::find(keys.begin(),keys.end(),"Cr")!=keys.end()){
        std::vector<double> pms;
        for(std::string str : loader.get("Cr")){
            pms.push_back(atof(str.c_str()));
        }
        if(pms.size()>1){
            parameter_id_map.push_back(std::pair<std::string,int>({"Cr",mgr->ParameterID}));
            static Parameter<double> p_Cr(mgr,&g_Cr,pms);
        }else if(pms.size()==1){
            g_Cr=pms.at(0);
        }
    }
    if(std::find(keys.begin(),keys.end(),"Pt")!=keys.end()){
        std::vector<double> pms;
        for(std::string str : loader.get("Pt")){
            pms.push_back(atof(str.c_str()));
        }
        if(pms.size()>1){
            parameter_id_map.push_back(std::pair<std::string,int>({"Pt",mgr->ParameterID}));
            static Parameter<double> p_Pt(mgr,&g_Pt,pms);
        }else if(pms.size()==1){
            g_Pt=pms.at(0);
        }
    }
    if(std::find(keys.begin(),keys.end(),"LTA")!=keys.end()){
        std::vector<std::vector<std::vector<double>>> pms;
        for(const std::vector<std::vector<std::string>> &l2_block : loader.getVectorOfVectors("LTA")){
            std::vector<std::vector<double>> l2_double_vec;
            for(const std::vector<std::string> &l1_tuple : l2_block){
                std::vector<double> l1_double_vec;
                for(const std::string &str : l1_tuple){
                    l1_double_vec.push_back(atof(str.c_str()));
                }
                l2_double_vec.push_back(l1_double_vec);
            }
            pms.push_back(l2_double_vec);
        }
        if(pms.size()>1){
            parameter_id_map.push_back(std::pair<std::string,int>({"LTA",mgr->ParameterID}));
            static Parameter<std::vector<std::vector<double>>> p_LTA(mgr,&g_vTA,pms);
        }else if(pms.size()==1){
            g_vTA=pms.at(0);
        }
    }
    if(std::find(keys.begin(),keys.end(),"LRA")!=keys.end()){
        std::vector<std::vector<std::vector<double>>> pms;
        for(const std::vector<std::vector<std::string>> &l2_block : loader.getVectorOfVectors("LRA")){
            std::vector<std::vector<double>> l2_double_vec;
            for(const std::vector<std::string> &l1_tuple : l2_block){
                std::vector<double> l1_double_vec;
                for(const std::string &str : l1_tuple){
                    l1_double_vec.push_back(atof(str.c_str()));
                }
                l2_double_vec.push_back(l1_double_vec);
            }
            pms.push_back(l2_double_vec);
        }
        if(pms.size()>1){
            parameter_id_map.push_back(std::pair<std::string,int>({"LRA",mgr->ParameterID}));
            static Parameter<std::vector<std::vector<double>>> p_LRA(mgr,&g_vRA,pms);
        }else if(pms.size()==1){
            g_vRA=pms.at(0);
        }
    }
    if(std::find(keys.begin(),keys.end(),"LfTA")!=keys.end()){
        std::vector<std::vector<std::vector<double>>> pms;
        for(const std::vector<std::vector<std::string>> &l2_block : loader.getVectorOfVectors("LfTA")){
            std::vector<std::vector<double>> l2_double_vec;
            for(const std::vector<std::string> &l1_tuple : l2_block){
                std::vector<double> l1_double_vec;
                for(const std::string &str : l1_tuple){
                    l1_double_vec.push_back(atof(str.c_str()));
                }
                l2_double_vec.push_back(l1_double_vec);
            }
            pms.push_back(l2_double_vec);
        }
        if(pms.size()>1){
            parameter_id_map.push_back(std::pair<std::string,int>({"LfTA",mgr->ParameterID}));
            static Parameter<std::vector<std::vector<double>>> p_LfTA(mgr,&g_vfTA,pms);
        }else if(pms.size()==1){
            g_vfTA=pms.at(0);
        }
    }
    if(std::find(keys.begin(),keys.end(),"LfRA")!=keys.end()){
        std::vector<std::vector<std::vector<double>>> pms;
        for(const std::vector<std::vector<std::string>> &l2_block : loader.getVectorOfVectors("LfRA")){
            std::vector<std::vector<double>> l2_double_vec;
            for(const std::vector<std::string> &l1_tuple : l2_block){
                std::vector<double> l1_double_vec;
                for(const std::string &str : l1_tuple){
                    l1_double_vec.push_back(atof(str.c_str()));
                }
                l2_double_vec.push_back(l1_double_vec);
            }
            pms.push_back(l2_double_vec);
        }
        if(pms.size()>1){
            parameter_id_map.push_back(std::pair<std::string,int>({"LfRA",mgr->ParameterID}));
            static Parameter<std::vector<std::vector<double>>> p_LfRA(mgr,&g_vfRA,pms);
        }else if(pms.size()==1){
            g_vfRA=pms.at(0);
        }
    }
    if(std::find(keys.begin(),keys.end(),"malicious_appear_time")!=keys.end()){
        std::vector<int> pms;
        for(std::string str : loader.get("malicious_appear_time")){
            pms.push_back(atoi(str.c_str()));
        }
        if(pms.size()>1){
            parameter_id_map.push_back(std::pair<std::string,int>({"malicious_appear_time",mgr->ParameterID}));
            static Parameter<int> p_malicious_appear_time(mgr,&g_malicious_appear_time,pms);
        }else if(pms.size()==1){
            g_malicious_appear_time=pms.at(0);
        }
    }
    if(std::find(keys.begin(),keys.end(),"malicious_appear_interval")!=keys.end()){
        std::vector<int> pms;
        for(std::string str : loader.get("malicious_appear_interval")){
            pms.push_back(atoi(str.c_str()));
        }
        if(pms.size()>1){
            parameter_id_map.push_back(std::pair<std::string,int>({"malicious_appear_interval",mgr->ParameterID}));
            static Parameter<int> p_malicious_appear_interval(mgr,&g_malicious_appear_interval,pms);
        }else if(pms.size()==1){
            g_malicious_appear_interval=pms.at(0);
        }
    }
    if(std::find(keys.begin(),keys.end(),"malicious_appear_num")!=keys.end()){
        std::vector<int> pms;
        for(std::string str : loader.get("malicious_appear_num")){
            pms.push_back(atoi(str.c_str()));
        }
        if(pms.size()>1){
            parameter_id_map.push_back(std::pair<std::string,int>({"malicious_appear_num",mgr->ParameterID}));
            static Parameter<int> p_malicious_appear_num(mgr,&g_malicious_appear_num,pms);
        }else if(pms.size()==1){
            g_malicious_appear_num=pms.at(0);
        }
    }
    if(std::find(keys.begin(),keys.end(),"malicious_appear_type")!=keys.end()){
        std::vector<int> pms;
        for(std::string str : loader.get("malicious_appear_type")){
            pms.push_back(atoi(str.c_str()));
        }
        if(pms.size()>1){
            parameter_id_map.push_back(std::pair<std::string,int>({"malicious_appear_type",mgr->ParameterID}));
            static Parameter<int> p_malicious_appear_type(mgr,&g_malicious_appear_type,pms);
        }else if(pms.size()==1){
            g_malicious_appear_type=pms.at(0);
        }
    }
    if(std::find(keys.begin(),keys.end(),"method")!=keys.end()){
        std::vector<int> pms;
        for(std::string str : loader.get("method")){
            pms.push_back(atoi(str.c_str()));
        }
        if(pms.size()>1){
            parameter_id_map.push_back(std::pair<std::string,int>({"method",mgr->ParameterID}));
            static Parameter<int> p_method(mgr,&g_method,pms);
        }else if(pms.size()==1){
            g_method=pms.at(0);
        }
    }
    if(std::find(keys.begin(),keys.end(),"start")!=keys.end()){
        std::vector<int> pms;
        for(std::string str : loader.get("start")){
            pms.push_back(atoi(str.c_str()));
        }
        if(pms.size()>1){
            parameter_id_map.push_back(std::pair<std::string,int>({"start",mgr->ParameterID}));
            static Parameter<int> p_start(mgr,&g_start,pms);
        }else if(pms.size()==1){
            g_start=pms.at(0);
        }
    }
    if(std::find(keys.begin(),keys.end(),"attack")!=keys.end()){
        std::vector<int> pms;
        for(std::string str : loader.get("attack")){
            pms.push_back(atoi(str.c_str()));
        }
        if(pms.size()>1){
            parameter_id_map.push_back(std::pair<std::string,int>({"attack",mgr->ParameterID}));
            static Parameter<int> p_attack(mgr,&g_attack,pms);
        }else if(pms.size()==1){
            g_attack=pms.at(0);
        }
    }
}

typedef struct Checker{
    double first_true_receive_time;
    double first_fake_receive_time;
    double last_true_receive_time;
    double last_fake_receive_time;
    double first_true_broadcast_time;
    double first_fake_broadcast_time;
    double last_true_broadcast_time;
    double last_fake_broadcast_time;
    long int max_true_node_count;
    long int max_fake_node_count;
    long int total_connection;
    long int total_broadcast;
    long int total_node_in_ra;
    // inRAについて記録を加える
    Checker(){reset();}
    void reset(){
        first_true_receive_time=0.0;
        first_fake_receive_time=0.0;
        last_true_receive_time=0.0;
        last_fake_receive_time=0.0;
        first_true_broadcast_time=0.0;
        first_fake_broadcast_time=0.0;
        last_true_broadcast_time=0.0;
        last_fake_broadcast_time=0.0;
        max_true_node_count=0;
        max_fake_node_count=0;
        total_connection=0;
        total_broadcast=0;
        total_node_in_ra=0;
    }
}Checker;

#endif