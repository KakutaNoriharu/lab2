#!/bin/bash
#make -B
g++ -I../utility/ -Wall -Wextra -Wunused -lm -pthread -O2 -o sim.out main.cpp
g++ -o data_checker.out data_folder_checker.cpp
./data_checker.out
for file in params/*.prm; do
    echo "Executing: ./sim.out -p $file"
    ./sim.out -p $file
done
./data_checker.out
date
python3 scripts/DiscordNotify.py FinishRun
