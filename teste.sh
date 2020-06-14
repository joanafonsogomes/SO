#!/bin/bash
./argus -m 5
./argus -e "sleep 3"
./argus -t 1
sleep 1
./argus -e "ls|wc"
./argus -l
./argus -h
./argus -e "date"
./argus -e "cut -f7 -d: /etc/passwd |uniq|wc -l"
./argus -o 2
./argus -e "./time.sh"
sleep 5
./argus -r
exit