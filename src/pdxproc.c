// Process Plugin for Spawning

#ifdef TARGET_OS_WINDOWS
#include "common/env.h"
#include "common/mem.h"
#include "common/ntmin/ntmin.h"
typedef NTSTATUS __stdcall tNtCreateUserProcess(PHANDLE ProcessHandle, PHANDLE ThreadHandle, ACCESS_MASK ProcessDesiredAccess, ACCESS_MASK ThreadDesiredAccess, POBJECT_ATTRIBUTES ProcessObjectAttributes, POBJECT_ATTRIBUTES ThreadObjectAttributes, ULONG ProcessFlags, ULONG ThreadFlags, PRTL_USER_PROCESS_PARAMETERS ProcessParameters, PVOID CreateInfo, PVOID AttributeList);
static tNtCreateUserProcess* ntdll_NtCreateUserProcess = NULL;
typedef struct _THREAD_BASIC_INFORMATION {
	NTSTATUS                ExitStatus;
	PVOID                   TebBaseAddress;
	CLIENT_ID               ClientId;
	KAFFINITY               AffinityMask;
	KPRIORITY               Priority;
	KPRIORITY               BasePriority;
} THREAD_BASIC_INFORMATION, * PTHREAD_BASIC_INFORMATION;
#else
#include <string.h>
#endif


#define LOADER_EXE_32 "dropkick32.exe"
#define LOADER_EXE_64 "dropkick64.exe"
#define BOOTSTRAP_32 "bootstrap32.dll"
#define BOOTSTRAP_64 "bootstrap64.dll"


#ifdef TARGET_OS_WINDOWS
void spawn_process(int is_wow64, PVOID pid, PVOID tid, int leave_suspended){
    // Determine if this is a 32 or 64bit target
    char loader_exe_path[1024] = {0x00};
    char bootstrap_path[1024] = {0x00};
    if(getenv("PDX_LOADERS_PATH")){
        strcpy(loader_exe_path,getenv("PDX_LOADERS_PATH"));
        strcat(loader_exe_path,"\\");
        strcpy(bootstrap_path,getenv("PDX_LOADERS_PATH"));
        strcat(bootstrap_path,"\\");
    }

    if(is_wow64){
        strcat(loader_exe_path,LOADER_EXE_32);
        strcat(bootstrap_path,BOOTSTRAP_32);
    }else{
        strcat(loader_exe_path,LOADER_EXE_64);
        strcat(bootstrap_path,BOOTSTRAP_64);
    }
    char cmd[1024] = {0x00};
    sprintf(cmd,"%s inject %s %d %d %d",loader_exe_path,bootstrap_path,pid,tid,leave_suspended);
    system(cmd);
}


NTSTATUS __stdcall x_NtCreateUserProcess(PHANDLE ProcessHandle, PHANDLE ThreadHandle, ACCESS_MASK ProcessDesiredAccess, ACCESS_MASK ThreadDesiredAccess, POBJECT_ATTRIBUTES ProcessObjectAttributes, POBJECT_ATTRIBUTES ThreadObjectAttributes, ULONG ProcessFlags, ULONG ThreadFlags, PRTL_USER_PROCESS_PARAMETERS ProcessParameters, PVOID CreateInfo, PVOID AttributeList) {
    ULONG original_process_flags = ProcessFlags;
    ProcessFlags |= CREATE_SUSPENDED;
    NTSTATUS status = ntdll_NtCreateUserProcess(ProcessHandle, ThreadHandle, ProcessDesiredAccess, ThreadDesiredAccess, ProcessObjectAttributes, ThreadObjectAttributes, ProcessFlags, ThreadFlags, ProcessParameters, CreateInfo, AttributeList);    
    // If create failed, we short-circuit.
    if (status) { return status; }
    // TODO - All the Process Context Switching Injection Stuff.
    THREAD_BASIC_INFORMATION tbi;
    NtQueryInformationThread(*ThreadHandle, ThreadBasicInformation, &tbi, sizeof(tbi), 0);

    char request[64] = { 0x00 };
    char response[64] = { 0x00 };
    int leave_suspended = 0;
    if (original_process_flags & CREATE_SUSPENDED) {
        leave_suspended = 1;
    }
    int is_wow64 = 0;
	IsWow64Process(*ProcessHandle, &is_wow64);
    spawn_process(is_wow64, tbi.ClientId.UniqueProcess, tbi.ClientId.UniqueThread, leave_suspended);    
    return status;
}
#endif

int init_library(){
// Perform any Syscall Hooks we Need at This Level
    #ifdef TARGET_OS_WINDOWS
        if (!inline_hook("ntdll.dll", "NtCreateUserProcess", SYSCALL_STUB_SIZE, (void*)x_NtCreateUserProcess, (void**)&ntdll_NtCreateUserProcess)) { return 0; }
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