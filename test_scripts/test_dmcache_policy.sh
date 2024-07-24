#!/bin/bash

# bash script settings for debugging and safety:
#  e - exit on return code != 0 (a.k.a an error)
#  u - exit on undefined variable
#  o - exit if fail in pipe
#  x - print all commands (best for debugging where problems occur)
# add and remove as needed, but please leave documentation
set -euo > /dev/null

root_check() {
	echo "root_check()"
	if [[ $EUID -ne 0 ]]; then
		echo "Need to be root" 1>&2
		exit 1
	fi
}

config_file=${1}

# empty defaults
workload_dir=''
workloads=''
target_disk=''
cache_sizes=''
write_types=''
algorithms=''
cache_disk=''
target_size=''
dmcache_extras=''
results_dir=''
results_folder=''
results_file=''

read_config() {
# set most of the above empty defaults
# not set:
#   results_folder
#   results_file
	echo "read_config()"
	source ${config_file}
	target_size=$(blockdev --getsz ${target_disk})
}

pretest_check() {
# get modules inserted
	echo "pretest_check()"
	if [[ ! $(lsmod | grep dm_cache) ]]; then
		echo " - dm-cache not inserted. inserting"
		# Gotta insert our version of dm-cache
		modprobe -i dm-cache
		rmmod dm_cache
		insmod ../dm-cache.ko
	fi
	if [[ ! $(lsmod | grep dmcache_policy) ]]; then
		echo " - dmcache-policy not inserted. inserting"
		insmod ../dmcache-policy.ko
	fi
}

create_set_results_folder() {
# create folder to store results in
#   format:
#     dmcache_policy_results/${extras}/${workload}/write(back/through)/${cache_size}/
# -p: no error if existing, make parent directories as needed
	echo "create_set_results_folder()"
	results_folder="${results_dir}/${dmcache_extras}/${workload}/${write_type}/${cache_size}"
	mkdir -p ${results_folder}
}

set_results_file() {
# set global variable results_file for output
	echo "set_results_file()"
	results_file="${results_folder}/${algorithm}.out"
}

create_metadata() {
	echo "create_metadata()"
	if [ ! -e /dev/mapper/ssd-metadata ]; then
		echo " - creating ssd-metadata"
		dmsetup create ssd-metadata --table \
			"0 20000 linear ${cache_disk} 0"
	fi
	echo " - zeroing out ssd-metadata"
	dd if=/dev/zero of=/dev/mapper/ssd-metadata conv=sync,noerror count=$(blockdev --getsz /dev/mapper/ssd-metadata)

}

remove_metadata() {
	echo "remove_metadata()"
	if [ -e /dev/mapper/ssd-metadata ]; then
		echo " - removing ssd-metadata"
		dmsetup remove ssd-metadata
	fi
}

create_blocks() {
	echo "create_blocks()"
	if [ ! -e /dev/mapper/ssd-blocks ]; then
		echo " - creating ssd-blocks"
		dmsetup create ssd-blocks --table \
			"0 ${cache_size} linear ${cache_disk} 20000"
	fi
}

remove_blocks() {
	echo "remove_blocks()"
	if [ -e /dev/mapper/ssd-blocks ]; then
		echo " - removing ssd-blocks"
		dmsetup remove ssd-blocks
	fi
}

create_cache() {
	echo "create_cache()"
	if [ ! -e /dev/mapper/target-cache ]; then
		echo " - creating target-cache"
		dmsetup create target-cache --table \
			"0 ${target_size} cache
			 /dev/mapper/ssd-metadata /dev/mapper/ssd-blocks ${target_disk}
			 64 1 ${write_type} ${algorithm} ${dmcache_extras}"
	fi
}

remove_cache() {
	echo "remove_cache()"
	if [ -e /dev/mapper/target-cache ]; then
		echo " - removing target-cache"
		dmsetup remove target-cache
	fi
}

setup_target_disk() {
# essentially the same to that found in test-cache-alg.sh in older stuff
	echo "setup_target_disk()"
	yes | mkfs.ext2 ${target_disk} > /dev/null
	mount ${target_disk} /mnt
	head -n -1 ${workload_dir}/${workload} > /tmp/tmp.f
	filebench -f /tmp/tmp.f > /dev/null
	sync
	echo 3 > /proc/sys/vm/drop_caches
	sleep 30
	umount /mnt
}

run_test() {
	echo "run_test() ${workload} ${write_type} ${cache_size} ${algorithm}"
	date
	mount /dev/mapper/target-cache /mnt -t auto
	filebench -f ${workload_dir}/${workload}
	umount /mnt

	dmsetup status target-cache

	sync
	echo 3 > /proc/sys/vm/drop_caches
	sleep 30

	dmesg | tail -n 20
}

run_tests() {
	echo "run_tests()"
	for workload in ${workloads[@]}; do
		for write_type in ${write_types[@]}; do
			for cache_size in ${cache_sizes[@]}; do
				for algorithm in ${algorithms[@]}; do
					remove_cache
					remove_blocks
					remove_metadata

					setup_target_disk

					create_metadata
					create_blocks
					create_cache

					create_set_results_folder 
					set_results_file
					run_test | tee /dev/tty > ${results_file}
				done
			done
		done
	done

	remove_cache
	remove_blocks
	remove_metadata
}

# main()
root_check
read_config

# read_config debugging
#echo "workload_dir=${workload_dir}"
#echo "workloads=${workloads}"
#echo "target_disk=${target_disk}"
#echo "cache_sizes=${cache_sizes}"
#echo "write_types=${write_types}"
#echo "algorithms=${algorithms}"
#echo "cache_disk=${cache_disk}"
#echo "target_size=${target_size}"
#echo "dmcache_extras=${dmcache_extras}"
#echo "results_dir=${results_dir}"

pretest_check

# pretest_check debugging
#lsmod | grep cache

run_tests
