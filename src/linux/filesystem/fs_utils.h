#pragma once
#include <stddef.h>
#define X_MAX_PATH 0x1000

int path_exists(char* path);
int path_is_symlink(char* path);
void resolve_abspath(char* in_path, char* out_path, int follow_symlinks);
void makedir(char* path);
void delete_path(char* path);
void copy_file(char* src_path, char* dst_path);

// Linux Only
const char* bypass_path_check(const char* in_path);