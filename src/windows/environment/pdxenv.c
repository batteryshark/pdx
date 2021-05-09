#include <windows.h>
#include <string.h>
#include "../common/iniparser/iniparser.h"
#include "../common/iniparser/dictionary.h"
#include "../common/mem.h"
#include "../../shared/dbg.h"

typedef BOOL __stdcall tGetUserNameA(LPSTR lpBuffer, LPDWORD pcbBuffer);
typedef BOOL __stdcall tGetUserNameW(LPWSTR lpBuffer, LPDWORD pcbBuffer);
typedef UINT __stdcall tGetDriveTypeA(LPCSTR lpRootPathName);
typedef UINT __stdcall tGetDriveTypeW(LPCWSTR lpRootPathName);
typedef DWORD __stdcall tGetLogicalDriveStringsA(DWORD nBufferLength, LPSTR lpBuffer);
typedef DWORD __stdcall tGetLogicalDriveStringsW(DWORD nBufferLength, LPWSTR lpBuffer);
typedef BOOL tGetVolumeInformationA(LPCSTR lpRootPathName,LPSTR lpVolumeNameBuffer, DWORD nVolumeNameSize, LPDWORD lpVolumeSerialNumber,LPDWORD lpMaximumComponentLength, LPDWORD lpFileSystemFlags, LPSTR lpFileSystemNameBuffer, DWORD nFileSystemNameSize);
typedef BOOL tGetVolumeInformationW(LPCWSTR lpRootPathName,LPWSTR lpVolumeNameBuffer,DWORD nVolumeNameSize,LPDWORD lpVolumeSerialNumber,LPDWORD lpMaximumComponentLength,LPDWORD lpFileSystemFlags,LPWSTR lpFileSystemNameBuffer,DWORD nFileSystemNameSize);

static tGetUserNameA* advapi32_GetUserNameA = NULL;
static tGetUserNameW* advapi32_GetUserNameW = NULL;
static tGetDriveTypeA* k32_GetDriveTypeA = NULL;
static tGetDriveTypeW* k32_GetDriveTypeW = NULL;
static tGetLogicalDriveStringsA* k32_GetLogicalDriveStringsA = NULL;
static tGetLogicalDriveStringsW* k32_GetLogicalDriveStringsW = NULL;
tGetVolumeInformationA* k32_GetVolumeInformationA = NULL;
tGetVolumeInformationW* k32_GetVolumeInformationW = NULL;

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

typedef struct _USER_ENVIRONMENT{
    char username[128];
}USER_ENVRIONMENT,*PUSER_ENVIRONMENT;

typedef struct _DRIVE_MAP_ENTRY{
    unsigned int drive_type;
    unsigned int drive_serial;
    char volume_label[32];
    char volume_filesystem[8];
    unsigned int max_component_length;
    unsigned int flags;

}DRIVE_MAP_ENTRY,*PDRIVE_MAP_ENTRY;

void load_config(){
    get_config_file_path();
    config = iniparser_load(path_to_config_file);
}


const char* get_username(){
    // Load Our Environment Configuration
    if(!config){load_config();}    
    if(!config){return NULL;}
    return iniparser_getstring(config,"global:username",NULL);
}

int get_drive_map_info(char root_letter,PDRIVE_MAP_ENTRY map_entry){
    if(!config){load_config();}        
    if(!config){return 0;}
    
    char kval[128] = {0x00};
    sprintf(kval,"drive_map_%c:type",root_letter);    
    map_entry->drive_type = iniparser_getint(config, kval, -1);
    
    if(map_entry->drive_type == -1){return 0;}
    sprintf(kval,"drive_map_%c:serial",root_letter);
    map_entry->drive_serial = iniparser_getint(config, kval, 0);
    
    sprintf(kval,"drive_map_%c:label",root_letter);
    const char* s = NULL;
    s = iniparser_getstring(config, kval, "New Volume");
    strncpy(map_entry->volume_label,s,sizeof(map_entry->volume_label));
    sprintf(kval,"drive_map_%c:filesystem",root_letter);    
    s = iniparser_getstring(config, kval, "NTFS");
    strncpy(map_entry->volume_filesystem,s,sizeof(map_entry->volume_filesystem));
    sprintf(kval,"drive_map_%c:flags",root_letter);    
    map_entry->flags = iniparser_getint(config, kval, 0);
    sprintf(kval,"drive_map_%c:max_component_length",root_letter);    
    map_entry->max_component_length = iniparser_getint(config, kval, 32768);
    return 1;
}


int get_drive_strings(unsigned char* drivestrings_buffer, int is_unicode){
    if(!config){load_config();}        
    if(!config){return 0;}
    char kval[128] = {0x00};
    char dstrings[8] = {0x00};
    unsigned char wbuffer[8] = {0x00,0x00,0x3A, 0x00, 0x5C, 0x00, 0x00, 0x00};
    char buffer[4] = {0x00, 0x3A, 0x5C, 0x00};
    int offset = 0;
    for(int i=0x61;i<0x7B;i++){
        sprintf(kval,"drive_map_%c:type",(char)i);    
        if(iniparser_getint(config, kval, -1) != -1){
            if(is_unicode){                
                wbuffer[0] = i;
                memcpy(drivestrings_buffer+offset,wbuffer,sizeof(wbuffer));
                offset += sizeof(wbuffer);
            }else{
                buffer[0] = i;            
                memcpy(drivestrings_buffer+offset,buffer,sizeof(buffer));
                offset += sizeof(buffer);
            }
            
        }
    }
    return offset;
}

int is_letter(char val){
    if(val > 0x60 && val < 0x7B){return 1;}
    if(val > 0x40 && val < 0x5B){return 1;}
    return 0;
}

int lower(int argument){
    if (argument >= 'A' && argument <= 'Z')
        return argument + 'a' - 'A';
    else
        return argument;
}

char find_drive_letter(unsigned char* in_path, unsigned int in_len){
    for(int i=0;i<in_len;i++){
        if(in_path[i] == ':'){            
            return (char)lower(in_path[i-1]);
            }
    }
    return 0;
}


BOOL __stdcall x_GetUserNameA(LPSTR lpBuffer, LPDWORD pcbBuffer){
    if(!get_username()){return advapi32_GetUserNameA(lpBuffer, pcbBuffer);}
    const char* username = get_username();

    if(*pcbBuffer < strlen(username)){
        *pcbBuffer = (strlen(username)+1);
        SetLastError(ERROR_MORE_DATA);
        return FALSE;
    }
    *pcbBuffer = strlen(username);
    strcpy(lpBuffer,username);
    return TRUE;
}

BOOL __stdcall x_GetUserNameW(LPWSTR lpBuffer, LPDWORD pcbBuffer){
    // Guard for Lack of Config Option
    if(!get_username()){return advapi32_GetUserNameW(lpBuffer, pcbBuffer);}
    wchar_t w_username[128] = {0x00};
    MultiByteToWideChar(CP_ACP, 0, get_username(), -1, w_username, sizeof(w_username));
    if(*pcbBuffer < wcslen(w_username)){
        *pcbBuffer = (wcslen(w_username)+1)*2;
        SetLastError(ERROR_MORE_DATA);
        return FALSE;
    }
    *pcbBuffer = wcslen(w_username);
    wcscpy(lpBuffer,w_username);
    return TRUE;
}

UINT __stdcall x_GetDriveTypeA(LPCSTR lpRootPathName){
	DRIVE_MAP_ENTRY dme = {0x00};    
    char drive_letter = find_drive_letter((unsigned char*)lpRootPathName,strlen(lpRootPathName));
    if(!drive_letter || !get_drive_map_info(drive_letter,&dme)){
        return k32_GetDriveTypeA(lpRootPathName);
    }
	return dme.drive_type;
}

UINT __stdcall x_GetDriveTypeW(LPCWSTR lpRootPathName){
	DRIVE_MAP_ENTRY dme = {0x00};
    char drive_letter = find_drive_letter((unsigned char*)lpRootPathName,wcslen(lpRootPathName)*2);
    if(!drive_letter || !get_drive_map_info(drive_letter,&dme)){
        return k32_GetDriveTypeW(lpRootPathName);
    }
	return dme.drive_type;
}

DWORD __stdcall x_GetLogicalDriveStringsA(DWORD nBufferLength, LPSTR lpBuffer){
	unsigned char response[256] = {0x00};
    int binsz = get_drive_strings(response,0);
    if(lpBuffer){
			memcpy((unsigned char*)lpBuffer,response,binsz);
	}
	return binsz;
}

DWORD __stdcall x_GetLogicalDriveStringsW(DWORD nBufferLength, LPWSTR lpBuffer){
	unsigned char response[256] = {0x00};
    int binsz = get_drive_strings(response,1);
    if(lpBuffer){
			memcpy((unsigned char*)lpBuffer,response,binsz);
	}
	return binsz;
}


BOOL x_GetVolumeInformationA(LPCSTR lpRootPathName,LPSTR lpVolumeNameBuffer, DWORD nVolumeNameSize, LPDWORD lpVolumeSerialNumber,LPDWORD lpMaximumComponentLength, LPDWORD lpFileSystemFlags, LPSTR lpFileSystemNameBuffer, DWORD nFileSystemNameSize){
	DRIVE_MAP_ENTRY dme;
    if(!get_drive_map_info(lpRootPathName[0],&dme)){
        return k32_GetVolumeInformationA(lpRootPathName,lpVolumeNameBuffer, nVolumeNameSize, lpVolumeSerialNumber,lpMaximumComponentLength, lpFileSystemFlags, lpFileSystemNameBuffer, nFileSystemNameSize);
    }
    
    if(lpVolumeNameBuffer && nVolumeNameSize > strlen(dme.volume_label)){
        strcpy(lpVolumeNameBuffer,dme.volume_label);
    }
    
    if(lpVolumeSerialNumber){
        *lpVolumeSerialNumber = dme.drive_serial;
    }

    if(lpMaximumComponentLength){
        *lpMaximumComponentLength = dme.max_component_length;
    }

    if(lpFileSystemFlags){
        *lpFileSystemFlags = dme.flags;
    }

    if(lpFileSystemNameBuffer && nFileSystemNameSize > strlen(dme.volume_filesystem)){
        strcpy(lpFileSystemNameBuffer,dme.volume_filesystem);
    }
	
	return TRUE;
}

BOOL x_GetVolumeInformationW(LPCWSTR lpRootPathName,LPWSTR lpVolumeNameBuffer,DWORD nVolumeNameSize,LPDWORD lpVolumeSerialNumber,LPDWORD lpMaximumComponentLength,LPDWORD lpFileSystemFlags,LPWSTR lpFileSystemNameBuffer,DWORD nFileSystemNameSize){
	DRIVE_MAP_ENTRY dme;
    if(!get_drive_map_info(lpRootPathName[0],&dme)){
        return k32_GetVolumeInformationW(lpRootPathName,lpVolumeNameBuffer,nVolumeNameSize,lpVolumeSerialNumber,lpMaximumComponentLength,lpFileSystemFlags,lpFileSystemNameBuffer,nFileSystemNameSize);
    }

    wchar_t wvolume_label[32] = {0x00};
    MultiByteToWideChar(CP_ACP, 0, dme.volume_label, -1, wvolume_label, sizeof(wvolume_label));
    wchar_t wvolume_filesystem[32] = {0x00};
    MultiByteToWideChar(CP_ACP, 0, dme.volume_filesystem, -1, wvolume_filesystem, sizeof(wvolume_filesystem));


    if(lpVolumeNameBuffer && nVolumeNameSize > strlen(dme.volume_label)*2){
        wcscpy(lpVolumeNameBuffer,wvolume_label);
    }
    
    if(lpVolumeSerialNumber){
        *lpVolumeSerialNumber = dme.drive_serial;
    }

    if(lpMaximumComponentLength){
        *lpMaximumComponentLength = dme.max_component_length;
    }

    if(lpFileSystemFlags){
        *lpFileSystemFlags = dme.flags;
    }

    if(lpFileSystemNameBuffer && nFileSystemNameSize > strlen(dme.volume_filesystem)*2){
        wcscpy(lpFileSystemNameBuffer,wvolume_filesystem);
    }
	
	return TRUE;    
}


int init_library(void){   
   
    
    // Perform any Syscall Hooks we Need at This Level
    if (!inline_hook("advapi32.dll", "GetUserNameW", SYSCALL_STUB_SIZE, (void*)x_GetUserNameW, (void**)&advapi32_GetUserNameW)) { return FALSE; }
    if (!inline_hook("advapi32.dll", "GetUserNameA", SYSCALL_STUB_SIZE, (void*)x_GetUserNameA, (void**)&advapi32_GetUserNameA)) { return FALSE; }

    #if __x86_64__
    if (!inline_hook("kernel32.dll", "GetDriveTypeA", 0x10, (void*)x_GetDriveTypeA, (void**)&k32_GetDriveTypeA)) { return FALSE; }   
    if (!inline_hook("kernel32.dll", "GetDriveTypeW", 0x12, (void*)x_GetDriveTypeW, (void**)&k32_GetDriveTypeW)) { return FALSE; }   
    if (!inline_hook("kernel32.dll", "GetLogicalDriveStringsA", 0x14, (void*)x_GetLogicalDriveStringsA, (void**)&k32_GetLogicalDriveStringsA)) { return FALSE; }   
    if (!inline_hook("kernel32.dll", "GetLogicalDriveStringsW", 0x13, (void*)x_GetLogicalDriveStringsW, (void**)&k32_GetLogicalDriveStringsW)) { return FALSE; }   
    if (!inline_hook("kernel32.dll", "GetVolumeInformationA", 0x15, (void*)x_GetVolumeInformationA, (void**)&k32_GetVolumeInformationA)) { return FALSE; }   
    if (!inline_hook("kernel32.dll", "GetVolumeInformationW", 0x18, (void*)x_GetVolumeInformationW, (void**)&k32_GetVolumeInformationW)) { return FALSE; }   
    #else
    if (!inline_hook("kernel32.dll", "GetDriveTypeA", 0x0F, (void*)x_GetDriveTypeA, (void**)&k32_GetDriveTypeA)) { return FALSE; }   
    if (!inline_hook("kernel32.dll", "GetDriveTypeW", 0x0E, (void*)x_GetDriveTypeW, (void**)&k32_GetDriveTypeW)) { return FALSE; }   
    if (!inline_hook("kernel32.dll", "GetLogicalDriveStringsA", 0x0B, (void*)x_GetLogicalDriveStringsA, (void**)&k32_GetLogicalDriveStringsA)) { return FALSE; }   
    if (!inline_hook("kernel32.dll", "GetLogicalDriveStringsW", 0x08, (void*)x_GetLogicalDriveStringsW, (void**)&k32_GetLogicalDriveStringsW)) { return FALSE; }   
    if (!inline_hook("kernel32.dll", "GetVolumeInformationA", 0x07, (void*)x_GetVolumeInformationA, (void**)&k32_GetVolumeInformationA)) { return FALSE; }   
    if (!inline_hook("kernel32.dll", "GetVolumeInformationW", 0x0B, (void*)x_GetVolumeInformationW, (void**)&k32_GetVolumeInformationW)) { return FALSE; }   
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
