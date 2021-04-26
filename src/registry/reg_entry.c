#include <Windows.h>
#include "reg_entry.h"
#include <stdio.h>
static REG_INDEX* first_entry = NULL;

void print_regdb(){
    REG_INDEX* last = first_entry;
    while(last){        
        printf("[VX] Reg entry %p: hKey:%08x Prev:%p Next:%p Path:%s",last,last->hKey,last->prev,last->next,last->path);
        last = last->next;
    }
}

REG_INDEX* regentry_lookup(void* hKey){
    
    if(!first_entry){return NULL;}
	REG_INDEX* cur_entry = first_entry;
	while(cur_entry){	
		if(cur_entry->hKey == hKey){
            return cur_entry;
		}
        cur_entry = cur_entry->next;
	}
	return NULL;
}

void regentry_create(void* hKey, char* n_path){
    if(!hKey || hKey == INVALID_HANDLE_VALUE || !n_path){return;}
    // If we already have this entry, skip it.
    if(regentry_lookup(hKey)){return;}
	REG_INDEX* n_entry = calloc(1,sizeof(REG_INDEX));
    char* nppath = calloc(1,strlen(n_path)+1);
    strcpy(nppath,n_path);
	n_entry->hKey = hKey;
	n_entry->path = nppath;
    n_entry->prev = NULL;
	n_entry->next = NULL;

    REG_INDEX* last = first_entry;

	// Check if First Entry is NULL
	if(!last){      
		first_entry = n_entry;
		return;
    }
    /* else traverse till the last node */
    while (last->next){}
        
 
    /* 6. Change the next of last node */
    last->next = n_entry;
 
    /* 7. Make last node as previous of new node */
    n_entry->prev = last;
    return;
}

int regentry_delete(void* hKey){
    if(!first_entry){return FALSE;}    
	REG_INDEX* target_entry = regentry_lookup(hKey);
    if(!target_entry){return FALSE;}    
    REG_INDEX* prev = target_entry->prev;
    REG_INDEX* next = target_entry->next;
    if(!prev && !next){
        first_entry = NULL;
        return TRUE;
    }
    if(prev){
        prev->next = next;
    }
    if(next){
        next->prev = prev;
    }
    if(target_entry->path){
        free(target_entry->path);
    }
    free(target_entry);
	return TRUE;
}


char* regentry_path_lookup(void* hKey){
    if(!first_entry){return NULL;}
	REG_INDEX* cur_entry = regentry_lookup(hKey);
    if(!cur_entry){return NULL;}
    return cur_entry->path;

}