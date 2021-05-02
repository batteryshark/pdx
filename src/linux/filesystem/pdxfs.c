// Filesystem Redirection Library
#include <sys/types.h>

#include <string.h>
#include <stdlib.h>
#include "../../shared/fs_redirect.h"
#include "fs_utils.h"

#include "../common/mem.h"

#ifndef O_RDONLY
#define O_RDONLY	     00
#endif
#ifndef O_WRONLY
#define O_WRONLY	     01
#endif

#ifndef O_RDWR
#define O_RDWR		     02
#endif

#ifndef O_CREAT
# define O_CREAT	   0100	/* Not fcntl.  */
#endif
#ifndef O_EXCL
# define O_EXCL		   0200	/* Not fcntl.  */
#endif

#ifndef O_DIRECTORY
# define O_DIRECTORY 0200000
#endif

// Helpers
int is_read(int flags){
    if(flags & O_RDWR){return 1;}
    if(flags & O_RDONLY){ return 1;}
    return 0;    
}

int is_write(int flags){
    if(flags & O_RDWR){return 1;}
    if(flags & O_WRONLY){ return 1;}
    return 0;
}

int is_directory(int flags){
    if(flags & O_DIRECTORY){return 1;}
    return 0;
}

int is_fail_if_exist(int flags){
    if((flags & O_CREAT) && (flags & O_EXCL)){return 1;}
    return 0;
}

int is_fail_if_not_exist(int flags){
    if(flags & O_CREAT){return 1;}
    return 0;   
}





// Various Hooks
static void *(*real_fopen)(const char *filename, const char *mode) = NULL;
static int (*real_open)(const char *path, int oflag) = NULL;
static int (*real_openat)(int dirfd, const char *pathname, int flags) = NULL;
static void* (*real_opendir)(const char *name) = NULL;
static int (*real__lxstat)(int ver, const char * path, void * stat_buf) = NULL;
static int (*real__xstat)(int ver, const char * path, void * stat_buf) = NULL;
static int (*real_access)(const char *pathname, int mode) = NULL;
static ssize_t (*real_readlink)(const char *restrict path, char *restrict buf, size_t bufsize) = NULL;
static int (*real_mkdir)(const char *path, int mode) = NULL;
static int (*real_unlink)(const char *pathname) = NULL;
static int (*real_unlinkat)(int dirfd, const char *pathname, int flags) = NULL;
static int (*real_rmdir)(const char *pathname) = NULL;
static char* (*real_realpath)(const char *restrict path, char *restrict resolved_path) = NULL;
static int (*real_rename)(const char *old, const char *new) = NULL;

void* fopen(const char *filename, const char *mode) {
    const char* bypass_path = bypass_path_check(filename);
    if(bypass_path){return real_fopen(bypass_path,mode);}    
    int is_read = (strstr(mode,"r") != 0);
    int is_write = (strstr(mode,"w") != 0);
    int fail_if_not_exist = (strstr(mode,"+") == 0);
    int fail_if_exist = 0;
    char* redirected_path = NULL;
    if(!fs_redirect((char*)filename,0,is_read,is_write,fail_if_exist,fail_if_not_exist,&redirected_path)){
        return real_fopen(filename,mode);
    }
    void* res = real_fopen(redirected_path,mode);
    free(redirected_path);
    return res;
}

int open(const char *path, int oflag){
    const char* bypass_path = bypass_path_check(path);
    if(bypass_path){return real_open(bypass_path,oflag);}
    char* redirected_path = NULL;
    if(!fs_redirect((char*)path,is_directory(oflag),is_read(oflag),is_write(oflag),is_fail_if_exist(oflag),is_fail_if_not_exist(oflag),&redirected_path)){
        return real_open(path,oflag);
    }
    int res = real_open(redirected_path,oflag);
    free(redirected_path);
    return res;
}

int openat(int dirfd, const char *pathname, int flags){
    const char* bypass_path = bypass_path_check(pathname);
    if(bypass_path){return real_openat(dirfd,bypass_path,flags);}
    char* redirected_path = NULL;
    if(!fs_redirect((char*)pathname,is_directory(flags),is_read(flags),is_write(flags),is_fail_if_exist(flags),is_fail_if_not_exist(flags),&redirected_path)){
        return real_openat(dirfd,pathname,flags);
    }
    int res = real_openat(dirfd,redirected_path,flags);
    free(redirected_path);
    return res;
}

void* opendir(const char *name){
    const char* bypass_path = bypass_path_check(name);
    if(bypass_path){return real_opendir(bypass_path);}
    char* redirected_path = NULL;
    if(!fs_redirect((char*)name,1,1,0,0,0,&redirected_path)){
        return real_opendir(name);
    }
    void* res = real_opendir(redirected_path);
    free(redirected_path);
    return res;  
}

int _lxstat(int ver, const char * path, void * stat_buf){
    const char* bypass_path = bypass_path_check(path);
    if(bypass_path){return real__lxstat(ver,bypass_path,stat_buf);}
    char* redirected_path = NULL;
    if(!fs_redirect((char*)path,0,1,0,0,0,&redirected_path)){
        return real__lxstat(ver,path,stat_buf);
    }
    int res = real__lxstat(ver,redirected_path,stat_buf);
    free(redirected_path);
    return res;    
}

int _xstat(int ver, const char * path, void * stat_buf){
    const char* bypass_path = bypass_path_check(path);
    if(bypass_path){return real__xstat(ver,bypass_path,stat_buf);}
    char* redirected_path = NULL;
    if(!fs_redirect((char*)path,0,1,0,0,0,&redirected_path)){
        return real__xstat(ver,path,stat_buf);
    }
    int res = real__xstat(ver,redirected_path,stat_buf);
    free(redirected_path);
    return res;    
}

int access(const char *pathname, int mode){
    const char* bypass_path = bypass_path_check(pathname);
    if(bypass_path){return real_access(bypass_path,mode);}
    char* redirected_path = NULL;
    if(!fs_redirect((char*)pathname,0,1,0,0,0,&redirected_path)){
        return real_access(pathname,mode);
    }
    int res = real_access(redirected_path,mode);
    free(redirected_path);
    return res;      
}

ssize_t readlink(const char *restrict path, char *restrict buf, size_t bufsize){
    const char* bypass_path = bypass_path_check(path);
    if(bypass_path){return real_readlink(bypass_path,buf,bufsize);}
    char* redirected_path = NULL;
    if(!fs_redirect((char*)path,0,1,0,0,0,&redirected_path)){
        return real_readlink(path,buf,bufsize);
    }
    ssize_t res = real_readlink(redirected_path,buf,bufsize);
    free(redirected_path);
    return res;      
}

int mkdir(const char *path, int mode){
    const char* bypass_path = bypass_path_check(path);
    if(bypass_path){return real_mkdir(bypass_path,mode);}
    char* redirected_path = NULL;
    if(!fs_redirect((char*)path,1,0,1,0,0,&redirected_path)){
        return real_mkdir(path,mode);
    }
    int res = real_mkdir(redirected_path,mode);
    free(redirected_path);
    return res;     
}

int unlink(const char *pathname){
    const char* bypass_path = bypass_path_check(pathname);
    if(bypass_path){return real_unlink(bypass_path);}
    char* redirected_path = NULL;
    if(!fs_redirect((char*)pathname,0,0,1,0,0,&redirected_path)){
        return real_unlink(pathname);
    }
    int res = real_unlink(redirected_path);
    free(redirected_path);
    return res;
}

int unlinkat(int dirfd, const char *pathname, int flags){
    const char* bypass_path = bypass_path_check(pathname);
    if(bypass_path){return real_unlinkat(dirfd,bypass_path,flags);}
    char* redirected_path = NULL;
    if(!fs_redirect((char*)pathname,0,0,1,0,0,&redirected_path)){
        return real_unlinkat(dirfd,pathname,flags);
    }
    int res = real_unlinkat(dirfd,redirected_path,flags);
    free(redirected_path);
    return res;
}

int rmdir(const char *pathname){
    const char* bypass_path = bypass_path_check(pathname);
    if(bypass_path){return real_rmdir(bypass_path);}
    char* redirected_path = NULL;
    if(!fs_redirect((char*)pathname,1,0,1,0,0,&redirected_path)){
        return real_rmdir(pathname);
    }
    int res = real_rmdir(redirected_path);
    free(redirected_path);
    return res;   
}

char *realpath(const char *restrict path, char *restrict resolved_path){
    const char* bypass_path = bypass_path_check(path);
    if(bypass_path){return real_realpath(bypass_path,resolved_path);}
    char* redirected_path = NULL;
    if(!fs_redirect((char*)path,0,1,0,0,0,&redirected_path)){
        return real_realpath(path,resolved_path);
    }
    char* res = real_realpath(redirected_path,resolved_path);
    free(redirected_path);
    return res;       
}

int rename(const char *old, const char *new){
    char* redirected_old = NULL;
    char* redirected_new = NULL;
    if(!fs_redirect((char*)old,0,1,0,0,0,&redirected_old) || !fs_redirect((char*)new,0,1,0,0,0,&redirected_new)){
        return real_rename(old,new);
    }

    int res = real_rename(redirected_old,redirected_new);
    free(redirected_old);
    free(redirected_new);    
    return res;           
}

void init_library(void){   
    get_function_address("libc.so.6","unlink",(void**)&real_unlink);
    get_function_address("libc.so.6","unlinkat",(void**)&real_unlinkat);    
    get_function_address("libc.so.6","rmdir",(void**)&real_rmdir);        
    get_function_address("libc.so.6","realpath",(void**)&real_realpath);       
    get_function_address("libc.so.6","open",(void**)&real_open);           
    get_function_address("libc.so.6","fopen",(void**)&real_fopen);               
    get_function_address("libc.so.6","openat",(void**)&real_openat);            
    get_function_address("libc.so.6","access",(void**)&real_access);     
    get_function_address("libc.so.6","readlink",(void**)&real_readlink);     
    get_function_address("libc.so.6","mkdir",(void**)&real_mkdir);       
    get_function_address("libc.so.6","opendir",(void**)&real_opendir);     
    get_function_address("libc.so.6","_lxstat",(void**)&real__lxstat);         
    get_function_address("libc.so.6","_xstat",(void**)&real__xstat);
    get_function_address("libc.so.6","rename",(void**)&real_rename);    
}

void init_library(void) __attribute__((constructor));