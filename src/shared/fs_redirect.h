#pragma once

int fs_redirect(char* in_abspath, int is_directory, int is_read, int is_write, int fail_if_exist, int fail_if_not_exist, char** redirected_path);