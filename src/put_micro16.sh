#!/bin/bash

date=`date +%m%d%H`

make

#git stage *.cc *.h
#git commit -m "Auto commit $date"

scp *_search.x86_64 aflab@supermicro16:~/workspace/pbnf
