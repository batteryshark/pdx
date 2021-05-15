// Logic for Virtual Registry Handle Manager (VRHM)
#include <windows.h>
#include "../common/vector/vector.h"
#include "vrhm.h"

static Vector vecreg;
static int vrhm_initialized = 0;



void init_vrhm(){
    vector_setup(&vecreg, 10, sizeof(PVRHM_ENTRY));
    vrhm_initialized = 1;
}

void vrhm_print(){
    if(!vrhm_initialized){init_vrhm();}
    for(int i=0;i < vecreg.size; i++){
        PVRHM_ENTRY ce = VECTOR_GET_AS(PVRHM_ENTRY, &vecreg, i);
        if(!ce){continue;}
        OutputDebugStringA(ce->path);
    }
}

PVRHM_ENTRY vrhm_lookup(void* hKey){
    if(!vrhm_initialized){init_vrhm();}
    for(int i=0;i < vecreg.size; i++){
        PVRHM_ENTRY ce = VECTOR_GET_AS(PVRHM_ENTRY, &vecreg, i);
        if(!ce){continue;}
        if(ce->hKey == hKey){
            return ce;
        }
    }
    return NULL;
}
void vrhm_add(void* hKey, char* n_path){
    if(!vrhm_initialized){init_vrhm();}
    // If we already have this entry, skip it.
    if(vrhm_lookup(hKey)){return;}
    // Otherwise, create the entry.
    PVRHM_ENTRY ne = calloc(1,sizeof(PVRHM_ENTRY));
    char* nppath = calloc(1,strlen(n_path)+1);
    strcpy(nppath,n_path);
    ne->hKey = hKey;
    ne->path = nppath;
    vector_push_back(&vecreg, &ne);
}
void vrhm_destroy(void* hKey){
    if(!vrhm_initialized){init_vrhm();}
    for(int i=0;i < vecreg.size; i++){
        PVRHM_ENTRY ce = VECTOR_GET_AS(PVRHM_ENTRY, &vecreg, i);
        if(!ce){continue;}
        if(ce->hKey == hKey){
            free(ce->path);
            vector_erase(&vecreg, i);
            return;
        }
    }
    return;
}

