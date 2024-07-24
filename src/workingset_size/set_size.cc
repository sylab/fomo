#include "tools/logs.h"
#include "trace_reader/trace_reader.h"
#include "set_size_args.h"
#include <iostream>
#include <unordered_set>

std::unordered_set<oblock_t> unique; 

// TODO set_size_options
struct set_size_options options = {
	.fp = NULL, .trace_name = NULL, .duration_hrs = 0,
};

int main(int argc, char **argv) {
	options.fp = stdin;

	handle_args(argc, argv, &options);

	struct trace_reader *reader = find_trace_reader(options.trace_name);
	LOG_ASSERT(reader != NULL);
	reader->init(options.fp, options.duration_hrs);

	struct trace_reader_result read_result = {0};

	while (!reader->read(&read_result)) {
		oblock_t oblock = read_result.oblock;

		if (unique.find(oblock) == unique.end()) {
			unique.insert(oblock);
		}
	}

	std::cout << unique.size() << '\n';

	return 0;
}
