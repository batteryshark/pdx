#include "../common/mem.h"
#include "../common/ntmin/ntmin.h"

#define LOADER_EXE_32 "dropkick32.exe"
#define LOADER_EXE_64 "dropkick64.exe"
#define BOOTSTRAP_32 "pdxproc32.dll"
#define BOOTSTRAP_64 "pdxproc64.dll"

typedef NTSTATUS __stdcall tNtCreateUserProcess(PHANDLE ProcessHandle, PHANDLE ThreadHandle, ACCESS_MASK ProcessDesiredAccess, ACCESS_MASK ThreadDesiredAccess, POBJECT_ATTRIBUTES ProcessObjectAttributes, POBJECT_ATTRIBUTES ThreadObjectAttributes, ULONG ProcessFlags, ULONG ThreadFlags, PRTL_USER_PROCESS_PARAMETERS ProcessParameters, PVOID CreateInfo, PVOID AttributeList);
typedef BOOL tCreateProcessA(LPCSTR lpApplicationName, LPSTR lpCommandLine, void* lpProcessAttributes, void* lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory, LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);

static tNtCreateUserProcess* ntdll_NtCreateUserProcess = NULL;
static tCreateProcessA* k32_CreateProcessA = NULL;

typedef struct _THREAD_BASIC_INFORMATION {
	NTSTATUS                ExitStatus;
	PVOID                   TebBaseAddress;
	CLIENT_ID               ClientId;
	KAFFINITY               AffinityMask;
	KPRIORITY               Priority;
	KPRIORITY               BasePriority;
} THREAD_BASIC_INFORMATION, * PTHREAD_BASIC_INFORMATION;

// I hate this
// TODO: Replace CreateProcessA with NtCreateUserProcess to call our dropkick and bypass
static int bypass_flag = 0;

void spawn_process(int is_wow64, PVOID pid, PVOID tid, int leave_suspended){
    if(!k32_CreateProcessA){
        get_function_address("kernel32.dll", "CreateProcessA", (void**)&k32_CreateProcessA);
        if(!k32_CreateProcessA){return;}
    }

    // Determine if this is a 32 or 64bit target
    char loader_exe_path[1024] = {0x00};
    char bootstrap_path[1024] = {0x00};
    if(getenv("PDXPROC")){
        strcpy(loader_exe_path,getenv("PDXPROC"));
        strcat(loader_exe_path,"\\");
        strcpy(bootstrap_path,getenv("PDXPROC"));
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

	PROCESS_INFORMATION pi;
	STARTUPINFOA Startup;
	memset(&Startup,0x00, sizeof(Startup));
	memset(&pi,0x00, sizeof(pi));
    bypass_flag = 1;
    k32_CreateProcessA(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &Startup, &pi);
    bypass_flag = 0;
}


NTSTATUS __stdcall x_NtCreateUserProcess(PHANDLE ProcessHandle, PHANDLE ThreadHandle, ACCESS_MASK ProcessDesiredAccess, ACCESS_MASK ThreadDesiredAccess, POBJECT_ATTRIBUTES ProcessObjectAttributes, POBJECT_ATTRIBUTES ThreadObjectAttributes, ULONG ProcessFlags, ULONG ThreadFlags, PRTL_USER_PROCESS_PARAMETERS ProcessParameters, PVOID CreateInfo, PVOID AttributeList) {
    if(bypass_flag){
        return ntdll_NtCreateUserProcess(ProcessHandle, ThreadHandle, ProcessDesiredAccess, ThreadDesiredAccess, ProcessObjectAttributes, ThreadObjectAttributes, ProcessFlags, ThreadFlags, ProcessParameters, CreateInfo, AttributeList);    
    }
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
    USHORT ProcessMachine = 0;
    USHORT NativeMachine = 0;
    RtlWow64GetProcessMachines(*ProcessHandle, &ProcessMachine, &NativeMachine);
    int is_wow64 = (ProcessMachine != 0);
    spawn_process(is_wow64, tbi.ClientId.UniqueProcess, tbi.ClientId.UniqueThread, leave_suspended);    
    return status;
}

void bootstrap_init(void){       
    char payload[0x1000] = {0x00};
    if(!getenv("PDXPL")){return;}    
    strcpy(payload,getenv("PDXPL"));    
    
    // Load Any Additional Modules
    char * token = strtok(payload, ";");
    if(!token){
        void* hLibrary = NULL;
        load_library(payload,&hLibrary);
    }else{
        while( token != NULL ) {
            void* hLibrary = NULL;          
            load_library(token,&hLibrary);
            token = strtok(NULL, ";");
        }         
    }
}

void init_library(){
    bootstrap_init();
    inline_hook("ntdll.dll", "NtCreateUserProcess", SYSCALL_STUB_SIZE, (void*)x_NtCreateUserProcess, (void**)&ntdll_NtCreateUserProcess);            
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        init_library();
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
