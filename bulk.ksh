#!/bin/ksh

integer i

mkdir -p output
while [[ $i -lt 10 ]]; do
    ./barzer_srvtest.py > output/shit.$i &
    ((i=i+1))
done

wait
