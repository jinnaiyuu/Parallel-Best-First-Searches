#!/bin/bash

date=`date +%m%d%H`

make

#git stage *.cc *.h
#git commit -m "Auto commit $date"

scp tiles_search.x86_64 grid_search.x86_64 yuu@david:/home/yuu/workspace/pbnf
