#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>


int main(int argc, char* argv[]){
    printf("Parent Process PID: %d\n",getpid());
    system(argv[1]);
return 0;
}