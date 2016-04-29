#!/bin/bash

date=`date +%m%d%H`

make

#git stage *.cc *.h
#git commit -m "Auto commit $date"

scp tiles_search.x86_64 tiles_search.x86_64.order jinnai@funlucy:/home/jinnai/workspace/pbnf
