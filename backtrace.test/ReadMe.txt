Simple test tool to verify backtracing using libgcc_s works
for perlasm code.

It builds on Bernd's idea here [1]. Instead of relying
on SIGALARM this test requires placing int3 instruction to
desired position in code where backtrace should be started from.

The int3 instruction triggers SIGTRAP which we handle
and walk stack using libvcc_s [2].

The test borrows symbol resolution from OpenBSD's btrace.
It uses libelf.

The main program (backtrace) arms signal handler and
performs sha256. It prints a backtrac before exits.
This is just unit test to verify SIGTRAP works as expected.

There is also libbacktrace.so which is meant to be preloaded
to any application. The library uses atexit(3) in constructor
to arm its exit handler which prints the collected stack.
The library constructor also stes up SIGTRAP to collect the stack
when application executes int3 instruction. The sigtrap
handler collects just one stack on the first execution of int3.

To use it you need to instrument target library at compile time.
For to investigate broken stack frames in sha256_block_data_order_avx2()
which is defined in crypto/sha/asm/sha512-x86_64.pl. We just add
a compile time breakpoint (int3) to sha256_block_data_order_avx2()
function by applying diff below:

----8<----
diff --git a/crypto/sha/asm/sha512-x86_64.pl b/crypto/sha/asm/sha512-x86_64.pl
index b37058ae03..c317c12a2d 100755
--- a/crypto/sha/asm/sha512-x86_64.pl
+++ b/crypto/sha/asm/sha512-x86_64.pl
@@ -592,6 +592,8 @@ $code.=<<___;
        movdqu          16($ctx),$CDGH          # HGFE
        movdqa          0x200-0x80($Tbl),$TMP   # byte swap mask

+       int3
+
        pshufd          \$0x1b,$ABEF,$Wi        # ABCD
        pshufd          \$0xb1,$ABEF,$ABEF      # CDAB
        pshufd          \$0x1b,$CDGH,$CDGH      # EFGH
----8<----

To hit the breakpoint added by patch above  at runtime we can use command
`LD_PRELOAD=./libbacktrace.so openssl speed -evp AES-128-CBC-HMAC-SHA256`.
This will produce output as follows

----8<----
sashan@ubuntu-24:~/work.openssl/ossl-scripts/backtrace.test$ LD_PRELOAD=./libbacktrace.so openssl speed -evp  AES-128-CBC-HMAC-SHA256
Doing AES-128-CBC-HMAC-SHA256 ops for 3s on 16 size blocks: 4573620 AES-128-CBC-HMAC-SHA256 ops in 0.37s
Doing AES-128-CBC-HMAC-SHA256 ops for 3s on 64 size blocks: 1175659 AES-128-CBC-HMAC-SHA256 ops in 0.32s
Doing AES-128-CBC-HMAC-SHA256 ops for 3s on 256 size blocks: 1105371 AES-128-CBC-HMAC-SHA256 ops in 0.42s
Doing AES-128-CBC-HMAC-SHA256 ops for 3s on 1024 size blocks: 920890 AES-128-CBC-HMAC-SHA256 ops in 0.83s
Doing AES-128-CBC-HMAC-SHA256 ops for 3s on 8192 size blocks: 361479 AES-128-CBC-HMAC-SHA256 ops in 2.17s
Doing AES-128-CBC-HMAC-SHA256 ops for 3s on 16384 size blocks: 213230 AES-128-CBC-HMAC-SHA256 ops in 2.45s
version: 3.5.0-dev
built on: Tue Mar  4 15:11:13 2025 UTC
options: bn(64,64)
compiler: gcc -fPIC -pthread -m64 -Wa,--noexecstack -Wall -O3 -DOPENSSL_USE_NODELETE -DL_ENDIAN -DOPENSSL_PIC -DOPENSSL_BUILDING_OPENSSL -DNDEBUG
CPUINFO: OPENSSL_ia32cap=0xfffa3203078bffff:0x00415f5ef1bf07ab:0x00000020bc000010:0x0000000000000000:0x0000000000000000
The 'numbers' are in 1000s of bytes per second processed.
type             16 bytes     64 bytes    256 bytes   1024 bytes   8192 bytes  16384 bytes
AES-128-CBC-HMAC-SHA256   197778.16k   235131.80k   673749.94k  1136134.17k  1364624.87k  1425942.99k
openssl: /lib/x86_64-linux-gnu/libc.so.6: .symtab: section not found
openssl: open: openssl: No such file or directory

sigtrap_hndl+0x3b
0x7c0525c45330
sha256_block_data_order_shaext+0x19
SHA256_Update+0xdf
sha256_update+0x5e
aesni_cbc_hmac_sha256_cipher+0x126
ossl_cipher_generic_stream_update+0x46
EVP_EncryptUpdate+0x83
0x5794e9fb0cd2
0x5794e9fb1af5
0x5794e9fba4b7
0x5794e9f8e511
0x5794e9f63358
0x7c0525c2a1ca
0x7c0525c2a28b
0x5794e9f634d5
----8<----

the `sigtrap-hndl()` in stack trace above is installed by libbacktrace.so. This is
where we collect the stack. The address (0x7c0525c45330) is where int3 got executed
it's `sha256_block_data_order_avx2()`. We are able to get frame but symbol resulution
failed.

[1] https://github.com/openssl/openssl/pull/10674

[2] https://refspecs.linuxfoundation.org/LSB_4.0.0/LSB-Core-S390/LSB-Core-S390/libgcc-sman.html
