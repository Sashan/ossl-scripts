This is a test to evaluate changes in [atexit() removal](https://github.com/openssl/openssl/pull/24148) in `OpenSSL` library. The pull request moves call to `OPENSSL_cleanup()` from _atexit_ handler (set via libc `atexit(3)`) to library destructor. The code here helps us to better understand and evaluate impact of the change on dynamically inked applications. In typical scenario runtime linker uses information from`DT_NEEDED` field found in elf header to see which libraries to use to construct the process. Desired libraries are then pulled in from standard locations (such as `/lib` for example). The `DT_NEEDED` field is set at build time. That kind of linking process is referred as [implicit](https://www.equestionanswers.com/dll/what-is-implicit-and-explicit-linking-in-dynamic-loading.php) linking. Another option how dynamic application can pull-in desired shared library is to use `dlopen()`/`LoadLibrary()` functions. The `dlopen()` way is referred as explicit linking. The tests here cover both scenarios. To perform the tests the Makefile builds those objects:
    - `test.implicit`, the application pulls `libcrypto` in via `DT_NEEDED` (`-lcrypto` in Makefile)
    - `test.exmplicit`, the application explicitly loads `base64.so`/`base64-static.so` explicitly (using `dlopen()`) the libcrypto then is pulled in as a `base64.so` dependency.
    - `base64.so` is simple shared library which pulls libcrypto in via `DT_NEEDED`
    - `base64-static.so` is statically linked with libcrypto 
To exercise code in `libcrypto` the `base64` module provides function as follows:
    - `base64Encode()`
    - `base64Decode()`
    - `base64ArmAtExit()`
The `base64ArmAtExit()` function calls `OPENSSL_atexit()` to set up `base64AtExit()` function as _atexit_ handler.

Makefile also provides `test` target to run tests. The `test` target depends on inddividual tests:
    - `test-atexit-implicit`
    - `test-atexit-explicit`
    - `test-atexit-explicit-static`
    - `test-cleanup-implicit`
    - `test-cleanup-explicit`
    - `test-cleanup-explicit-static`
The `test-atexit-*` targets verify the _atexit_ handler fires on test application exit. The `test-clenaup-*` targets verify `OPENSSL_cleanup()` function gets called at application exit. The `test-clenaup` tests require customized build of librcypto library. The patch here must be applied to openssl source code:
```
--- a/crypto/init.c
+++ b/crypto/init.c
@@ -34,6 +34,10 @@
 #include <openssl/trace.h>
 #include "crypto/ctype.h"
 
+#ifdef _TEST_CLEANUP
+#include <stdio.h>
+#endif
+
 static int stopped = 0;
 static uint64_t optsdone = 0;
 
@@ -318,6 +322,9 @@ void OPENSSL_cleanup(void)
 {
     OPENSSL_INIT_STOP *currhandler, *lasthandler;
 
+#ifdef _TEST_CLEANUP
+    printf("%s\n", __func__);
+#endif
     /*
      * At some point we should consider looking at this function with a view to
      * moving most/all of this into onfree handlers in OSSL_LIB_CTX.
```
Once diff is applied the library must be rebuilt with `-D_TEST_CLEANUP` option (use add `-D_TEST_CLEANUP` option to your `./Configure` command). To run/build test you must point `Makefile` to place where your custom openssl library for testing got installed. Set `TARGET_OSSL_LIBRARY_PATH` and `TARGET_OSSL_INCLUDE_PATH` environement variables accordingly.
