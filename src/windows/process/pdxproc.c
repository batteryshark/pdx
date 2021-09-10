#include "../common/mem.h"
#include "../common/ntmin/ntmin.h"

#define LOADER_EXE_32 "VXBootstrap32.exe"
#define LOADER_EXE_64 "VXBootstrap64.exe"

typedef void tOutputDebugStringA(const char* lpOutputString);

typedef NTSTATUS __stdcall tNtCreateUserProcess(PHANDLE ProcessHandle, PHANDLE ThreadHandle, ACCESS_MASK ProcessDesiredAccess, ACCESS_MASK ThreadDesiredAccess, POBJECT_ATTRIBUTES ProcessObjectAttributes, POBJECT_ATTRIBUTES ThreadObjectAttributes, ULONG ProcessFlags, ULONG ThreadFlags, PRTL_USER_PROCESS_PARAMETERS ProcessParameters, PVOID CreateInfo, PVOID AttributeList);
typedef BOOL tCreateProcessA(LPCSTR lpApplicationName, LPSTR lpCommandLine, void* lpProcessAttributes, void* lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory, LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);

static tNtCreateUserProcess* ntdll_NtCreateUserProcess = NULL;
static tCreateProcessA* k32_CreateProcessA = NULL;
static tOutputDebugStringA* k32_OutputDebugStringA = NULL;


// I hate this
// TODO: Replace CreateProcessA with NtCreateUserProcess to call our dropkick and bypass
static int bypass_flag = 0;

void spawn_process(int is_wow64, HANDLE pid, HANDLE tid, int leave_suspended){
    if(!k32_CreateProcessA){
        get_function_address("kernel32.dll", "CreateProcessA", (void**)&k32_CreateProcessA);
        get_function_address("kernel32.dll", "OutputDebugStringA", (void**)&k32_OutputDebugStringA);
        if(!k32_CreateProcessA){return;}        
    }

    // Determine if this is a 32 or 64bit target
    char bootstrap_exe_path[1024] = {0x00};
    char bootstrap_path[1024] = {0x00};
    if(getenv("PDXPROC")){
        strcpy(bootstrap_exe_path,getenv("PDXPROC"));
        strcat(bootstrap_exe_path,"\\");
    }

    
    if(is_wow64){
        strcat(bootstrap_exe_path,LOADER_EXE_32);
    }else{
        strcat(bootstrap_exe_path,LOADER_EXE_64);
    }
    char cmd[1024] = {0x00};
    sprintf(cmd,"\"%s\" vxcmd=inject vxpid=%d vxtid=%d vxls=%d",bootstrap_exe_path,pid,tid,leave_suspended);
    //k32_OutputDebugStringA(cmd);
    PROCESS_INFORMATION pi;
	STARTUPINFOA Startup;
	memset(&Startup,0x00, sizeof(Startup));
	memset(&pi,0x00, sizeof(pi));
    bypass_flag = 1;
    k32_CreateProcessA(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &Startup, &pi);
    bypass_flag = 0;
}

int ProcessIsWoW64(HANDLE hProcess){
    USHORT ProcessMachine = 0;
    USHORT NativeMachine = 0;
    RtlWow64GetProcessMachines(hProcess, &ProcessMachine, &NativeMachine);
    return (ProcessMachine != 0);
}

void FormatArchSpecificPreload(char* path){
    char* ext = strstr(path,".dlldynamic");
    if(!ext){return;}
    if(ProcessIsWoW64(NtCurrentProcess())){
        strcpy(ext,"32.dll");
    }else{
        strcpy(ext,"64.dll");
    }
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
    memset(&tbi,0,sizeof(THREAD_BASIC_INFORMATION));
    ULONG retlen = 0;
    NtQueryInformationThread(*ThreadHandle, ThreadBasicInformation, &tbi, sizeof(THREAD_BASIC_INFORMATION), &retlen);

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



void init_library(){
   //get_function_address("kernel32.dll","OutputDebugStringA",(void**)&k32_OutputDebugStringA);
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
