#include <Windows.h>
#include "../common/ntmin/ntdll.h"
#include "../common/mem.h"

#include "reg_glue.h"
#include "reg_entry.h"
#include "reg_config.h"



typedef NTSTATUS NTAPI tNtCreateKey(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, ULONG TitleIndex, PUNICODE_STRING Class, ULONG CreateOptions, PULONG Disposition);
typedef NTSTATUS NTAPI tNtCreateKeyTransacted(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, ULONG TitleIndex, PUNICODE_STRING Class, ULONG CreateOptions, HANDLE TransactionHandle, PULONG Disposition);
typedef NTSTATUS NTAPI tNtOpenKey(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes);

typedef NTSTATUS NTAPI tNtOpenKeyEx(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, ULONG OpenOptions);
typedef NTSTATUS NTAPI tNtOpenKeyTransacted(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, HANDLE TransactionHandle);
typedef NTSTATUS NTAPI tNtOpenKeyTransactedEx(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes,ULONG OpenOptions, HANDLE TransactionHandle);
typedef NTSTATUS NTAPI tNtQueryKey(HANDLE KeyHandle, KEY_INFORMATION_CLASS KeyInformationClass, PVOID KeyInformation, ULONG Length, PULONG ResultLength);
typedef NTSTATUS NTAPI tNtFlushKey(HANDLE KeyHandle);
typedef NTSTATUS NTAPI tNtSetInformationKey(HANDLE KeyHandle, KEY_SET_INFORMATION_CLASS KeySetInformationClass, PVOID KeySetInformation, ULONG KeySetInformationLength);
typedef NTSTATUS NTAPI tNtSetValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName, ULONG TitleIndex, ULONG Type, PVOID Data, ULONG DataSize);
typedef NTSTATUS NTAPI tNtQueryValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName, KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass, PVOID KeyValueInformation, ULONG Length, PULONG ResultLength);
typedef NTSTATUS NTAPI tNtDeleteValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName);
typedef NTSTATUS NTAPI tNtDeleteKey(HANDLE KeyHandle);
typedef NTSTATUS NTAPI tNtEnumerateKey(HANDLE KeyHandle, ULONG Index, KEY_INFORMATION_CLASS KeyInformationClass, PVOID KeyInformation, ULONG Length, PULONG ResultLength);
typedef NTSTATUS NTAPI tNtQueryMultipleValueKey(HANDLE KeyHandle, PKEY_VALUE_ENTRY ValueEntries, ULONG EntryCount, PVOID ValueBuffer, PULONG BufferLength, PULONG RequiredBufferLength);

// Generic Types
typedef NTSTATUS NTAPI tNtClose(HANDLE ObjectHandle);

static tNtCreateKey* ntdll_NtCreateKey = NULL;
static tNtCreateKeyTransacted* ntdll_NtCreateKeyTransacted = NULL;
static tNtClose* ntdll_NtClose = NULL;
static tNtQueryKey* ntdll_NtQueryKey = NULL; // This one is gonna suck
static tNtOpenKey* ntdll_NtOpenKey = NULL;
static tNtOpenKeyEx* ntdll_NtOpenKeyEx = NULL;
static tNtOpenKeyTransacted* ntdll_NtOpenKeyTransacted = NULL;
static tNtOpenKeyTransactedEx* ntdll_NtOpenKeyTransactedEx = NULL;
static tNtFlushKey* ntdll_NtFlushKey = NULL;
static tNtSetInformationKey* ntdll_NtSetInformationKey = NULL;
static tNtSetValueKey* ntdll_NtSetValueKey = NULL;
static tNtDeleteValueKey* ntdll_NtDeleteValueKey = NULL;
static tNtDeleteKey* ntdll_NtDeleteKey = NULL;
static tNtEnumerateKey* ntdll_NtEnumerateKey = NULL;
static tNtQueryMultipleValueKey* ntdll_NtQueryMultipleValueKey = NULL;
static tNtQueryValueKey* ntdll_NtQueryValueKey = NULL;




// Bindings
NTSTATUS NTAPI x_NtClose(HANDLE ObjectHandle){
    // Check for Registry Handles and Delete if Necessary
    regentry_delete(ObjectHandle);
    return ntdll_NtClose(ObjectHandle);
}

NTSTATUS NTAPI x_NtCreateKey(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, ULONG TitleIndex, PUNICODE_STRING Class, ULONG CreateOptions, PULONG Disposition){

    if(nt_redirect_registry_key(ObjectAttributes,TRUE,FALSE,KeyHandle)){return STATUS_SUCCESS;}
	return ntdll_NtCreateKey(KeyHandle, DesiredAccess, ObjectAttributes, TitleIndex, Class, CreateOptions, Disposition);
}	

NTSTATUS NTAPI x_NtCreateKeyTransacted(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, ULONG TitleIndex, PUNICODE_STRING Class, ULONG CreateOptions, HANDLE TransactionHandle, PULONG Disposition){

    if(nt_redirect_registry_key(ObjectAttributes,TRUE,FALSE,KeyHandle)){return STATUS_SUCCESS;}
    return ntdll_NtCreateKeyTransacted(KeyHandle,DesiredAccess,ObjectAttributes, TitleIndex, Class, CreateOptions, TransactionHandle, Disposition);
}

NTSTATUS NTAPI x_NtOpenKey(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes){

    if(DesiredAccess != GENERIC_READ_BYPASS && nt_redirect_registry_key(ObjectAttributes,TRUE,FALSE,KeyHandle)){return STATUS_SUCCESS;}
    if(DesiredAccess == GENERIC_READ_BYPASS){DesiredAccess = GENERIC_READ;}
    return ntdll_NtOpenKey(KeyHandle,DesiredAccess,ObjectAttributes);
}
NTSTATUS NTAPI x_NtOpenKeyEx(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, ULONG OpenOptions){

    if(nt_redirect_registry_key(ObjectAttributes,TRUE,FALSE,KeyHandle)){return STATUS_SUCCESS;}
    return ntdll_NtOpenKeyEx(KeyHandle, DesiredAccess,ObjectAttributes,OpenOptions);
}
NTSTATUS NTAPI x_NtOpenKeyTransacted(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, HANDLE TransactionHandle){

    if(nt_redirect_registry_key(ObjectAttributes,TRUE,FALSE,KeyHandle)){return STATUS_SUCCESS;}
return ntdll_NtOpenKeyTransacted(KeyHandle,DesiredAccess,ObjectAttributes,TransactionHandle);
}
NTSTATUS NTAPI x_NtOpenKeyTransactedEx(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes,ULONG OpenOptions, HANDLE TransactionHandle){

    if(nt_redirect_registry_key(ObjectAttributes,TRUE,FALSE,KeyHandle)){return STATUS_SUCCESS;}
return ntdll_NtOpenKeyTransactedEx(KeyHandle,DesiredAccess,ObjectAttributes,OpenOptions,TransactionHandle);
}

NTSTATUS NTAPI x_NtQueryKey(HANDLE KeyHandle, KEY_INFORMATION_CLASS KeyInformationClass, PVOID KeyInformation, ULONG Length, PULONG ResultLength){
    NTSTATUS status = 0;

    if(nt_query_key(KeyHandle, KeyInformationClass, KeyInformation, Length, ResultLength,&status)){return status;}
    return ntdll_NtQueryKey(KeyHandle,KeyInformationClass,KeyInformation,Length,ResultLength);
}

NTSTATUS NTAPI x_NtFlushKey(HANDLE KeyHandle){
    // Check for Fake Registry Handles and Delete if Necessary

    if(regentry_path_lookup(KeyHandle)){
        return STATUS_SUCCESS;    
    }
    return ntdll_NtFlushKey(KeyHandle);
}

NTSTATUS NTAPI x_NtDeleteKey(HANDLE KeyHandle){

    if(nt_delete_key(KeyHandle)){return STATUS_SUCCESS;}
    return ntdll_NtDeleteKey(KeyHandle);
}

NTSTATUS NTAPI x_NtSetInformationKey(HANDLE KeyHandle, KEY_SET_INFORMATION_CLASS KeySetInformationClass, PVOID KeySetInformation, ULONG KeySetInformationLength){

    if(nt_set_key_info(KeyHandle, KeySetInformationClass, KeySetInformation, KeySetInformationLength)){ return STATUS_SUCCESS;}
    return ntdll_NtSetInformationKey(KeyHandle,KeySetInformationClass,KeySetInformation, KeySetInformationLength);
}

NTSTATUS NTAPI x_NtSetValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName, ULONG TitleIndex, ULONG Type, PVOID Data, ULONG DataSize){

    if(nt_set_value_key(KeyHandle, ValueName, TitleIndex, Type, Data, DataSize)){ return STATUS_SUCCESS;}
    return ntdll_NtSetValueKey(KeyHandle,ValueName,TitleIndex,Type,Data,DataSize);
}

NTSTATUS NTAPI x_NtQueryValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName, KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass, PVOID KeyValueInformation, ULONG Length, PULONG ResultLength){
    NTSTATUS status = 0;

    if(nt_query_value_key(KeyHandle,ValueName,KeyValueInformationClass,KeyValueInformation,Length,ResultLength,&status)){return status;}
    return ntdll_NtQueryValueKey(KeyHandle,ValueName,KeyValueInformationClass,KeyValueInformation,Length,ResultLength);
}

NTSTATUS NTAPI x_NtDeleteValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName){

    if(nt_delete_value_key(KeyHandle,ValueName)){return STATUS_SUCCESS;}
    return ntdll_NtDeleteValueKey(KeyHandle,ValueName);
}

NTSTATUS NTAPI x_NtEnumerateKey(HANDLE KeyHandle, ULONG Index, KEY_INFORMATION_CLASS KeyInformationClass, PVOID KeyInformation, ULONG Length, PULONG ResultLength){

    NTSTATUS status = 0;
    if(nt_enumerate_key(KeyHandle, Index, KeyInformationClass, KeyInformation, Length, ResultLength,&status)){return status;}
    return ntdll_NtEnumerateKey(KeyHandle,Index,KeyInformationClass,KeyInformation,Length,ResultLength);
}
NTSTATUS NTAPI x_NtQueryMultipleValueKey(HANDLE KeyHandle, PKEY_VALUE_ENTRY ValueEntries, ULONG EntryCount, PVOID ValueBuffer, PULONG BufferLength, PULONG RequiredBufferLength){    
    NTSTATUS status = 0;

    if(nt_query_mvkey(KeyHandle, ValueEntries, EntryCount, ValueBuffer, BufferLength, RequiredBufferLength,&status)){return status;}
    return ntdll_NtQueryMultipleValueKey(KeyHandle,ValueEntries,EntryCount,ValueBuffer,BufferLength,RequiredBufferLength);
}



__declspec(dllexport) void imoldreg(){};

int init_library(void){   

    if(!init_registry()){return 0;}
    // Perform any Syscall Hooks we Need at This Level
    if (!inline_hook("ntdll.dll", "NtClose", SYSCALL_STUB_SIZE, (void*)x_NtClose, (void**)&ntdll_NtClose)) { return FALSE; }
    if (!inline_hook("ntdll.dll", "NtCreateKey", SYSCALL_STUB_SIZE, (void*)x_NtCreateKey, (void**)&ntdll_NtCreateKey)) { return FALSE; }
    if (!inline_hook("ntdll.dll", "NtCreateKeyTransacted", SYSCALL_STUB_SIZE, (void*)x_NtCreateKeyTransacted, (void**)&ntdll_NtCreateKeyTransacted)) { return FALSE; }
    if (!inline_hook("ntdll.dll", "NtOpenKey", SYSCALL_STUB_SIZE, (void*)x_NtOpenKey, (void**)&ntdll_NtOpenKey)) { return FALSE; }
    if (!inline_hook("ntdll.dll", "NtOpenKeyEx", SYSCALL_STUB_SIZE, (void*)x_NtOpenKeyEx, (void**)&ntdll_NtOpenKeyEx)) { return FALSE; }
    if (!inline_hook("ntdll.dll", "NtOpenKeyTransacted", SYSCALL_STUB_SIZE, (void*)x_NtOpenKeyTransacted, (void**)&ntdll_NtOpenKeyTransacted)) { return FALSE; }
    if (!inline_hook("ntdll.dll", "NtOpenKeyTransactedEx", SYSCALL_STUB_SIZE, (void*)x_NtOpenKeyTransactedEx, (void**)&ntdll_NtOpenKeyTransactedEx)) { return FALSE; }            
    if (!inline_hook("ntdll.dll", "NtQueryKey", SYSCALL_STUB_SIZE, (void*)x_NtQueryKey, (void**)&ntdll_NtQueryKey)) { return FALSE; }                
    if (!inline_hook("ntdll.dll", "NtFlushKey", SYSCALL_STUB_SIZE, (void*)x_NtFlushKey, (void**)&ntdll_NtFlushKey)) { return FALSE; }                
    if (!inline_hook("ntdll.dll", "NtDeleteKey", SYSCALL_STUB_SIZE, (void*)x_NtDeleteKey, (void**)&ntdll_NtDeleteKey)) { return FALSE; }                
    if (!inline_hook("ntdll.dll", "NtSetInformationKey", SYSCALL_STUB_SIZE, (void*)x_NtSetInformationKey, (void**)&ntdll_NtSetInformationKey)) { return FALSE; }                
    if (!inline_hook("ntdll.dll", "NtSetValueKey", SYSCALL_STUB_SIZE, (void*)x_NtSetValueKey, (void**)&ntdll_NtSetValueKey)) { return FALSE; }                
    if (!inline_hook("ntdll.dll", "NtQueryValueKey", SYSCALL_STUB_SIZE, (void*)x_NtQueryValueKey, (void**)&ntdll_NtQueryValueKey)) { return FALSE; }                
    if (!inline_hook("ntdll.dll", "NtDeleteValueKey", SYSCALL_STUB_SIZE, (void*)x_NtDeleteValueKey, (void**)&ntdll_NtDeleteValueKey)) { return FALSE; }                
    if (!inline_hook("ntdll.dll", "NtEnumerateKey", SYSCALL_STUB_SIZE, (void*)x_NtEnumerateKey, (void**)&ntdll_NtEnumerateKey)) { return FALSE; }                
    if (!inline_hook("ntdll.dll", "NtQueryMultipleValueKey", SYSCALL_STUB_SIZE, (void*)x_NtQueryMultipleValueKey, (void**)&ntdll_NtQueryMultipleValueKey)) { return FALSE; }     
   return 1;
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        return init_library();
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
