#!/bin/sh

#
# generate all tests target
#
echo "test-all: \\"
for i in static implicit base64 base64-static ; do
	echo "\ttest-all-$i \\"
done
echo "\ttest-done"
echo ''

#
# generate targets:
#	test-all-static, test-all-implicit
# targets run all atexit variants
#
for variant in static implicit ; do
	echo "test-all-$variant: \\"
	for atexit in mainAtExit base64AtExit noatexit ; do
		echo "\ttest-$variant-$atexit \\"
	done
	echo "\ttest-done"
	echo ''
done

#
# generate test-all-base64 and test-all-base64-static targets
# targets excercises all atexit variants
#
for so in base64 base64-static ; do
	echo "test-all-$so: \\"
	for bind in now lazy ; do
		for local in global local ; do
			for atexit in noatexit mainAtExit base64AtExit ; do
				echo "\ttest-explicit-$so-$bind-$local-$atexit \\"
			done
		done
	done
	echo "\ttest-done"
	echo ''
done

#
# generate targets
# test-all-noatexit, test-all-mainAtExit and test-all-base64AtExit
# targets which allow selection of exit handler to arm
#
for atexit in noatexit mainAtExit base64AtExit ; do
	echo "test-all-$atexit: \\"
	for mode in static implicit explicit-base64 explicit-base64-static ; do
		echo "\ttest-$mode-atexit \\"
	done
	echo "\ttest-done"
	echo ''
done

#
# generate test-{base64, base64-static}-{noatexit, base64AtExit, mainAtExit}
# targets to select atexit handler to arm.
#
for so in base64 base64-static ; do
	for atexit in noatexit base64AtExit mainAtExit ; do
		echo "test-$so-$atexit:	\\"
		for bind in now lazy ; do
			for local in global local ; do
				echo "\ttest-explicit-$so-$bind-$local-$atexit \\"	
			done
		done
		echo "\ttest-done"
		echo ''
	done
done

#
# generate test targets for explicit linking
#
cat <<-EOF
#
# Here we define targets for test which use dlopen() to
# load base64 library. There are two variants of base64
# library:
#    - base64.so which dynamically loads libcypto via DT_NEEDED
#    - base64-static.so which is statically linked with libcrypto
# The tests also covers all combinations of dlopen() function:
#    RTLD_NOW, RTLD_LAZY, RTLD_LOCAL, RTLD_GLOBAL
# And finally test excercises OPENSSL_atexit(), there are those
# variants covered in targets below:
#    - no atexit handler set
#    - we set base64AtExit() exit handler which defined in base64*.so
#    - we set mainAtExit() exit handler which is defined along the main()
#
EOF
for so in base64 base64-static ; do
	for bind in now lazy ; do
		for local in global local ; do
			# place atExit not armed here
			echo "test-explicit-$so-$bind-$local-noatexit: test.explicit $so.so"
			echo "\t./test.explicit -l ./$so.so -f '$bind|$local'|grep OPENSSL_cleanup"
			echo ''
			for atexit in  mainAtExit base64AtExit ; do
				echo "test-explicit-$so-$bind-$local-$atexit: test.explicit $so.so"
				echo "\t./test.explicit -a $atexit -l ./$so.so -f '$bind|$local'|grep $atexit"
				echo "\t./test.explicit -a $atexit -l ./$so.so -f '$bind|$local'|grep OPENSSL_cleanup"
				echo ''
			done
		done
	done
done

#
# generate test targets for static binary
#
cat <<-EOF

#
# Here we define targets to test statically and implicitly linked binaries
# test exit handler
#
EOF
for style in static implicit ; do
	echo "test-$style-noatexit: test.$style"
	echo "\t./test.$style | grep OPENSSL_cleanup"
	echo ''
	for atexit in mainAtExit base64AtExit ; do
		echo "test-$style-$atexit: test.$style"
		echo "\t./test.$style -a $atexit | grep $atexit"
		echo "\t./test.$style -a $atexit | grep OPENSSL_cleanup"
		echo ''
	done
done

echo 'test-done:'
echo "\t@ echo"
echo ''

echo 'clean:'
echo '\trm -f *.so *.o test.implicitly test.explicit test.static'
