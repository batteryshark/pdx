#include <Windows.h>
#include "../common/ntmin/ntmin.h"
#include "reg_config.h"
#include "reg_entry.h"
#include "reg_glue.h"



static int reg_hpath_initialized = 0;
static OBJECT_ATTRIBUTES regOa = {0};
static UNICODE_STRING reg_hpath;

BOOL get_device_path_from_handle(HANDLE hObject, POBJECT_NAME_INFORMATION* pobj) {
    ULONG ReturnLength = 0;
    ULONG InfoLength = 0;
    
    if (NtQueryObject(hObject, (OBJECT_INFORMATION_CLASS)ObjectNameInformation, *pobj, InfoLength, &ReturnLength) && !ReturnLength) {
        return FALSE;
    }

    // Allocate Memory
    *pobj = (POBJECT_NAME_INFORMATION)calloc(1, ReturnLength);
    InfoLength = ReturnLength;

    if (NtQueryObject(hObject, (OBJECT_INFORMATION_CLASS)ObjectNameInformation, *pobj, InfoLength, &ReturnLength)) {
        free(*pobj);
        return FALSE;
    }
    return TRUE;
}

BOOL get_full_registry_path_from_handle(HANDLE hKey, char* out_path){
    if(!hKey){return FALSE;}
    POBJECT_NAME_INFORMATION RootName = NULL;
if (!get_device_path_from_handle(hKey, &RootName) || !RootName) {
        return FALSE;
    }    
    return TRUE;
}

BOOL get_full_registry_path(POBJECT_ATTRIBUTES ou, char* out_path){
    char tmp_path[1024] = {0x00};
    // Bypass if there isn't a valid handle.
    if (!ou->RootDirectory || ou->RootDirectory == INVALID_HANDLE_VALUE) {
        UnicodeStringtoChar(ou->ObjectName,tmp_path);
        strcpy(out_path,tmp_path);
        
        return FALSE;
    }

    // We have to short-circuit if the handle was passed to our registry handler already.
    char* path_to_monitored_key = regentry_path_lookup(ou->RootDirectory);
    if(path_to_monitored_key){
        strcpy(out_path,path_to_monitored_key);
        strcat(out_path,"\\");
        UnicodeStringtoChar(ou->ObjectName,tmp_path);
        strcat(out_path,tmp_path);        
        return TRUE;
    }

    // Use NtQueryObject to get full device path.
    POBJECT_NAME_INFORMATION RootName = NULL;
    if (!get_device_path_from_handle(ou->RootDirectory, &RootName) || !RootName) {
        UnicodeStringtoChar(ou->ObjectName,tmp_path);
        strcpy(out_path,tmp_path);
        return FALSE;
    }
    // If we were able to get the full path from the root handle, we have to add that
    // first.
    UnicodeStringtoChar(&RootName->Name,tmp_path);
    free(RootName);
    strcpy(out_path,tmp_path);    
    
     if(ou->ObjectName){
        strcat(out_path,"\\");
        UnicodeStringtoChar(ou->ObjectName,tmp_path);
        strcat(out_path,tmp_path);
     }
    return TRUE;
}


BOOL nt_redirect_registry_key(POBJECT_ATTRIBUTES ObjectAttributes, BOOL is_read, BOOL is_write,PHANDLE KeyHandle){
    if(!ObjectAttributes || !ObjectAttributes->ObjectName){return FALSE;}
    if(!reg_hpath_initialized){        
        ChartoUnicodeString("\\registry\\machine",&reg_hpath);
        regOa.Length = sizeof(regOa);
        regOa.ObjectName = &reg_hpath;
        regOa.Attributes = OBJ_CASE_INSENSITIVE;
        regOa.RootDirectory = NULL;        
        reg_hpath_initialized = 1;
    }
    char rp[1024] = {0x00};
    get_full_registry_path(ObjectAttributes,rp);
    //OutputDebugStringA(rp);
    if(!key_path_exists(rp)){return FALSE;}
    if(NtOpenKey(KeyHandle, GENERIC_READ_BYPASS, &regOa)){
        return FALSE;
    }
    regentry_create(*KeyHandle,rp);
    return TRUE;
}


BOOL get_registry_kvic(char* key_path, char* value_name, KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass, unsigned char** data, unsigned int* data_length){
unsigned int value_type = 0;
unsigned int value_length = 0;
unsigned char* value_data = NULL;
KEY_VALUE_FULL_INFORMATION kvfi;
KEY_VALUE_PARTIAL_INFORMATION kvpi;
KEY_VALUE_BASIC_INFORMATION kvbi;
wchar_t wvalue_name[255] = {0x00};
if(!get_registry_value(key_path,value_name,&value_type,&value_data,&value_length)){return FALSE;}
switch(KeyValueInformationClass){
    case KeyValueBasicInformation:
        *data_length = sizeof(KEY_VALUE_BASIC_INFORMATION);
        MultiByteToWideChar(CP_ACP,0,value_name,-1,wvalue_name,255);
        kvbi.NameLength = wcslen(wvalue_name)+1*2;
        kvbi.TitleIndex = 0;
        kvbi.Type = value_type;
        *data_length += kvbi.NameLength;
        *data = malloc(*data_length);
        memcpy(*data,&kvbi,12);
        memcpy(*data+12,wvalue_name,kvbi.NameLength);
        break;
    case KeyValueFullInformation:
    case KeyValueFullInformationAlign64:
        *data_length = sizeof(KEY_VALUE_FULL_INFORMATION);
        kvfi.TitleIndex = 0;
        kvfi.Type = value_type;
        MultiByteToWideChar(CP_ACP,0,value_name,-1,wvalue_name,255);
        kvfi.NameLength = wcslen(wvalue_name)+1*2;
        kvfi.DataLength = value_length;
        kvfi.DataOffset = 20 + kvfi.NameLength;
        *data_length += kvfi.NameLength;
        *data_length += kvfi.DataLength;
        *data = malloc(*data_length);
        memcpy(*data,&kvfi,20);
        memcpy(*data+20,wvalue_name,kvfi.NameLength);
        memcpy(*data+kvfi.DataOffset,value_data,value_length);
        break;  
    case KeyValuePartialInformation:
    case KeyValuePartialInformationAlign64:    
        *data_length = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + value_length;
        *data = malloc(*data_length);        
        kvpi.TitleIndex = 0;
        kvpi.Type = value_type;
        kvpi.DataLength = value_length;
        memcpy(*data,&kvpi,12);
        memcpy(*data+12,value_data,value_length);
        break;
    default:
        OutputDebugStringA("KeyValueInformationClass Not Supported");    
        if(value_data){free(value_data);}
        return FALSE;
}
if(value_data){free(value_data);}

return TRUE;
}

/*
typedef struct _KEY_VALUE_FULL_INFORMATION {
  ULONG TitleIndex;
  ULONG Type;
  ULONG DataOffset;
  ULONG DataLength;
  ULONG NameLength;
  WCHAR Name[1];
} KEY_VALUE_FULL_INFORMATION, *PKEY_VALUE_FULL_INFORMATION;

*/

BOOL nt_query_value_key(HANDLE KeyHandle, PUNICODE_STRING ValueName, KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass, PVOID KeyValueInformation, ULONG Length, PULONG ResultLength, PNTSTATUS status){
    char* path_to_monitored_key = regentry_path_lookup(KeyHandle);
    if(!path_to_monitored_key){return FALSE;}
    
    char value_name[1024] = {0x00};
    UnicodeStringtoChar(ValueName,value_name);

    unsigned char* response = NULL;
    
    if(!get_registry_kvic(path_to_monitored_key,value_name,KeyValueInformationClass,&response,(unsigned int*)ResultLength)){
        if(response){free(response);}
        return FALSE;
    }
    
    // IM A MORON - this has to be the kvinfoclass shit and not this...


    if(!Length){

        *status = 0xC0000023; // STATUS_BUFFER_TOO_SMALL
        free(response);
        return TRUE;
    }else if(Length < *ResultLength){

            *status = 0x80000005; // STATUS_BUFFER_OVERFLOW
            if(KeyValueInformation){ 
                memcpy(KeyValueInformation,response,Length);
                free(response);
            }
            return TRUE;
    }

    if(KeyValueInformation){ 
        memcpy(KeyValueInformation,response,*ResultLength);
    }
    free(response);
    *status = 0;
    return TRUE;
}

BOOL nt_delete_key(HANDLE KeyHandle){
    char* path_to_monitored_key = regentry_path_lookup(KeyHandle);
    if(!path_to_monitored_key){return FALSE;}
    delete_registry_key(path_to_monitored_key,NULL);
    return TRUE;
}

BOOL nt_delete_value_key(HANDLE KeyHandle, PUNICODE_STRING ValueName){
    char* path_to_monitored_key = regentry_path_lookup(KeyHandle);
    if(!path_to_monitored_key){return FALSE;}
    char value_name[1024] = {0x00};
    UnicodeStringtoChar(ValueName,value_name);
    delete_registry_key(path_to_monitored_key,value_name);    
    return TRUE; 
}

BOOL nt_set_key_info(HANDLE KeyHandle,KEY_SET_INFORMATION_CLASS KeySetInformationClass, PVOID KeySetInformation, ULONG KeySetInformationLength){
    char* path_to_monitored_key = regentry_path_lookup(KeyHandle);
    if(!path_to_monitored_key){return FALSE;}
    OutputDebugStringA("REG_SET_KEY_INFO Not Implemented");
    return FALSE;
}


BOOL nt_set_value_key(HANDLE KeyHandle, PUNICODE_STRING ValueName, ULONG TitleIndex, ULONG Type, PVOID Data, ULONG DataSize){
    char value_name[1024] = {0x00};
    UnicodeStringtoChar(ValueName,value_name);
    char* path_to_monitored_key = regentry_path_lookup(KeyHandle);
    if(!path_to_monitored_key){return FALSE;}
    create_registry_entry(path_to_monitored_key, value_name, TitleIndex, Type,Data, DataSize);
    return TRUE;       
}

BOOL nt_enumerate_key(HANDLE KeyHandle, ULONG Index, KEY_INFORMATION_CLASS KeyInformationClass, PVOID KeyInformation, ULONG Length, PULONG ResultLength,PNTSTATUS status){
    char* path_to_monitored_key = regentry_path_lookup(KeyHandle);
    if(!path_to_monitored_key){return FALSE;}
    //REG_ENUM_KEY
    OutputDebugStringA("REG_ENUM_KEY Not Implemented");
    return FALSE;
}

BOOL nt_query_mvkey(HANDLE KeyHandle, PKEY_VALUE_ENTRY ValueEntries, ULONG EntryCount, PVOID ValueBuffer, PULONG BufferLength, PULONG RequiredBufferLength,PNTSTATUS status){
    char* path_to_monitored_key = regentry_path_lookup(KeyHandle);
    if(!path_to_monitored_key){return FALSE;}
    //REG_QMVKEY
    OutputDebugStringA("REG_QMVKEY Not Implemented");
    return FALSE;   
}

/*
        KeyBasicInformation,                    // 0x00
        KeyNodeInformation,                     // 0x01
        KeyFullInformation,                     // 0x02
        
        KeyCachedInformation,                   // 0x04
        KeyFlagsInformation,                    // 0x05
*/
BOOL nt_query_key(HANDLE KeyHandle, KEY_INFORMATION_CLASS KeyInformationClass, PVOID KeyInformation, ULONG Length, PULONG ResultLength,PNTSTATUS status){
    char* path_to_monitored_key = regentry_path_lookup(KeyHandle);
    if(!path_to_monitored_key){return FALSE;}
    // If we're doing this class, we can just do it here.
    if(KeyInformationClass == KeyNameInformation){
        KEY_NAME_INFORMATION* KeyInformation;
        unsigned int total_sz = ((strlen(path_to_monitored_key) + 1) * 2) + 4;
        if(ResultLength){
            *ResultLength = total_sz;
        }        
        if(!Length){
            *status = 0xC0000023; // STATUS_BUFFER_TOO_SMALL        
            return TRUE;
        }else if(Length < total_sz){
            *status = 0x80000005; // STATUS_BUFFER_OVERFLOW
            if(Length <= 4){
                if(KeyInformation){
                    memcpy(KeyInformation,&total_sz,Length);
                }                
            }else{
                if(KeyInformation){
                    memcpy(KeyInformation,&total_sz,4);
                    wchar_t wpath[512] = {0x00};
                    ChartoWideChar(path_to_monitored_key,wpath);
                    memcpy(KeyInformation,(unsigned char*)wpath,Length - 4);                
                }
                              
            }
            return TRUE;  
        }
        if(!KeyInformation){return TRUE;}
        KeyInformation->NameLength = total_sz - 4;
        ChartoWideChar(path_to_monitored_key,KeyInformation->Name);
        *status = 0;
        return TRUE;
    }else{
        char msg[64] = {0x00};
        wsprintfA(msg,"Unhandled Key Info Class: %d\n",KeyInformationClass);
        OutputDebugStringA(msg);
    }
    //REG_QUERY_KEY
    return FALSE;     
}
