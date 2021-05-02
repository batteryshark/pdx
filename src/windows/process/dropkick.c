#include <stdio.h>
#include <string.h>
#include <windows.h>


#define UNC_MAX_PATH 0x7FFF

// Structures from ntdll
typedef struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR  Buffer;
} UNICODE_STRING, * PUNICODE_STRING;

typedef NTSTATUS __stdcall tLdrLoadDll(PWCHAR PathToFile, ULONG Flags, PUNICODE_STRING ModuleFileName, HMODULE* ModuleHandle);

typedef struct _load_library_t {
	tLdrLoadDll* ldr_load_dll;
	UNICODE_STRING filepath;
} load_library_t;

// Shellcode Replicates the Following:
/*
#define NOINLINE __declspec(noinline)
void NOINLINE load_library_worker(load_library_t* s){
	HMODULE module_handle; unsigned int ret = 0;
	s->ldr_load_dll(NULL, 0, &s->filepath, &module_handle);
}
*/
#if __x86_64__
BOOL is_wow64_loader = FALSE;
unsigned char load_library_worker[] = {
	0x55, 0x48, 0x89, 0xE5, 0x48, 0x83, 0xEC, 0x30, 0x48, 0x89, 0x4D, 0x10, 0xC7, 0x45, 0xFC, 0x00,
	0x00, 0x00, 0x00, 0x48, 0x8B, 0x45, 0x10, 0x48, 0x8B, 0x00, 0x48, 0x8B, 0x55, 0x10, 0x48, 0x8D,
	0x4A, 0x08, 0x48, 0x8D, 0x55, 0xF0, 0x49, 0x89, 0xD1, 0x49, 0x89, 0xC8, 0xBA, 0x00, 0x00, 0x00,
	0x00, 0xB9, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xD0, 0x90, 0x48, 0x83, 0xC4, 0x30, 0x5D, 0xC3
};
#else
BOOL is_wow64_loader = TRUE;
unsigned char load_library_worker[] = {
	0x55, 0x89, 0xE5, 0x83, 0xEC, 0x28, 0xC7, 0x45, 0xF4, 0x00, 0x00, 0x00, 0x00, 0x8B, 0x45, 0x08,
	0x8B, 0x00, 0x8B, 0x55, 0x08, 0x8D, 0x4A, 0x04, 0x8D, 0x55, 0xF0, 0x89, 0x54, 0x24, 0x0C, 0x89,
	0x4C, 0x24, 0x08, 0xC7, 0x44, 0x24, 0x04, 0x00, 0x00, 0x00, 0x00, 0xC7, 0x04, 0x24, 0x00, 0x00,
	0x00, 0x00, 0xFF, 0xD0, 0x83, 0xEC, 0x10, 0x90, 0xC9, 0xC2, 0x04, 0x00
};
#endif

void usage(char* exe_name) {
	printf("Usage: %s start PATH_TO_SHIM PATH_TO_EXE [EXE_ARGS]\n", exe_name);
	printf("Usage: %s start_in PATH_TO_SHIM PATH_TO_EXE START_PATH [EXE_ARGS]\n", exe_name);	
	printf("Usage: %s inject PATH_TO_SHIM PID TID LEAVE_SUSPENDED\n",exe_name);
	exit(-1);
}

BOOL GetDirectoryPath(LPSTR lpFilename, LPSTR in_path) {
	strcpy(lpFilename, in_path);
    char* last_delimiter = strrchr(lpFilename, 0x5C);
    if (!last_delimiter) { return FALSE; }
    memset(last_delimiter + 1, 0x00, 1);
    return TRUE;
}

// Process Manipulation Stuff
void* write_to_process(DWORD target_pid, const void* data, unsigned int length){

	HANDLE hProcess = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE , FALSE, target_pid);

	if (!hProcess || hProcess == INVALID_HANDLE_VALUE) { printf("Error Opening Process Handle\n"); return NULL; }
	
	void* addr = VirtualAllocEx(hProcess, NULL, length,
		MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (addr == NULL) {
		printf("[-] Error allocating memory in process: %ld!\n",
			GetLastError());
			CloseHandle(hProcess);
			return NULL;
	}

	DWORD_PTR bytes_written;
	if (WriteProcessMemory(hProcess, addr, data, length,
		&bytes_written) == FALSE || bytes_written != length) {
		printf("[-] Error writing data to process: %ld\n", GetLastError());
		CloseHandle(hProcess);
		return NULL;
	}	
	CloseHandle(hProcess);
	return addr;
}

HANDLE open_thread(unsigned int tid){
	HANDLE thread_handle = OpenThread(THREAD_ALL_ACCESS, FALSE, tid);
	if (thread_handle == NULL) {
		printf("[-] Error getting access to thread: %ld!\n", GetLastError());
	}

	return thread_handle;
}

void resume_thread(unsigned int tid){
	HANDLE thread_handle = open_thread(tid);
	ResumeThread(thread_handle);
	CloseHandle(thread_handle);
}

void kill_process_by_pid(DWORD pid){
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	TerminateProcess(hProcess,0);
}

int inject_process(DWORD target_pid, DWORD target_tid, char* path_to_shim, BOOL leave_suspended){
	printf("Inject Process: %d %d %s %d\n",target_pid, target_tid, path_to_shim, leave_suspended);
	wchar_t wshim_path[1024] = {0x00};
    MultiByteToWideChar(CP_ACP, 0, path_to_shim, -1, wshim_path, sizeof(wshim_path));
	load_library_t s;
	ZeroMemory(&s,sizeof(s));

	s.ldr_load_dll = (tLdrLoadDll*)GetProcAddress(GetModuleHandleA("ntdll.dll"), "LdrLoadDll");
	s.filepath.Length = wcslen(wshim_path) * sizeof(wchar_t);
	s.filepath.MaximumLength = s.filepath.Length+2;
	s.filepath.Buffer = (PWSTR)write_to_process(target_pid, (const void*)wshim_path, s.filepath.MaximumLength);	
	void* settings_addr = write_to_process(target_pid, &s, sizeof(s));	
	void* shellcode_addr = write_to_process(target_pid, load_library_worker, sizeof(load_library_worker));
	
	// Make sure we wrote before going any further.
	if(!s.filepath.Buffer || !settings_addr || !shellcode_addr){
		kill_process_by_pid(target_pid); 
		return -1;
	}

	HANDLE thread_handle = open_thread(target_tid);

	// Add LdrLoadDll(..., dll_path, ...) to the APC queue.
	if (QueueUserAPC((PAPCFUNC)shellcode_addr, thread_handle,
		(ULONG_PTR)settings_addr) == 0) {
		printf("[-] Error adding task to APC queue: %ld\n", GetLastError());
		kill_process_by_pid(target_pid); 
		return -1;		
	}

	CloseHandle(thread_handle);

	if (!leave_suspended) {
		resume_thread(target_tid);
	}
	Sleep(800);
	return 0;
}

// Entrypoint to Spawn a New Process
int start_process(int argc, char* argv[], BOOL start_in){
	// Set our APP ID and Channel Name
	char* path_to_shim = argv[2];
	char* path_to_exe = argv[3];
	char cmd[4096] = {0x00};
	// Construct the Command String
	sprintf(cmd,"\"%s\"",path_to_exe);
	
	int param_offset = 4;
	char* exe_base_path = NULL;

	if(!start_in){
		// Get Full Path of Executable as our starting directory.
		exe_base_path = (char*)malloc(UNC_MAX_PATH);
		GetDirectoryPath(exe_base_path,path_to_exe);
	}else{
		exe_base_path = argv[4];
		param_offset = 5;
	}

	// Set Parameters
	for(int i=param_offset;i<argc;i++){
		strcat(cmd," ");
		strcat(cmd,argv[i]);
	}

	// Start the Process in a Suspended State
	PROCESS_INFORMATION pi;
	STARTUPINFOA Startup;
	ZeroMemory(&Startup, sizeof(Startup));
	ZeroMemory(&pi, sizeof(pi));
	if (!CreateProcessA(NULL, (LPSTR)cmd, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, exe_base_path, &Startup, &pi)) {
		printf("CreateProcess Failed: %d!\n", GetLastError());
		free(exe_base_path);
		return -1;
	}
	free(exe_base_path);

	// Determine if this is a 32 or 64bit target
	BOOL is_wow64 = FALSE;
	IsWow64Process(pi.hProcess, &is_wow64);
	// Ensure the Target Matches the Architecture of our Loader
	if(is_wow64_loader != is_wow64){
		printf("Loader Architecture Mismatch\n");
		TerminateProcess(pi.hProcess,0);
		return -1;
	}

	return inject_process(pi.dwProcessId,pi.dwThreadId,path_to_shim,FALSE);
}

int main(int argc, char* argv[]) {
	if (argc < 2) { usage(argv[0]); }
	char* cmd = argv[1];
    if(!stricmp(cmd,"start")){
        if(argc < 4){usage(argv[0]);}
	    return start_process(argc,argv,0);
    }else if(!stricmp(cmd,"start_in")){
        if(argc < 5){usage(argv[0]);}
		return start_process(argc,argv,1);
    }else if(!stricmp(cmd,"inject")){
        if(argc != 6){usage(argv[0]);}
		char* path_to_shim = argv[2];
        DWORD target_pid = atoi(argv[3]);
		DWORD target_tid = atoi(argv[4]);		
		int leave_suspended = atoi(argv[5]);
		return inject_process(target_pid,target_tid,path_to_shim,leave_suspended);
    }else{
        usage(argv[0]);
    }
    return 0;
}