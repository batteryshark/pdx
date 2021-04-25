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
    PPEB pb = (PPEB)NtCurrentTeb()->ProcessEnvironmentBlock;
    return (HANDLE)pb->ProcessHeap;
}

void* malloc(size_t _size){
    size_t nSize = ROUND_SIZE(_size);

    if (nSize < _size)
        return NULL;

    return RtlAllocateHeap(get_process_heap(), 0, nSize);
}


void free(void* _ptr){
    RtlFreeHeap(get_process_heap(), 0, _ptr);
}

void* calloc(size_t _nmemb, size_t _size){
    size_t nSize = _nmemb * _size;
    size_t cSize = ROUND_SIZE(nSize);

    if ((_nmemb > ((size_t)-1 / _size)) || (cSize < nSize))
        return NULL;

    return RtlAllocateHeap(get_process_heap(), HEAP_ZERO_MEMORY, cSize);
}
