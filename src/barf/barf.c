/*

*/

#include "barf/barf.h"

typedef int (*EntryFN)(int argc, char** argv);

#ifdef OS_WINDOWS
    #define WIN32_MEAN_AND_LEAN
    #include "Windows.h"
#endif
#ifdef OS_LINUX
    #include "sys/mman.h"
#endif

u8* alloc_memory(u64 size, BarfSectionFlags flags) {
    #ifdef OS_WINDOWS
        // u32 mem_flags;
        // if ((flags & BARF_FLAG_EXEC)) {
        //     mem_flags = PAGE_EXECUTE_READWRITE;
        // } else {
        //     mem_flags = PAGE_READWRITE;
        // }
        // return VirtualAlloc(NULL, size, MEM_COMMIT, mem_flags);
        return VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);
    #else
        return mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_ANONYMOUS, -1, 0);
    #endif
}

void flag_memory(u8* address, u64 size, BarfSectionFlags flags) {
    #ifdef OS_WINDOWS
        u32 mem_flags;
        if ((flags & BARF_FLAG_EXEC) && (flags & BARF_FLAG_WRITE)) {
            mem_flags = PAGE_EXECUTE_READWRITE;
        } else if ((flags & BARF_FLAG_EXEC)) {
            mem_flags = PAGE_EXECUTE;
        } else if ((flags & BARF_FLAG_WRITE)) {
            mem_flags = PAGE_READWRITE;
        } else {
            mem_flags = PAGE_READONLY;
        }
        DWORD prev_flags;
        BOOL res = VirtualProtect(address, size, mem_flags, &prev_flags);
        if (!res) {
            DWORD val = GetLastError();
            fprintf(stderr, "flag_memory: win error %u\n", val);
        }
    #else
        u32 mem_flags = PROT_READ;
        if ((flags & BARF_FLAG_EXEC)) {
            mem_flags |= PROT_EXEC;
        } else if ((flags & BARF_FLAG_WRITE)) {
            mem_flags |= PROT_WRITE;
        }
        mprotect(address, size, mem_flags);
    #endif
}


void free_memory(u8* address, u64 size) {
    #ifdef OS_WINDOWS
        VirtualFree(address, size, MEM_RELEASE);
    #else
        munmap(address, size);
    #endif
}

void* barf_get_address(BarfLoader* loader, const char* name) {
    BarfObject* object = &loader->objects[0];

    for (int i=0;i<object->header.symbol_count;i++) {
        BarfSymbol* symbol = &object->symbols[i];
        const char* symbol_name = object->strings + symbol->string_offset;
        if (symbol->section_index == -1) {
            ASSERT(symbol->type == BARF_SYMBOL_EXTERNAL);
            // @TODO This check should not be necessary because all symbols should be resolved.
            //   Loader will give an error.
            continue;
        }
        if (!strcmp(name, symbol_name)) {
            // BarfSection* section = &object->sections[symbol->section_index];
            BarfSegment* segment = &object->segments[symbol->section_index];
            return (u8*)segment->address + symbol->offset;
        }
    }
    return NULL;
}


bool barf_load_file(const char* path) {
    BarfLoader* loader = NULL;
    BarfObject* object = NULL;

    loader = malloc(sizeof(*loader));
    if(!loader) {
        fprintf(stderr, "barf: malloc failed\n");
        goto cleanup;
    }
    memset(loader, 0, sizeof(*loader));


    object = barf_parse_header_from_file(path);
    if (!object) {
        return false;
    }

    loader->objects = object;
    loader->object_count = 1;

    object->segments = malloc(sizeof(*object->segments) * object->header.section_count);
    memset(object->segments, 0, sizeof(*object->segments) * object->header.section_count);

    FILE* file = fopen(path, "rb");

    for (int i=0; i< object->header.section_count;i++) {
        BarfSection* section = &object->sections[i];
        BarfSegment* segment = &object->segments[i];

        if (section->flags & BARF_FLAG_IGNORE) {
            continue; 
        }

        // @TODO Ensure section alignment
        segment->address = alloc_memory(section->data_size, section->flags);

        if ((section->flags & BARF_FLAG_ZEROED) == 0) {
            int res = fseek(file, section->data_offset, SEEK_SET);
            ASSERT(res == 0);
            size_t read_bytes = fread(segment->address, 1, section->data_size, file);
            ASSERT(read_bytes == section->data_size);
        }

        flag_memory(segment->address, section->data_size, section->flags);
    }

    fclose(file);

    // Apply relocations

    // Find entry symbol
    const char* entry_name = "main";
    EntryFN entry = (EntryFN)barf_get_address(loader, entry_name);
    if (!entry) {
        fprintf(stderr, "barf: Could not find entry point '%s'\n", entry_name);
        return false;
    }


    int result = entry(0, NULL);
    printf("Exit code: %d", result);

    // alloc memory

    for (int i=0; i< object->header.section_count;i++) {
        BarfSection* section = &object->sections[i];
        BarfSegment* segment = &object->segments[i];
        if (section->flags & BARF_FLAG_IGNORE) {
            continue;
        }
        free_memory(segment->address, section->data_size);
    }

    return true;

cleanup:
    return false;
}