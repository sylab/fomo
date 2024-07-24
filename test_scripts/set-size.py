#!/bin/python3

import sys

def readFIU(line):
	block_size = 512
	page_size = 4096
	blocks_per_page = page_size // block_size

	line = line.split(' ')
	lba = int(line[3])
	size = int(line[4])

	align = lba % blocks_per_page
	lba -= align
	size += align

	# adds 1 page if just a little bit of a page is used
	if size % blocks_per_page > 0:
		size += blocks_per_page

	for offset in range(size // blocks_per_page):
		yield lba + (blocks_per_page * offset)

def readMSR(line):
	block_size = 512

	line = line.split(',')
	lba = int(line[4])
	size = int(line[5])

	align = lba % block_size
	lba -= align
	size += align

	# adds 1 page if just a little bit of a page is used
	if size % block_size > 0:
		size += block_size

	for offset in range(size // block_size):
		yield lba + (block_size * offset)

if __name__ == "__main__":
	filename = sys.argv[1]

	read = readFIU
	if sys.argv[2] == 'msr':
		read = readMSR
	elif sys.argv[2] != 'fiu':
		exit(1)

	dm_cache = False
	if len(sys.argv) > 3:
		dm_cache = sys.argv[3] == 'dm-cache'

	working_set = set()

	with open(filename, 'r') as f:
		for line in f:
			for x in read(line):
				if dm_cache:
					if read == readFIU:
						# in 4k blocks to 64k blocks
						align = x % 16
						x -= align
					elif read == readMSR:
						# in bytes, aligned to 512, let's align to 64k
						align = x % (16 * 4096)
						x -= align
				working_set.add(x)

	print(len(working_set))
