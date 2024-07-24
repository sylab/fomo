#!/bin/bash

set -e

function sim_test {
	trace_path=${1}
	trace=${2}
	trace_type=${3}
	algorithm=${4}
	cache_size=${5}
	cache_size_real=${6}
	duration=${7}
	output_csv=${8}

	# TODO convert result
	touch -a ${output_csv}

	if [[ ${duration} == "0" ]]; then
		result=($(./cache_nucleus/cache-sim ${algorithm} ${cache_size_real} ${trace_type} < ${trace_path}))
		echo "finished: ${trace} ${cache_size} ${algorithm}"
		local IFS=$','
		shift
		echo "${trace_type},${trace},${cache_size},${cache_size_real},${algorithm},,${result[*]}" >> ${output_csv}
	else
		result=($(./cache_nucleus/cache-sim ${algorithm} ${cache_size_real} ${trace_type} -d ${duration} < ${trace_path}))
		echo "finished: ${trace} ${cache_size} ${algorithm} ${duration}"
		local IFS=$','
		shift
		echo "${trace_type},${trace},${cache_size},${cache_size_real},${algorithm},${duration},${result[*]}" >> ${output_csv}
	fi


}

#folders=("/disk/cache-research/FIU_SRCMap/traces_split/"
#         "/disk/cache-research/FIU_SRCMap/traces/")
folders=("/disk/cache-research/FIU_SRCMap/traces/")
trace_type=fiu

#folders=("/disk/cache-research/MSR_Cambridge/traces/")
#trace_type=msr

#folders=("/disk/cache-research/CloudVPS/"
#         "/disk/cache-research/CloudCache/")
#trace_type=visa

#folders=("/disk/cache-research/CloudPhysics/")
#trace_type=vscsi

cache_sizes=(0.01 0.02 0.05 0.10 0.15 0.20)
cache_size=0.01
cache_size_real=10
algorithms=("lru" "arc" "lirs" "larc" "marc" "fomo_lru" "fomo_arc" "fomo_lirs")
#algorithms=("lirs" "fomo_lirs")
algorithm=lru
#duration=1d
duration=7d
output_csv=tmp.csv

min_unique=$(python -c "print(int(100 / ${cache_sizes[0]}))")

trace_paths=()

for folder in "${folders[@]}"; do
	traces=($(ls ${folder}))
	for trace in "${traces[@]}"; do
		trace_paths+=("${folder}${trace}")
	done
done

tests=()

for trace_path in "${trace_paths[@]}"; do
	trace=$(basename ${trace_path})

	# TODO get unique count, save locally in trace_type.unique
	unique=$(python3 get_unique.py ${trace_path} ${trace} ${trace_type} ${duration})
	if [ "${unique}" -lt "${min_unique}" ]; then
		continue
	fi

	# TODO for cache_size in cache_sizes
	for cache_size in "${cache_sizes[@]}"; do

		# TODO calculate cache_size
		cache_size_real=$(python -c "print(int(${cache_size} * ${unique}))")

		# have a way to continue or break and skip trace if it winds up too small on 0.01 (1%) of unique (size < 100)

		for algorithm in "${algorithms[@]}"; do
			if [ -z "${duration}" ]; then
				tests+=("sim_test ${trace_path} ${trace} ${trace_type} ${algorithm} ${cache_size} ${cache_size_real} 0 ${output_csv}")
			else
				tests+=("sim_test ${trace_path} ${trace} ${trace_type} ${algorithm} ${cache_size} ${cache_size_real} ${duration} ${output_csv}")
			fi
		done
	done

	
	#tests+=("sim_test ${trace_path} ${trace} ${trace_type} ${algorithm} ${cache_size} ${cache_size_real} ${output_csv}\n")
done

export -f sim_test

IFS=$'\n'
#echo -e "${tests[*]}" | xargs -I % echo '%'
echo -e "${tests[*]}" | xargs --max-procs=6 -I % bash -c '%'
