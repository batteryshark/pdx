#pragma once

// Registry Tracking Object Helpers
typedef struct _REG_INDEX {
	void* prev;
	void* next;
	void* hKey;
	char* path;
}REG_INDEX;

void print_regdb();
REG_INDEX* regentry_lookup(void* hKey);
char* regentry_path_lookup(void* hKey);
void regentry_create(void* hKey, char* n_path);
int regentry_delete(void* hKey);