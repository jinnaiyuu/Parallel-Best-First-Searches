#!/bin/bash

date=`date +%m%d%H`

make

#git stage *.cc *.h
#git commit -m "Auto commit $date"

scp *_search.x86_64 supermicro@supermicro:/home/supermicro/workspace/pbnf
