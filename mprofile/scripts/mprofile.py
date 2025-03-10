#!/usr/bin/env python3
import json
import argparse

def is_realloc(mr):
	return mr["state"] == "realloc"

def is_alloc(mr):
	return mr["state"] == "allocated"

def is_free(mr):
	return mr["state"] == "free"

def get_addr(mr):
	return mr["addr"]

def get_realloc(mr):
	return mr["realloc"]

def get_rsize(mr):
	return mr["rsize"]

def get_id(mr):
	return mr["id"]

def get_index(mr):
	return mr["id"] - 1

def get_rsize(mr):
	return mr["rsize"]

def get_operation(mr):
	return mr["state"]


class MProfile:
	def __create_pairs(self):
		def create_filter_f(mr):
			def matches_alloc(mr_a, mr_r):
				return get_addr(mr_a) == get_addr(mr_r) and \
				    mr_a["prev_id"] == 0 and is_free(mr_a) or \
				    get_realloc(mr_a) == get_addr(mr_r) and \
				    mr_a["prev_id"] == 0 and is_realloc(mr_a)

			return lambda x: True if matches_alloc(x, mr) else False

		index = 1
		for mr in self._mem_records:
			f = create_filter_f(mr)
			mem_records = list(self._mem_records)[index:]
			index = index + 1
			release_ops = list(filter(f, mem_records))
			if (len(release_ops) > 0):
				r_op = release_ops[0]
				self._mem_records[get_index(mr)]["next_id"] =\
				    get_id(r_op)
				self._mem_records[get_index(r_op)]["prev_id"] =\
				    get_id(mr)

			
	def __init__(self, json_data):
		self._mem_records = json_data["allocations"]
		for mr in self._mem_records:
			mr["prev_id"] = 0
			mr["next_id"] = 0
		self._stacks = json_data["stacks"]
		self.__create_pairs()

	def alloc_failures(self):
		return filter(
		    lambda x: True if is_aloc(x) and get_addr(x) == 0
			else False,
		    self._mem_records)

	def realloc_failures(self):
		return filter(
		    lambda x: True if is_realloc(x) and get_addr(x) == 0 and \
			get_size(x) > 0 else False, self._mem_records)

	def alloc_ops(self):
		return filter(
		    lambda x: True if is_alloc(x) and get_addr(x) != 0
			else False,
		    self._mem_records)

	def realloc_ops(self):
		return filter(
		    lambda x: True if is_realloc(x) and get_addr(x) != 0
			else False,
		    self._mem_records)

	def release_ops(self):
		f = filter(
		    lambda x: True if is_free(x) and get_addr(x) != 0
			else False,
		    self._mem_records)

	def get_op(self, op_id):
		if op_id < 0 or op_id > len(self._mem_records):
			return None
		op_id = op_id - 1
		return self._mem_records[op_id]

	def get_size(self, mr):
		if is_alloc(mr):
			return get_rsize(mr)
		if is_free(mr):
			if mr["prev_id"] != 0:
				mr = self.get_op(mr["prev_id"])
				return get_rsize(mr) * (-1)
			else:
				return 0
		if is_realloc(mr):
			if mr["prev_id"] != 0:
				mr_p = self.get_op(mr["prev_id"])
				return get_rsize(mr) - get_rsize(mr_p)
			else:
				return get_rsize(mr)
		else:
			return 0

	def dump(self):
		print(self._mem_records)

def create_parser():
	parser = argparse.ArgumentParser()
	parser.add_argument("json_file",
	    type = str,
	    help = "mprofile json data")
	return parser

if __name__ == "__main__":
	parser = create_parser()
	args = parser.parse_args()
	if args.json_file == None:
		parser.usage()

	j = json.load(open(args.json_file))

	mp = MProfile(j)

	for ma in mp.alloc_ops():
		print("allocated {0}".format(get_rsize(ma)), end='')
		while ma["next_id"] != 0:
			ma = mp.get_op(ma["next_id"])
			print("  -> {0} {1}".format(get_operation(ma), mp.get_size(ma)), end='')
		print("\n")
