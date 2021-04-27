#include <Windows.h>
#include "../common/ntmin/ntmin.h"
#include "fs_utils_win.h"
#include "fs_redirect.h"


int fs_redirect_nt(wchar_t* src_buffer, unsigned int src_len, HANDLE root_handle, int is_directory, int is_read, int is_write,int fail_if_exist, int fail_if_not_exist, PUNICODE_STRING unicodestr_redirected_path){
    // Make a copy of our instr to ensure it is properly truncated with nulls.
    wchar_t* src_clean_buffer = (wchar_t*)calloc(1,src_len+2);
    memcpy(src_clean_buffer,src_buffer,src_len);
    
    // Resolve an Absolute Path to our Inpath
    char* abspath = calloc(1,X_MAX_PATH);

    get_abspath_from_handle(root_handle, src_clean_buffer, abspath);
    free(src_clean_buffer); 

    // Send Path to our internal fs_redirect()
    char* redirected_path = NULL;
    int result = fs_redirect(abspath, is_directory, is_read, is_write, fail_if_exist, fail_if_not_exist, &redirected_path);
    free(abspath);
    if(!result){return 0;}
    // Handle bypass responses
    ChartoUnicodeString(redirected_path,unicodestr_redirected_path);
    free(redirected_path);
    return 1;
}