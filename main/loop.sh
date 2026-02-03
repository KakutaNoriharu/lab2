#! /bin/bash

# 何らかのパラメータについてループしてシミュレーションを行う

g++ -I../utility/ -Wall -Wextra -Wunused -lm -pthread -g -O2 -o d.out main.cpp
for cnt in {1..30}	#1,2,3を順番にcntにセットしてdo～doneを処理する
do
  ./d.out -n 1 0 -loop 100000 -s $cnt
done

for cnt in {1..30}	#1,2,3を順番にcntにセットしてdo～doneを処理する
do
  ./d.out -n 5 6 -loop 100000 -s $cnt
done

for cnt in {1..30}	#1,2,3を順番にcntにセットしてdo～doneを処理する
do
  ./d.out -n 21 22 -x 21000 -y 22000 -loop 100000 -s $cnt
done
