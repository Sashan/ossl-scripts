#!/bin/sh

#
# the gdb info inferior command provides output as follows:
# 
# (gdb) info inferior
#   Num  Description       Executable        
#   * 1    process 67735     /home/sashan/work.openssl/openssl.hpe/test/evp_test 
#
# 'set -f' prevents shell from expanding '*' so we won't grab content
# of current directory
#
# the script works on OpenBSD 7.5
#
# this script is invoked by gdb. It expects output of info inferior
# gdb command. The script reads that on its stdin so gdb
# can feed it via pipe [1].
#
# script parses output to find a PID of process. The PID is passed
# to procmap(1). The command shows memory mapping in form like this:
#
#Start            End                 Size  Offset           rwxSeIpc  RWX  I/W/A Dev     Inode - File
#00000a189a1ba000-00000a189a1bafff       4k 0000000000011000 r----Ip- (rwx) 1/0/0 04:05 3317828 - /usr/libexec/ld.so [0xfffffd8769716520]
#00000a189a1bb000-00000a189a1bbfff       4k 0000000000012000 rw---Ip- (rwx) 1/0/0 04:05 3317828 - /usr/libexec/ld.so [0xfffffd8769716520]
#00000a189a1bc000-00000a189a1bcfff       4k 0000000000000000 rw---Ip- (rwx) 1/0/0 00:00       0 -   [ anon ]
#00000a189a89c000-00000a189a89dfff       8k 0000000000000000 rw----p- (rwx) 1/0/0 00:00       0 -   [ anon ]
#00000a189b48b000-00000a189b48cfff       8k 0000000000000000 rw----p- (rwx) 1/0/0 00:00       0 -   [ anon ]
#00000a189c1da000-00000a189c1dbfff       8k 0000000000000000 r-----p+ (rwx) 1/0/0 04:07 1583107 - /usr/local -unknown-  [0xfffffd8702e46558]
#00000a189c1dc000-00000a189c1dffff      16k 0000000000001000 --x---p+ (rwx) 1/0/0 04:07 1583107 - /usr/local -unknown-  [0xfffffd8702e46558]
#00000a189c1e0000-00000a189c1e0fff       4k 0000000000004000 r-----p- (rwx) 1/0/0 04:07 1583107 - /usr/local -unknown-  [0xfffffd8702e46558]
#00000a189c1e1000-00000a189c1e1fff       4k 0000000000004000 rw----p- (rwx) 1/0/0 04:07 1583107 - /usr/local -unknown-  [0xfffffd8702e46558]
#00000a189d06a000-00000a189d06bfff       8k 0000000000000000 rw----p- (rwx) 1/0/0 00:00       0 -   [ anon ]
#
# we do care about second column size. We also filter only [anon] memory
# where process allocates its stack and heap.
#
# the output of procmap is processed by script we dump to tmpfile.
# perhaps this should be turned to shell function.
#
# [1] https://sourceware.org/gdb/current/onlinedocs/gdb.html/Shell-Commands.html
#
set -f

PMAP=`mktemp`

trap "rm -rf $PMAP" EXIT

cat >$PMAP <<EOF
#!/bin/sh

SUM=0;

for i in \`procmap -p \$1 | grep anon | grep ' rw' | awk '{ print(\$2); }'\` ; do
	echo \$i |grep -e '[kK]' > /dev/null
	if [[ \$? -eq 0 ]] ; then
		SIZE=\`echo \$i | sed -e 's/[kK]$//g'\`;
		SUM=\`expr \$SUM + \$SIZE\`;
		continue;
	else
		echo "Skipping \$i";
	fi
done

echo "Total anon memory: \$SUM"
EOF

chmod +x $PMAP

while read line ;
do
	echo $line |grep 'process' > /dev/null;
	if [[ $? -eq 0 ]] ; then
		print $line
		PID=`print $line | grep process |awk '{print($4);}'`;
		$PMAP $PID
	fi
done
