#pragma once
#include "ntdll.h"
#include "win32crt.h"
#include "printf.h"
void ChartoWideChar(char* in_str, wchar_t* dest_wc);
void WideChartoChar(wchar_t* wc, char* dest_str);
void UnicodeStringtoChar(PUNICODE_STRING uc, char* dest_str);
void ChartoUnicodeString(char* in_str,PUNICODE_STRING ou);
// Heap Related Stuff
void* malloc(size_t _size);
void* calloc(size_t _nmemb, size_t _size);
void free(void* _ptr);