#!/bin/bash
echo 'Content-type: application/json'
echo ''
rownum=$(shuf -i 0-259477 -n 1)
echo "{ \"rownum\": $rownum }"

