// Bootstrap Shim for Loading a list of libraries

#include "common/mem.h"

#ifdef TARGET_OS_WINDOWS
#include "common/ntmin/ntmin.h"
#include "common/env.h"
#else
#include <stdlib.h>
#include <string.h>
#endif
#ifdef TARGET_ARCH_64
#define BOOTSTRAP_CMD "VXBOOT64"
#else
#define BOOTSTRAP_CMD "VXBOOT32"
#endif


int bootstrap_init(void){       
    char payload[0x1000] = {0x00};
    if(!getenv(BOOTSTRAP_CMD)){return 0;}    
    strcpy(payload,getenv(BOOTSTRAP_CMD));    
    
    // Load Any Additional Modules
    char * token = strtok(payload, ";");
    if(!token){
        void* hLibrary = NULL;
        if(!load_library(payload,&hLibrary)){return 0;}
    }else{
        while( token != NULL ) {
            void* hLibrary = NULL;          
            if(!load_library(token,&hLibrary)){return 0;}
            token = strtok(NULL, ";");
        }         
    }
       
    return 1;
}

#ifdef TARGET_OS_WINDOWS
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        return bootstrap_init();
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
#else
void __attribute__((constructor)) initialize(void){bootstrap_init();}
#endif