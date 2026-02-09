/*

*/

#include "barf/barf.h"

#include "platform/platform.h"

typedef int (*EntryFN)(const char* path, const char* data, int size);

#ifdef OS_WINDOWS
    #define WIN32_MEAN_AND_LEAN
    #include "Windows.h"
#endif
#ifdef OS_LINUX
    #include "sys/mman.h"
#endif

u8* alloc_memory(u64 size) {
    #ifdef OS_WINDOWS
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
            fprintf(stderr, "flag_memory: win error %u\n", (u32)val);
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
        if (symbol->type != BARF_SYMBOL_GLOBAL)
            continue;

        if (!strcmp(name, symbol_name)) {
            // BarfSection* section = &object->sections[symbol->section_index];
            BarfSegment* segment = &object->segments[symbol->section_index];
            return (u8*)segment->address + symbol->offset;
        }
    }
    return NULL;
}

void* barF_find_name(BarfLoader* loader, const char* name) {
    for (int i=0;i<loader->external_names_len;i++) {
        const char* str = loader->external_names[i];
        if (!strcmp(str, name)) {
            void* ptr = (char*)loader->external_segment + JUMP_ENTRY_STRIDE * i;
            return ptr;
        }
    }
    return NULL;
}

// returns false if there were external symbols
bool barf_apply_relocations(BarfLoader* loader) {

    BarfObject* object = &loader->objects[0];
    for (int si=0;si<object->header.section_count;si++) {
        BarfSection* section = &object->sections[si];
        BarfSegment* segment = &object->segments[si];
        if (section->flags == BARF_FLAG_IGNORE) {
            continue;
        }

        for (int ri=0;ri<section->relocation_count;ri++) {
            BarfRelocation* relocation = &object->relocations[si][ri];
            BarfSymbol* symbol = &object->symbols[relocation->symbol_index];
            const char* name = object->strings + symbol->string_offset;

            if (relocation->type == BARF_RELOC_REL32) {
                void* target_address;
                if (symbol->section_index == -1) {
                    target_address = barF_find_name(loader, name);
                    if (!target_address) {
                        fprintf(stderr, "barf: Cannot relocate external symbol '%s' at %s+0x%x\n", name, section->name, relocation->offset);
                        return false;
                    }
                } else {
                    BarfSegment* target_segment = &object->segments[symbol->section_index];
                    target_address = (void*)(target_segment->address + symbol->offset);
                }

                u32* rel_value = (u32*)(segment->address + relocation->offset);
                
                // If assert is triggered then distance between rel value and target function is too far for
                // a relative jump.
                ASSERT(labs((uint64_t)target_address - (uint64_t)rel_value) < 0x7FFFFFFF);
                // Very important, relocation from COFF on windows we shall ADD
                // the offset to .rdata section, COFF puts the relative offset into the immediate displacement already.

                *rel_value += (u8*)target_address - ((u8*)rel_value + 4);
            } else {
                fprintf(stderr, "barf: Unhandled relocation type %u, %s\n", (u32)relocation->type, name);
            }
        }
    }
    return true;
}

void emit_jmp(void* code_address, void* function_address) {
    ASSERT(sizeof(void*) == 8);
    u8 bytes[] = {
        0x48, 0xb8,
        0,0,0,0, 0,0,0,0,
        0xff, 0xe0,
    };
    ASSERT(sizeof(bytes) == JUMP_ENTRY_STRIDE);
    memcpy(bytes + 2, &function_address, sizeof(void*));
    memcpy(code_address, bytes, sizeof(bytes));
}

void create_platform(BarfLoader* loader) {
    int max_funcs = 128;
    int size = JUMP_ENTRY_STRIDE * max_funcs;
    void* ptr = alloc_memory(size);
    loader->external_segment = ptr;

    char** names = malloc(8 * max_funcs);
    char* text = malloc(max_funcs * 50);
    int text_len = 0;
    
    loader->external_names = names;
    loader->external_names_len = 0;
    
    #undef ADD
    #define ADD(NAME) {                                                 \
        emit_jmp(ptr, (void*)(NAME));                                   \
        ptr =  (char*)ptr + JUMP_ENTRY_STRIDE;                          \
        char* name = text + text_len;                                   \
        memcpy(name, #NAME, strlen(#NAME));                             \
        name[strlen(#NAME)] = '\0';                                     \
        text_len += strlen(#NAME) + 1;                                  \
        loader->external_names[loader->external_names_len++] = name;    \
    }

    ADD(mem__alloc)
    ADD(fs__open)
    ADD(fs__close)
    ADD(fs__info)
    ADD(fs__read)
    ADD(fs__write)
    ADD(log__printf)

    #undef ADD
    

    flag_memory(ptr, size, BARF_FLAG_EXEC);
}

bool barf_init_refptr(BarfLoader* loader) {
    // @TODO Go through all objects
    BarfObject* object = &loader->objects[0];
    for (int si=0;si<object->header.symbol_count;si++) {
        BarfSymbol*  symbol = &object->symbols[si];
        const char* name = object->strings + symbol->string_offset;
        if (strncmp(name, ".refptr.", 8)) {
            continue;
        }
        const char* target_name = name + 8;

        BarfSegment* segment = &object->segments[symbol->section_index];
        
        void* symbol_address = barf_get_address(loader, target_name);
        if (!symbol_address) {
            fprintf(stderr, "barf: Could not find %s referred to by %s\n", target_name, name);
            return false;
        }
        *(void**)(segment->address + symbol->offset) = symbol_address;
    }
    return true;
}


bool barf_load_file(const char* path, int argc, const char** argv) {
    BarfLoader* loader = NULL;
    BarfObject* object = NULL;

    loader = malloc(sizeof(*loader));
    if(!loader) {
        fprintf(stderr, "barf: malloc failed\n");
        goto cleanup;
    }
    memset(loader, 0, sizeof(*loader));

    create_platform(loader);

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
        segment->address = alloc_memory(section->data_size);

        if ((section->flags & BARF_FLAG_ZEROED) == 0) {
            int res = fseek(file, section->data_offset, SEEK_SET);
            ASSERT(res == 0);
            size_t read_bytes = fread(segment->address, 1, section->data_size, file);
            ASSERT(read_bytes == section->data_size);
        }
    }

    fclose(file);

    // Apply relocations
    bool res = barf_apply_relocations(loader);
    if (!res) {
        goto cleanup;
    }

    res = barf_init_refptr(loader);
    if (!res) {
        goto cleanup;
    }
    
    // Set protection flags, previously we set read and write flags so we could memcpy and apply relocations.
    for (int i=0; i< object->header.section_count;i++) {
        BarfSection* section = &object->sections[i];
        BarfSegment* segment = &object->segments[i];

        if (section->flags & BARF_FLAG_IGNORE) {
            continue; 
        }
       flag_memory(segment->address, section->data_size, section->flags);
    }

    // Find entry symbol
    const char* entry_name = "entry";
    // const char* entry_name = "main";
    EntryFN entry = (EntryFN)barf_get_address(loader, entry_name);
    if (!entry) {
        fprintf(stderr, "barf: Could not find entry point '%s'\n", entry_name);
        goto cleanup;
    }

    // @TODO Setup segfault handler

    char* arg_data = NULL;
    int arg_data_len = 0;
    if (argc > 0) {
        int arg_data_cap = argc * 50;
        arg_data = malloc(arg_data_cap);
        for (int i = 0; i < argc;i++) {
            int len = strlen(argv[i]);
            if (arg_data_len != 0) {
                arg_data[arg_data_len] = ' ';
                arg_data_len++;
            }
            memcpy(arg_data + arg_data_len, argv[i], len);
            arg_data_len += len;
        }
    }

    int result = entry(path, arg_data, arg_data_len);
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