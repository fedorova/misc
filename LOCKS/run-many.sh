#!/bin/sh

DATE=`date +"%d"."%m"-"%T"`
OUTPUT=./output-$DATE

echo $OUTPUT

for t in 1 2 4 8 16;
do
    ./locks $t $1 | tee $OUTPUT-$t-threads
done

grep 'per second' $OUTPUT* | awk '{print $4}'
