#pragma once

int vreg_key_exists(const char* path);
void vreg_create_key(char* key_path);
void vreg_delete_key(char* path);
void vreg_create_key_value(char* path, char* name, unsigned int title_index, unsigned int type, void* data, unsigned int data_length);
int vreg_get_key_value(char* path, char* name,unsigned int* value_type, unsigned char** value_data, unsigned int* value_data_length);
void vreg_delete_key_value(char* path, char* name);
void vreg_anonymize_key_path(char* in_path);
int vreg_get_count_values(char* key_path);
