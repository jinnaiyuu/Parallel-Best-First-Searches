#!/bin/bash

for (( threads=1; threads <= 8; threads++ ))
do
    ./scripts/run_tiles.sh multiastar -t $threads $@
done
