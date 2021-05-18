// Generic Debug Print Functionality

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "strutils.h"
//#define DEBUG_BUILD

#if _WIN32
#include <windows.h>
void DBG_printf(const char* fmt, ...) {
#ifdef DEBUG_BUILD
    char s[0x100] = { 0x00 };
    va_list args;
    va_start(args, fmt);
    vsnprintf(s, sizeof(s) - 1, fmt, args);
    va_end(args);
    s[sizeof(s) - 1] = 0x00;
    OutputDebugStringA(s);
#endif
}

void DBG_print_buffer(unsigned char* data, unsigned int len){
#ifdef DEBUG_BUILD    
    char s[0x1000] = {0x00};

    BinToHex(data,len,s,sizeof(s));
    OutputDebugStringA(s);
#endif
}
#else
void DBG_printf(const char* fmt, ...){
   #ifdef DEBUG_BUILD    
        char s[0x1000] = { 0x00 };
        va_list args;
        va_start(args, fmt);
        vsnprintf(s, sizeof(s) - 1, fmt, args);
        va_end(args);
        s[sizeof(s) - 1] = 0x00;
        printf("%s\n",s);
    #endif    
}
void DBG_print_buffer(unsigned char* data, unsigned int len){
#ifdef DEBUG_BUILD    
    char s[0x1000] = {0x00};
    char sp[4] = {0x00};
    for(int i=0;i<len;i++){
        printf("%02x",data[i]);
    }
    printf("\n");
#endif
}
#endif
