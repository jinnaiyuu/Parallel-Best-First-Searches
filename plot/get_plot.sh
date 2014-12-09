#!/bin/bash

echo "directory = "
read directory

mkdir ${directory}
scp yuu@david:/home/yuu/workspace/pbnf/*.png /home/yuu/workspace/pbnf/plot/${directory}
