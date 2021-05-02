#pragma once
#include <stddef.h>
#define X_MAX_PATH 1024
#define FLAG_BYPASS 0x04000000

int path_exists(char* path);
int path_is_symlink(char* path);
void resolve_abspath(char* in_path, char* out_path, int follow_symlinks);
void makedir(char* path);
void delete_path(char* path);
void copy_file(char* src_path, char* dst_path);

// WinNT Only
int get_abspath_from_handle(void* hObject, wchar_t* in_path, wchar_t* out_path);