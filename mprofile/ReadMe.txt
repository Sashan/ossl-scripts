The idea is to preload mprofile with any application dynamically
linked with openssl library. The mprofiler uses CRYPTO_set_mem_functions(3)
to customize memory allocation functions.
The intended use looks as follows:
	export MPROFILE_OUTF=/tmp/mprofile.json
	LD_PRELOAD+/path/to/mprofile/libmprofile.so ./simple
The ./simple is application linked with libcrypto.so
Upon exit of ./simple application the .json data are generated.

Each call to CRYPTO_malloc(), CRYPTO_free() and CRYPTO_realloc()
is tracked and associated with callstack. The josn file
contains two lists:
    - list with all memory allocations/releases and reallocations,
      each record contains a reference to its stack

    - list of stacks


