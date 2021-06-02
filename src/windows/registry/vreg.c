#include <windows.h>
#include "../common/iniparser/iniparser.h"
#include "../common/iniparser/dictionary.h"
#include "../../shared/strutils.h"
#include "../../shared/dbg.h"

#include "vreg.h"

static char path_to_config_file[1024] = {0x00};
static dictionary* registry = NULL;
static int enable_anon_keypaths = 1;
static int ini_lock = 0;

# define BSWAP32(x)  ((((x) & 0x000000ff) << 24) |      \
                      (((x) & 0x0000ff00) << 8)  |      \
                      (((x) & 0x00ff0000) >> 8)  |      \
                      (((x) & 0xff000000) >> 24))

#define REG_USER_ROOT "\\REGISTRY\\USER\\"
#define REG_USER_SID "\\REGISTRY\\USER\\S-1-5"
#define REG_USER_CLASSES "_Classes\\"

// Remove User SID from Key Path Request
void vreg_anonymize_key_path(char* in_path){
    if(!enable_anon_keypaths){return;}
    // If we aren't looking at a user registry key with a SID, fail out.
    if(strlen(in_path)< strlen(REG_USER_SID)){return;}
    // If our path doesn't start with the REG_USER_SID prefix, fail out.
    if(strncmp(in_path,REG_USER_SID,strlen(REG_USER_SID))){return;}
    // Get a pointer to our path starting prior to the SID.
    char* before_sid = in_path + strlen(REG_USER_ROOT);
    // Determine if this is a classes suffix
    int is_classes = (strstr(before_sid,REG_USER_CLASSES) != NULL);
    // Look for the next dividing slash (after the SID)
    if(!strstr(before_sid,"\\")){return;}   
    // Get a pointer to the path following the SID and prepending slash.
    char* after_sid = strstr(before_sid,"\\") + 1;
    // Splice "after_sid" to "before_sid" - effectively cutting it out.
    if(is_classes){
        after_sid -= 8;
    }
    strcpy(before_sid,after_sid);
}



// HouseKeeping Stuff

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

void get_anon_keypaths(){
    char akp_setting[8] = {0x00};
    if(getenv("PDXREG_AKP")){
        strncpy(path_to_config_file,getenv("PDXREG_AKP"),sizeof(akp_setting));
        enable_anon_keypaths = atoi(akp_setting);
    }
}

int init_registry(){
    // Guard for single init.
    if(registry){return 1;}
    get_config_file_path();
    get_anon_keypaths();
    registry = iniparser_load(path_to_config_file);
    return registry != NULL;
}

void cleanup_registry(){
    commit_changes();
    iniparser_freedict(registry);
}

int vreg_key_exists(const char* path){
    if(!registry){init_registry();}
    char processed_path[1024] = {0x00};
    strcpy(processed_path,path);
    vreg_anonymize_key_path(processed_path);
    return iniparser_find_entry(registry, processed_path) ;
}
void vreg_create_key(char* path){
    if(!registry){init_registry();}
    char processed_path[1024] = {0x00};
    strcpy(processed_path,path);
    vreg_anonymize_key_path(processed_path);    
    if(!vreg_key_exists(processed_path)){
        iniparser_set(registry, processed_path, NULL);
        commit_changes();        
    }
}
void vreg_delete_key(char* path){
    if(!registry){init_registry();}
    char processed_path[1024] = {0x00};
    strcpy(processed_path,path);
    vreg_anonymize_key_path(processed_path);        
    iniparser_unset(registry,processed_path);
    commit_changes();    
}
void vreg_create_key_value(char* path, char* name, unsigned int title_index, unsigned int type, void* data, unsigned int data_length){
    if(!registry){init_registry();}
    char full_path[1024] = {0x00};    
    if(strlen(name)){
        sprintf(full_path,"%s:%s",path,name);
    }else{
        sprintf(full_path,"%s:@",path,name);
    }
    vreg_anonymize_key_path(full_path);
    unsigned char data_buffer[1024] = {0x00};
    switch(type){
        case REG_SZ:
            WideCharToMultiByte(CP_ACP,0,data,data_length,(LPSTR)data_buffer,sizeof(data_buffer),NULL,NULL);
            break;
        case REG_BINARY:
            BinToHex(data,data_length,data_buffer,sizeof(data_buffer));
            break;
        case REG_DWORD:
            itoa(*(DWORD*)data,data_buffer,10);
            break;
        case REG_DWORD_BIG_ENDIAN:
            itoa(BSWAP32(*(DWORD*)data),data_buffer,10);
            break;
        case REG_QWORD:
            itoa(*(unsigned long long*)data,data_buffer,10);
            break;
        default:
            DBG_printf("[create_key_value] Error - Datatype not supported.");
            return;
    }
    char val[1024] = {0x00};
    sprintf(val,"\"%d,%s\"", type, data_buffer);
    iniparser_set(registry, full_path, val);
    commit_changes();    
}
int vreg_get_key_value(char* path, char* name,unsigned int* value_type, unsigned char** value_data, unsigned int* value_data_length){
    if(!registry){init_registry();}
    char full_path[1024] = {0x00};
    if(strlen(name)){
        sprintf(full_path,"%s:%s",path,name);
    }else{
        sprintf(full_path,"%s:@",path,name);
    }
    vreg_anonymize_key_path(full_path);  
    const char* sval = iniparser_getstring(registry,full_path,NULL);
    if(!sval){return 0;}

    // Strip Value type from value.
    char* sval_data = strstr(sval,",");
    if(!sval_data){return 0;}

    char* sval_type = malloc((sval_data-sval));
    strncpy(sval_type,sval,(sval_data-sval));
    *value_type = atoi(sval_type);
    free(sval_type);
    sval_data++;
    // Deserialize the data based on type.
    DWORD val32 = 0;
    unsigned long long val64 = 0;
    const char* valstr;
    switch(*value_type){
        case REG_SZ:
            *value_data_length = (strlen(sval_data)+1)*2;
            *value_data = malloc(*value_data_length);
            MultiByteToWideChar(CP_ACP, 0, sval_data, -1, (LPWSTR)*value_data, *value_data_length);
            break; 
        case REG_BINARY:
            *value_data_length = strlen(sval_data) / 2;
            *value_data = malloc(*value_data_length);
            HexToBin(sval_data,*value_data,*value_data_length);
            break;
        case REG_DWORD:
            val32 = atoi(sval_data);
            *value_data_length = 4;
            *value_data = malloc(*value_data_length);
            memcpy(*value_data,&val32,4);            
            break;
        case REG_DWORD_BIG_ENDIAN:
            val32 = BSWAP32(atoi(sval_data));
            *value_data_length = 4;
            *value_data = malloc(*value_data_length);
            memcpy(*value_data,&val32,4);        
            break;
        case REG_QWORD:
            val64 = atof(sval_data);
            *value_data_length = 8; 
            *value_data = malloc(*value_data_length);
            memcpy(*value_data,&val64,8);  
            break;
        default:
            DBG_printf("[get_key_value] Error - Datatype not supported.");
            return 0;        
    }
    return 1;    


}
void vreg_delete_key_value(char* path, char* name){
    if(!registry){init_registry();}
    char full_path[1024] = {0x00};
    sprintf(full_path,"%s:%s",path,name);
    vreg_anonymize_key_path(full_path);    
    iniparser_unset(registry,full_path);
    commit_changes();    
}

int vreg_get_count_values(char* key_path){
    if(!registry){init_registry();}
    char full_path[1024] = {0x00};
    strcpy(full_path,key_path);
    vreg_anonymize_key_path(full_path);           
    return iniparser_getsecnkeys(registry,full_path);
}