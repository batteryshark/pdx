#pragma once

typedef struct _VRHM_ENTRY{
	void* hKey;
	char* path;
}VRHM_ENTRY,*PVRHM_ENTRY;

PVRHM_ENTRY vrhm_lookup(void* hKey);
void vrhm_add(void* hKey, char* n_path);
void vrhm_destroy(void* hKey);
void vrhm_print();