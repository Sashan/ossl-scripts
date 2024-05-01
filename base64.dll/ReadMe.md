The code here tests [atexit](https://github.com/openssl/openssl/pull/241480)
changes on windows.
    - build .dll which is linked with statically linked openssl
    - create a test app, which loads the .dll explicitly (via. LoadLibrary())
    - the application calls function found in test dll. The function installs
      exit handler by calling `OPENSSL_atexit()

Solution contains two projects:
    - base64Dll
    - TestDll

= DllTest

The project for this test application is created following [Create a C++ console app](https://learn.microsoft.com/en-us/cpp/build/vscpp-step-1-create?view=msvc-170).

the only file woth to look at is ``DllTest.cpp` it loads the base64.dll
and installs functions provided by the .dll:
    - base64Encode()
    - base64Decode()
    - base64Init()

The most important function here is base64Init() which installs
`atexit` handler by calling `OPENSSL_atexit()`. The other two function
are doing some pseudo-important work to easily verify module works
as expected.

= base64.dll
To create the project I did follow [Walkthrough: Create and use...](https://learn.microsoft.com/en-us/cpp/build/walkthrough-creating-and-using-a-dynamic-link-library-cpp?view=msvc-170).
The functionality provided by .dll is implemented in `base64.cpp`. The file
provides functions as fllows:
    - `int base64Decode(const char *, size_t, char *, size_t)`
    - `int base64Encode(const char *, size_t, char *, size_t)`
    - `base64Init(void)`
`base64*code()` functions use `BIO` subsystem to provide base64
encoding/decoding functionality.

The most important code to perform desired test is `base64Init()` which
installs exit handler. The code reads as follows;
```
static void
fakeCleanup(void)
{
        printf("%s done\n", __func__);
}

void
base64Init(void)
{
        (void)OPENSSL_atexit(fakeCleanup);
}
```

= caveats (TODO)
I have not figured out how to define paths in projects relative to solution
root. So if you clone/check out repository the first thing to do is to
adjust those paths. Another option is to convert everything to cmake.



