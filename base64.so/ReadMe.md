This is a test to evaluate impact of [atexit() removal](https://github.com/openssl/openssl/pull/24148) on `OPENSSL_atexit()` function
(see [OPENSSL\_atexit(3)](https://www.openssl.org/docs/man3.1/man3/OPENSSL_atexit.html).
Current OpenSSL library (3.3) installs its own atexit(3) handler. The handler
calls `OPENSSL\_cleanup()` function upon process exit. The OpenSSL library also
provides function `OPENSSL\_atexit()` which allows application to install its
own atexit handlers. Handlers installed by `OPENSSL\_atexit()` function are
invoked by OPENSSL\_cleanup().

The [PR]((https://github.com/openssl/openssl/pull/24148) stops OpenSSL to install its own `atexit` handler
using libc's `atexit(3)`.  Instead of using `atexit(3)` provided by libc the OpenSSL uses runtime linker.
Runtime linker allows dynamically linked applications and libraries to provide constructors (legacy _init() function)
 and destructors (_fini()). See article [here](https://www.geeksforgeeks.org/__attribute__constructor-__attribute__destructor-syntaxes-c/) to find
out how constructors/destructors are handled these days.

Refer to Makefile to see how things are supposed to work. To build all programs
and libraries run `make all`. Use `TARGET_OSSL_LIBRARY_PATH` and
`TARGET_OSSL_INCLUDE_PATH` environment variables to point build process to
desired version of OpenSSL libraries.

To perform testing run `make test` command.

There are two tests programs built by Makefile:
    - `test.implicit` which relies on runtime linker to load desired
      OpenSSL library when application is executed
    - `test.explicit` uses `dlopen()` to load desired test library at
       runtime.

Both test programs use functionality provided by `base64.c` module. The
module provides three functions:
    - `base64Init()`
    - `base64Encode()`
    - `base64Decode()`

For test purposes the most important one is `base64Init()` which installs simple
atexit handler using `OPENSSL_atexit()` The code for exit handler reads as follows:
```
static void
fakeCleanup(void)
{
	printf("%s done\n", __func__);
}

```
Both test applications do call `base64Init()` to install exit handler. If things go
well, then each application must print 'fakeCleanup done' message on standard output.

The Makefile also produces two ``base64` shared libraries:
    - base64.so is dynamic library linked with dynamic libcrypto.so
    - base64-static.so is dynamic library linked with static variant of libcrypto

The test verifies if exit handler gets invoked by library destructor.
