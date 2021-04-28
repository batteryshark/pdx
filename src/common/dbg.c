#include <Windows.h>
#include <stdio.h>

#define DEBUG_MODE


void DBG_printf(const char* fmt, ...) {
#ifdef DEBUG_MODE
    
    char s[4096] = { 0x00 };
    va_list args;
    va_start(args, fmt);
    vsnprintf(s, sizeof(s) - 1, fmt, args);
    va_end(args);
    s[sizeof(s) - 1] = 0x00;
    OutputDebugStringA(s);
    #endif
}


void DBG_print_buffer(unsigned char* data, unsigned int len){
#ifdef DEBUG_MODE    
    char s[0x1000] = {0x00};
    for(int i=0;i<len;i++){
        sprintf(s,"02x",data[i]);
    }
    OutputDebugStringA(s);
    #endif
}

