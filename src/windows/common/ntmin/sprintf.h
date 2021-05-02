#pragma once

#ifdef sprintf
int _sprintf(char* buffer, const char* format, ...);
#else
int sprintf(char* buffer, const char* format, ...);
#endif



