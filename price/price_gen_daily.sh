#! /bin/bash

# summary: generate raw stock prices A, B, C with different characteristics concurrently
#        : wait. then, rearrange the csv files for visualization
#
# this script will run in the following cases:
#   1. when Docker initiates (for initialization)
#   2. every 00:00 AM (for daily price generation) with help from crontab
#
# todos:
#   1. specify stock name instead of A, B, C
#   2. specify statistical characterstics of each of them

start=$(date +'%Y-%m-%d')

# generate raw stockA price
pricegenerator --timeframe 1 --horizon 1440 --start "$start 00:00" --max-close 100 --min-close 50 --trend-deviation 0.03 --outliers-prob 0.05 --generate --save-to csv/rawA.csv &
P1=$!

# generate raw stockB price concurrently
pricegenerator --timeframe 1 --horizon 1440 --start "$start 00:00" --max-close 100 --min-close 50 --trend-deviation 0.03 --outliers-prob 0.05 --generate --save-to csv/rawB.csv &
P2=$!

# generate raw stockC price concurrently
pricegenerator --timeframe 1 --horizon 1440 --start "$start 00:00" --max-close 100 --min-close 50 --trend-deviation 0.03 --outliers-prob 0.05 --generate --save-to csv/rawC.csv &
P3=$!

# wait for concurrent jobs to finish
wait $P1 $P2 $P3

# rearrange rawA.csv to cookedA.csv
./price_history_all.out csv/rawA.csv csv/cookedA.csv &
P4=$!

# rearrange rawB.csv to cookedB.csv
./price_history_all.out csv/rawB.csv csv/cookedB.csv &
P5=$!

# rearrange rawC.csv to cookedC.csv
./price_history_all.out csv/rawC.csv csv/cookedC.csv &
P6=$!

# wait for concurrent jobs to finish
wait $P4 $P5 $P6
