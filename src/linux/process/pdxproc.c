#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "../common/mem.h"
#include "../../shared/dbg.h"


static int (*real_execve)(const char *pathname, char *const argv[],char *const envp[]) = NULL;


int x_execve(const char *pathname, char *const argv[],char *const envp[]){


char ** envp2;

char* str_to_add = getenv("PDXPL");

int index=0;
int preload_index = 0;
for(index=0; envp[index]; index++){
    if(!strncmp(envp[index],"LD_PRELOAD",strlen("LD_PRELOAD"))){        
        preload_index = index;
    }
}  

// At this point, index should be the null value.
// That means, we need to allocate index+1 (our new value and the new null)
// We also need to copy everything that's already there.
envp2 = calloc(1,(index+1)*sizeof(char*));
memcpy(envp2,envp,index*sizeof(char*));

// If we don't have an LD_PRELOAD already, the added entry will be the LD_PRELOAD.
if(!preload_index){
    DBG_printf("No Original Preload Detected\n");
    envp2[index] = malloc(strlen("LD_PRELOAD=") + strlen(str_to_add) + 1);
    sprintf(envp2[index],"LD_PRELOAD=%s",str_to_add);
    envp2[index+1] = NULL;

}else{
    // If we already have an LD_PRELOAD, there are a few extra steps.    
    char* after_orig_ldp = strstr(envp[preload_index],"LD_PRELOAD=") + strlen("LD_PRELOAD=");
    envp2[preload_index] = malloc(strlen("LD_PRELOAD=") + strlen(str_to_add)+ strlen(after_orig_ldp) + 2);    
    sprintf(envp2[preload_index],"LD_PRELOAD=%s:%s",str_to_add,after_orig_ldp);
    envp2[index] = calloc(1,strlen(after_orig_ldp)+8);
    sprintf(envp2[index],"PDXOPL=%s",after_orig_ldp);
    envp2[index+1] = NULL;
    DBG_printf("Original Preload Backed up\n");    
}


return real_execve(pathname,argv,(char * const*)envp2);
}

__attribute__((constructor)) void init(void) {
    DBG_printf("pdxproc init from: %d\n",getpid());

    if(getenv("PDXOPL")){
            DBG_printf("PDXOPL Detected: %s\n",getenv("PDXOPL"));
            setenv("LD_PRELOAD",getenv("PDXOPL"),1);
            
        }else{
            DBG_printf("Unsetting our Used LD_PRELOAD\n");
            unsetenv("LD_PRELOAD");
    }
    // Perform any Syscall Hooks we Need at This Level

    if (!inline_hook("libc.so.6", "execve", 0x0B, (void*)x_execve, (void**)&real_execve)) { return; }    

}