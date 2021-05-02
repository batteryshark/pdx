#include <dlfcn.h>
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS 0x20
#endif

#include "mem.h"

#define PAGE_SIZE 0x1000

#if __x86_64__
#define HOTPATCH_ADDRESS_OFFSET 2
static unsigned char hotpatch_stub[] = {
    0x48,0xB8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // mov rax, [Abs Jump Address]
    0xFF,0xE0,                                         // jmp rax
    0xC3,                                              // ret
};
#else
#define HOTPATCH_ADDRESS_OFFSET 1
static unsigned char hotpatch_stub[] = {
        0xB8, 0x00, 0x00, 0x00, 0x00, // mov eax, [Abs Jump Address]
        0xFF,0xE0,                    // jmp eax
        0xC3                          // ret
};
#endif

    
int load_library(const char* lib_name, void** h_library){
    // If we didn't specify a handle, use a throwaway one.
    if(!h_library){
        void* tmp_h_library = NULL;
        h_library = &tmp_h_library;
    }

    *h_library = dlopen(lib_name, RTLD_NOW);
    if(!*h_library){printf("Failed to dlopen\n");return 0;}
    return 1;

}


int get_function_address(const char* lib_name, const char* func_name, void** func_address) {
    if (!func_address) { return 0; }
    *func_address = NULL;
    void* hLibrary = NULL;
    if(!load_library(lib_name,&hLibrary)){printf("Loadlibrary failed wtf\n");return 0;}
    *func_address = dlsym(hLibrary,func_name);
    if(!*func_address){printf("Failed to bind function address\n"); return 0;}
    return 1;
}

int alloc(size_t amt, int is_exec, void** addr) {
    unsigned int access_mask = 0;
    if (!amt) {amt = PAGE_SIZE;}

    if (is_exec) { access_mask = PROT_EXEC | PROT_READ | PROT_WRITE; }
    else { access_mask = PROT_READ | PROT_WRITE; }
    *addr = mmap(NULL, amt, access_mask, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);    
    if(*addr){return 1;}
    return 0;
}



int heap_alloc_rwe_page(void** page_addr) {
    *page_addr = NULL;
    return alloc(PAGE_SIZE, 1, page_addr);
}

int heap_alloc_rw_page(void** page_addr) {
    *page_addr = NULL;
    return alloc(PAGE_SIZE, 0, page_addr);
}

int set_permission(void* target_addr, size_t data_len, unsigned char enable_read, unsigned char enable_write, unsigned char enable_execute, unsigned int* old_prot) {
    // Default to PAGE_SIZE if no len.
    if (!data_len) { data_len = PAGE_SIZE; }
    unsigned int n_prot = 0;
    // TODO: Get Old Protection Flags
    if (enable_write) { n_prot |= PROT_WRITE; }
    if (enable_read) { n_prot |= PROT_READ; }
    if (enable_execute) { n_prot |= PROT_EXEC; }
    mprotect((void*)(target_addr - ((size_t)target_addr % PAGE_SIZE)), data_len, n_prot);
    return 1;
}

int patch_memory(void* target_addr, void* data_ptr, size_t data_len, int is_write, int is_exec) {
    unsigned int old_prot = 0;
    if (!is_write) {
        if (!set_permission(target_addr, data_len, 1, 1, is_exec, &old_prot)) { return 0; }
    }

    memcpy(target_addr, data_ptr, data_len);
    if (!is_write) {
        if (!set_permission(target_addr, data_len, 1, 0, is_exec, &old_prot)) { return 0; }
    }

    return 1;
}

int patch_ret0(void* target_addr) {
    unsigned char x86_ret_0[3] = { 0x31,0xC0, 0xC3 };
    return patch_memory(target_addr, x86_ret_0, sizeof(x86_ret_0), 0, 1);
}

int patch_ret1(void* target_addr) {
    unsigned char x86_ret_1[4] = { 0x31,0xC0, 0x40, 0xC3 };
    return patch_memory(target_addr, x86_ret_1, sizeof(x86_ret_1), 0, 1);
}

int unhotpatch_function(struct HotPatch_Info* ctx) {
    return patch_memory(ctx->target_function_address, ctx->target_original_bytes, ctx->target_original_bytes_size, 0, 1);
}

int hotpatch_function(void* target_function_address, void* replacement_function_address, size_t target_original_bytes_size, struct HotPatch_Info* ctx, void** ptrampoline_address) {

    if (!ptrampoline_address || !target_function_address || !replacement_function_address) {printf("hotpatch sanity check failed\n"); return 0; }


    unsigned char hook_stub[sizeof(hotpatch_stub)] = { 0 };
    unsigned char trampoline_stub[sizeof(hotpatch_stub)] = { 0 };

    memcpy(hook_stub, hotpatch_stub, sizeof(hotpatch_stub));
    memcpy(trampoline_stub, hotpatch_stub, sizeof(hotpatch_stub));
    ctx->target_function_address = target_function_address;
    ctx->replacement_function_address = replacement_function_address;
    ctx->target_original_bytes_size = target_original_bytes_size;

    // We'll default the original bytes size to the size of our stub if it's not specified.
    if (!ctx->target_original_bytes_size || ctx->target_original_bytes_size < sizeof(hotpatch_stub)) {
        ctx->target_original_bytes_size = sizeof(hotpatch_stub);
    }
    ctx->target_original_bytes = NULL;
    alloc(ctx->target_original_bytes_size, 0, (void**)&ctx->target_original_bytes);

    // Create our Hook Stub - Backup instructions we're about to modify.
    memcpy(hook_stub + HOTPATCH_ADDRESS_OFFSET, &ctx->replacement_function_address, sizeof(void*));
    memcpy(ctx->target_original_bytes, ctx->target_function_address, ctx->target_original_bytes_size);

    // Allocate an executable page for our trampoline.
    if (!heap_alloc_rwe_page(ptrampoline_address)) { printf("allocate rwe page failed\n");return 0; }
    ctx->trampoline_address = *ptrampoline_address;
    // Write our stolen instructions to the head of the trampoline.
    memcpy(ctx->trampoline_address, ctx->target_original_bytes, ctx->target_original_bytes_size);

    // Calculate what address we're coming back to (the original offset post jump stub).
    unsigned char* actual_return_address = (unsigned char*)ctx->target_function_address;
    actual_return_address = (unsigned char*)(actual_return_address + ctx->target_original_bytes_size);
    memcpy(trampoline_stub + HOTPATCH_ADDRESS_OFFSET, &actual_return_address, sizeof(void*));

    // Write this to our return trampoline.
    memcpy((unsigned char*)ctx->trampoline_address + ctx->target_original_bytes_size, trampoline_stub, sizeof(hotpatch_stub));

    ctx->trampoline_size = ctx->target_original_bytes_size + sizeof(hotpatch_stub);

    // Finally, write the hook stub.
    if (!patch_memory(ctx->target_function_address, hook_stub, sizeof(hotpatch_stub), 0, 1)) { return 0; }

    return 1;
}

int inline_hook(const char* module_name, const char* func_name, size_t target_original_bytes_size, void* replacement_function, void** original_function) {
    void* cf_address = NULL;
    void* throwaway_function_addr = NULL;
    if (!original_function) { original_function = &throwaway_function_addr; }
    struct HotPatch_Info ctx;
    memset(&ctx,0x00,sizeof(struct HotPatch_Info));
    if (!get_function_address(module_name, func_name, &cf_address)) { return 0; }
    if (!hotpatch_function(cf_address, replacement_function, target_original_bytes_size, &ctx, original_function)) { return 0; }
    return 1;
}



