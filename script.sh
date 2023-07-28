#!/bin/bash

g++ -O3 -std=c++17 main.cpp -o tsp
wait
# Number of times to launch the program
numLaunches=5

# Loop to launch the program multiple times
for ((i=1; i<=numLaunches; i++))
do
    for k in 1 2 4 8 16 20 32 40 64;
    do
      for ((j=0; j<=2; j++))
      do
        if [ "$j" -eq 0 ] && [ "$k" != 1 ]; then
            continue
        else
# type | number of threads | chromosome number | number of cities | number of iterations | mutation percentage
            ./tsp "$j" "$k" 10000 10000 10 30
            wait
            sleep 1

            ./tsp "$j" "$k" 10000 15000 10 30
            wait
            sleep 1

            ./tsp "$j" "$k" 10000 20000 10 30
            wait
            sleep 1
        fi
      done
    done
done