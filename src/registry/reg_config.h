#pragma once


int key_path_exists(char* key_path);
int init_registry();
void cleanup_registry();

void create_registry_entry(char* key_path, char* value_name, unsigned int title_index, unsigned int type, void* data, unsigned int data_length);
int get_registry_value(char* key_path, char* value_name,unsigned int* value_type, unsigned char** value_data, unsigned int* value_data_length);
void delete_registry_key(char* key_path, char* value_name);