#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "fs_utils.h"

// Our mutually understood flag with the hooks will be comprised of a path prefixed with ";;;;"
const char* FLAG_BYPASS = ";;;;";

const char* bypass_path_check(const char* in_path){
    if(!strncmp(in_path,FLAG_BYPASS,strlen(FLAG_BYPASS))){
        return in_path + strlen(FLAG_BYPASS);
    }    
    return NULL;
}


int path_exists(char* path){
    char bypassed_in_path[X_MAX_PATH];
    sprintf(bypassed_in_path,"%s%s",FLAG_BYPASS,path);
    struct stat sb;
    return lstat(bypassed_in_path,&sb) == 0;
}

int path_is_symlink(char* path){
    char linkdata[X_MAX_PATH];
    strcpy(linkdata,FLAG_BYPASS);
    strcat(linkdata,path);    
    return (readlink(linkdata,linkdata,(size_t)X_MAX_PATH) > 0);
}
void resolve_abspath(char* in_path, char* out_path, int follow_symlinks){
    int is_symlink = path_is_symlink(in_path);
    char bypassed_in_path[X_MAX_PATH];
    sprintf(bypassed_in_path,"%s%s",FLAG_BYPASS,in_path);
    if(!is_symlink){
        realpath(bypassed_in_path,out_path);
        return;
    }
    
    if(follow_symlinks){
        readlink(bypassed_in_path,out_path,(size_t)X_MAX_PATH);
        return;
    }
    // For now, we'll just copy it verbatim if it's the literal link
    strcpy(out_path,in_path);
    return;   
}

void makedir(char* path){
    char bypassed_in_path[X_MAX_PATH];
    sprintf(bypassed_in_path,"%s%s",FLAG_BYPASS,path);
    mkdir(bypassed_in_path,0777);
}
void delete_path(char* path){
    char bypassed_in_path[X_MAX_PATH];
    sprintf(bypassed_in_path,"%s%s",FLAG_BYPASS,path);
    unlink(path);
}
void copy_file(char* src_path, char* dst_path){
    char bypassed_src_path[X_MAX_PATH];
    char bypassed_dst_path[X_MAX_PATH];    
    sprintf(bypassed_src_path,"%s%s",FLAG_BYPASS,src_path);
    sprintf(bypassed_dst_path,"%s%s",FLAG_BYPASS,dst_path);

    int f = open(bypassed_src_path,O_RDONLY);
    if(f < 0){return;}
    int g = open(bypassed_dst_path, O_WRONLY | O_CREAT | O_EXCL, 0666);
    if(g < 0){ close(f); return; }

    unsigned int chunk_size = (1024*1024);
    unsigned char* data_buffer = malloc(chunk_size);
    unsigned long long offset = 0;
    ssize_t nread;
    while (nread = read(f, data_buffer, chunk_size), nread > 0){
        write(g, data_buffer, nread);
    }    
    close(f);
    close(g);    
}

