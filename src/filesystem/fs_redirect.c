// Filesystem Redirection Logic
#include "fs_redirect.h"

#include "fs_utils_win.h"
#include "../common/strutils.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#ifdef TARGET_OS_WINDOWS
#define OS_SEP '\\'
#define OS_SSEP "\\"
#define FAKE_HOME "c\\users\\user"
#else
#define OS_SEP '/'
#define OS_SSEP "/"
#define FAKE_HOME "home/user"
#endif

typedef struct _IGNORE_ENTRY{
    char* path;
    void* next;
}FS_IGNORE_ENTRY, *PFS_IGNORE_ENTRY;

static PFS_IGNORE_ENTRY first_ignore_entry = {0x00};

// Should our path be skipped?
int path_ignored(char* in_path){
    if(!first_ignore_entry){return 0;}
    PFS_IGNORE_ENTRY centry = first_ignore_entry;
    while(centry){
        if(!strncmp(in_path,centry->path,strlen(centry->path))){
            return 1;
        }
        centry = centry->next;
    }
    return 0;
}

static char fs_root[1024] = {0x00};
static char fs_home[1024] = {0x00};
static int fs_init = 0;


void add_path_to_ignore_list(char* in_path){
    PFS_IGNORE_ENTRY n_entry = calloc(1,sizeof(FS_IGNORE_ENTRY));

    #ifdef TARGET_OS_WINDOWS
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
    if(!getenv("PIEE_FS_IGNORE")){return;}
    const char* envbl = getenv("PIEE_FS_IGNORE");
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
    const char* env_fs_root = getenv("PIEE_FS_ROOT");
    if(!env_fs_root){return;}
    #ifdef TARGET_OS_WINDOWS
    strcpy(fs_root,NT_PREFIX);
    strcat(fs_root,env_fs_root);
    to_lowercase(fs_root);
    #else
    strcpy(fs_root,env_fs_root);
    #endif
}


void init_fs_home(){
    #ifdef TARGET_OS_WINDOWS
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

void init_fs(){
    init_fs_root();
    init_fs_home();
}

typedef struct _PIEE_PATH{
    char* redirected_path;
    char original_path[1024];
    char original_parent[1024];
    char redirected_parent[1024];
    char resolved_original_path[1024];
    int original_path_exists;
    int original_path_is_symlink;
    int original_parent_exists;
    int redirected_path_exists;
    int redirected_path_is_symlink;
    int redirected_parent_exists;    
    int is_redirected_subpath;
    int is_wildcard_path;
    int create_parent_path;
}PIEE_PATH,*PPIEE_PATH;

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

// Replaces duplicate separators
void fix_separators(char* in_path){
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

void create_parent_path(char* parent_path){
    char working_path[1024] = {0x00};
    strcpy(working_path,parent_path);
    char* cptr = working_path;
    if(!strncmp(working_path,NT_PREFIX,strlen(NT_PREFIX))){
        cptr = working_path + strlen(NT_PREFIX);
    }
    
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

void generate_path_info(PPIEE_PATH sp){
    #ifdef TARGET_OS_WINDOWS
    to_lowercase(sp->original_path);
    #endif
    fix_separators(sp->original_path);    
    char working_path[1024];
    sp->original_path_exists = path_exists(sp->original_path);

    get_parent_path(sp->original_path,sp->original_parent);
    sp->original_parent_exists = path_exists(sp->original_parent);

    sp->is_wildcard_path =  strstr(sp->original_path,"*") != 0;
    sp->is_redirected_subpath = (!strncmp(fs_root,sp->original_path,strlen(fs_root)));
    // If this is a home path, we have to make adjustments for the mapped path.
    sp->redirected_path = calloc(1,1024);
    strcpy(sp->redirected_path,fs_root);
    strcat(sp->redirected_path,OS_SSEP);
    if(!strncmp(fs_home,sp->original_path,strlen(fs_home))){
        strcat(sp->redirected_path,FAKE_HOME);
        strcat(sp->redirected_path,sp->original_path + strlen(fs_home));
    }else{
        // We have to remove any ADS crap and drive colon
        // We're also skipping the NT Prefix
        if(strstr(sp->original_path,NT_PREFIX)){
            strcpy(working_path,sp->original_path+strlen(NT_PREFIX));
        }else{
            strcpy(working_path,sp->original_path);
        }
        
        #ifdef TARGET_OS_WINDOWS
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
        strcat(sp->redirected_path,working_path);        
        #endif
    }

    get_parent_path(sp->redirected_path,sp->redirected_parent);
    sp->redirected_parent_exists = path_exists(sp->redirected_parent);

    if(sp->is_wildcard_path){
        sp->redirected_path_is_symlink = 0;
        sp->original_path_is_symlink = 0;
        sp->original_path_exists = sp->original_parent_exists;
        sp->redirected_path_exists = sp->redirected_parent_exists;
    }else{
        sp->original_path_is_symlink = path_is_symlink(sp->original_path);
        if(sp->original_path_exists && sp->original_path_is_symlink){
            resolve_abspath(sp->original_path,sp->resolved_original_path,1);
        }
        if(sp->redirected_path_is_symlink){
            resolve_abspath(sp->redirected_path,sp->resolved_original_path,1);
        }
    }
    sp->create_parent_path = (sp->original_parent_exists && ! sp->redirected_parent_exists);
}

void print_sp_info(PPIEE_PATH sp){
    printf("--------\n");
    printf("Original Path: %s\n",sp->original_path);
    printf("Original Path Exists: %d\n",sp->original_path_exists);
    printf("Original Path is Symlink: %d\n",sp->original_path_is_symlink);
    printf("Original Parent: %s\n",sp->original_parent);
    printf("Original Parent Exists: %d\n",sp->original_parent_exists);
    printf("Wildcard Path: %d\n",sp->is_wildcard_path);
    printf("Is a Redirected Subpath: %d\n",sp->is_redirected_subpath);
    printf("Redirected Path: %s\n",sp->redirected_path);
    printf("Redirected Path Exists: %d\n",sp->redirected_path_exists);
    printf("Redirected Path is Symlink: %d\n",sp->redirected_path_is_symlink);
    printf("Redirected Parent: %s\n",sp->redirected_parent);
    printf("Redirected Parent Exists: %d\n",sp->redirected_parent_exists);  
    printf("Resolved Original Path: %s\n",sp->resolved_original_path);
    printf("Should Create Parent Path: %d\n",sp->create_parent_path);
    printf("--------\n");
}


int fs_redirect(char* in_abspath, int is_directory, int is_read, int is_write, int fail_if_exist, int fail_if_not_exist, char** redirected_path){
    if(!fs_init){
        init_fs();
        printf("Redirected Root: %s\n",fs_root);
        printf("Real Home: %s\n",fs_home);
    }
    // If we don't have a redirected root, we can't do anything.
    if(!fs_root){return 0;}
    // Check ignore list and return at this point.    
    if(path_ignored(in_abspath)){return 0;}    
    printf("in abspath: %s\n",in_abspath);
    PPIEE_PATH sp = calloc(1,sizeof(PIEE_PATH));
    strcpy(sp->original_path,in_abspath);
    generate_path_info(sp);
    *redirected_path = sp->redirected_path;
    print_sp_info(sp);
    // 'Fix' for access case
    if(!is_read && !is_write){is_read = 1;}
    
    // Read Only Handling First
    if(is_read && !is_write){
        
    }
    if(is_write){
        if(sp->create_parent_path){
            create_parent_path(sp->redirected_parent);
        }
    }

    // If the real write path already exists, we're going to just bypass and write to the real path
    // We may end up reverting this later or making it a configuration option
    free(sp);
    return 0;
}


