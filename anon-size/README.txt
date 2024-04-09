There are two scripts:
	gdb-runner.sh
	calc-anon.sh

calc-anon.sh is being invoked from gdb which runs
on behalf of gdb-runner.sh. Everything has been tested
on OpenBSD 7.5

One has to create directory where gdb-runner.sh will store
its results. I use the script to determine memory footprint
for various test runs.

I use gdb-runner.sh script in 'shell script glue' which
is executed by unit tests. The shell glue looks as follows:
----8<----
#/bin/sh

COMMAND=`echo $0 |sed -e 's/\.sh//g'`

GDB_RUN=/path/to/gdb-runner.sh

$GDB_RUN $COMMAND $*

----8<----

The unit test part (OpenSSL tests for example) is modified.  Typically where
unit test driver invokes some binary (evp_test for example) I change the
invocation for script above which is kept in file evp_test.sh.
