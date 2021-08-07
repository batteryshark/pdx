// Filesystem Redirection Library
#include <windows.h>
#include "../common/ntmin/ntmin.h"
#include "../common/mem.h"
#include "../../shared/dbg.h"
#include "../../shared/fs_redirect.h"
#include "fs_utils.h"



typedef NTSTATUS __stdcall tNtCreateFile(PHANDLE FileHandle, DWORD DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize, ULONG FileAttributes, ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PVOID EaBuffer, ULONG EaLength);
typedef NTSTATUS __stdcall tNtOpenFile(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, ULONG ShareAccess, ULONG OpenOptions);
typedef NTSTATUS __stdcall tNtOpenDirectoryObject(PHANDLE DirectoryHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes);
typedef NTSTATUS __stdcall tNtCreateDirectoryObject(PHANDLE DirectoryHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes);
typedef NTSTATUS __stdcall tNtCreateDirectoryObjectEx(PHANDLE DirectoryHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, HANDLE ShadowDirectoryHandle, ULONG Flags);
typedef NTSTATUS __stdcall tNtQueryAttributesFile(POBJECT_ATTRIBUTES ObjectAttributes, PFILE_BASIC_INFORMATION FileInformation);
typedef NTSTATUS __stdcall tNtQueryFullAttributesFile(POBJECT_ATTRIBUTES ObjectAttributes, PVOID Attributes);
typedef NTSTATUS __stdcall tLdrLoadDll(PWCHAR PathToFile, PULONG Flags, PUNICODE_STRING ModuleFileName, PHANDLE ModuleHandle);

static tNtCreateFile* ntdll_NtCreateFile = NULL;
static tNtOpenFile* ntdll_NtOpenFile = NULL;
static tNtOpenDirectoryObject* ntdll_NtOpenDirectoryObject = NULL;
static tNtCreateDirectoryObject* ntdll_NtCreateDirectoryObject = NULL;
static tNtCreateDirectoryObjectEx* ntdll_NtCreateDirectoryObjectEx = NULL;
static tNtQueryAttributesFile* ntdll_NtQueryAttributesFile = NULL;
static tNtQueryFullAttributesFile* ntdll_NtQueryFullAttributesFile = NULL;
static tLdrLoadDll* ntdll_LdrLoadDll = NULL;

// Helpers
BOOL get_is_read(DWORD DesiredAccess) {
    if (DesiredAccess & 1) { return TRUE; }
    if (DesiredAccess & 4) { return TRUE; }
    if (DesiredAccess & 0x20) { return TRUE; }
    if (DesiredAccess & 0x80) { return TRUE; }
    if (DesiredAccess & READ_CONTROL) {return TRUE;}
    if (DesiredAccess & GENERIC_READ) { return TRUE; }
    if (DesiredAccess & GENERIC_EXECUTE) { return TRUE; }
    return FALSE;
}

BOOL get_is_write(DWORD DesiredAccess) {
    if (DesiredAccess & 2) { return TRUE; }
    if (DesiredAccess & 4) { return TRUE; }
    if (DesiredAccess & 0x40) { return TRUE; }
    if (DesiredAccess & 0x100) { return TRUE; }
    if (DesiredAccess & 0x00010000) { return TRUE; }
    if (DesiredAccess & GENERIC_WRITE) { return TRUE; }
    return FALSE;
}

BOOL get_is_directory(DWORD options){
    return options & FILE_DIRECTORY_FILE;
}

BOOL get_fail_if_exists(DWORD flags){
    if(flags == FILE_CREATE){return TRUE;}
    return FALSE;
}

BOOL get_fail_if_not_exists(DWORD flags){
    if(flags == FILE_OPEN){return TRUE;}
    if(flags == FILE_OVERWRITE){return TRUE;}
    return FALSE;
}

int fs_redirect_nt(wchar_t* src_buffer, unsigned int src_len, HANDLE root_handle, int is_directory, int is_read, int is_write,int fail_if_exist, int fail_if_not_exist, PUNICODE_STRING unicodestr_redirected_path){
    
    // Make a copy of our instr to ensure it is properly truncated with nulls.
    wchar_t* src_clean_buffer = (wchar_t*)calloc(1,src_len+2);
    memcpy(src_clean_buffer,src_buffer,src_len);
    
        
    // Resolve an Absolute Path to our Inpath
    wchar_t* w_abspath = calloc(1,X_MAX_PATH*2);
    char* abspath = calloc(1,X_MAX_PATH);
    get_abspath_from_handle(root_handle, src_clean_buffer, w_abspath);
    free(src_clean_buffer); 
    WideChartoChar(w_abspath,abspath);
    free(w_abspath);
    // If a target doesn't have a DOS drive for now, we'll bypass.
    if(!strstr(abspath,":")){ free(abspath); return 0;}

    // Send Path to our internal fs_redirect()
    char* redirected_path = NULL;
    int result = fs_redirect(abspath, is_directory, is_read, is_write, fail_if_exist, fail_if_not_exist, &redirected_path);
    
    if(!result){DBG_printf("Result: Bypass %s",abspath); free(abspath); return 0;}
    free(abspath);
    // Handle bypass responses
    if(redirected_path){
    ChartoUnicodeString(redirected_path,unicodestr_redirected_path);
    free(redirected_path);
    DBG_printf("Result: Redirect");
    }

    return 1;
}

// Bindings
NTSTATUS __stdcall x_NtCreateFile(PHANDLE FileHandle, DWORD DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize, ULONG FileAttributes, ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PVOID EaBuffer, ULONG EaLength) {

    if (!(DesiredAccess & FLAG_BYPASS) && ObjectAttributes && ObjectAttributes->ObjectName && ObjectAttributes->ObjectName->Length) {

        UNICODE_STRING redirected_path = {0};

                
        if (fs_redirect_nt(ObjectAttributes->ObjectName->Buffer, 
                ObjectAttributes->ObjectName->Length, 
                ObjectAttributes->RootDirectory, 
                get_is_directory(CreateOptions), 
                get_is_read(DesiredAccess), 
                get_is_write(DesiredAccess), 
                get_fail_if_exists(CreateDisposition),
                get_fail_if_not_exists(CreateDisposition),
                &redirected_path)) {
            
            PUNICODE_STRING orig_name = ObjectAttributes->ObjectName;
            HANDLE orig_root = ObjectAttributes->RootDirectory;
            ObjectAttributes->RootDirectory = NULL;
            ObjectAttributes->ObjectName = &redirected_path;

            NTSTATUS status = ntdll_NtCreateFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);

            RtlFreeUnicodeString(&redirected_path);
            ObjectAttributes->ObjectName = orig_name;
            ObjectAttributes->RootDirectory = orig_root;
            return status;
        }
    }

    if(DesiredAccess & FLAG_BYPASS){
        DesiredAccess &= ~FLAG_BYPASS;
    }
    return ntdll_NtCreateFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);
}


NTSTATUS __stdcall x_NtOpenFile(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, ULONG ShareAccess, ULONG OpenOptions) {
        
        if (!(DesiredAccess & FLAG_BYPASS) && ObjectAttributes && ObjectAttributes->ObjectName && ObjectAttributes->ObjectName->Length) {

        UNICODE_STRING redirected_path = {0};
        if (fs_redirect_nt(ObjectAttributes->ObjectName->Buffer, 
                ObjectAttributes->ObjectName->Length, 
                ObjectAttributes->RootDirectory, 
                get_is_directory(OpenOptions), 
                get_is_read(DesiredAccess), 
                get_is_write(DesiredAccess), 
                0,
                0,
                &redirected_path)) {
                PUNICODE_STRING orig_name = ObjectAttributes->ObjectName;
                PVOID orig_root = ObjectAttributes->RootDirectory;
                ObjectAttributes->RootDirectory = NULL;
                ObjectAttributes->ObjectName = &redirected_path;

                NTSTATUS status = ntdll_NtOpenFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, OpenOptions);

                RtlFreeUnicodeString(&redirected_path);
                ObjectAttributes->ObjectName = orig_name;
                ObjectAttributes->RootDirectory = orig_root;
                return status;
            }
        }

    if(DesiredAccess & FLAG_BYPASS){
            DesiredAccess &= ~FLAG_BYPASS;
    }
    return ntdll_NtOpenFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, OpenOptions);
}

NTSTATUS __stdcall x_NtOpenDirectoryObject(PHANDLE DirectoryHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes) {

    if (ObjectAttributes && ObjectAttributes->ObjectName) {
        UNICODE_STRING redirected_path = {0};        
        if (fs_redirect_nt(ObjectAttributes->ObjectName->Buffer, ObjectAttributes->ObjectName->Length, ObjectAttributes->RootDirectory, TRUE, get_is_read(DesiredAccess), get_is_write(DesiredAccess),0,0, &redirected_path)) {

            PUNICODE_STRING orig_name = ObjectAttributes->ObjectName;
            PVOID orig_root = ObjectAttributes->RootDirectory;
            ObjectAttributes->RootDirectory = NULL;
            ObjectAttributes->ObjectName = &redirected_path;

            NTSTATUS status = ntdll_NtOpenDirectoryObject(DirectoryHandle, DesiredAccess, ObjectAttributes);

            RtlFreeUnicodeString(&redirected_path);
            ObjectAttributes->ObjectName = orig_name;
            ObjectAttributes->RootDirectory = orig_root;
            return status;
        }

    }
    return ntdll_NtOpenDirectoryObject(DirectoryHandle, DesiredAccess, ObjectAttributes);
}

NTSTATUS __stdcall x_NtCreateDirectoryObject(PHANDLE DirectoryHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes) {
    if (ObjectAttributes && ObjectAttributes->ObjectName) {
        UNICODE_STRING redirected_path = {0};

        if (fs_redirect_nt(ObjectAttributes->ObjectName->Buffer, ObjectAttributes->ObjectName->Length, ObjectAttributes->RootDirectory, TRUE, get_is_read(DesiredAccess), get_is_write(DesiredAccess),0,0, &redirected_path)) {

            PUNICODE_STRING orig_name = ObjectAttributes->ObjectName;
            PVOID orig_root = ObjectAttributes->RootDirectory;
            ObjectAttributes->RootDirectory = NULL;
            ObjectAttributes->ObjectName = &redirected_path;

            NTSTATUS status = ntdll_NtCreateDirectoryObject(DirectoryHandle, DesiredAccess, ObjectAttributes);
            
            RtlFreeUnicodeString(&redirected_path);
            ObjectAttributes->ObjectName = orig_name;
            ObjectAttributes->RootDirectory = orig_root;
            return status;
        }

    }
    return ntdll_NtCreateDirectoryObject(DirectoryHandle, DesiredAccess, ObjectAttributes);
}

NTSTATUS __stdcall x_NtCreateDirectoryObjectEx(PHANDLE DirectoryHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, HANDLE ShadowDirectoryHandle, ULONG Flags) {
    if (ObjectAttributes && ObjectAttributes->ObjectName) {
        UNICODE_STRING redirected_path = {0};


        if (fs_redirect_nt(ObjectAttributes->ObjectName->Buffer, ObjectAttributes->ObjectName->Length, ObjectAttributes->RootDirectory, TRUE, get_is_read(DesiredAccess), get_is_write(DesiredAccess),0,0, &redirected_path)) {

            PUNICODE_STRING orig_name = ObjectAttributes->ObjectName;
            PVOID orig_root = ObjectAttributes->RootDirectory;
            ObjectAttributes->RootDirectory = NULL;
            ObjectAttributes->ObjectName = &redirected_path;

            NTSTATUS status = ntdll_NtCreateDirectoryObjectEx(DirectoryHandle, DesiredAccess, ObjectAttributes,ShadowDirectoryHandle,Flags);

            RtlFreeUnicodeString(&redirected_path);
            ObjectAttributes->ObjectName = orig_name;
            ObjectAttributes->RootDirectory = orig_root;
            return status;
        }

    }
    return ntdll_NtCreateDirectoryObjectEx(DirectoryHandle, DesiredAccess, ObjectAttributes, ShadowDirectoryHandle, Flags);
}

NTSTATUS __stdcall x_NtQueryAttributesFile(POBJECT_ATTRIBUTES ObjectAttributes, PFILE_BASIC_INFORMATION FileInformation) {
    if (ObjectAttributes) {
        if (ObjectAttributes->ObjectName) {
            UNICODE_STRING redirected_path = {0};

            if (fs_redirect_nt(ObjectAttributes->ObjectName->Buffer, ObjectAttributes->ObjectName->Length, ObjectAttributes->RootDirectory, FALSE,TRUE, FALSE,0,0, &redirected_path)) {

                PUNICODE_STRING orig_name = ObjectAttributes->ObjectName;
                PVOID orig_root = ObjectAttributes->RootDirectory;
                ObjectAttributes->RootDirectory = NULL;
                ObjectAttributes->ObjectName = &redirected_path;

                NTSTATUS status = ntdll_NtQueryAttributesFile(ObjectAttributes, FileInformation);

                
                RtlFreeUnicodeString(&redirected_path);
                ObjectAttributes->ObjectName = orig_name;
                ObjectAttributes->RootDirectory = orig_root;
                return status;
            }
        }
    }

    return ntdll_NtQueryAttributesFile(ObjectAttributes, FileInformation);
}

NTSTATUS __stdcall x_NtQueryFullAttributesFile(POBJECT_ATTRIBUTES ObjectAttributes, PVOID Attributes) {
    if (ObjectAttributes) {
        if (ObjectAttributes->ObjectName) {
            UNICODE_STRING redirected_path = {0};
            
            if (fs_redirect_nt(ObjectAttributes->ObjectName->Buffer, ObjectAttributes->ObjectName->Length, ObjectAttributes->RootDirectory, FALSE, TRUE, FALSE,0,0, &redirected_path)) {

                PUNICODE_STRING orig_name = ObjectAttributes->ObjectName;
                PVOID orig_root = ObjectAttributes->RootDirectory;
                ObjectAttributes->RootDirectory = NULL;
                ObjectAttributes->ObjectName = &redirected_path;

                NTSTATUS status = ntdll_NtQueryFullAttributesFile(ObjectAttributes, Attributes);

                RtlFreeUnicodeString(&redirected_path);
                ObjectAttributes->ObjectName = orig_name;
                ObjectAttributes->RootDirectory = orig_root;
                return status;
            }
        }
    }
    return ntdll_NtQueryFullAttributesFile(ObjectAttributes, Attributes);
}

NTSTATUS __stdcall x_LdrLoadDll(PWCHAR PathToFile, ULONG *Flags, PUNICODE_STRING ModuleFileName, PHANDLE ModuleHandle) {

    UNICODE_STRING redirected_path = {0};
    wchar_t tmod[2048] = { 0x00 };
    if(ModuleFileName && ModuleFileName->Buffer && ModuleFileName->Length){
        //wcscat(tmod,PathToFile);
        memcpy(tmod + wcslen(tmod) * 2, ModuleFileName->Buffer, ModuleFileName->Length);
        if (!wcsstr(tmod, L".")) {
            wcscat(tmod, L".dll");
        }
        if (fs_redirect_nt(tmod, (unsigned int)wcslen(tmod) * 2, NULL, FALSE, TRUE, FALSE,0,0, &redirected_path)) {
            NTSTATUS status = ntdll_LdrLoadDll(PathToFile, Flags, &redirected_path, ModuleHandle);
            RtlFreeUnicodeString(&redirected_path);
            return status;
        }
    }

    return  ntdll_LdrLoadDll(PathToFile, Flags, ModuleFileName, ModuleHandle);
}


int init_library(void){

    if (!inline_hook("ntdll.dll", "NtCreateFile", SYSCALL_STUB_SIZE, (void*)x_NtCreateFile, (void**)&ntdll_NtCreateFile)) { return FALSE; }
    if (!inline_hook("ntdll.dll", "NtOpenFile", SYSCALL_STUB_SIZE, (void*)x_NtOpenFile, (void**)&ntdll_NtOpenFile)) { return FALSE; }
    if (!inline_hook("ntdll.dll", "NtOpenDirectoryObject", SYSCALL_STUB_SIZE, (void*)x_NtOpenDirectoryObject, (void**)&ntdll_NtOpenDirectoryObject)) { return FALSE; }
    if (!inline_hook("ntdll.dll", "NtCreateDirectoryObject", SYSCALL_STUB_SIZE, (void*)x_NtCreateDirectoryObject, (void**)&ntdll_NtCreateDirectoryObject)) { return FALSE; }
    if (!inline_hook("ntdll.dll", "NtCreateDirectoryObjectEx", SYSCALL_STUB_SIZE, (void*)x_NtCreateDirectoryObjectEx, (void**)&ntdll_NtCreateDirectoryObjectEx)) { return FALSE; }
    if (!inline_hook("ntdll.dll", "NtQueryAttributesFile", SYSCALL_STUB_SIZE, (void*)x_NtQueryAttributesFile, (void**)&ntdll_NtQueryAttributesFile)) { return FALSE; }
    if (!inline_hook("ntdll.dll", "NtQueryFullAttributesFile", SYSCALL_STUB_SIZE, (void*)x_NtQueryFullAttributesFile, (void**)&ntdll_NtQueryFullAttributesFile)) { return FALSE; }    

    #if __x86_64__
        if (!inline_hook("ntdll.dll", "LdrLoadDll", 0x10, (void*)x_LdrLoadDll, (void**)&ntdll_LdrLoadDll)) { return FALSE; }	
    #else
    if (!inline_hook("ntdll.dll", "LdrLoadDll", 0x08, (void*)x_LdrLoadDll, (void**)&ntdll_LdrLoadDll)) {  return FALSE; }
    #endif


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
