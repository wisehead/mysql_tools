#!/bin/bash

SLEEP_TIME=$1

while [ true ]
do
    iostat -xkt 1 1
    sleep $((SLEEP_TIME-1))
done

