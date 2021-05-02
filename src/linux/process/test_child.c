#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#if __x86_64__
#define MSG "Child Process 64"
#else
#define MSG "Child Process 32"
#endif


int main(){
    printf("Hi From the %s : %d !\n",MSG,getpid());
    char* ldpr = getenv("LD_PRELOAD");
    if(!ldpr){
        printf("No LD_PRELOAD Set\n");
        
    }else{
    printf("LD_PRELOAD = %s\n", ldpr);
    }
    return 0;
    
}