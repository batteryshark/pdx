#pragma once

#ifndef memset
extern void* memset(void* dest, int c, size_t count);
#endif

#ifndef memcpy
extern void *memcpy(void *restrict s1, const void *restrict s2, size_t n);
#endif

#ifndef memcmp
extern int memcmp(const void* str1, const void* str2, size_t count);
#endif

#ifndef strstr
extern char* __cdecl strstr(const char* s, const char* find);
#endif

#ifndef strcmp
extern int strcmp(const char *s1, const char *s2);
#endif

#ifndef strncmp
extern int strncmp(const char* s1, const char* s2, size_t n);
#endif

#ifndef strcpy
extern char *strcpy(char *dest, const char *src);
#endif

#ifndef strlen
extern size_t __cdecl strlen(const char* str);
#endif

#ifndef wcscat
extern wchar_t * wcscat(wchar_t *s1, const wchar_t *s2);
#endif

#ifndef wcsstr
extern wchar_t* wcsstr(const wchar_t *big, const wchar_t *little);
#endif

#ifndef wcslen
extern size_t wcslen(const wchar_t *s);
#endif

#ifndef wcscpy
extern wchar_t * wcscpy(wchar_t *s1, const wchar_t *s2);
#endif

#ifndef strcat
extern char * strcat(char *dest, const char *src);
#endif

#ifndef strtok
extern char *strtok (char *__restrict s, const char *__restrict delim);
#endif

