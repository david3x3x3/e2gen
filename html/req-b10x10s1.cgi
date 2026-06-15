#!/bin/bash
echo 'Content-type: application/json'
echo ''
rownum=$(< b10x10s1-5220-list.txt cut -d' ' -f1 | shuf -n1)
echo "{ \"rownum\": $rownum }"

