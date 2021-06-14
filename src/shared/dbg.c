// Generic Debug Print Functionality

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include "strutils.h"

static int check_debug_mode = 0;
static int debug_mode_enabled = 0;
int debug_mode(){ 
    if(check_debug_mode){return debug_mode_enabled;}
    debug_mode_enabled = getenv("PDXDBG") != NULL;
    check_debug_mode = 1;
    return debug_mode_enabled;
}

#if _WIN32
#include <windows.h>
void DBG_printf(const char* fmt, ...) {
    if(!debug_mode()){return;}

    char s[0x100] = { 0x00 };
    va_list args;
    va_start(args, fmt);
    vsnprintf(s, sizeof(s) - 1, fmt, args);
    va_end(args);
    s[sizeof(s) - 1] = 0x00;
    OutputDebugStringA(s);

}

void DBG_print_buffer(unsigned char* data, unsigned int len){
    if(!debug_mode()){return;}    
   
    char s[0x1000] = {0x00};

    BinToHex(data,len,s,sizeof(s));
    OutputDebugStringA(s);

}
#else
void DBG_printf(const char* fmt, ...){
    if(!debug_mode()){return;}    

        char s[0x1000] = { 0x00 };
        va_list args;
        va_start(args, fmt);
        vsnprintf(s, sizeof(s) - 1, fmt, args);
        va_end(args);
        s[sizeof(s) - 1] = 0x00;
        printf("%s\n",s);
  
}
void DBG_print_buffer(unsigned char* data, unsigned int len){
    if(!debug_mode()){return;}    
 
    char s[0x1000] = {0x00};
    char sp[4] = {0x00};
    for(int i=0;i<len;i++){
        printf("%02x",data[i]);
    }
    printf("\n");

}
#endif
