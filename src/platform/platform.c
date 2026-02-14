#include "platform/platform.h"

#ifdef OS_WINDOWS
    #define WIN32_MEAN_AND_LEAN
    #include "Windows.h"
    #include <stdarg.h>
    #include <stdio.h>
    #include <string.h>
    #include <stdlib.h>
#endif
#ifdef OS_LINUX
    #include "sys/mman.h"
    #include <stdarg.h>
    #include <stdio.h>
    #include <string.h>
    #include <stdlib.h>
#endif


// #define platform_log(...) log__printf(__VA_ARGS__)
#define platform_log(...)


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

#if defined(OS_WINDOWS) || defined(OS_LINUX)
    #define MAX_FILE_HANDLES 100
    static FILE* handles[MAX_FILE_HANDLES];
    static int handles_len;
#endif

FSHandle fs__open(const char* path, uint32_t flags) {
    #if defined(OS_WINDOWS) || defined(OS_LINUX)
        FILE* file;
        if (flags & FS_WRITE) {
            file = fopen(path, "wb");
        } else if (flags & FS_READ) {
            file = fopen(path, "rb");
        }
        if (!file)
            return FS_INVALID_HANDLE;


        // @TODO Atomic operation
        if (handles_len >= MAX_FILE_HANDLES)
            return FS_INVALID_HANDLE;
        FSHandle handle = handles_len;
        handles[handles_len] = file;
        handles_len++;

        platform_log("Open [%d] = %s, %u\n", (int)handle, path, flags);
        return handle;
    #endif
}
void fs__close(FSHandle handle) {
    #if defined(OS_WINDOWS) || defined(OS_LINUX)
        FILE* file = handles[handle];

        int res = fclose(file);

        // @TODO Atomic operation
        handles[handle] = NULL;
        while (handles_len > 0 && handles[handles_len-1] == NULL) {
            handles_len--;
        }
        platform_log("Close [%d]\n", (int)handle);
    #endif
}

void fs__info(FSHandle handle, FSInfo* info) {
    #if defined(OS_WINDOWS) || defined(OS_LINUX)
        FILE* file = handles[handle];
        
        size_t cur_pos = ftell(file);
        fseek(file, 0, SEEK_END);
        size_t file_size = ftell(file);
        fseek(file, cur_pos, SEEK_SET);

        info->file_size = file_size;
        info->is_directory = false;

        platform_log("FSInfo [%d] = %u\n", (int)handle, (unsigned)file_size);
    #endif
}
uint64_t fs__read(FSHandle handle, uint64_t offset, void* buffer, uint64_t size) {
    #if defined(OS_WINDOWS) || defined(OS_LINUX)
        FILE* file = handles[handle];
        fseek(file, offset, SEEK_SET);
        size_t res = fread(buffer, 1, size, file);
        
        platform_log("Read [%d] = %u\n", (int)handle, (unsigned)res);
        return res;
    #endif
}
uint64_t fs__write(FSHandle handle, uint64_t offset, void* buffer, uint64_t size) {
    #if defined(OS_WINDOWS) || defined(OS_LINUX)
        FILE* file = handles[handle];
        fseek(file, offset, SEEK_SET);
        size_t res = fwrite(buffer, 1, size, file);
        platform_log("Write [%d] %d, %d = written %u\n", (int)handle, (int)offset, (int)size, (unsigned)res);
        return res;
    #endif
}

// ##########################
//      Memory
// ##########################

void* mem__alloc(uint64_t size, void* old_ptr) {
    if (size == 0) {
        free(old_ptr);
        return NULL;
    } else if (!old_ptr) {
        return malloc(size);
    }
    return realloc(old_ptr, size);
}


void* mem__map(void* address, uint64_t size, int flags) {
    #ifdef OS_WINDOWS
        return VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);
    #endif
    #ifdef OS_LINUX
        return mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_ANONYMOUS, -1, 0);
    #endif
}
void mem__mapflag(void* address, uint64_t size, int flags) {
    #ifdef OS_WINDOWS
        int mem_flags;
        if ((flags & MEM_EXEC) && (flags & MEM_WRITE)) {
            mem_flags = PAGE_EXECUTE_READWRITE;
        } else if ((flags & MEM_EXEC)) {
            mem_flags = PAGE_EXECUTE;
        } else if ((flags & MEM_WRITE)) {
            mem_flags = PAGE_READWRITE;
        } else {
            mem_flags = PAGE_READONLY;
        }
        DWORD prev_flags;
        BOOL res = VirtualProtect(address, size, mem_flags, &prev_flags);
        if (!res) {
            DWORD val = GetLastError();
            log__printf("flag_memory: win error %u\n", (unsigned)val);
        }
    #endif
    #ifdef OS_LINUX
        int mem_flags = PROT_READ;
        if ((flags & MEM_EXEC)) {
            mem_flags |= PROT_EXEC;
        } else if ((flags & MEM_WRITE)) {
            mem_flags |= PROT_WRITE;
        }
        mprotect(address, size, mem_flags);
    #endif
}
void mem__unmap(void* address, uint64_t size) {
    #ifdef OS_WINDOWS
        VirtualFree(address, size, MEM_RELEASE);
    #endif
    #ifdef OS_LINUX
        munmap(address, size);
    #endif
}


// ##########################
//      Debug/logging
// ##########################

void log__printf(const char* format, ...) {
    va_list va;
    va_start(va, format);
    vfprintf(stderr, format, va);
    va_end(va);
}
