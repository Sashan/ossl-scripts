#!/bin/sh

#
# script reads results from run-perf.pl script and
# generates barcharts
#
TMPDIR=`mktemp -d`
trap "rm -rf $TMPDIR" EXIT
GPLOT=`mktemp`
trep "rm -rf $GPLOT" EXIT

for i in *.md ; do
	PREFIX=`echo $i | gsed -e 's/\.md//g' -e 's/\([0-9]\+-\)\(.*\)/\2/g'`
	csplit -k -f $PREFIX. $i /###*/ {30}
	for j in $PREFIX.* ; do
		TESTNAME=`grep '###' $j | cut -d ' ' -f 2`
		OUTFILE="$TMPDIR/$PREFIX-$TESTNAME.dat"
		PNG="$PREFIX-$TESTNAME.png"
		COLUMNS=`sed -e 's/-//g' $j`
		COLUMNS=`echo $COLUMNS | wc -c`
		gsed	-e '/^ *$/d'	\
			-e '/^#\+.*/d'	\
			-e '/^|thr.*/d'	\
			-e '/^|-.*/d'	\
			-e 's/|//g'	\
			-e 's/N\/A/0/g'	\
			$j | cat -n  |sed -e 's/\(^ *\)\(.*\)$/\2/g' > $OUTFILE
		rm $j
		TITLE=`echo $PREFIX $TESTNAME | sed -e 's/_/ /g'`
		if [ $COLUMNS -eq 15 ] ; then
			cat > $GPLOT <<EOF
set term png
set output '$PNG'
set title '$TITLE'
set xlabel 'Threads'
set xrange[0:9]
set yrange [0:*]
set xtics ("0" 0, "1" 1, "2" 2, "4" 3, "8" 4, "16" 5, "32" 6, "64" 7, "128" 8) norotate
set ylabel 'time'
set boxwidth .1
set style fill solid

plot '$OUTFILE' using 1:4 \\
	title "1.1.1" with boxes linecolor rgb "#FF0000", \\
	'$OUTFILE' using (\$1+.1):6 \\
	title "3.0" with boxes linecolor rgb "#00FF00", \\
	'$OUTFILE' using (\$1+.2):8 \\
	title "3.1" with boxes linecolor rgb "#0000FF", \\
	'$OUTFILE' using (\$1+.3):10 \\
	title "3.2" with boxes linecolor rgb "#00FFFF", \\
	'$OUTFILE' using (\$1+.4):12 \\
	title "3.3" with boxes linecolor rgb "#F00FFF", \\
	'$OUTFILE' using (\$1+.5):14 \\
	title "master" with boxes linecolor rgb "#FFFF00"
EOF
		else
			cat > $GPLOT <<EOF
set term png
set output '$PNG'
set title '$TITLE'
set xlabel 'Threads'
set xrange[0:9]
set yrange [0:*]
set xtics ("0" 0, "1" 1, "2" 2, "4" 3, "8" 4, "16" 5, "32" 6, "64" 7, "128" 8) norotate
set ylabel 'time'
set boxwidth .1
set style fill solid

plot '$OUTFILE' using 1:4 \\
	title "1.1.1" with boxes linecolor rgb "#FF0000", \\
	'$OUTFILE' using (\$1+.1):6 \\
	title "3.0" with boxes linecolor rgb "#00FF00", \\
	'$OUTFILE' using (\$1+.2):8 \\
	title "3.3" with boxes linecolor rgb "#0000FF", \\
	'$OUTFILE' using (\$1+.3):10 \\
	title "master" with boxes linecolor rgb "#FFFF00"
EOF
		fi
		gnuplot < $GPLOT
	done
done
