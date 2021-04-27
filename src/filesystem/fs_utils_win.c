#include <Windows.h>
#include "../common/ntmin/ntmin.h"
#include "../common/strutils.h"
#include "fs_utils_win.h"
#include <stdio.h>



typedef struct _MOUNTMGR_TARGET_NAME {
    USHORT DeviceNameLength;
    WCHAR  DeviceName[1];
} MOUNTMGR_TARGET_NAME, * PMOUNTMGR_TARGET_NAME;

typedef struct _MOUNTMGR_VOLUME_PATHS {
    ULONG MultiSzLength;
    WCHAR MultiSz[1];
}MOUNTMGR_VOLUME_PATHS, * PMOUNTMGR_VOLUME_PATHS;

#define MOUNTMGRCONTROLTYPE ((ULONG) 'm')
#define IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH \
    CTL_CODE(MOUNTMGRCONTROLTYPE, 12, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef union _ANY_BUFFER {
    MOUNTMGR_TARGET_NAME TargetName;
    MOUNTMGR_VOLUME_PATHS TargetPaths;
    FILE_NAME_INFORMATION NameInfo;
    UNICODE_STRING UnicodeString;
    WCHAR Buffer[USHRT_MAX];
}ANY_BUFFER;


#define STATUS_OBJECT_NAME_NOT_FOUND 0xC0000034
#define STATUS_OBJECT_PATH_NOT_FOUND 0xC000003A



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


NTSTATUS wrapped_createdir(wchar_t* path){
    OBJECT_ATTRIBUTES pathOa = {0x00};
    UNICODE_STRING uPath;
    RtlInitUnicodeString(&uPath,path);
    pathOa.Length = sizeof(pathOa);
    pathOa.ObjectName = &uPath;
    pathOa.Attributes = OBJ_CASE_INSENSITIVE;
    pathOa.RootDirectory = NULL;
    IO_STATUS_BLOCK IoSb = {0};
    DWORD desired_access = FILE_READ_DATA | FILE_LIST_DIRECTORY | SYNCHRONIZE;
    HANDLE hObject = 0;
    #ifdef BYPASS_ENABLED
    desired_access |= FLAG_BYPASS;
    #endif

    NTSTATUS status = NtCreateFile( &hObject, desired_access, &pathOa,&IoSb, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_CREATE, FILE_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0 );
    if(hObject){
    NtClose(hObject);
    }

    return status;
}

NTSTATUS wrapped_createfile(wchar_t* path,PHANDLE hObject, DWORD add_create_options){
    OBJECT_ATTRIBUTES pathOa = {0x00};
    UNICODE_STRING uPath;
    RtlInitUnicodeString(&uPath,path);
    pathOa.Length = sizeof(pathOa);
    pathOa.ObjectName = &uPath;
    pathOa.Attributes = OBJ_CASE_INSENSITIVE;
    pathOa.RootDirectory = NULL;
    DWORD create_options = FILE_OPEN_FOR_BACKUP_INTENT | FILE_SYNCHRONOUS_IO_NONALERT;
    create_options += add_create_options;
    IO_STATUS_BLOCK IoSb = {0};
    DWORD desired_access = FILE_READ_ATTRIBUTES | SYNCHRONIZE;
    
    #ifdef BYPASS_ENABLED
    desired_access |= FLAG_BYPASS;
    #endif
    return NtCreateFile(hObject, desired_access , &pathOa,&IoSb,NULL, 0, 0, FILE_OPEN,  create_options, NULL, 0);
}

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

BOOL get_device_root_string(wchar_t* in_path, wchar_t* out_path, unsigned int *tail_offset) {
    wchar_t* dptr = in_path;
    size_t device_root_size = 0;
    dptr = wcsstr(dptr, L"\\");
    if (!dptr) { return FALSE; }
    dptr++;
    dptr = wcsstr(dptr, L"\\");
    if (!dptr) { return FALSE; }
    dptr++;
    dptr = wcsstr(dptr, L"\\");
    if (!dptr) { return FALSE; }
    device_root_size = dptr - in_path;
    *tail_offset = device_root_size+1;
    memcpy((unsigned char*)out_path, (unsigned char*)in_path, device_root_size*2);
    return TRUE;
}

BOOL get_mpm_handle(PHANDLE hp) {
    OBJECT_ATTRIBUTES mpmOa = {0x00};
    IO_STATUS_BLOCK IoSb = {0x00};
    UNICODE_STRING mpmPath;
    RtlInitUnicodeString(&mpmPath, L"\\??\\MountPointManager");
    mpmOa.Length = sizeof(mpmOa);
    mpmOa.ObjectName = &mpmPath;
    mpmOa.Attributes = OBJ_CASE_INSENSITIVE;
    mpmOa.RootDirectory = NULL;    
    DWORD desired_access = FILE_READ_ATTRIBUTES | SYNCHRONIZE;
    #ifdef BYPASS_ENABLED
    desired_access |= FLAG_BYPASS;
    #endif
    if(NtCreateFile(hp, desired_access, &mpmOa, &IoSb, NULL, 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0)){return FALSE;}
    return TRUE;
}


BOOL get_dos_mountpoint_from_device(wchar_t* device_path, wchar_t* dos_mountpoint) {
    HANDLE hmpm = INVALID_HANDLE_VALUE;
    get_mpm_handle(&hmpm);
    if (!hmpm || hmpm == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    // Step 5: DeviceIOControl to query dos volume, get the drive letter string.
    IO_STATUS_BLOCK iosb;
    DWORD bytesReturned;
    ANY_BUFFER nameMnt;
    nameMnt.TargetName.DeviceNameLength = (USHORT)(2 * wcslen(device_path));
    wcscpy(nameMnt.TargetName.DeviceName, device_path);
    NTSTATUS res = NtDeviceIoControlFile(hmpm, NULL, 0, NULL, &iosb, IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH, &nameMnt, sizeof(nameMnt), &nameMnt, sizeof(nameMnt));
    NtClose(hmpm);
    if (res || !nameMnt.TargetPaths.MultiSzLength) {
        return FALSE;
    }

    wcscat(dos_mountpoint, nameMnt.TargetPaths.MultiSz);
    wcscat(dos_mountpoint, L"\\");
    return TRUE;
}

int get_abspath_from_handle(HANDLE hObject, wchar_t* in_path, wchar_t* out_path){
    // Bypass if there isn't a valid handle.
    if (!hObject || hObject == INVALID_HANDLE_VALUE) {
        wcscpy(out_path,in_path);
        return FALSE;
    }

    // STEP 1: Use NtQueryObject to get full device path.
    POBJECT_NAME_INFORMATION ObjectInfo = NULL;
    if (!get_device_path_from_handle(hObject, &ObjectInfo) || !ObjectInfo) {
        wcscpy(out_path,in_path);
        return FALSE;
    }

    // Step 2: Separate the device root path from the path.
    wchar_t device_root_name[64] = { 0x00 };
    unsigned int tail_offset = 0;
    if (!get_device_root_string(ObjectInfo->Name.Buffer, device_root_name, &tail_offset)) {
        free(ObjectInfo);
        wcscpy(out_path,in_path);
        return FALSE;
    }

    // Step 3: Get the mounted DOS path from the device root.
    wchar_t* working_path = (wchar_t*)calloc(1, X_MAX_PATH);
    if (!working_path) { 
        free(ObjectInfo);        
        wcscpy(out_path,in_path); 
        return FALSE; 
    }
    wcscat(working_path, L"\\??\\");
    if (!get_dos_mountpoint_from_device(device_root_name, working_path)) {
        free(ObjectInfo);
        wcscpy(out_path,in_path); 
        return FALSE;
    }

    // Step 4: Construct our full DOS path
    if (tail_offset) {
        wcscat(working_path, ObjectInfo->Name.Buffer + tail_offset);
    }
    if (in_path) {
        wcscat(working_path, L"\\");
        wcscat(working_path, in_path);
    }
    free(ObjectInfo);
    wcscpy(out_path,working_path); 
    free(working_path);
    
    return TRUE;
}

void resolve_abspath_native(wchar_t* in_path, wchar_t* out_path, int follow_symlinks){
    HANDLE hObject = 0;
    DWORD add_flags = 0;
    if(!follow_symlinks){add_flags |= FILE_FLAG_OPEN_REPARSE_POINT;}
    if(wrapped_createfile(in_path,&hObject,add_flags)){return;}
    get_abspath_from_handle(hObject,NULL,out_path);
    NtClose(hObject);
}

void resolve_abspath(char* in_path, char* out_path, int follow_symlinks){
    wchar_t w_in_path[1024] = {0x00};
    wchar_t w_out_path[1024] = {0x00};
    ChartoWideChar(in_path,w_in_path);
    resolve_abspath_native(w_in_path,w_out_path,follow_symlinks);
    WideChartoChar(w_out_path,out_path);
    to_lowercase(out_path);
}

int path_exists_native(wchar_t* path){
    HANDLE hObject = 0;
    NTSTATUS status = wrapped_createfile(path,&hObject,0);
    if(hObject){NtClose(hObject);}
    if(status == STATUS_OBJECT_NAME_NOT_FOUND || status == STATUS_OBJECT_PATH_NOT_FOUND){return 0;}
    return 1;
}

int path_exists(char* in_path){
    wchar_t w_in_path[1024] = {0x00};    
    ChartoWideChar(in_path,w_in_path);   
    return path_exists_native(w_in_path);
}

int path_is_symlink_native(wchar_t* path){
    HANDLE hObject = 0;
    if(wrapped_createfile(path,&hObject,FILE_FLAG_OPEN_REPARSE_POINT)){return 0;}
    IO_STATUS_BLOCK IoSb = {0};
    FILE_ATTRIBUTE_TAG_INFORMATION tag_info = {0x00};
    NtQueryInformationFile(hObject, &IoSb, &tag_info, sizeof(tag_info), FileAttributeTagInformation );
    NtClose(hObject);
    if(tag_info.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT){return 1;}
    return 0;
}

int path_is_symlink(char* in_path){
    wchar_t w_in_path[1024] = {0x00};    
    ChartoWideChar(in_path,w_in_path);   
    return path_is_symlink_native(w_in_path);
}

void makedir(char* path){
    wchar_t w_in_path[1024] = {0x00};    
    ChartoWideChar(path,w_in_path);   
    wrapped_createdir(w_in_path);
}