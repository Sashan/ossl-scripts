The idea is to preload libmemstats with any application dynamically
linked with openssl library. The openssl library must be built with
linker options:

    '-Wl,--wrap=CRYPTO_malloc -Wl,--wrap=CRYPTO_free'

the option above instructs the linker to create so called wrappers
for CRYPTOL_malloc and CRYPTO_free symbols in library.

The preloaded libmemstats supplies __wrap_OPENSSL_malloc
and __wrap_OPENSSL_free symbol so it can snoop on all
memory allocations to collect those stats.
