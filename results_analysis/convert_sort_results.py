import sys

# results dictionary to store results
# [trace_type] [trace_name] [duration] [cache_size] [algorithm]
results = {}

any_missing = False

group_mapping = {}


def get_groupings():
    with open('workload.groups', 'r') as f:
        for line in f:
            group, trace = line.strip().split(',')
            group_mapping[trace] = group
    print(group_mapping)


def create_path(trace_type, trace_name, duration, cache_size, algorithm):
    if not trace_type in results:
        results[trace_type] = {}
    if not trace_name in results[trace_type]:
        results[trace_type][trace_name] = {}
    if not duration in results[trace_type][trace_name]:
        results[trace_type][trace_name][duration] = {}
    if not cache_size in results[trace_type][trace_name][duration]:
        results[trace_type][trace_name][duration][cache_size] = {}
    if not algorithm in results[trace_type][trace_name][duration][cache_size]:
        results[trace_type][trace_name][duration][cache_size][algorithm] = {}


def convert_to_dict():
    for line_orig in open(sys.argv[1], 'r'):
        line_orig = line_orig.strip()
        line = line_orig.split(',')

        if len(line) == 16:
            line.insert(5, '')
        elif len(line) != 17:
            #print(line_orig, end='')
            continue

        # replace trace_type with grouping
        line[0] = group_mapping[line[1]]

        # work with new 17 length line
        trace_type, trace_name, cache_size, real_size, algorithm = line[:5]
        duration, hits, misses, filters = line[5:9]
        promotions, demotions, private = line[9:12]
        read_hits, read_misses = line[12:14]
        write_hits, write_misses, dirty_evicts = line[14:]

        create_path(trace_type, trace_name, duration, cache_size, algorithm)

        results[trace_type][trace_name][duration][cache_size][algorithm] = {
            "hits": hits,
            "read_hits": read_hits,
            "write_hits": write_hits,
            "misses": misses,
            "read_misses": read_misses,
            "write_misses": write_misses,
            "filters": filters,
            "promotions": promotions,
            "dirty_evicts": dirty_evicts,
            "line": ",".join(line)
        }


# check that all sizes are accounted for
sizes = ("0.01", "0.02", "0.05", "0.10", "0.15", "0.20")

# check that all algorithms are accounted for
algorithms = ("lru", "arc", "lirs", "fomo_lru", "fomo_arc", "fomo_lirs",
              "marc", "larc")

file_results = []


def check_for_missing():
    for trace_type in results:
        for trace_name in results[trace_type]:
            any_missing = False

            for duration in results[trace_type][trace_name]:
                for size in sizes:
                    if size not in results[trace_type][trace_name][duration]:
                        print(
                            ",".join([trace_type, trace_name, duration, size]),
                            "missing")
                        any_missing = True
                    else:
                        for algorithm in algorithms:
                            if algorithm not in results[trace_type][
                                    trace_name][duration][size]:
                                print(
                                    ",".join([
                                        trace_type, trace_name, duration, size,
                                        algorithm
                                    ]), "missing")
                                any_missing = True
                        if not any_missing:
                            for algorithm in algorithms:
                                file_results.append(
                                    results[trace_type][trace_name][duration]
                                    [size][algorithm]['line'])


def convert_to_file():
    with open('tmp.csv', 'w') as f:
        f.write("\n".join(sorted(file_results)))


def convert_sort_results():
    get_groupings()
    convert_to_dict()
    check_for_missing()
    # convert dict to sorted results file
    convert_to_file()


convert_sort_results()
