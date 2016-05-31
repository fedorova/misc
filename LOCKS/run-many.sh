#!/bin/sh

DATE=`date +"%d"."%m"-"%T"`
OUTPUT=./output-$DATE

for t in 1 2 4 8;
do
    ./locks $t | tee $OUTPUT-$t-threads
done
