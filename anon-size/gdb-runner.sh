#!/bin/sh

#
# this is gdb wrapper command. It instruments debugger to do run desired
# program under the debugger. It also passes all command lines arguments to
# program in debugger. It uses a 'set args' gdb command to do that.
#
# the gdb commands are stored in  temporal file which is passed
# via -x option to gdb.
# 
# besides this the gdb command file does those things:
#	- it sets breakpoint to main (b main)
#	- sarts the program (run)
#	- executes 'info inferior' and pipes its output
#	  to calc-anon.sh script
#	- sets breakpoint to exit (b exit)
#	- continues program execution (cont)
#	- executes 'info inferior' again and pipes its output
#	  cacl-anon.sh script
#	- quits
#
# all output is captured in /tmp/x directory (directory must
# be created first). The output is saved in form of progname.log
#
# Note I'm using procmap(1) because command 'info proc mappings'
# is not supported in gdb on OpenBSD. On other OSes you
# can use info proc mappings in gdb prompt right away.
#
GDBCMDS=`mktemp`

trap "rm -rf $GDBCMDS" EXIT

PROGRAM=$1
shift 1

cat >$GDBCMDS <<EOF
set args $*
b main
run > /dev/null 2>&1
pipe info inferior | `dirname $0`/calc-anon.sh
b exit
cont
pipe info inferior | `dirname $0`/calc-anon.sh
quit
EOF

echo "$PROGRAM $*" >> /tmp/x/`basename $PROGRAM`.log
egdb -x $GDBCMDS $PROGRAM >> /tmp/x/`basename $PROGRAM`.log

