#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>

//#include "filesystem/fs_utils_win.h"
#include "../../shared/fs_redirect.h"

int main(int argc, char *argv[]){
    //printf("Path Exists: %d\n",path_exists(L"\\??\\C:\\VX"));
    //printf("Path Exists: %d\n",path_exists(L"\\??\\D:\\vxregistryf.ini"));
    //printf("Path is Symlink: %d\n",path_is_symlink(L"\\??\\D:\\vxregistryf.ini"));
   //printf("Path is Symlink: %d\n",path_is_symlink(L"\\??\\D:\\vxregistry.ini"));
    //printf("Path is Symlink: %d\n",path_is_symlink(L"\\??\\D:\\vxsymlink.ini"));
//    wchar_t out_path[1024] = {0x00};
 //   resolve_abspath(L"\\??\\D:\\vxsymlink.ini",out_path,1);
  //  wprintf(L"out path: %ls\n",out_path);
        
    putenv("PDXFS_ROOT=C:\\vxtmp\\CPB-WV7-UJ4\\map");
    char* redirected_path = NULL;
    char* inst = "\\??\\c:\\vxtmp\\cpb-wv7-uj4\\map\\c\\programdata\\nvidia corporation\\nv_cache\\35c5bbd7ddb5aa7de69f6e9399aa4605_fce8395c8fd8a9cb_387ce0633917037e_0_0.0.toc";
    
    int res = fs_redirect(inst,0, 1,0, 0, 0, &redirected_path);
    if(res){
        printf("Redirected: %d %s\n",res,redirected_path);
    }else{
        printf("Bypass\n");
    }
    
    //create_parent_path("\\??\\D:\\new\\paths\\are\\fun");
    return 0;
}

