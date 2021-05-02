#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
static int is_init=0;



__attribute__((constructor)) void init_library(){
if(!is_init){
printf("HOOKED LIBRARY 01 in pid: %d\n",getpid());
is_init=1;
}
}
