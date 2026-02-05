#include "platform/platform.h"

#ifdef _WIN32

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// ##########################
//      Execution Domain
// ##########################


EDHandle ed__create(const EDOptions options) {
    return 0;
}

EDOptions ed__options(EDHandle handle) {
    EDOptions options = { 0 };
    return options;
}

void ed__update(EDHandle handle, const EDOptions options) {

}

void ed__destroy(EDHandle handle) {

}

// ##########################
//      File System
// ##########################


#define MAX_FILE_HANDLES 100
static FILE* handles[MAX_FILE_HANDLES];
static int handles_len;

FSHandle fs__open(const char* path, uint32_t flags) {
    FILE* file;
    if (flags & FS_WRITE) {
        file = fopen(path, "wb");
    } else if (flags & FS_READ) {
        file = fopen(path, "rb");
    }
    if (!file)
        return FS_INVALID_HANDLE;

    // @TODO Atomic operation
    FSHandle handle = handles_len;
    handles[handles_len] = file;
    handles_len++;
    return handle;
}
void fs__close(FSHandle handle) {
    FILE* file = handles[handle];

    int res = fclose(file);

    // @TODO Atomic operation
    handles[handle] = NULL;
    while (handles_len > 0 && handles[handles_len] == NULL) {
        handles_len--;
    }
}

void fs__info(FSHandle handle, FSInfo* info) {
    FILE* file = handles[handle];
    
    size_t cur_pos = ftell(file);
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, cur_pos, SEEK_SET);

    info->file_size = file_size;
    info->is_directory = false;
}
uint64_t fs__read(FSHandle handle, uint64_t offset, void* buffer, uint64_t size) {
    FILE* file = handles[handle];
    
    fseek(file, offset, SEEK_SET);
    return fread(buffer, 1, size, file);
}
uint64_t fs__write(FSHandle handle, uint64_t offset, void* buffer, uint64_t size) {
    FILE* file = handles[handle];
    
    fseek(file, offset, SEEK_SET);
    return fwrite(buffer, 1, size, file);
}

// ##########################
//      Memory
// ##########################

void* mem__alloc(uint64_t size, void* old_ptr) {
    if (size == 0) {
        free(old_ptr);
    } else if (!old_ptr) {
        return malloc(size);
    }
    return realloc(old_ptr, size);
}


// ##########################
//      Debug/logging
// ##########################

void log__printf(const char* format, ...) {
    va_list va;
    va_start(va, format);
    vprintf(format, va);
    va_end(va);
}

#else


#endif
