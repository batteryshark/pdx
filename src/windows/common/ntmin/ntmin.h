#pragma once

#ifndef sprintf
#include "sprintf.h"
#endif


#include "ntdll.h"
#include "win32crt.h"

#ifndef malloc
void* malloc(size_t _size);
#endif

#ifndef calloc
void* calloc(size_t _nmemb, size_t _size);
#endif

#ifndef free
void free(void* _ptr);
#endif



void ChartoWideChar(char* in_str, wchar_t* dest_wc);
void WideChartoChar(wchar_t* wc, char* dest_str);
void UnicodeStringtoChar(PUNICODE_STRING uc, char* dest_str);
void ChartoUnicodeString(char* in_str,PUNICODE_STRING ou);
