#include <string.h>

#include "../common/iniparser/iniparser.h"
#include "../common/iniparser/dictionary.h"
#include "../common/mem.h"

static char path_to_config_file[1024] = {0x00};
static dictionary* config = NULL;

void get_config_file_path(){
    if(getenv("PDXENV")){
        strcpy(path_to_config_file,getenv("PDXENV"));
    }else{
    // Fallback to CWD
    strcpy(path_to_config_file,"pdxenv.ini");
    }
}

const char* get_username(){
    if(!config){return NULL;}
    return iniparser_getstring(config,"global:username",NULL);
}

void load_config(){
    get_config_file_path();
    config = iniparser_load(path_to_config_file);
}

#include <sys/utsname.h>
typedef struct _ENV_USER {
    const char   *pw_name;       /* username */
    const char   *pw_passwd;     /* user password */
    uid_t   pw_uid;        /* user ID */
    gid_t   pw_gid;        /* group ID */
    const char   *pw_gecos;      /* user information */
    const char   *pw_dir;        /* home directory */
    const char   *pw_shell;      /* shell program */
}ENV_USER,*PENV_USER;

static struct _fake_environment{
    struct utsname os_info;
}fake_environment;

void init_os_data(){
    // OS Information Data
    strcpy(fake_environment.os_info.sysname,iniparser_getstring(config,"osinfo:sysname","Linux"));
    strcpy(fake_environment.os_info.nodename,iniparser_getstring(config,"osinfo:nodename","system"));
    strcpy(fake_environment.os_info.release,iniparser_getstring(config,"osinfo:release","2.6.32-5-686"));
    strcpy(fake_environment.os_info.version,iniparser_getstring(config,"osinfo:version","#1 SMP Mon Jan 16 16:04:25 UTC 2012"));
    strcpy(fake_environment.os_info.machine,iniparser_getstring(config,"osinfo:machine","i686"));
    strcpy(fake_environment.os_info.__domainname,iniparser_getstring(config,"osinfo:domainname","GNU/Linux"));

}

int get_uid_data(unsigned int uid, PENV_USER data){
    if(!config){return 0;}
    char request[64] = {0x00};
    sprintf(request,"uid_%d:pw_name",uid);
    data->pw_name = iniparser_getstring(config,request,NULL);
    if(!data->pw_name){return 0;}

    sprintf(request,"uid_%d:pw_passwd",uid);
    data->pw_passwd = iniparser_getstring(config,request,NULL);
    data->pw_uid = uid;
    sprintf(request,"uid_%d:pw_gid",uid);
    data->pw_gid = iniparser_getint(config,request,0);

    sprintf(request,"uid_%d:pw_gecos",uid);
    data->pw_gecos = iniparser_getstring(config,request,NULL);

    sprintf(request,"uid_%d:pw_dir",uid);
    data->pw_dir = iniparser_getstring(config,request,NULL);

    sprintf(request,"uid_%d:pw_shell",uid);
    data->pw_shell = iniparser_getstring(config,request,NULL);
}


typedef int tgetlogin_r(char *buf, size_t bufsize);
typedef PENV_USER tgetpwuid(uid_t uid);
typedef int tuname(struct utsname* name);

static tgetlogin_r* real_getlogin_r = NULL;
static tgetpwuid* real_getpwuid = NULL;
static tuname* real_uname = NULL;


// Our actual hooks
int getlogin_r(char *buf, size_t bufsize){
    if(!get_username()){        
        return real_getlogin_r(buf,bufsize);
    }
    strcpy(buf,get_username());
    return 0;
}

PENV_USER getpwuid(uid_t uid){
    PENV_USER eu = calloc(1,sizeof(ENV_USER));
    if(!get_uid_data(uid,eu)){
        return real_getpwuid(uid);
    }
    return eu;
}

int uname(struct utsname* name){
    strcpy(name->sysname,fake_environment.os_info.sysname);
    strcpy(name->nodename,fake_environment.os_info.nodename);
    strcpy(name->release,fake_environment.os_info.release);
    strcpy(name->version,fake_environment.os_info.version);
    strcpy(name->machine,fake_environment.os_info.machine);
    strcpy(name->__domainname,fake_environment.os_info.__domainname);
    return 0;
}

void init_library(void){   
    // Load Our Environment Configuration
    load_config();
    if(!config){return;}
    // Perform any Syscall Hooks we Need at This Level
    init_os_data();
    // Initialize our Stubs
    get_function_address("libc.so.6", "uname", (void**)&real_uname);
    get_function_address("libc.so.6", "getlogin_r", (void**)&real_getlogin_r);
    get_function_address("libc.so.6", "getpwuid", (void**)&real_getpwuid);
}

void init_library(void) __attribute__((constructor));