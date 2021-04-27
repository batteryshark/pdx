#pragma once
#include <stddef.h>
#define X_MAX_PATH 1024
#define NT_PREFIX_W L"\\??\\"
#define NT_PREFIX "\\??\\"
#define FLAG_BYPASS 0xF0000000

int path_exists_native(wchar_t* path);
int path_exists(char* path);
int path_is_symlink_native(wchar_t* path);
int path_is_symlink(char* path);
int get_abspath_from_handle(void* hObject, wchar_t* in_path, wchar_t* out_path);
void resolve_abspath_native(wchar_t* in_path, wchar_t* out_path, int follow_symlinks);
void resolve_abspath(char* in_path, char* out_path, int follow_symlinks);
void makedir(char* path);