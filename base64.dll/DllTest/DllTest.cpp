#include "pch.h"
#include <stdio.h>
#include "windows.h"

#define BASE64_DLL "C:\\Users\\anedv\\openssl\\samples\\base64.dll\\x64\\Debug\\base64.dll"

typedef int(*Base64FuncT)(const char*, size_t, char*, size_t);
typedef void(*Base64InitFuncT)(void);

int
main(int argc, const char *argv[])
{
    HINSTANCE   hBase64DLL;
    Base64FuncT base64Encode, base64Decode;
    Base64InitFuncT base64Init;
    
    char    base64Buf[80];
    char    plainBuf[40];
    int     len;

    printf("Hello World\n");

    memset(base64Buf, 0, sizeof (*base64Buf));
    memset(plainBuf, 0, sizeof (*plainBuf));
    hBase64DLL = LoadLibrary(TEXT(BASE64_DLL));
    if (hBase64DLL == NULL) {
        fprintf(stderr, "Unable to load library %s\n", BASE64_DLL);
        return (1);
    }

    base64Encode = (Base64FuncT)GetProcAddress(hBase64DLL, "base64Encode");
    base64Decode = (Base64FuncT)GetProcAddress(hBase64DLL, "base64Decode");
    base64Init = (Base64InitFuncT)GetProcAddress(hBase64DLL, "base64Init");
    if ((base64Encode == NULL) || (base64Decode == NULL) || base64Init == NULL) {
        FreeLibrary(hBase64DLL);
        fprintf(stderr, "Unable to obtain functions\n");
        return (1);
    }

    /* this caslls openssl_atexit() the installed handler prints to stdout.*/
    base64Init();

    len = base64Encode("Hello World!", sizeof ("Hello World!") - 1, base64Buf, 80);
    if (len > 0) {
        (void)base64Decode(base64Buf, len, plainBuf, 40);
        printf("%s\n", plainBuf);
    } else {
        printf("base64Encode has failed\n");
    }

    FreeLibrary(hBase64DLL);
    return (0);
}
