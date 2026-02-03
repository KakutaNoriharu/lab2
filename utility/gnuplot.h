#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include "fileManager.h"

/**
 * gnuplotの操作をより簡素にまとめようと試みたもの．
 * 
 * todo
 * ・対応オブジェクトの追加，
 * ・plotをline with boxes etc.の追加
*/

#define GNUPLOT "gnuplot -persist > logs/gnuplot/z_gnuplot_log.txt 2>&1"
#define GNUPLOT_BUF_LEN 1024
#define GNUPLOT_MAX_MULTI_PLOT 4

typedef enum GNUPLOT_OBJECT_COUNTER{
    Label_C,
    Arrow_C,
    Object_C,
    GNUPLOT_OBJECT_COUNTER_SIZE
}GNUPLOT_OBJECT_COUNTER;

typedef struct Plot{
    FILE *gp; // gnuplotへのpipe
    FILE *fp; // 描画スクリプトを生成するためのファイルポインタ
    bool file_open_state;
    // 描画オブジェクトのidを管理，reset_counterでそれ以前のオブジェクトを保存
    char STRING_BUFFER[GNUPLOT_BUF_LEN];
    int multiplot_counter;
    int active_multi_plot_id;
    int counter[GNUPLOT_MAX_MULTI_PLOT][GNUPLOT_OBJECT_COUNTER_SIZE];
    Plot() : gp(0),fp(0),file_open_state(false),STRING_BUFFER{0},multiplot_counter(0),active_multi_plot_id(0),counter{0} {
        make_file_directory("logs/gnuplot/");
    }
}Plot;

Plot plt;

#define PLOT(...) fprintf(plt.gp, __VA_ARGS__);
#define FILE_PLOT(...) fprintf(plt.fp, __VA_ARGS__);
#define PLOT_DATA(x,y) fprintf(plt.gp, "%f, %f\n",x,y);
#define LABEL(x,y,position,fmt,...) fprintf(plt.gp, "set label " position " at first %f, %f '" fmt "' rotate by %d\n",x,y,__VA_ARGS__);
#define FRAME(...) fprintf(plt.gp, "set multiplot\n");__VA_ARGS__;fprintf(plt.gp, "unset multiplot\n\n");
#define FIGURE(figure_id,...) plotSetFig(figure_id);__VA_ARGS__;plotUnsetFig();

/**gnuplotに出力するか，scriptを生成するかを設定*/
void plotInit(bool use){
    Plot newPlot;
    plt=newPlot;
    make_file_directory("logs/gnuplot/");
    if(use){
        if((plt.gp = popen(GNUPLOT, "w")) == NULL){
            fprintf(stderr, "Error: cannot open \"%s\".\n", GNUPLOT);
            exit(1);
        }
    }else{
        if((plt.gp = fopen("logs/gnuplot/gnuplot_script.gp", "w")) == NULL){
            fprintf(stderr, "Error: cannot open \"%s\".\n", GNUPLOT);
            exit(1);
        }
    }
}

/**プロットする領域を追加
 * origin_x/y : 領域の始点，0,0で左下，,1,1で右上
 * size_x/y : 大きさ．originに足して1を超えないように．
 * (origin_x/y)->(origin_x/y+size_x/y) */
void plotAddFig(double origin_x,double origin_y,double size_x,double size_y){
    if(plt.file_open_state){
        if(fclose(plt.fp) == EOF){
            fprintf(stderr, "Error: fp cannot close.\n");
            exit(2);
        }
        plt.file_open_state=false;
    }
    if(plt.multiplot_counter<GNUPLOT_MAX_MULTI_PLOT){
        sprintf(plt.STRING_BUFFER, "logs/gnuplot/gnuplot_figure_template_%d.gp",plt.multiplot_counter);
        if((plt.fp = fopen(plt.STRING_BUFFER, "w")) == NULL){
            fprintf(stderr, "Error: cannot open \"%s\".\n", plt.STRING_BUFFER);
            exit(1);
        }
        plt.file_open_state=true;
        plt.active_multi_plot_id=plt.multiplot_counter;
        plt.multiplot_counter+=1;
    }
    fprintf(plt.fp, "set origin %f, %f\n",origin_x,origin_y);
    fprintf(plt.fp, "set size %f, %f\n",size_x,size_y);
}

/**表示する領域を指定． */
void plotCallFig(int figure_id){
    if(plt.file_open_state){
        if(fclose(plt.fp) == EOF){
            fprintf(stderr, "Error: fp cannot close.\n");
            exit(2);
        }
        plt.file_open_state=false;
    }
    if(figure_id<plt.multiplot_counter){
        sprintf(plt.STRING_BUFFER, "logs/gnuplot/gnuplot_figure_template_%d.gp",figure_id);
        if((plt.fp = fopen(plt.STRING_BUFFER, "a")) == NULL){
            fprintf(stderr, "Error: cannot open \"%s\".\n", plt.STRING_BUFFER);
            exit(1);
        }
        plt.file_open_state=true;
        plt.active_multi_plot_id=figure_id;
    }
}

/**現在指定しているグラフ領域をリセット */
void plotUnsetFig(){
    PLOT("reset\n\n");
}

/** グラフ領域を指定し，初期化 */
void plotSetFig(int figure_id){
    if(plt.file_open_state){
        if(fclose(plt.fp) == EOF){
            fprintf(stderr, "Error: fp cannot close.\n");
            exit(2);
        }
        plt.file_open_state=false;
    }
    if(figure_id<plt.multiplot_counter){
        sprintf(plt.STRING_BUFFER, "logs/gnuplot/gnuplot_figure_template_%d.gp",figure_id);
        plotUnsetFig();
        PLOT("load '%s'\n",plt.STRING_BUFFER);
    }
}

/** 1frameを描画する宣言 */
void plotFrame(){
    fprintf(plt.gp, "set multiplot\n");
}

/** 1frameの描画を終了する宣言 */
void plotEndFrame(){
    plotUnsetFig();
    fprintf(plt.gp, "unset multiplot\n\n");
}

/** 出力をpngで指定 */
void plotSetOutputPng(int size_x,int size_y){
    fprintf(plt.gp, "set terminal png optimize size %d,%d large\n",size_x,size_y);
}

/** 出力をgifで指定 */
void plotSetOutputGif(int delay,int gif_size_x,int gif_size_y){
    fprintf(plt.gp, "set terminal gif animate delay %d optimize size %d,%d large\n",delay,gif_size_x,gif_size_y);
    // gif delayは画像のフレームレート，"large" は文字列を表示するために必要．
}

/** スクリプトのx/yの描画範囲を指定 */
void plotInitSetRange(double x_start,double x_end,double y_start,double y_end){
    fprintf(plt.fp, "set xrange[%f:%f]\n", x_start, x_end);//x軸範囲
    fprintf(plt.fp, "set yrange[%f:%f]\n", y_start, y_end);//y軸範囲
}

/** x/yの描画範囲を再指定，スクリプトには記されない */
void plotSetRange(double x_start,double x_end,double y_start,double y_end){
    fprintf(plt.gp, "set xrange[%f:%f]\n", x_start, x_end);//x軸範囲
    fprintf(plt.gp, "set yrange[%f:%f]\n", y_start, y_end);//y軸範囲
}

/* 初期設定でx/y軸にラベルを設ける*/
void plotInitSetLabel(const char* xlabel,const char* ylabel){
    if(xlabel!=NULL){
        fprintf(plt.fp, "set xlabel '%s'\n",xlabel);
    }else{
        fprintf(plt.fp, "set noxlabel\n");
    }
    if(ylabel!=NULL){
        fprintf(plt.fp, "set ylabel '%s'\n",ylabel);
    }else{
        fprintf(plt.fp, "set noylabel\n");
    }
}

/* x/y軸にラベルを設ける*/
void plotSetLabel(const char* xlabel,const char* ylabel){
    if(xlabel!=NULL){
        fprintf(plt.gp, "set xlabel '%s'\n",xlabel);
    }else{
        fprintf(plt.gp, "set noxlabel\n");
    }
    if(ylabel!=NULL){
        fprintf(plt.gp, "set ylabel '%s'\n",ylabel);
    }else{
        fprintf(plt.gp, "set noylabel\n");
    }
}

/* x/y軸にtickを初期設定*/
void plotInitSetTics(const char* xtics,const char* ytics){
    if(xtics!=NULL){
        fprintf(plt.fp, "set xtics %s\n",xtics);
    }else{
        fprintf(plt.fp, "set noxtics\n");
    }
    if(ytics!=NULL){
        fprintf(plt.fp, "set ytics %s\n",ytics);
    }else{
        fprintf(plt.fp, "set noytics\n");
    }
}
/* x/y軸にtickを設定*/
void plotSetTics(const char* xtics,const char* ytics){
    if(xtics!=NULL){
        fprintf(plt.gp, "set xtics %s\n",xtics);
    }else{
        fprintf(plt.gp, "set noxtics\n");
    }
    if(ytics!=NULL){
        fprintf(plt.gp, "set ytics %s\n",ytics);
    }else{
        fprintf(plt.gp, "set noytics\n");
    }
}
/** ラベル描画を初期設定 */
void plotInitLabelConst(const char *label,double x,double y,int rotate){
    fprintf(plt.fp, "set label %d center at first %f,%f '%s' rotate by %d\n",
    ++plt.counter[plt.active_multi_plot_id][Label_C],x,y,label,rotate);
}
/** ラベルを描画 */
void plotLabelConst(const char *label,double x,double y,int rotate){
    fprintf(plt.gp, "set label %d center at first %f,%f '%s' rotate by %d\n",
    ++plt.counter[plt.active_multi_plot_id][Label_C],x,y,label,rotate);
}

/**
 * 矢印を初期設定
 * shapes  from - to 
 * filled   : --->
 * heads    : <-->
 * nohead   : ----
 * backhead : <--- */
void plotInitArrow(const char *shape,double from_x,double from_y,double to_x,double to_y,const char *color,double width){
    fprintf(plt.fp, "set arrow %d from %f, %f to %f, %f %s lw %f lc rgb '%s'\n",
    ++plt.counter[plt.active_multi_plot_id][Arrow_C],from_x,from_y,to_x,to_y,shape,width,color);
}

/**
 * 矢印を描画する
 * shapes  from - to 
 * filled   : --->
 * heads    : <-->
 * nohead   : ----
 * backhead : <---
 * width    : default 1
 * linetype : default 1 */
void plotArrow(const char *shape,double from_x,double from_y,double to_x,double to_y,const char *color,double width,int linetype){
    fprintf(plt.gp, "set arrow %d from %f, %f to %f, %f %s lw %f lt %d lc rgb '%s'\n",
    ++plt.counter[plt.active_multi_plot_id][Arrow_C],from_x,from_y,to_x,to_y,shape,width,linetype,color);
}

/** 円を初期設定 */
void plotInitCircle(double x,double y,double x_size,double y_size){
    fprintf(plt.fp, "set object %d circle at %f,%f size %f,%f\n",
    ++plt.counter[plt.active_multi_plot_id][Object_C],x,y,x_size,y_size);
}
/** 円を描画する */
void plotCircle(double x,double y,double x_size,double y_size){
    fprintf(plt.gp, "set object %d circle at %f,%f size %f,%f\n",
    ++plt.counter[plt.active_multi_plot_id][Object_C],x,y,x_size,y_size);
}

/** 矩形を初期設定 */
void plotInitRectangle(double x,double y,double x_size,double y_size){
    fprintf(plt.fp, "set object %d rect at %f,%f size %f,%f\n",
    ++plt.counter[plt.active_multi_plot_id][Object_C],x,y,x_size,y_size);
}
/** 矩形を描画する x/y:中心の座標,x/y_size:大きさ */
void plotRectangle(double x,double y,double x_size,double y_size){
    fprintf(plt.gp, "set object %d rect at %f,%f size %f,%f\n",
    ++plt.counter[plt.active_multi_plot_id][Object_C],x,y,x_size,y_size);
}

/** 描画内容を宣言
 * x:描画する要素の数 */
void plotGraph(int x, ...){
    va_list ap;
    va_start(ap, x);
    fprintf(plt.gp, "plot ");
    for(int i=0;i<x;i++){
        fprintf(plt.gp, "%s",va_arg(ap, char*));
        if(i+1<x){
            fprintf(plt.gp, ", ");
        }
    }
    fprintf(plt.gp,"\n");
}

/** setだけで描画したい場合 */
void plotSets(){
    PLOT("plot 0 lw 2 lc rgb 'white' notitle\n");
}

/**
 * gnuplotを解放 */
void plotClose(){
    if(plt.file_open_state){
        if(fclose(plt.fp) == EOF){
            fprintf(stderr, "Error: fp cannot close.\n");
            exit(2);
        }
        plt.file_open_state=false;
    }
    if(plt.gp!=0){
        if(pclose(plt.gp) == EOF){
            fprintf(stderr, "Error: cannot close \"%s\".\n", GNUPLOT);
            exit(2);
        }
    }
}


// 定数定義
#define COLOR_STEPS 20
#define MAX_INDEX (COLOR_STEPS - 1) // 19

/**
 * 0.0から1.0の値を、20段階の青(低)から赤(高)へのグラデーションのRGBカラーコードに変換します。
 *
 * @param x          0.0から1.0までの値。
 * @param output_str 結果のカラーコード ("#RRGGBB") を格納するための、
 * 最低8バイトのバッファ（例: char buffer[8];）
 */
void get_heatmap_color(double x, char* output_str) {
    // 1. 範囲チェックとクリッピング
    if (x < 0.0) x = 0.0;
    if (x > 1.0) x = 1.0;

    // 2. 20段階 (0～19) に量子化
    // x * 19 の結果を四捨五入して、0から19までの整数 S を得る
    int S = (int)round(x * MAX_INDEX); 

    // 3. 量子化された値 x' を計算 (0/19, 1/19, 2/19, ..., 19/19)
    // これが色計算の基準となる離散的な値
    double x_quantized = (double)S / MAX_INDEX;

    // 4. 色の計算 (青 -> 紫 -> 赤 のグラデーション)
    
    // R成分 (赤): x_quantized が増えると 0 から 255 に増加
    int r_val = (int)(255.0 * x_quantized);
    
    // B成分 (青): x_quantized が増えると 255 から 0 に減少
    int b_val = (int)(255.0 * (1.0 - x_quantized));
    
    // G成分 (緑): 常に 0
    int g_val = 0; 

    // 5. バッファにカラーコード文字列を書き込み
    // snprintf の第2引数にはバッファサイズ (8) を指定 (終端NULL文字を含む)
    snprintf(output_str, 8, "#%02X%02X%02X", r_val, g_val, b_val);
}


/*
// sample code
typedef struct Point{
    double x,y;
}Point;

#define point_num 4
int main(){
    int max_loop=100;
    double field_size=20.0;
    Point p[point_num]={0};
    plotInit(true);
    plotSetOutputGif(20,640,800);
    PLOT("set output '%s_help.gif'\n","test");
    plotAddFig(0.0,0.0,1.0,0.8);
    plotSetRange(0.0,field_size,0.0,field_size);
    plotArrow("nohead",0.0,0.0,field_size,field_size);
    plotArrow("nohead",0.0,field_size,field_size,0);
    plotAddFig(0.0,0.8,1.0,0.2);
    plotSetRange(0.0,20,0.0,point_num);
    plotSetTics("","");
    p[0].x=0.0;
    p[0].y=0.0;
    p[1].x=field_size*0.9;
    p[1].y=field_size*0.9;
    for(int loop=0;loop<max_loop;++loop){
        for(int i=2;i<point_num;++i){
            p[i].x+=i*0.01/(point_num-i);
            p[i].y+=i*0.02*i;
            if(p[i].x>field_size)p[i].x-=field_size;
            if(p[i].y>field_size)p[i].y-=field_size;
        }

        FRAME(
            FIGURE(1,
                //PLOT("set title 'frame:%d'\n",loop);
                plotGraph(1,"'-' with boxes title 'hist'\n");
                int hist[20]={0};
                for(int i=0;i<20;++i){
                    for(int j=0;j<point_num;++j){
                        if(p[j].x>=i && p[j].x<i+1){
                            hist[i]+=1;
                        }
                    }
                    PLOT("%.2f, %d\n",i+0.5,hist[i]);
                }
                PLOT("e\n");
            );
            
            FIGURE(0,
                //PLOT("set title 'frame:%d'\n",loop);
                for(int i=0;i<point_num;++i){
                    LABEL(p[i].x,p[i].y,"left","%f, %f",p[i].x,p[i].y,90*(i%4));
                }
                plotGraph(1,"'-' pt 7 ps 1 lc 'blue' title 'test'");
                for(int i=0;i<point_num;++i){
                    PLOT("%.2f, %.2f\n",p[i].x,p[i].y);
                }
                PLOT("e\n");
            );
        );
    }
    plotClose();
    return 0;
}
*/

/*
// sample code png
    plotInit(1);
    PLOT("set terminal png size 800,800\n");
    PLOT("set output 'test.png'\n");
    plotSetRange(-100,2500,-100,2500);
    for(size_t i=0;i<map.map.size();++i){
        for(size_t j=0;j<map.map.at(i).size();++j){
            Edge &e=map.map.at(i).at(j);
            plotArrow("nohead", e.from->point.x,e.from->point.y,e.to->point.x,e.to->point.y);
        }
    }
    PLOT("set object rect at %f, %f size %f, %f fc rgb 'blue' fillstyle transparent solid 0.2 noborder\n",1200.0,1200.0,1000.0,1000.0);
    PLOT("plot 0 lw 2 lc rgb 'white' notitle\n");
*/