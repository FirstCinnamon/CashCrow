#! /bin/bash

# summary: compile cpp files with relevant flags and options
#        : this script will run only when Docker initiates
#        : this script should run before price_gen_daily.sh
#
# todo: adjust the command later according to Docker environment

g++ -std=c++17 price_history_all.cpp -I../include -o price_history_all.out &
g++ -std=c++17 price_history_now.cpp -I../include -o price_history_now.out
