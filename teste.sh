#!/bin/bash
while [ : ]
do
        #clear
        # Get the system time
        now="$(date +"%r")"
        # Show main - menu, server name and time
        echo "time is $HOSTNAME - $now"
        sleep 1
done