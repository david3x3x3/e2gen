#!/bin/bash
# 9x9-row-7-259478
# python et2-new.py 10x10 10 > 10x10-list.txt
# grep ' 5220 ' 10x10-list.txt  | cut -d' ' -f1 | shuf -n1
while :; do 
  cmd="$1"
  puzz=$(echo $cmd | cut -d- -f1)
  ord=$(echo $cmd | cut -d- -f2)
  rowsize=$(echo $cmd | cut -d- -f3)
  numrows=$(echo $cmd | cut -d- -f4)
  # seed=30000000
  # while [ "$seed" -ge "$numrows" ]; do
  #   seed=$(($RANDOM*607+$RANDOM))
  # done
  seed=$(grep ' 5220 ' 10x10-list.txt  | cut -d' ' -f1 | shuf -n1)
  echo seed = $seed
  ./$cmd 0 $seed | while IFS= read -r line; do
    if [[ "$line" == "status"* ]]; then
       echo -en "$line   \r"
       if [[ "$line" == *"last=1" ]]; then
          echo row=$seed $line endtime=$(date +%Y-%m-%d_%H:%M:%S) >> results-$puzz.txt
       fi
    else
       echo "$line"
    fi
  done
done
