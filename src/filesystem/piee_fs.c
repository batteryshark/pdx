// Filesystem Redirection for PIEE
#include <Windows.h>
#include "../common/ntmin/ntmin.h"

#include "fs_glue.h"

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


// Bindings
NTSTATUS __stdcall x_NtCreateFile(PHANDLE FileHandle, DWORD DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize, ULONG FileAttributes, ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PVOID EaBuffer, ULONG EaLength) {

    if (!(DesiredAccess & FLAG_BYPASS) && ObjectAttributes && ObjectAttributes->ObjectName && ObjectAttributes->ObjectName->Length) {

        UNICODE_STRING redirected_path = {0};

        BOOL is_read = get_is_read(DesiredAccess);
        BOOL is_write = get_is_write(DesiredAccess);
        BOOL is_directory = get_is_directory(CreateOptions);

        if (nt_resolve_path(ObjectAttributes->ObjectName->Buffer, ObjectAttributes->ObjectName->Length, ObjectAttributes->RootDirectory, is_directory, is_read, is_write, CreateDisposition, &redirected_path)) {

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

            BOOL is_read = get_is_read(DesiredAccess);
            BOOL is_write = get_is_write(DesiredAccess);
            BOOL is_directory = get_is_directory(OpenOptions);

            if (nt_resolve_path(ObjectAttributes->ObjectName->Buffer, ObjectAttributes->ObjectName->Length, ObjectAttributes->RootDirectory, is_directory, is_read, is_write,0, &redirected_path)) {

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
        BOOL is_read = get_is_read(DesiredAccess);
        BOOL is_write = get_is_write(DesiredAccess);
        BOOL is_directory = TRUE;


        if (nt_resolve_path(ObjectAttributes->ObjectName->Buffer, ObjectAttributes->ObjectName->Length, ObjectAttributes->RootDirectory, is_directory, is_read, is_write,0, &redirected_path)) {

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
        BOOL is_read = get_is_read(DesiredAccess);
        BOOL is_write = get_is_write(DesiredAccess);
        BOOL is_directory = TRUE;

        if (nt_resolve_path(ObjectAttributes->ObjectName->Buffer, ObjectAttributes->ObjectName->Length, ObjectAttributes->RootDirectory, is_directory, is_read, is_write,0, &redirected_path)) {

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
        BOOL is_read = get_is_read(DesiredAccess);
        BOOL is_write = get_is_write(DesiredAccess);
        BOOL is_directory = TRUE;

        if (nt_resolve_path(ObjectAttributes->ObjectName->Buffer, ObjectAttributes->ObjectName->Length, ObjectAttributes->RootDirectory, is_directory, is_read, is_write,0, &redirected_path)) {

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
            BOOL is_read = TRUE;
            BOOL is_write = FALSE;
            BOOL is_directory = FALSE;


            if (nt_resolve_path(ObjectAttributes->ObjectName->Buffer, ObjectAttributes->ObjectName->Length, ObjectAttributes->RootDirectory, is_directory, is_read, is_write,0, &redirected_path)) {

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
            BOOL is_read = TRUE;
            BOOL is_write = FALSE;
            BOOL is_directory = FALSE;

            if (nt_resolve_path(ObjectAttributes->ObjectName->Buffer, ObjectAttributes->ObjectName->Length, ObjectAttributes->RootDirectory, is_directory, is_read, is_write,0, &redirected_path)) {

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
        if (nt_resolve_path(tmod, (unsigned int)wcslen(tmod) * 2, NULL, FALSE, TRUE, FALSE,0, &redirected_path)) {
            NTSTATUS status = ntdll_LdrLoadDll(PathToFile, Flags, &redirected_path, ModuleHandle);
            RtlFreeUnicodeString(&redirected_path);
            return status;
        }
    }

    return  ntdll_LdrLoadDll(PathToFile, Flags, ModuleFileName, ModuleHandle);
}