#pragma once

#include <stdint.h>
#include <stdbool.h>


// ##########################
//      Execution Domain
// ##########################

typedef struct {
    const char*  file;
    uint32_t     storage_mb; // UINT32_MAX = 4096 TB
    uint32_t     memory_mb;  // UINT32_MAX = 4096 TB
    const char** accesible_paths;
    // how to provide devices: keyboard 1, mouse 1, keyboard 2, storage device 1 (nvme), storage device 2 (hdd)
    // ui, rendering options
} EDOptions;


typedef uint32_t EDHandle;

EDHandle ed__create(const EDOptions options);

EDOptions ed__options(EDHandle handle);

void ed__update(EDHandle handle, const EDOptions options);

// The signature of 'entry' function:
//    void entry(const char* path, const char* data, int size)
void ed__load(EDHandle handle, const char* path, const char* entry, const char* data, int size);

void ed__destroy(EDHandle handle);




// ##########################
//      File System
// ##########################

typedef uint32_t FSHandle;

#define FS_READ  0x1
#define FS_WRITE 0x2
#define FS_INVALID_HANDLE 0xFFFFFFFF
typedef struct {
    uint64_t file_size;
    bool is_directory;
} FSInfo;

FSHandle fs__open(const char* path, uint32_t flags);
void fs__close(FSHandle handle);

void fs__info(FSHandle handle, FSInfo* info);

uint64_t fs__read(FSHandle handle, uint64_t offset, void* buffer, uint64_t size);
uint64_t fs__write(FSHandle handle, uint64_t offset, void* buffer, uint64_t size);

// @TODO Iterate directory, recursively

// ##########################
//      Memory
// ##########################

// allocate:    ptr = mem_alloc(4096, NULL)
// reallocate:  ptr = mem_alloc(4096, ptr)
// free:        mem_alloc(0, ptr)
void* mem__alloc(uint64_t size, void* old_ptr);



// ##########################
//      Debug/logging
// ##########################

void log__printf(const char* format, ...);

