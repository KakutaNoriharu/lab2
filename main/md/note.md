cd ~/lab2
source .venv/bin/activate

# 20260508

パラメータファイルモード (-p 指定あり)                                                                                                 
                                                                                                                                         
  repeat_simulation() が自動的に seed をループします。                                                                                   
                                                                                                                                         
  // main.cpp:118                                       
  for(int i=csv_seed_progress; i<=MAX_SEED; ++i){                                                                                        
      run_simulation(fp, i, ...);  // seed = 1, 2, ..., 10
  }                                                                                                                                      
   
  - MAX_SEED = 10（固定、main.cpp:18）                                                                                                   
  - 再開機能あり: 出力CSVの行数を確認し (csv_seed_progress)、途中で止まった場合は続きから再開する
  - 結果は data/<result_id>/sim_<param_string>.csv に追記される                                                                          
  - 複数パラメータ組み合わせを4スレッドで並列処理                                                                                        
                                                                                                                                         
  単体実行モード (-p 指定なし)                                                                                                           
                                                                                                                                         
  1回だけ実行。seed は -s オプションで指定（デフォルト=3）。                                                                             
   
  run_simulation(NULL, seed, ...);  // seed は固定                                                                                       
                                                        
  繰り返したい場合は loop.sh のようにシェル側でループする想定：                                                                          
   
  for cnt in {1..30}; do                                                                                                                 
      ./d.out -s $cnt                                   
  done                                                                                                                                   
   
  ---                                                                                                                                    
  まとめ                                                

  
| 用途 | 方法 |
| --- | --- | 
| 本番実験（パラメータスイープ）| -p <file> → seed 1〜MAX_SEED を自動ループ |
  
  for cnt in {1..30}; do
      ./d.out -s $cnt
  done

  ---
  まとめ

  
| 用途 | 方法 |
| --- | --- |
| 本番実験（パラメータスイープ） | -p <file> → seed 1〜MAX_SEED を自動ループ |
| 単発確認 | -s <値> で1回実行 |
| 旧スタイルの大量実行 | loop.sh でシェルループ |
  

  -p モードが「多seed実験の主な想定ユーザーフロー」になっており、MAX_SEED（現在10）を変えることで試行回数を調整できる設計です。


  main.cpp:178 を：

  int main(/*int argc, char *argv[]*/){

  こう直すだけです：

  int main(int argc, char *argv[]){

  これで arg.parse(argc, argv) が機能し、-p <file> を渡せば parameter_filename != NULL になって repeat_simulation() が走ります。

  make
  ./a.out -p params/test.prm