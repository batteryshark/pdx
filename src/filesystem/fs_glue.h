#pragma once

#ifdef TARGET_OS_WINDOWS
#include "../common/ntmin/ntdll.h"
int fs_redirect_nt(wchar_t* src_buffer, unsigned int src_len, HANDLE root_handle, int is_directory, int is_read, int is_write,int fail_if_exist, int fail_if_not_exist, PUNICODE_STRING unicodestr_redirected_path);
#else

#endif