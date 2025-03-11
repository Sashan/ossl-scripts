#!/usr/bin/env python3
import json
import argparse
from jinja2 import Environment, FileSystemLoader

#
# Functions here create 'API' between .json data which
# come from mprofile.so and the mprofile.py script which
# process those data. So instead of doing
#	mr_id = mr["id"]
# we will be doing
#	mr_id = get_id(mr)
# this should help us to minimize impact of changes to .json
# format in mprofile.so. Perhaps it is overcautious.
#
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
	# memory record is operation on address. There are
	# three operations:
	#	alloc, realloc, free
	# chain of records captures a history/lifecycle
	# of particular buffer identified by its address
	#
	# typical lifecycle looks like a chain of two
	# records:
	#	alloc -> free
	#
	# or it can be more eventful:
	#	alloc -> realloc -> realloc -> ... -> realloc -> free
	#
	# __create_chains() private method constructs chains
	# from records kept _mem_records list.
	#
	def __create_chains(self):
		#
		# create a filter function to find all operations
		# which belong to particular chain. The mr_a is
		# memory record which allocates the buffer. Function
		# helps us to find all remaining operations on address
		# kept in mr_a record.
		#
		# mr_w is the address which comes from list of all memory
		# operations where we search for or desired chain links.
		# mr_a holds a record which tracks allocation of a buffer
		#
		def create_chain_filter_f(mr_a):
			def matches_alloc(mr_w, mr_a):
				return get_addr(mr_a) == get_addr(mr_w) and \
				    get_previd(mr_w) == 0 and is_free(mr_w) or \
				    get_realloc(mr_w) == get_addr(mr_a) and \
				    get_previd(mr_w) == 0 and is_realloc(mr_w)

			return lambda mr_w: True if matches_alloc(mr_w, mr_a) \
			    else False

		index = 1
		for mr in self._mem_records:
			if is_free(mr):
				continue # skip free operations
			#
			# the search starts at memory record
			# which follows mr mem_records holds
			# a temporal list where we are searching
			# for chain links.
			#
			index = index + 1
			mem_records = list(self._mem_records)[index:]
			chain_filter_f = create_chain_filter_f(mr)
			release_ops = list(filter(chain_filter_f, mem_records))
			if (len(release_ops) > 0):
				r_op = release_ops[0]
				set_nextid(mr, get_id(r_op))
				set_previd(r_op, get_id(mr))
			else:
				#
				# there is no free operation in chain,
				# so it is a memory leak then.
				#
				self.__add_leak(mr)

	#
	# retrieve a record chain for allocation record ar.
	#
	def __get_chain(self, ar):
		chain = []
		while get_nextid(ar) != 0:
			mr = self.get_mr(get_nextid(ar))
			chain.append(ar)
		return chain

	#
	# constructor receives a json data. json data is dictionary
	# which holds two lists:
	#	allocations (list of memory records (operations)
	#	stacks list of stack traces
	#
	def __init__(self, json_data):
		self._leaks = []
		self._mem_records = json_data["allocations"]
		#
		# jjosn data come with no chain links. we just
		# add link keys to record dictionary
		#
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

	#
	# count allocation failures
	#
	def alloc_failures(self):
		return filter(
		    lambda x: True if is_alloc(x) and get_addr(x) == 0
			else False,
		    self._mem_records)

	#
	# count reallocation failures
	#
	def realloc_failures(self):
		return filter(
		    lambda x: True if is_realloc(x) and get_addr(x) == 0 and \
			get_size(x) > 0 else False, self._mem_records)

	#
	# return list of memory leaks
	#
	def leaks(self):
		return self._leaks

	#
	# return the size of given memory leak
	#
	def get_leak_sz(self, leak_mr):
		next_mr = self.get_next(leak_mr)
		while next_mr != None:
			leak_mr = next_mr
			next_mr = self.get_next(leak_mr)
		return get_rsize(leak_mr)

	#
	# return list of allocation operations
	#
	def alloc_ops(self):
		return filter(
		    lambda x: True if is_alloc(x) and get_addr(x) != 0
			else False,
		    self._mem_records)

	#
	# return list of reallocation operations
	#
	def realloc_ops(self):
		return filter(
		    lambda x: True if is_realloc(x) and get_addr(x) != 0
			else False,
		    self._mem_records)

	#
	# return list of all free (release) operations
	#
	def release_ops(self):
		f = filter(
		    lambda x: True if is_free(x) and get_addr(x) != 0
			else False,
		    self._mem_records)

	#
	# get memory record for given id. returns None when
	# id not found
	# 
	def get_mr(self, op_id):
		if op_id < 1 or op_id > len(self._mem_records):
			return None
		op_id = op_id - 1
		return self._mem_records[op_id]

	#
	# get callstack for given memory record
	#
	def get_stack(self, mr):
		if mr == None or get_stackid(mr) == 0:
			return None
		return get_trace(self._stacks[get_stackid(mr) - 1])

	#
	# get next link in memory lifecycle chain
	#
	def get_next(self, mp):
		if get_nextid(mp) != 0:
			return self.get_mr(get_nextid(mp))
		else:
			return None

	#
	# get previous link in memory lifecycle chain
	#
	def get_prev(self, mp):
		if get_previd(mp) != 0:
			return self.get_mr(get_previd(mp))
		else:
			return None

	#
	# get the size of memory operation associated with
	# record.. Only allocations records have memory size.
	# sizes of free and realloc operations must be calculated
	# using the data in previous link in chain.
	#
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

	#
	# calculate total number of bytes allocated
	#
	def get_total_mem(self):
		# summary of all memory allocated by malloc()
		alloc_sz = sum(map(lambda x: get_rsize(x), mp.alloc_ops()))
		# summary of all memory allocated via realloc, we deliberately
		# ignore memory freed by realloc (rsize < 0)
		alloc_sz = alloc_sz + sum(map(lambda x: 0 if get_rsize(x) < 0 \
		    else get_rsize(x), mp.realloc_ops()))
		return alloc_sz

	#
	# calculate total number of operations (malloc/realloc)
	# which allocate memory
	#
	def get_total_allocs(self):
		# all allocations
		ops = len(list(mp.alloc_ops()))
		# only those reallocs which got memory (rsize > 0)
		ops = ops + sum(map(
		    lambda x: 1 if get_rsize(x) > 0 else 0, mp.realloc_ops()))
		return ops

def create_parser():
	parser = argparse.ArgumentParser()
	parser.add_argument("json_file",
	    type = str,
	    help = "mprofile json data")
	parser.add_argument("-v", "--verbose", action = "store_true")
	parser.add_argument("-l", "--leaks", help = "report memory leaks",
	    action = "store_true")
	parser.add_argument("-a", "--allocated", help = "report all memory allocated",
	    action = "store_true")

	return parser

def report_leaks(mp, parser_args):
	leaks = mp.leaks()
	if len(leaks) == 0:
		print("There are no leaks")
		return

	print("{0} bytes lost in {1} leaks".format(
	    sum(map(lambda x: mp.get_leak_sz(x), leaks)), len(leaks)))

	if parser_args.verbose:
#		e = Environment(loader = FileSystemLoader("templates/"))
#		t = e.get_template("leaks.txt")
		i = 1
		for l_mr in mp.leaks():
			print("Leak {0}:".format(i))
			i = i + 1
			print("\t{0} bytes".format(mp.get_leak_sz(l_mr)))
			print("\t{0}({1}) bytes".format(get_operation(l_mr),
			    get_rsize(l_mr)), end='')
			l_mr = mp.get_next(l_mr)
			while l_mr != None:
				print("-> {0}({1}) bytes".format(
				    get_operation(l_mr),
				    get_rsize(l_mr)), end='')
				l_mr = mp.get_next(l_mr)
			print("\n------------")
	return

def report_mem_total(mp, parser_args):
	print("Total memory allocated: {0} in {1} operations".format(
	    mp.get_total_mem(), mp.get_total_allocs()))
	return


if __name__ == "__main__":
	parser = create_parser()
	args = parser.parse_args()
	if args.json_file == None:
		parser.usage()

	j = json.load(open(args.json_file))

	mp = MProfile(j)

	if args.allocated:
		report_mem_total(mp, args)

	if args.leaks:
		report_leaks(mp, args)

