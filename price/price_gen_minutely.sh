#! /bin/bash

# summary: generate partial stock history for users based on current minute(00:00(0)~23:59(1439))
#
# this script will run in the following cases:
#   1. when Docker initiates (for initialization)
#   2. every minutes (for minutely price generation) with help from crontab
#
# todos:
#   1. specify stock name instead of A, B, C
cd /docker/CashCrow/price

hour=`TZ=UTC-9 date +"%H"`
min=`TZ=UTC-9 date +"%M"`

minutes=$(((60*hour) + min))

# update nowA.csv from cookedA.csv
./price_history_now.out $minutes csv/cookedA.csv csv/nowA.csv &

# update nowB.csv from cookedB.csv
./price_history_now.out $minutes csv/cookedB.csv csv/nowB.csv &

# update nowC.csv from cookedC.csv
./price_history_now.out $minutes csv/cookedC.csv csv/nowC.csv
