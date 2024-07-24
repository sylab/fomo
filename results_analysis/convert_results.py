# From bimodal-caching project, altered a bit for this project

import os
import itertools
import sys
"""
MIN_CACHE_SIZE=1047488
MAX_MULT=2
"""

MIN_CACHE_SIZE = 16367
MAX_MULT = 6


def run():
    result_dir = "/home/sylab/dmcache_results/0"
    if not os.path.exists(result_dir):
        raise ValueError("results folder not in this directory")

    data_dir = os.curdir
    if not os.path.exists(data_dir):
        raise ValueError("data folder not in this directory")

    workloads = ["fileserver.f.2g", "webserver.f.2g"]

    write_types = ["writeback", "writethrough"]

    cache_sizes = [MIN_CACHE_SIZE * (2**mult) for mult in range(0, MAX_MULT)]

    algs = [
        "lru", "arc", "larc", "mlru", "marc", "mlarc", "mlru_lw", "marc_lw",
        "mlarc_lw", "lecar"
    ]

    for write_type in write_types:
        data_file = "{}/{}.results".format(data_dir, write_type)

        with open(data_file, 'w') as f:
            for workload, cache_size in itertools.product(
                    workloads, cache_sizes):
                current_dir = "{}/{}/{}/{}".format(result_dir, workload,
                                                   write_type, cache_size)

                for alg in algs:
                    file = "{}/{}.out".format(current_dir, alg)
                    converted_line = convert_result(file, alg=alg, \
                     workload=workload, write_type=write_type, \
                     cache_size=cache_size)
                    f.write(converted_line)
    """
    drives = ["sdd"]

    drive_file = "{}/drive.results".format(data_dir)

    with open(drive_file, 'w') as f:
        for drive in drives:
            for workload in workloads:
                data = []
                data.append(drive)

                disk_file = "{}/{}/{}.out".format(result_dir, workload, drive)

                with open(disk_file, 'r') as rf:
                    lines = rf.readlines()
                    summary = lines[-2].strip().split()

                    data.append(workload)

                    # ops
                    # ops/sec
                    # rd/wr
                    # mb/s
                    # msec/op
                    data.append(summary[3])
                    data.append(summary[5])
                    data.append(summary[7])

                    tmp = summary[9]
                    data.append(tmp[:tmp.index('mb/s')])

                    tmp = summary[10]
                    data.append(tmp[:tmp.index('ms/op')])
                f.write(','.join(data) + '\n')
    """


# convert result from file, otherwise fail
def convert_result(file,
                   *,
                   alg=None,
                   workload=None,
                   write_type=None,
                   cache_size=None):
    if not alg or not workload or not write_type or not cache_size:
        raise ValueError("missing argument(s) for convert_result")

    data = []

    # alg
    data.append(alg)

    # workload
    data.append(workload)

    # cache_size
    data.append(str(cache_size))

    if not os.path.isfile(file):
        raise ValueError("file not found: {}".format(file))

    with open(file, 'r') as f:
        lines = f.readlines()

        if len(lines) < 20:
            raise ValueError("file is too short: {}".format(file))

        summary = None
        for line in lines:
            if "IO Summary:" in line:
                summary = line.strip().split()
            if "cache" in line and "write" in line and "dmsetup" not in line:
                status = line.strip().split()
            if "error" in line or "Error" in line:
                print("error found in result, please check: {}".format(file))

        if not summary:
            print("{} {} {} {}".format(alg, workload, cache_size, write_type))
            raise ValueError("summary not found: {}".format(file))

        if len(summary) != 11:
            raise ValueError("summary incomplete: {}".format(file))

        # ops
        # ops/sec
        # rd/wr
        # mb/s
        # msec/op
        data.extend((summary[3], summary[5], summary[7],
                     summary[9][:summary[9].index('mb/s')],
                     summary[10][:summary[10].index('ms/op')]))

        # read hits
        # read misses
        # write hits
        # write misses
        # demotions
        # promotions
        # blocks in cache
        # dirty
        # TODO: update with missing data name
        data.extend(status[4:12])

        # sylab_stats
        # ios
        # hits
        # misses
        # filtered
        # lockfail
        # skipped
        # promoted
        # demoted
        # migration_skips
        if 'policy_stats' in status or 'sylab_stats' in status:
            if 'policy_stats' in status:
                tmp = status.index('policy_stats')
            elif 'sylab_stats' in status:
                tmp = status.index('sylab_stats')
                print("sylab_stats found in {}".format(file))
            stats = list(map(int, status[tmp + 1:tmp + 10]))

            for stat in stats:
                data.append(str(stat))

    return ','.join(data) + '\n'


if __name__ == "__main__":
    run()
