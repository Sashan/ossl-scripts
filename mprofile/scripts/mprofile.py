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

def get_stackid(mr):
	return mr["stack_id"]

def get_nextid(mr):
	return mr["next_id"]

def get_previd(mr):
	return mr["prev_id"]

def set_nextid(mr, next_id):
	mr["next_id"] = next_id
	return

def set_previd(mr, prev_id):
	mr["prev_id"] = prev_id
	return

def get_trace(st):
	return st["stack_trace"]

class MProfile:
	#
	# traverse allocation chain back to the first operation
	# in chain which causes the leak.
	#
	def __add_leak(self, mr):
		leak = mr
		while mr != None:
			leak = mr
			mr = self.get_prev(mr)
		self._leaks.append(leak)
	#
	# correct memory operations come in pairs
	#	alloc -> free
	# or it can be a chain of
	#	alloc -> realloc -> realloc -> ... -> realloc -> free
	#
	# __create_chains() private method constructs chains
	# of memory records to track memory operations
	#
	def __create_chains(self):
		#
		# create a filter function which helps to
		# find matching realloc/free for given 
		# memory allocation record mr
		#
		def create_filter_f(mr_r):
			def matches_alloc(mr_a, mr_w):
				return get_addr(mr_a) == get_addr(mr_w) and \
				    get_previd(mr_a) == 0 and is_free(mr_a) or \
				    get_realloc(mr_a) == get_addr(mr_w) and \
				    get_previd(mr_a) == 0 and is_realloc(mr_a)

			return lambda mr_a: True if matches_alloc(mr_a, mr_r) \
			    else False

		index = 1
		for mr in self._mem_records:
			if is_free(mr):
				continue
			f = create_filter_f(mr)
			mem_records = list(self._mem_records)[index:]
			index = index + 1
			release_ops = list(filter(f, mem_records))
			if (len(release_ops) > 0):
				r_op = release_ops[0]
				set_nextid(mr, get_id(r_op))
				set_previd(r_op, get_id(mr))
			else:
				self.__add_leak(mr)

	def __get_chain(self, mr):
		chain = []
		while get_nextid(mr) != 0:
			mr = self.get_mr(get_nextid(mr))
			chain.append(mr)
		return chain

	def __init__(self, json_data):
		self._leaks = []
		self._mem_records = json_data["allocations"]
		for mr in self._mem_records:
			set_previd(mr, 0)
			set_nextid(mr, 0)
		self._stacks = json_data["stacks"]
		#
		# call stacks in json data are dump of RB-tree,
		# we need to sort the array/list by 'id'
		#
		self._stacks.sort(key = lambda x : x["id"])
		self.__create_chains()

	def alloc_failures(self):
		return filter(
		    lambda x: True if is_alloc(x) and get_addr(x) == 0
			else False,
		    self._mem_records)

	def realloc_failures(self):
		return filter(
		    lambda x: True if is_realloc(x) and get_addr(x) == 0 and \
			get_size(x) > 0 else False, self._mem_records)

	def leaks(self):
		return self._leaks

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

	def get_mr(self, op_id):
		if op_id < 1 or op_id > len(self._mem_records):
			return None
		op_id = op_id - 1
		return self._mem_records[op_id]

	def get_stack(self, mr):
		if mr == None or get_stackid(mr) == 0:
			return None
		return get_trace(self._stacks[get_stackid(mr) - 1])

	def get_next(self, mp):
		if get_nextid(mp) != 0:
			return self.get_mr(get_nextid(mp))
		else:
			return None

	def get_prev(self, mp):
		if get_previd(mp) != 0:
			return self.get_mr(get_previd(mp))
		else:
			return None

	def get_size(self, mr):
		if is_alloc(mr):
			return get_rsize(mr)
		if is_free(mr):
			prev = self.get_prev(mr)
			if prev != None:
				return get_rsize(prev) * (-1)
			else:
				return 0
		if is_realloc(mr):
			prev = self.get_prev(mr)
			if prev != None:
				return get_rsize(mr) - get_rsize(prev)
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

	#for ma in mp.alloc_ops():
	#	print("allocated {0} [{1}]".format(get_rsize(ma), get_id(ma)), end='')
	#	while get_nextid(ma) != 0:
	#		ma = mp.get_mr(get_nextid(ma))
	#		print("  -> {0} [{1}]  {2}".format(get_operation(ma), get_id(ma), mp.get_size(ma)), end='')
	#	print("\n")

	leaks = mp.leaks()
	if len(leaks) > 0:
		print("Found {0} memory leaks".format(len(leaks)))
		for l in leaks:
			print("{0} [{1}] bytes".format(get_rsize(l), get_id(l)))
			stack = mp.get_stack(l)
			if stack == None:
				stack = []
			for frame in stack:
				print(frame)
			print("allocated {0} [{1}]".format(get_rsize(l), get_id(l)), end='')
			next_record = mp.get_next(l)
			while next_record != None:
				print(" -> {0} [{1}] {2}".format(get_operation(next_record), get_id(next_record), mp.get_size(next_record)), end='')
				next_record = mp.get_next(next_record)
			print("\n--------------")
