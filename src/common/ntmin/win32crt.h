#pragma once


extern void* memset(void* dest, int c, size_t count);
extern void *memcpy(void *restrict s1, const void *restrict s2, size_t n);

extern int memcmp(const void* str1, const void* str2, size_t count);


extern char* __cdecl strstr(const char* s, const char* find);
extern int strcmp(const char *s1, const char *s2);
extern int strncmp(const char* s1, const char* s2, size_t n);
#ifndef strcpy
extern char *strcpy(char *dest, const char *src);
#endif
#ifndef strlen
extern size_t __cdecl strlen(const char* str);
#endif

extern int lower(int argument);

extern wchar_t * wcscat(wchar_t *s1, const wchar_t *s2);
extern wchar_t* wcsstr(const wchar_t *big, const wchar_t *little);
extern size_t wcslen(const wchar_t *s);
extern wchar_t * wcscpy(wchar_t *s1, const wchar_t *s2);
extern char * strcat(char *dest, const char *src);
#ifdef strtok
extern char *strtok(char *s, char *delim);
#endif
