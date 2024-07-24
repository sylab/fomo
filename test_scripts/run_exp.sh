#!/bin/bash

#if [[ $EUID -ne 0 ]]; then
#	echo "Need to be root" 1>&2
#	exit 1
#fi

workloads=("CAMWEBDEV-lvm0-day1.csv")
uniques=("530419")
cache_sizes=("0.0005" "0.001" "0.005" "0.01" "0.05" "0.1")
size=
S_sizes=("2" "2.75")
algs=("lirs")

for workload in ${workloads[@]}; do
	for cache_size in ${cache_sizes[@]}; do
		for unique in ${uniques[@]}; do
			for alg in ${algs[@]}; do 

				
				size=$(echo "(($cache_size*$unique+0.5)/1)" | bc)
				echo "blocks: $size $workload cache_size: $cache_size unique: $unique" 

				./cache-sim $alg $size msr -f /home/liana/cache_data/$workload | tee /dev/tty > "results/${alg}_${workload}.out"
				#hits=$(echo "$output" | cut -f1 -d" " )
				#misses=$(echo "$output" | cut -f2 -d" " )
				#echo "HITS: $hits MISSES: $misses"
				#echo "scale=2; ((100 * ($hits / ($hits + $misses))))" | bc
			done
		done
	done
done 

for 
