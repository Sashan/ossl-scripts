Simple test tool to verify backtracing using libgcc_s works
for perlasm code.

It builds on Bernd's idea here [1]. Instead of relying
on SIGALARM this test requires placing int3 instruction to
desired position in code where backtrace should be started from.

The int3 instruction triggers SIGTRAP which we handle
and walk stack using libvcc_s [2].

The test borrows symbol resolution from OpenBSD's btrace.
It uses libelf.

[1] https://github.com/openssl/openssl/pull/10674

[2] https://refspecs.linuxfoundation.org/LSB_4.0.0/LSB-Core-S390/LSB-Core-S390/libgcc-sman.html
