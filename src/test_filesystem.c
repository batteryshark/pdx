#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>

//#include "filesystem/fs_utils_win.h"
#include "filesystem/fs_redirect.h"

int main(int argc, char *argv[]){
    //printf("Path Exists: %d\n",path_exists(L"\\??\\C:\\VX"));
    //printf("Path Exists: %d\n",path_exists(L"\\??\\D:\\vxregistryf.ini"));
    //printf("Path is Symlink: %d\n",path_is_symlink(L"\\??\\D:\\vxregistryf.ini"));
   //printf("Path is Symlink: %d\n",path_is_symlink(L"\\??\\D:\\vxregistry.ini"));
    //printf("Path is Symlink: %d\n",path_is_symlink(L"\\??\\D:\\vxsymlink.ini"));
//    wchar_t out_path[1024] = {0x00};
 //   resolve_abspath(L"\\??\\D:\\vxsymlink.ini",out_path,1);
  //  wprintf(L"out path: %ls\n",out_path);
        
    putenv("PIEE_FS_ROOT=c:\\tmp\\vx\\00000000");
    char* redirected_path = NULL;
    int res = fs_redirect(argv[1],0, 1,0, 0, 0, &redirected_path);
    printf("Redirected: %d %s\n",res,redirected_path);
    //create_parent_path("\\??\\D:\\new\\paths\\are\\fun");
    return 0;
}