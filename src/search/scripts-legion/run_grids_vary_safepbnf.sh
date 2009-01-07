#!/bin/bash

for min in 5 60 64 100 120 180 200 256
#for min in 10 30 60 80
do
    for nblocks in 625 1600 2500 6400
    do
        for threads in 1 2 4 8 16 32
	do
	    ./scripts/run_grids.sh \
		-m $min \
		-t $threads \
		-n $nblocks \
                $@ \
		safepbnf
	done
    done
done
