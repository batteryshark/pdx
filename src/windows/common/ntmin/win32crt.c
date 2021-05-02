#include <stddef.h>
#include <stdarg.h>

#include "win32crt.h"

#ifndef memset
void* memset(void* dest, int c, size_t count){
    char* bytes = (char*)dest;
    while (count--){
        *bytes++ = (char)c;
    }
    return dest;
}
#endif

#ifndef memcpy
void *memcpy(void *restrict s1, const void *restrict s2, size_t n) {
  char *c1 = (char *)s1;
  const char *c2 = (const char *)s2;
  for (size_t i = 0; i < n; ++i)
    c1[i] = c2[i];
  return s1;
}
#endif

#ifndef strlen
size_t __cdecl strlen(const char* str){
        const char* s;
        if (str == 0) return 0;
        for (s = str; *s; ++s);
        return s - str;
}
#endif


#ifndef strncmp
int strncmp(const char* s1, const char* s2, size_t n){
    if (n == 0) return 0;

    do
    {
        if (*s1 != *s2++) return *s1 - *--s2;
        if (*s1++ == 0) break;
    } while (--n != 0);

    return 0;
}
#endif

#ifndef strstr
char* __cdecl strstr(const char* s, const char* find){
    char c, sc;
    size_t len;
    
    if ((c = *find++) != 0){
        len = strlen(find);
        do {
            do {
                if ((sc = *s++) == 0)
                    return 0;
            } while (sc != c);
        } while (strncmp(s, find, len) != 0);
        s--;
    }
    return (char*)((size_t)s);
}
#endif

#ifndef wcslen
size_t wcslen(const wchar_t *s) {
	const wchar_t *p;

	p = s;
	while (*p)
		p++;

	return p - s;
}
#endif


#ifndef wcscat
wchar_t * wcscat(wchar_t *s1, const wchar_t *s2) {
	wchar_t *p;
	wchar_t *q;
	const wchar_t *r;

	p = s1;
	while (*p)
		p++;
	q = p;
	r = s2;
	while (*r)
		*q++ = *r++;
	*q = '\0';
	return s1;
}
#endif

#ifndef wcsstr
wchar_t* wcsstr(const wchar_t *big, const wchar_t *little) {
	const wchar_t *p;
	const wchar_t *q;
	const wchar_t *r;

	if (!*little) {
		return (wchar_t *)big;
	}
	if (wcslen(big) < wcslen(little))
		return NULL;

	p = big;
	q = little;
	while (*p) {
		q = little;
		r = p;
		while (*q) {
			if (*r != *q)
				break;
			q++;
			r++;
		}
		if (!*q) {
			return (wchar_t *)p;
		}
		p++;
	}
	return NULL;
}
#endif

#ifndef wcscpy
wchar_t * wcscpy(wchar_t *s1, const wchar_t *s2)
{
	wchar_t *p;
	const wchar_t *q;

	p = s1;
	q = s2;
	while (*q)
		*p++ = *q++;
	*p = '\0';

	return s1;
}
#endif

#ifndef strcmp
int strcmp(const char *s1, const char *s2) {
	while (*s1 == *s2++)
		if (*s1++ == 0)
			return (0);
	return (*(unsigned char *)s1 - *(unsigned char *)--s2);
}
#endif

#ifndef strcpy
char *strcpy(char *dest, const char *src){
    size_t i;
    for (i = 0; src[i] != '\0'; i++)
        dest[i] = src[i];
    dest[i] = '\0';
    return dest;
}
#endif

#ifndef strcat
char * strcat(char *dest, const char *src){
  strcpy(dest + strlen(dest), src);
  return dest;
}
#endif


#ifndef strtok
int is_delim(char c, const char *__restrict delim)
{
  while(*delim != '\0')
  {
    if(c == *delim)
      return 1;
    delim++;
  }
  return 0;
}


char *strtok (char *__restrict s, const char *__restrict delim){
  static char *p; // start of the next search 
  if(!s)
  {
    s = p;
  }
  if(!s)
  {
    // user is bad user 
    return NULL;
  }

  // handle beginning of the string containing delims
  while(1)
  {
    if(is_delim(*s, delim))
    {
      s++;
      continue;
    }
    if(*s == '\0')
    {
      return NULL; // we've reached the end of the string
    }
    // now, we've hit a regular character. Let's exit the
    // loop, and we'd need to give the caller a string
    // that starts here.
    break;
  }

  char *ret = s;
  while(1){
    if(*s == '\0')
    {
      p = s; // next exec will return NULL
      return ret;
    }
    if(is_delim(*s, delim))
    {
      *s = '\0';
      p = s + 1;
      return ret;
    }
    s++;
  }
}
#endif

#ifndef memcmp
int memcmp (const void* str1, const void* str2, size_t count){
  register const unsigned char *s1 = (const unsigned char*)str1;
  register const unsigned char *s2 = (const unsigned char*)str2;

  while (count-- > 0)
    {
      if (*s1++ != *s2++)
	  return s1[-1] < s2[-1] ? -1 : 1;
    }
  return 0;
}
#endif


