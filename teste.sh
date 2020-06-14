#!/bin/bash
./argus -m 5
./argus -e "ls|wc"
./argus -l
./argus -e "./time.sh"
./argus -l
./argus -h
sleep 5
./argus -r
exit