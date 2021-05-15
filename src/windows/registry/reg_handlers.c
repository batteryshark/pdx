#include <windows.h>
#include "../common/ntmin/ntmin.h"
#include "../../shared/dbg.h"
#include "vrhm.h"
#include "vreg.h"

#include "reg_handlers.h"

static int appcompat_shimmed = 0;
static int elevated_process = 0;

BOOL IsElevated( ) {
    BOOL fRet = FALSE;
    HANDLE hToken = NULL;
    if( OpenProcessToken( GetCurrentProcess( ),TOKEN_QUERY,&hToken ) ) {
        TOKEN_ELEVATION Elevation;
        DWORD cbSize = sizeof( TOKEN_ELEVATION );
        if( GetTokenInformation( hToken, TokenElevation, &Elevation, sizeof( Elevation ), &cbSize ) ) {
            fRet = Elevation.TokenIsElevated;
        }
    }
    if( hToken ) {
        CloseHandle( hToken );
    }
    return fRet;
}

int is_elevated_process(){
    return elevated_process;
}

void set_is_elevated(){
    elevated_process = IsElevated();
}

// Helpers
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

BOOL get_full_registry_path(POBJECT_ATTRIBUTES ou, char* out_path){
    char tmp_path[1024] = {0x00};
    
    // Bypass if there isn't a valid handle or absolute path.
    if (!ou->RootDirectory || ou->RootDirectory == INVALID_HANDLE_VALUE) {
        if(ou->ObjectName && ou->ObjectName->Length){
            UnicodeStringtoChar(ou->ObjectName,out_path);
            return TRUE;
        }
        return FALSE;
    }

    // If our root is a redirected key, we need to adjust this to point at our fake path.
    PVRHM_ENTRY entry = vrhm_lookup(ou->RootDirectory);
    if(entry){
        strcpy(out_path,entry->path);
        if(ou->ObjectName && ou->ObjectName->Length){
            UnicodeStringtoChar(ou->ObjectName,tmp_path);
            strcat(out_path,"\\");
            strcat(out_path,tmp_path);
            return TRUE;
        }
        return TRUE;
    }

    // Use NtQueryObject to get full device path.
    POBJECT_NAME_INFORMATION RootName = NULL;
    if (!get_device_path_from_handle(ou->RootDirectory, &RootName) || !RootName) {
        if(ou->ObjectName && ou->ObjectName->Length){
            UnicodeStringtoChar(ou->ObjectName,out_path);
            return TRUE;
        }
        return FALSE;
    }
    // If we were able to get the full path from the root handle, we have to add that
    // first.
    if(RootName->Name.Length){
        UnicodeStringtoChar(&RootName->Name,out_path);
    }
    
    free(RootName);

    if(ou->ObjectName && ou->ObjectName->Length){
        strcat(out_path,"\\");
        UnicodeStringtoChar(ou->ObjectName,tmp_path);
        strcat(out_path,tmp_path);
     }
    return TRUE;
}

// Handlers
void handler_nt_close_registry_key(HANDLE hKey){vrhm_destroy(hKey);}
int handler_nt_flush_key(HANDLE hKey){return vrhm_lookup(hKey) != NULL;}
int handler_nt_delete_key(HANDLE hKey){
    PVRHM_ENTRY entry = vrhm_lookup(hKey);
    if(!entry){return 0;}
    vreg_delete_key(entry->path);
    return 1;
}
int handler_nt_delete_value_key(HANDLE hKey, PUNICODE_STRING ValueName){
    PVRHM_ENTRY entry = vrhm_lookup(hKey);
    if(!entry){return 0;}
    char value_name[1024] = {0x00};
    UnicodeStringtoChar(ValueName,value_name);
    vreg_delete_key_value(entry->path,value_name);
    return 1; 
}

int handler_nt_query_mvkey(HANDLE KeyHandle, PKEY_VALUE_ENTRY ValueEntries, ULONG EntryCount, PVOID ValueBuffer, PULONG BufferLength, PULONG RequiredBufferLength,PNTSTATUS status){
    PVRHM_ENTRY entry = vrhm_lookup(KeyHandle);
    if(!entry){return 0;}
    DBG_printf("handler_nt_query_mvkey: NOT IMPLEMENTED");
    return 0;
}
int handler_nt_enumerate_key(HANDLE KeyHandle, ULONG Index, KEY_INFORMATION_CLASS KeyInformationClass, PVOID KeyInformation, ULONG Length, PULONG ResultLength,PNTSTATUS status){
    PVRHM_ENTRY entry = vrhm_lookup(KeyHandle);
    if(!entry){return 0;}
    DBG_printf("handler_nt_enumerate_key: NOT IMPLEMENTED");
    return 0;
}

int handler_nt_set_key_info(HANDLE KeyHandle,KEY_SET_INFORMATION_CLASS KeySetInformationClass, PVOID KeySetInformation, ULONG KeySetInformationLength){
    PVRHM_ENTRY entry = vrhm_lookup(KeyHandle);
    if(!entry){return 0;}    
    DBG_printf("handler_nt_set_key_info: NOT IMPLEMENTED");
    return 0;
}

int handler_nt_set_value_key(HANDLE KeyHandle, PUNICODE_STRING ValueName, ULONG TitleIndex, ULONG Type, PVOID Data, ULONG DataSize){
    PVRHM_ENTRY entry = vrhm_lookup(KeyHandle);
    if(!entry){return 0;}    
    char value_name[1024] = {0x00};
    UnicodeStringtoChar(ValueName,value_name);    
    vreg_create_key_value(entry->path,value_name,TitleIndex,Type, Data, DataSize);
    return 1;
}

int handler_nt_redirect_registry_key(POBJECT_ATTRIBUTES ObjectAttributes, PHANDLE KeyHandle){
    if(!ObjectAttributes || !ObjectAttributes->ObjectName){return 0;}

    char rp[1024] = {0x00};
    // Reconstruct full registry path to key
    if(!get_full_registry_path(ObjectAttributes,rp)){return 0;}
    // Check vreg for key path
    if(!vreg_key_exists(rp)){return 0;}
    UNICODE_STRING reg_hpath;
    RtlInitUnicodeString(&reg_hpath, L"\\REGISTRY");
    OBJECT_ATTRIBUTES regOa;
    memset(&regOa,0,sizeof(OBJECT_ATTRIBUTES));
    regOa.Length = sizeof(OBJECT_ATTRIBUTES);
    regOa.ObjectName = &reg_hpath;
    regOa.Attributes = OBJ_CASE_INSENSITIVE;
    regOa.RootDirectory = NULL;    

    if(NtOpenKey(KeyHandle, MAXIMUM_ALLOWED_BYPASS, &regOa)){
        DBG_printf("Failed to Open Replacement Key");
        return FALSE;
    }

    // If we aren't in an elevated process, we have to bump the handle due to some bullshit shadowing?
    if(!elevated_process){
        vrhm_add(*KeyHandle+2,rp);
    }else{
        vrhm_add(*KeyHandle,rp);
    }
    
    return TRUE;
}




int handler_nt_query_key(HANDLE KeyHandle, KEY_INFORMATION_CLASS KeyInformationClass, PVOID KeyInformation, ULONG Length, PULONG ResultLength,PNTSTATUS status){
    PVRHM_ENTRY entry = vrhm_lookup(KeyHandle);
    if(!entry){return 0;}

    UNICODE_STRING uc_path;
    RtlCreateUnicodeStringFromAsciiz(&uc_path,entry->path);

    void* tmp_info = NULL;
    ULONG tmp_length = 0;

    PKEY_BASIC_INFORMATION k_b_i = NULL;
    PKEY_FULL_INFORMATION k_f_i = NULL;
    PKEY_NODE_INFORMATION k_node_i = NULL;
    PKEY_NAME_INFORMATION k_name_i = NULL;
    PKEY_CACHED_INFORMATION k_c_i = NULL;
    PKEY_HANDLE_TAGS_INFORMATION k_ht_i = NULL;

    switch(KeyInformationClass){
        case KeyBasicInformation:
            tmp_length = sizeof(KEY_BASIC_INFORMATION) + uc_path.Length - 2;
            k_b_i = calloc(1,tmp_length);
            k_b_i->LastWriteTime.LowPart = 0;
            k_b_i->TitleIndex = 0;
            k_b_i->NameLength = uc_path.Length;
            memcpy(k_b_i->Name,uc_path.Buffer,uc_path.Length);
            tmp_info = k_b_i;
            break;
        case KeyNodeInformation:
            tmp_length = sizeof(KEY_NODE_INFORMATION) + uc_path.Length;
            k_node_i = calloc(1,tmp_length);
            k_node_i->LastWriteTime.LowPart = 0;
            k_node_i->TitleIndex = 0;
            k_node_i->ClassOffset = sizeof(KEY_NODE_INFORMATION) + uc_path.Length - 2;
            k_node_i->ClassLength = 0;
            k_node_i->NameLength = uc_path.Length;
            memcpy(k_node_i->Name,uc_path.Buffer,uc_path.Length);
            tmp_info = k_node_i;            
            break;
        case 7: // KeyHandleTagsInformation
            tmp_length = sizeof(KEY_HANDLE_TAGS_INFORMATION);
            k_ht_i = calloc(1,tmp_length);
            k_ht_i->HandleTags = 0x401; // I have no idea what I'm doing...
            tmp_info = k_ht_i;
            break;
        case KeyFullInformation:
            tmp_length = sizeof(KEY_FULL_INFORMATION) + uc_path.Length - 2;
            k_f_i = calloc(1,tmp_length);
            k_f_i->LastWriteTime.LowPart = 0;
            k_f_i->TitleIndex = 0;
            k_f_i->ClassOffset = sizeof(KEY_FULL_INFORMATION) - 1;
            k_f_i->ClassLength = 0;
            k_f_i->SubKeys = 0;
            k_f_i->MaxNameLen = 128;
            k_f_i->MaxClassLen = 128;
            k_f_i->Values = vreg_get_count_values(entry->path);
            k_f_i->MaxValueNameLen = 128;
            k_f_i->MaxValueDataLen = 4096;
            tmp_info = k_f_i;
            break;
        case KeyNameInformation:
            tmp_length = sizeof(KEY_NAME_INFORMATION) + uc_path.Length - 2;
            k_name_i = calloc(1,tmp_length);
            k_name_i->NameLength = uc_path.Length;
            memcpy(k_name_i->Name,uc_path.Buffer,uc_path.Length);
            tmp_info = k_name_i;              
            break;
        case KeyCachedInformation:
            tmp_length = sizeof(KEY_CACHED_INFORMATION) + uc_path.Length - 2;
            k_c_i = calloc(1,tmp_length);
            k_c_i->LastWriteTime.LowPart = 0;
            k_c_i->MaxNameLen = 128;
            k_c_i->MaxValueDataLen = 4096;
            k_c_i->MaxValueNameLen = 128;
            k_c_i->SubKeys = 0;
            k_c_i->TitleIndex = 0;
            k_c_i->Values = vreg_get_count_values(entry->path);
            k_c_i->NameLength = uc_path.Length;
            memcpy(k_c_i->Name,uc_path.Buffer,uc_path.Length);
            tmp_info = k_c_i;
            break;
        default:
            DBG_printf("Unhandled KeyInformationClass: %d",KeyInformationClass);
            RtlFreeUnicodeString(&uc_path);
            return 0;
    }
    RtlFreeUnicodeString(&uc_path);
    *ResultLength = tmp_length;
    // Result Logic
    if(!Length){        
        *status = STATUS_BUFFER_TOO_SMALL;
        free(tmp_info);
        return 1;
    }
    else if(Length < tmp_length){        
        *status = 0x80000005; // Buffer Overflow
        memcpy(KeyInformation,tmp_info,Length);
        free(tmp_info);
        return 1;
    }
    // Otherwise, we're good.
    *status = STATUS_SUCCESS;
    memcpy(KeyInformation,tmp_info,tmp_length);
    free(tmp_info);
    return 1;
}

int handler_nt_query_value_key(HANDLE KeyHandle, PUNICODE_STRING ValueName, KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass, PVOID KeyValueInformation, ULONG Length, PULONG ResultLength, PNTSTATUS status){
    PVRHM_ENTRY entry = vrhm_lookup(KeyHandle);
    if(!entry){return 0;} 
    char value_name[1024] = {0x00};
    unsigned int value_data_length = 0;
    unsigned char* value_data = NULL;
    unsigned int value_type = 0;
    UnicodeStringtoChar(ValueName,value_name);

    if(!vreg_get_key_value(entry->path, value_name, &value_type, &value_data, &value_data_length)){
        *status = STATUS_OBJECT_NAME_NOT_FOUND;
        return 1;
        }

    void* tmp_info = NULL;
    ULONG tmp_length = 0;
    PKEY_VALUE_FULL_INFORMATION kvfi;
    PKEY_VALUE_PARTIAL_INFORMATION kvpi;
    PKEY_VALUE_BASIC_INFORMATION kvbi;    
    switch(KeyValueInformationClass){
        case KeyValueBasicInformation:
            tmp_length = sizeof(KEY_VALUE_BASIC_INFORMATION) + ValueName->Length - 2;
            kvbi = calloc(1,tmp_length);
            kvbi->TitleIndex = 0;
            kvbi->Type = value_type;
            kvbi->NameLength = ValueName->Length;
            memcpy(kvbi->Name,ValueName->Buffer,ValueName->Length);
            tmp_info = kvbi;
            break;
        case KeyValuePartialInformation:
        case KeyValuePartialInformationAlign64:           
            tmp_length = (sizeof(KEY_VALUE_PARTIAL_INFORMATION) - 1) + value_data_length;
            kvpi = calloc(1,tmp_length);
            kvpi->TitleIndex = 0;
            kvpi->Type = value_type;
            kvpi->DataLength = value_data_length;
            memcpy(kvpi->Data,value_data,value_data_length);
            tmp_info = kvpi;
            break;            
        case KeyValueFullInformation:
        case KeyValueFullInformationAlign64:
            tmp_length = sizeof(KEY_VALUE_FULL_INFORMATION)  + ValueName->Length + value_data_length -1;
            kvfi = calloc(1,tmp_length);
            kvfi->TitleIndex = 0;
            kvfi->Type = value_type;
            kvfi->NameLength = ValueName->Length;
            kvfi->DataLength = value_data_length;
            memcpy(kvfi->Name,ValueName->Buffer,ValueName->Length);
            memcpy(kvfi->Name+kvfi->NameLength,value_data,value_data_length);
            kvfi->DataOffset = 20 + ValueName->Length;
            tmp_info=kvfi;
            break;            
        default:
            DBG_printf("Unsupported KVIC: %d",KeyValueInformationClass);
            if(value_data){free(value_data);}
            return 0;
    }

    *ResultLength = tmp_length;
    // Result Logic
    if(!Length){        
        *status = STATUS_BUFFER_TOO_SMALL;
        free(tmp_info);
        return 1;
    }
    
    // Otherwise, we're good.
    *status = STATUS_SUCCESS;
    memcpy(KeyValueInformation,tmp_info,tmp_length);
    free(tmp_info);
    return 1;
}