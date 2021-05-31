#include "ntmin.h"

#define ROUND_DOWN(n, align) \
     (((ULONG)n) & ~((align) - 1l))

#define ROUND_UP(n, align) \
     ROUND_DOWN(((ULONG)n) + (align) - 1, (align))

/* round to 16 bytes + alloc at minimum 16 bytes */
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#define ROUND_SIZE(size) (max(16, ROUND_UP(size, 16)))

void UnicodeStringtoChar(PUNICODE_STRING uc, char* dest_str) {
    if(!dest_str || !uc){return;}
    ANSI_STRING as;
    RtlUnicodeStringToAnsiString(&as, uc, TRUE);
    RtlZeroMemory(dest_str, as.MaximumLength);
    RtlCopyMemory(dest_str, as.Buffer, as.Length);
    RtlFreeAnsiString(&as);
}

void ChartoUnicodeString(char* in_str,PUNICODE_STRING ou){
    if(!ou || !in_str){return;}
    ANSI_STRING as;
    RtlInitAnsiString(&as, in_str);
    RtlAnsiStringToUnicodeString(ou, &as, TRUE);
}

void WideChartoChar(wchar_t* wc, char* dest_str) {
    if(!dest_str || !wc){return;}
    ANSI_STRING as;
    UNICODE_STRING uc;
    RtlInitUnicodeString(&uc, wc);
    RtlUnicodeStringToAnsiString(&as, &uc, TRUE);
    RtlZeroMemory(dest_str, as.MaximumLength);
    RtlCopyMemory(dest_str, as.Buffer, as.Length);
    RtlFreeAnsiString(&as);
}

void ChartoWideChar(char* in_str, wchar_t* dest_wc) {
    if(!in_str || !dest_wc){return;}
    ANSI_STRING as;
    UNICODE_STRING uc;
    RtlInitAnsiString(&as, in_str);
    RtlAnsiStringToUnicodeString(&uc, &as, TRUE);
    RtlCopyMemory(dest_wc, uc.Buffer, uc.MaximumLength);
    RtlFreeUnicodeString(&uc);
}

HANDLE get_process_heap() {
    HANDLE pHeap = NULL;
    RtlGetProcessHeaps(1,&pHeap);
    return pHeap;
}

#ifndef malloc
void* malloc(size_t _size){
    size_t nSize = ROUND_SIZE(_size);

    if (nSize < _size)
        return NULL;

    return RtlAllocateHeap(get_process_heap(), 0, nSize);
}
#endif

#ifndef free
void free(void* _ptr){
    RtlFreeHeap(get_process_heap(), 0, _ptr);
}
#endif

#ifndef calloc
void* calloc(size_t _nmemb, size_t _size){
    size_t nSize = _nmemb * _size;
    size_t cSize = ROUND_SIZE(nSize);

    if ((_nmemb > ((size_t)-1 / _size)) || (cSize < nSize))
        return NULL;

    return RtlAllocateHeap(get_process_heap(), HEAP_ZERO_MEMORY, cSize);
}
#endif

#ifndef getenv
char* getenv(const char* name){
    // Set up Key -- Allocate
    UNICODE_STRING uk;
    ChartoUnicodeString((char*)name,&uk);
    // Set up Value -- Assign
    UNICODE_STRING uv;
    uv.Buffer = NULL;
    uv.Length = 0;
    uv.MaximumLength = 0;
    NTSTATUS res = RtlQueryEnvironmentVariable_U(0, &uk, &uv);
    if(res != STATUS_BUFFER_TOO_SMALL){return NULL;}
    // Allocate the amount of space we were told to.
    uv.MaximumLength = uv.Length + 2;
    uv.Buffer = calloc(1,uv.MaximumLength);
    res = RtlQueryEnvironmentVariable_U(0, &uk, &uv);
    RtlFreeUnicodeString(&uk);
    if(res){
        free(uv.Buffer);
        return NULL;
    }
    char* sval = calloc(1,uv.MaximumLength / 2);
    WideChartoChar(uv.Buffer, sval);
    free(uv.Buffer);
    return sval;
}
#endif