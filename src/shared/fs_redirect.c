// Filesystem Redirection Logic

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#if defined _WIN32
#include "../windows/filesystem/fs_utils.h"
#define OS_SEP '\\'
#define OS_SSEP "\\"
#define FAKE_HOME "c\\users\\user"
#define NT_PREFIX_W L"\\??\\"
#define NT_PREFIX "\\??\\"
#else
#include "../linux/filesystem/fs_utils.h"
#define OS_SEP '/'
#define OS_SSEP "/"
#define FAKE_HOME "home/user"
#endif

#include "dbg.h"
#include "strutils.h"
#include "fs_redirect.h"

typedef struct _IGNORE_ENTRY{
    char* path;
    void* next;
}FS_IGNORE_ENTRY, *PFS_IGNORE_ENTRY;

static PFS_IGNORE_ENTRY first_ignore_entry = {0x00};

// Replaces duplicate separators
void fix_separators(char* in_path){
    // Fix POSIX Path Fuckups
    #if defined _WIN32
    for(int i=0;i<strlen(in_path);i++){
        if(in_path[i] == '/'){
            in_path[i] = OS_SEP;
        }
    }
    #endif
    for(int i=0;i<strlen(in_path)-1;i++){
        if(in_path[i] == OS_SEP && in_path[i+1] == OS_SEP){
            if(i < strlen(in_path)-1){
                strcpy(in_path+i,in_path+(i+1));
            }else{
                in_path[i+1] = 0x00;
            }
            
        }
    }
}

// Should our path be skipped?
int path_ignored(char* in_path){
    char orig_path[1024] = {0x00};
    strcpy(orig_path,in_path);
    #if defined _WIN32
    to_lowercase(orig_path);
    #endif
    fix_separators(orig_path);

    if(!first_ignore_entry){return 0;}
    PFS_IGNORE_ENTRY centry = first_ignore_entry;
    while(centry){
        
        #if defined _WIN32
            int result = strnicmp(orig_path,centry->path,strlen(centry->path));
        #else
            int result = strncmp(orig_path,centry->path,strlen(centry->path));
        #endif
        if(!result){/*DBG_printf("Path Ignored:%s!",orig_path);*/return 1;}
        centry = centry->next;
    }
    return 0;
}

static char fs_root[1024] = {0x00};
static char fs_home[1024] = {0x00};
static int fs_write_isolate = 1;
static int fs_read_isolate = 0;
static int fs_init = 0;


void add_path_to_ignore_list(char* in_path){
    PFS_IGNORE_ENTRY n_entry = calloc(1,sizeof(FS_IGNORE_ENTRY));

    #if defined _WIN32
    n_entry->path = malloc(strlen(in_path)+5);
    strcpy(n_entry->path,NT_PREFIX);
    strcat(n_entry->path,in_path);
    to_lowercase(n_entry->path);
    #else
    n_entry->path = malloc(strlen(in_path)+1);
    strcpy(n_entry->path,in_path);
    #endif

    // If it's our first, we'll just add it
    if(!first_ignore_entry){
        first_ignore_entry = n_entry;
        return;
    }    
    // Otherwise, we gotta play the tape out
    PFS_IGNORE_ENTRY centry = first_ignore_entry;
    if(centry){
        while(centry->next){centry = centry->next;}
    }
    centry->next = n_entry;
}

void init_fs_ignore_list(){
    if(!getenv("PDXFS_IGNORE")){return;}
    const char* envbl = getenv("PDXFS_IGNORE");
    char* envbl_tmp = calloc(1,strlen(envbl)+1);
    strcpy(envbl_tmp,envbl);

    char * token = strtok(envbl_tmp, ";");
    if(!token){
        add_path_to_ignore_list(token);        
    }else{
        while( token != NULL ) {
            add_path_to_ignore_list(token);
            token = strtok(NULL, ";");
        }         
    }

    free(envbl_tmp);
}

void init_fs_root(){
    const char* env_fs_root = getenv("PDXFS_ROOT");
    if(!env_fs_root){return;}
    #if defined _WIN32
    strcpy(fs_root,NT_PREFIX);
    strcat(fs_root,env_fs_root);
    to_lowercase(fs_root);
    #else
    strcpy(fs_root,env_fs_root);
    #endif
    
}


void init_fs_home(){
    #if defined _WIN32
    const char* env_fs_home = getenv("USERPROFILE");
    if(!env_fs_home){return;}    
    strcpy(fs_home,NT_PREFIX);
    strcat(fs_home,env_fs_home);
    to_lowercase(fs_home);
    #else
    const char* env_fs_home = getenv("HOME");
    if(!env_fs_home){return;}        
    strcpy(fs_home,env_fs_home);
    #endif
}

void init_fs_mode(){
    const char* env_fs_mode = getenv("PDXFS_MODE");
    if(!env_fs_mode){return;}
    int fs_mode = atoi(env_fs_mode);
    switch(fs_mode){
        case 1:
            fs_read_isolate = 0;
            fs_write_isolate = 1;
            break;
        case 2:
            fs_read_isolate = 0;
            fs_write_isolate = 0;
            break;
        case 3:
            fs_read_isolate = 1;
            fs_write_isolate = 0;
            break;
        case 4:
            fs_read_isolate = 1;
            fs_write_isolate = 1;
            break;            
        default:
            break;
    }    
}

void init_fs(){
    init_fs_ignore_list();
    init_fs_root();
    init_fs_home();
    fs_init=1;
}

typedef struct _PDXPATH{
    char* redirected_path;
    char original_path[1024];
    char original_parent[1024];
    char redirected_parent[1024];
    char resolved_original_path[1024];
    int resolved_original_path_exists;
    int original_path_exists;
    int original_path_is_symlink;
    int original_parent_exists;
    int redirected_path_exists;
    int redirected_path_is_symlink;
    int redirected_parent_exists;   
    int is_home_subpath; 
    int is_redirected_subpath;
    int is_wildcard_path;
    int create_parent_path;
    int nt_path;
}PDXPATH,*PPDXPATH;

void get_parent_path(char* in_path, char* out_parent){
    int i;
    for(i=strlen(in_path);i>0;i--){
        if(in_path[i] == OS_SEP){
            break;
        }
    }
    if(!i){return;}
    strncpy(out_parent,in_path,i+1);
}



void create_parent_path(char* parent_path){
    char working_path[1024] = {0x00};
    strcpy(working_path,parent_path);
    char* cptr = working_path;
    #if defined _WIN32    
    if(!strncmp(working_path,NT_PREFIX,strlen(NT_PREFIX))){
        cptr = working_path + strlen(NT_PREFIX);
    }
    #endif
    
    char* parent_path_end = working_path + strlen(working_path);
    for(;cptr < parent_path_end;cptr++){
        if(cptr[0] == OS_SEP){
            cptr[0] = 0x00;
            if(!path_exists(working_path)){
                makedir(working_path);
            }
            cptr[0] = OS_SEP;
        }
    }
}

void generate_path_info(PPDXPATH sp){
    sp->nt_path = 0;
    #if defined _WIN32
    to_lowercase(sp->original_path);
    #endif
    fix_separators(sp->original_path);   
    if(strstr(sp->original_path,NT_PREFIX)){
        sp->nt_path = 1;
    }
    char working_path[1024];
    sp->is_home_subpath = 0;
    sp->original_path_exists = path_exists(sp->original_path);

    get_parent_path(sp->original_path,sp->original_parent);
    sp->original_parent_exists = path_exists(sp->original_parent);

    sp->is_wildcard_path =  strstr(sp->original_path,"*") != 0;
    sp->is_redirected_subpath = (!strncmp(fs_root,sp->original_path,strlen(fs_root)));
    // If this is a home path, we have to make adjustments for the mapped path.
    sp->redirected_path = calloc(1,1024);
    if(sp->is_redirected_subpath){
        strcpy(sp->redirected_path,sp->original_path);
    }else{
        strcpy(sp->redirected_path,fs_root);
        strcat(sp->redirected_path,OS_SSEP);
        if(!strncmp(fs_home,sp->original_path,strlen(fs_home))){
            sp->is_home_subpath = 1;
            strcat(sp->redirected_path,FAKE_HOME);
            strcat(sp->redirected_path,sp->original_path + strlen(fs_home));
        }else{
            // We have to remove any ADS crap and drive colon
            // We're also skipping the NT Prefix        
            #if defined _WIN32
            if(strstr(sp->original_path,NT_PREFIX)){
                strcpy(working_path,sp->original_path+strlen(NT_PREFIX));
            }else{
                strcpy(working_path,sp->original_path);
            }        
            for(int i=0;i<strlen(working_path);i++){
                if(working_path[i] == ':'){
                    strcpy(working_path+i,working_path+(i+1));
                    break;
                }
            }

            // Deal with Alternate data streams -_-
            int is_ads = 0;
            for(int i=0;i<strlen(working_path);i++){
                if(working_path[i] == ':'){
                    working_path[i] = '_';
                    is_ads = 1;
                }
            }
            strcat(sp->redirected_path,working_path);
            if(is_ads){
                strcat(sp->redirected_path,".ads");
            }
            #else
            strcat(sp->redirected_path,sp->original_path);        
            #endif
        }
    }
 
    sp->redirected_path_exists = path_exists(sp->redirected_path);
    sp->redirected_path_is_symlink = path_is_symlink(sp->redirected_path);
    get_parent_path(sp->redirected_path,sp->redirected_parent);
    sp->redirected_parent_exists = path_exists(sp->redirected_parent);
    sp->original_path_is_symlink = path_is_symlink(sp->original_path);
    sp->resolved_original_path_exists = 0;

    if(sp->is_wildcard_path){
        sp->redirected_path_is_symlink = 0;
        sp->original_path_is_symlink = 0;
        sp->original_path_exists = sp->original_parent_exists;
        sp->redirected_path_exists = sp->redirected_parent_exists;
    }else{
        if(sp->redirected_path_exists){
            resolve_abspath(sp->redirected_path,sp->resolved_original_path,1);
            sp->resolved_original_path_exists = path_exists(sp->resolved_original_path);
        }else if(sp->original_path_exists){
            resolve_abspath(sp->original_path,sp->resolved_original_path,1);
            sp->resolved_original_path_exists = path_exists(sp->resolved_original_path);
        }        
    }

    sp->create_parent_path = (sp->original_parent_exists && ! sp->redirected_parent_exists);
}
static int info_lock = 0;
void print_sp_info(PPDXPATH sp, int is_directory, int is_read, int is_write, int fail_if_exist, int fail_if_not_exist){
    while(info_lock){}
    info_lock = 1;
    DBG_printf("--------\n");
    DBG_printf("Original Path: %s\n",sp->original_path);
    DBG_printf("Read: %d Write: %d DIR: %d FIE: %d FINE: %d",is_read, is_write, is_directory, fail_if_exist, fail_if_not_exist);    
    DBG_printf("Original Path Exists: %d\n",sp->original_path_exists);
    DBG_printf("Original Path is Symlink: %d\n",sp->original_path_is_symlink);
    DBG_printf("Original Parent: %s\n",sp->original_parent);
    DBG_printf("Original Parent Exists: %d\n",sp->original_parent_exists);
    DBG_printf("Wildcard Path: %d\n",sp->is_wildcard_path);
    DBG_printf("Is a Redirected Subpath: %d\n",sp->is_redirected_subpath);
    DBG_printf("Is Home Subpath: %d\n",sp->is_home_subpath);
    DBG_printf("Redirected Path: %s\n",sp->redirected_path);
    DBG_printf("Redirected Path Exists: %d\n",sp->redirected_path_exists);
    DBG_printf("Redirected Path is Symlink: %d\n",sp->redirected_path_is_symlink);
    DBG_printf("Redirected Parent: %s\n",sp->redirected_parent);
    DBG_printf("Redirected Parent Exists: %d\n",sp->redirected_parent_exists);  
    DBG_printf("Resolved Original Path: %s\n",sp->resolved_original_path);
    DBG_printf("Resolved Original Path Exists: %d\n",sp->resolved_original_path_exists);
    DBG_printf("Should Create Parent Path: %d\n",sp->create_parent_path);
    DBG_printf("--------\n");
    info_lock = 0;
}



int fs_redirect(char* in_abspath, int is_directory, int is_read, int is_write, int fail_if_exist, int fail_if_not_exist, char** redirected_path){
    if(!fs_init){
        init_fs();
        DBG_printf("Redirected Root: %s\n",fs_root);
        DBG_printf("Real Home: %s\n",fs_home);
    }
    // If we don't have a redirected root, we can't do anything.
    if(!fs_root){return 0;}
    // Check ignore list and return at this point.    
    if(path_ignored(in_abspath)){return 0;}    
    
    PPDXPATH sp = calloc(1,sizeof(PDXPATH));
    strcpy(sp->original_path,in_abspath);
    generate_path_info(sp);
    *redirected_path = sp->redirected_path;
    print_sp_info(sp, is_directory, is_read, is_write, fail_if_exist, fail_if_not_exist);
    // 'Fix' for access case
    if(!is_read && !is_write){is_read = 1;}
    
    // Read Only Handling First - I'm going to long-hand this logic because it will make it easier to follow...
    if(is_read && !is_write){

        // If we're using read isolation, always redirect to our paths.
        if(fs_read_isolate){
            free(sp);
            if(!sp->nt_path){strcpy(*redirected_path, *redirected_path+strlen(NT_PREFIX));}
            return 1;
        }

        // If our read path is a redirected subpath already, we don't need to redirect.
        if(sp->is_redirected_subpath){
            free(sp);
            free(*redirected_path);
            return 0;            
        }

        // If the redirected path does not exist, but the original path does, bypass (fallback to original).
        if(!sp->redirected_path_exists && sp->original_path_exists){
            free(sp);
            free(*redirected_path);
            return 0;
        }
        // Otherwise, just redirect.
        free(sp);
        if(!sp->nt_path){strcpy(*redirected_path, *redirected_path+strlen(NT_PREFIX));}
        return 1;
    }

    // From this Point Onward - We're in WriteLand!

    // If this is Windows and the path isn't a filesystem path (e.g. no :), we'll skip it for now...
    #if defined _WIN32
    if(!strstr(in_abspath,":")){
            free(sp);
            free(*redirected_path);
            return 0;
    }
    #endif

    // If the real write path already exists, we're going to just bypass and write to the real path
    // We may end up reverting this later or making it a configuration option
    if(!fs_write_isolate){
        if(sp->original_path_exists && strcmp(sp->original_path,sp->redirected_path)){
            free(sp);
            free(*redirected_path);
            return 0;        
        }
    }


    // Because we're going to write a file, its parent needs to exist...
    if(sp->create_parent_path){
        create_parent_path(sp->redirected_parent);
    }

    // If it's write-only (i.e. not read) or readwrite, we need to delete the symlink if it exists
    // Also, at this point, we're done if this isn't readwrite
    if(!is_read){
        if(sp->redirected_path_is_symlink){
            delete_path(sp->redirected_path);
        }
        free(sp);
        if(!sp->nt_path){strcpy(*redirected_path, *redirected_path+strlen(NT_PREFIX));}
        return 1;        
    }

    // From this point onward, we're in READWRITE LAND!!!
    if(strlen(sp->resolved_original_path) && sp->resolved_original_path_exists && !is_directory){
        if(sp->redirected_path_is_symlink){
            delete_path(sp->redirected_path);
        }
        if(strcmp(sp->resolved_original_path,sp->redirected_path)){
            // Copy Original File
            //DBG_printf("Copy File: %s => %s\n",sp->resolved_original_path,sp->redirected_path);
            copy_file(sp->resolved_original_path,sp->redirected_path);
        }
    }

    free(sp);
    if(!sp->nt_path){strcpy(*redirected_path, *redirected_path+strlen(NT_PREFIX));}
    return 1;
}
