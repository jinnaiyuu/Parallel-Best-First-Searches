#!/bin/bash

gen_param=$1
number=$2

./grid_gen $gen_param $number

# This produces grid instances with filename ${i}_${gen_param}
# These instances are not generated to have solutions.
# Here we have to make sure they do.

for ((i=0; i<$number ; ++i))
do
    cat ./${i}_2000-2000-${gen_param} | ../src/grid_search.x86_64 astar > sol_${i}_${gen_param}
#    cat sol_${i}_${gen_param} | awk '/No Solution/{printf("no solution\n") ; end;} $1=="wall_time:"{print $2}' > sol
    sol=`grep Solution sol_${i}_${gen_param}`
    if [ ! -z "$sol" -a "$sol" != " " ]; then
	# if sol has return then this instance has no solution
	echo "${i} no solution"
	rm ${i}_2000-2000-${gen_param}
    else
	cat sol_${i}_${gen_param} | awk '$1=="wall_time:"{print $2}'
    fi
done

# rm sol_*




