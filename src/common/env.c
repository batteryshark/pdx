#ifdef TARGET_OS_WINDOWS
    #include "ntmin/ntmin.h"
    char* getenv(const char* name){
        // Set up Key -- Allocate
        UNICODE_STRING uk;
        ChartoUnicodeString((char*)name,&uk);
        // Set up Value -- Assign
        UNICODE_STRING uv;
        uv.Buffer = NULL;
        uv.Length = 0;
        uv.MaximumLength = 0;
        NTSTATUS res = RtlQueryEnvironmentVariable_U(0, &uk, &uv);
        if(res != STATUS_BUFFER_TOO_SMALL){return NULL;}
        // Allocate the amount of space we were told to.
        uv.MaximumLength = uv.Length + 2;
        uv.Buffer = calloc(1,uv.MaximumLength);
        res = RtlQueryEnvironmentVariable_U(0, &uk, &uv);
        RtlFreeUnicodeString(&uk);
        if(res){
            free(uv.Buffer);
            return NULL;
        }
        char* sval = calloc(1,uv.MaximumLength / 2);
        WideChartoChar(uv.Buffer, sval);
        free(uv.Buffer);
        return sval;
    }
#else
#include <stdlib.h>
#include <string.h>
#endif

int get_envar(char* key, char* value, unsigned int value_len){
    if(getenv(key)){
        strcpy(value,getenv(key));
        return 1;
    }
}

int get_socket_address(char* socket_name, char* socket_path){
    if(!getenv("VXTMP")){return 0;}

    strcpy(socket_path,getenv("VXTMP"));
    strcat(socket_path,"/");
    strcat(socket_path,socket_name);
    strcat(socket_path,".sock");
    return 1;
}

int get_logging_enabled(void){
    if(!getenv("VXLOG")){return 0;}
    return 1;
}