#include <windows.h>
#include <stdio.h>

#include "../common/iniparser/iniparser.h"
#include "../common/iniparser/dictionary.h"
#include "../../shared/strutils.h"

static char path_to_config_file[1024] = {0x00};
static dictionary* registry = NULL;
static int ini_lock = 0;


static unsigned int bswap32(unsigned int x) {
        return ((( x & 0xff000000u ) >> 24 ) |
                (( x & 0x00ff0000u ) >> 8  ) |
                (( x & 0x0000ff00u ) << 8  ) |
                (( x & 0x000000ffu ) << 24 ));
    }
static unsigned long long bswap64(unsigned long long x) {
        return ((( x & 0xff00000000000000ull ) >> 56 ) |
                (( x & 0x00ff000000000000ull ) >> 40 ) |
                (( x & 0x0000ff0000000000ull ) >> 24 ) |
                (( x & 0x000000ff00000000ull ) >> 8  ) |
                (( x & 0x00000000ff000000ull ) << 8  ) |
                (( x & 0x0000000000ff0000ull ) << 24 ) |
                (( x & 0x000000000000ff00ull ) << 40 ) |
                (( x & 0x00000000000000ffull ) << 56 ));
    }

void anonymize_key_path(char* in_path){
    // If we aren't looking at a user registry key with a SID, fail out.
    if(strlen(in_path)< strlen("\\registry\\user\\s-1-5")){return;}
    if(strncmp(in_path,"\\registry\\user\\s-1-5",strlen("\\registry\\user\\s-1-5"))){return;}
    char* before_sid = strstr(in_path,"\\registry\\user\\") + strlen("\\registry\\user\\");
    // Look for the next dividing slash [after the sid]
    if(!strstr(before_sid,"\\")){return;}
    char* after_sid = strstr(before_sid,"\\") + 1;
    strcpy(before_sid,after_sid);
}

void preprocess_path(char* in_path){
    to_lowercase(in_path);
    anonymize_key_path(in_path);
}

// Saves Current Registry Settings and Reloads from Disk.
void commit_changes(){
    while(ini_lock){};
    ini_lock = 1;
    FILE* fp = fopen(path_to_config_file,"wb");
    iniparser_dump_ini(registry,fp);
    fclose(fp);
    registry = iniparser_load(path_to_config_file);
    ini_lock = 0;
}

void get_config_file_path(){
    if(getenv("PDXREG")){
        strcpy(path_to_config_file,getenv("PDXREG"));
    }else{
    // Fallback to CWD
    strcpy(path_to_config_file,"pdxreg.ini");
    }
}

int init_registry(){
    // Guard for single init.
    if(registry){return 1;}
    get_config_file_path();
    registry = iniparser_load(path_to_config_file);
    return registry != NULL;
}

void cleanup_registry(){
    commit_changes();
    iniparser_freedict(registry);
}

int key_path_exists(char* key_path){
    preprocess_path(key_path);
    int num_sections = iniparser_getnsec(registry);
    for(int i=0;i<num_sections;i++){
        const char* secname = iniparser_getsecname(registry, i) ;
        if(strstr(secname,key_path)){
            return 1;
        }
    }
    return 0;
}

void create_registry_entry(char* key_path, char* value_name, unsigned int title_index, unsigned int type, void* data, unsigned int data_length){
    char full_path[1024] = {0x00};
    char data_buffer[1024] = {0x00};
    if(!strlen(value_name)){
        value_name = "@";
    }
    sprintf(full_path,"%s\\%s",key_path,value_name);
    preprocess_path(full_path);
    iniparser_set(registry,full_path,NULL);
    sprintf(full_path,"%s\\%s:type",key_path,value_name);
    preprocess_path(full_path);
    itoa(type,data_buffer,10);
    iniparser_set(registry,full_path,data_buffer);
    sprintf(full_path,"%s\\%s:data",key_path,value_name);
    preprocess_path(full_path);
    DWORD val32_be = 0;
    switch(type){
        case REG_SZ:
            WideCharToMultiByte(CP_ACP,0,data,data_length,data_buffer,1024,NULL,NULL);

            break;
        case REG_BINARY:
            BinToHex(data,data_length,data_buffer,sizeof(data_buffer));
            break;
        case REG_DWORD:
            itoa(*(DWORD*)data,data_buffer,10);
            break;
        case REG_DWORD_BIG_ENDIAN:
            val32_be = *(DWORD*)data;
            itoa(bswap32(val32_be),data_buffer,10);
            break;
        case REG_QWORD:
            itoa(*(unsigned long long*)data,data_buffer,10);
            break;
        default:
            printf("Error - Datatype not supported.");
            return;
    }
    iniparser_set(registry,full_path,data_buffer);
    commit_changes();
}

int get_registry_value(char* key_path, char* value_name,unsigned int* value_type, unsigned char** value_data, unsigned int* value_data_length){
    char full_path[1024] = {0x00};
    
    unsigned int type = 0; 
    if(!strlen(value_name)){
        value_name = "@";
    }    
    if(!key_path_exists(key_path)){return 0;}
    
    sprintf(full_path,"%s\\%s:type",key_path,value_name);
    preprocess_path(full_path);

    *value_type = iniparser_getint(registry,full_path,-1);
    if(*value_type == -1){OutputDebugStringA("Could not Get Registry Type from Key");return 0;}    
    sprintf(full_path,"%s\\%s:data",key_path,value_name);
    preprocess_path(full_path);
   
    DWORD val32 = 0;
    unsigned long long val64 = 0;
    const char* valstr;
    switch(*value_type){
        case REG_SZ:
            valstr = iniparser_getstring(registry,full_path,NULL);
            if(!valstr){return 0;}            
            *value_data_length = (strlen(valstr)+1)*2;
            *value_data = malloc(*value_data_length);
            MultiByteToWideChar(CP_ACP, 0, valstr, -1, (LPWSTR)*value_data, *value_data_length);
            break; 
        case REG_BINARY:
            valstr = iniparser_getstring(registry,full_path,NULL);
            *value_data_length = strlen(valstr) / 2;
            *value_data = malloc(*value_data_length);
            HexToBin(valstr,*value_data,*value_data_length);
            break;
        case REG_DWORD:
            val32 = iniparser_getint(registry,full_path,0);
            *value_data_length = 4;
            *value_data = malloc(*value_data_length);
            memcpy(*value_data,&val32,4);            
            break;
        case REG_DWORD_BIG_ENDIAN:
            val32 = iniparser_getint(registry,full_path,0);
            val32 = bswap32(val32);
            *value_data_length = 4;
            *value_data = malloc(*value_data_length);
            memcpy(*value_data,&val32,4);        
            break;
        case REG_QWORD:
            val64 = iniparser_getdouble(registry,full_path,0);   
            *value_data_length = 8; 
            *value_data = malloc(*value_data_length);
            memcpy(*value_data,&val64,8);  
            break;
        default:
            OutputDebugStringA("Error - Datatype not supported.");
            return 0;        
    }
    return 1;
}

void delete_registry_key(char* key_path, char* value_name){
    char full_path[1024] = {0x00};
    strcpy(full_path,key_path);
    // We may be deleting a key or value.
    if(value_name){
        strcat(full_path,value_name);
    }
    preprocess_path(full_path);
    iniparser_unset(registry,full_path);
    commit_changes();
}



