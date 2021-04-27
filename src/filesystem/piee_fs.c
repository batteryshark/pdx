// Filesystem Redirection for PIEE
#include "fs_utils_win.h"
#include "fs_glue.h"
#include "../common/mem.h"
#ifdef TARGET_OS_WINDOWS
#include <Windows.h>
#include "../common/ntmin/ntmin.h"

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

// Helpers - Windows
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
    if(flags & FILE_CREATE){return TRUE;}
    return FALSE;
}

BOOL get_fail_if_not_exists(DWORD flags){
    if(flags & FILE_OPEN){return TRUE;}
    if(flags & FILE_OVERWRITE){return TRUE;}
    return FALSE;
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

#else
// TODO - LINUX STUFF HERE
#endif

int init_library(void){   
   
    // Perform any Syscall Hooks we Need at This Level
    #ifdef TARGET_OS_WINDOWS
    if (!inline_hook("ntdll.dll", "NtCreateFile", SYSCALL_STUB_SIZE, (void*)x_NtCreateFile, (void**)&ntdll_NtCreateFile)) { return FALSE; }
    if (!inline_hook("ntdll.dll", "NtOpenFile", SYSCALL_STUB_SIZE, (void*)x_NtOpenFile, (void**)&ntdll_NtOpenFile)) { return FALSE; }
    if (!inline_hook("ntdll.dll", "NtOpenDirectoryObject", SYSCALL_STUB_SIZE, (void*)x_NtOpenDirectoryObject, (void**)&ntdll_NtOpenDirectoryObject)) { return FALSE; }
    if (!inline_hook("ntdll.dll", "NtCreateDirectoryObject", SYSCALL_STUB_SIZE, (void*)x_NtCreateDirectoryObject, (void**)&ntdll_NtCreateDirectoryObject)) { return FALSE; }
    if (!inline_hook("ntdll.dll", "NtCreateDirectoryObjectEx", SYSCALL_STUB_SIZE, (void*)x_NtCreateDirectoryObjectEx, (void**)&ntdll_NtCreateDirectoryObjectEx)) { return FALSE; }
    if (!inline_hook("ntdll.dll", "NtQueryAttributesFile", SYSCALL_STUB_SIZE, (void*)x_NtQueryAttributesFile, (void**)&ntdll_NtQueryAttributesFile)) { return FALSE; }
    if (!inline_hook("ntdll.dll", "NtQueryFullAttributesFile", SYSCALL_STUB_SIZE, (void*)x_NtQueryFullAttributesFile, (void**)&ntdll_NtQueryFullAttributesFile)) { return FALSE; }    

    #ifdef TARGET_ARCH_64
        if (!inline_hook("ntdll.dll", "LdrLoadDll", 0x10, (void*)x_LdrLoadDll, (void**)&ntdll_LdrLoadDll)) { return FALSE; }	
    #else
        if (!inline_hook("ntdll.dll", "LdrLoadDll", 0x0E, (void*)x_LdrLoadDll, (void**)&ntdll_LdrLoadDll)) {  return FALSE; }
    #endif
    #else // TODO
    /*
        static int (*real_open)(const char *path, int oflag) = NULL;
        static int (*real_access)(const char *pathname, int mode) = NULL;
        static int (*real_ioctl)(int fd, unsigned long request, void* data) = NULL;
        static ssize_t (*real_readlink)(const char *restrict path, char *restrict buf, size_t bufsize) = NULL;
        static int (*real_mkdir)(const char *path, int mode) = NULL;
        static FILE *(*real_fopen)(const char *filename, const char *mode) = NULL;
        static void* (*real_opendir)(const char *name) = NULL;
        static int (*real_openat)(int dirfd, const char *pathname, int flags) = NULL;
        static int (*real__lxstat)(int ver, const char * path, void * stat_buf) = NULL;
        static int (*real__xstat)(int ver, const char * path, void * stat_buf) = NULL;
        static int (*real_rename)(const char *old, const char *new) = NULL;
        static int (*real_remove)(const char *) = NULL;
    */
    #endif
   return 1;
}

#ifdef TARGET_OS_WINDOWS
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
#else
void __attribute__((constructor)) initialize(void){init_library();}
#endif