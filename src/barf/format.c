#include "barf/barf.h"

#include "barf/elf.h"
#include "barf/coff.h"

#define debug(...) fprintf(stderr, __VA_ARGS__)
// #define debug(...)

BarfObject* barf_parse_header_from_file(const char* path) {
    BarfObject* object = NULL;
    FILE* file = NULL;
    
    file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "ERROR barf: '%s' not found\n", path);
        goto cleanup;
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    object = malloc(sizeof(*object));
    if (!object) {
        fprintf(stderr, "ERROR barf: malloc failed, when parsing '%s'\n", path);
        goto cleanup;
    }
    memset(object, 0, sizeof(*object));

    if (file_size < sizeof(object->header)) {
        fprintf(stderr, "ERROR barf: file to small for header, when parsing '%s'\n", path);
        goto cleanup;
    }

    fread(&object->header, 1, sizeof(object->header),  file);

    if (object->header.magic != BARF_MAGIC) {
        union {
            u32 raw;
            char name[4];
        } magic = { BARF_MAGIC };
        fprintf(stderr, "ERROR barf: magic incorrect %u == %.4s (should be %4.s)\n", object->header.magic, (char*)&object->header.magic, magic.name);
        goto cleanup;
    }
    
    u64 size_of_sections = object->header.section_count * sizeof(*object->sections);

    if (file_size < size_of_sections) {
        fprintf(stderr, "ERROR barf: file to small for sections, when parsing '%s'\n", path);
        goto cleanup;
    }

    object->sections = malloc(size_of_sections);
    if (!object->sections) {
        fprintf(stderr, "ERROR barf: malloc failed, when parsing '%s'\n", path);
        goto cleanup;
    }
    memset(object->sections, 0, size_of_sections);
    
    fread(object->sections, sizeof(*object->sections), object->header.section_count, file);

    
    u64 size_of_symbols = object->header.symbol_count * sizeof(*object->symbols);
    object->symbols = malloc(size_of_symbols);
    memset(object->symbols, 0, size_of_symbols);
    fread(object->symbols, sizeof(*object->symbols), object->header.symbol_count, file);

    object->strings = malloc(object->header.string_size);
    memset(object->strings, 0, object->header.string_size);
    fread(object->strings, 1, object->header.string_size, file);

    u64 size_of_relocations_list = object->header.section_count * sizeof(*object->relocations);
    
    object->relocations = malloc(size_of_relocations_list);
    memset(object->relocations, 0, size_of_relocations_list);

    for (int i=0;i<object->header.section_count;i++) {
        BarfSection* section = &object->sections[i];

        if (section->relocation_count == 0) {
            continue;
        }

        u64 size_of_relocations = section->relocation_count * sizeof(**object->relocations);
        BarfRelocation* relocations = malloc(size_of_relocations);
        memset(relocations, 0, size_of_relocations);

        fseek(file, section->relocation_offset, SEEK_SET);
        fread(relocations, sizeof(**object->relocations), section->relocation_count, file);

        object->relocations[i] = relocations;
    }


    fclose(file);

    return object;

cleanup:
    if (file)
        fclose(file);
    if (object) {
        if (object->sections)
            free(object->sections);
        free(object);
    }
    return NULL;
}



void barf_free_object(BarfObject* object) {
    free(object->sections);
    free(object);
}


void barf_dump(BarfObject* object) {
    #define log(...) fprintf(stderr, __VA_ARGS__)

    log("BARF Format\n");
    log(" version: %u\n", object->header.version);
    log(" target: %s\n", object->header.target);
    log(" flags: ");
    if (object->header.flags & BARF_FLAG_BIG_ENDIAN) {
        log("Big Endian");
    } else {
        log("Little Endian");
    }
    log("\n");
    log(" symbols: %u\n", object->header.symbol_count);
    log(" strings: %u\n", object->header.string_size);
    log(" sections: %u\n", object->header.section_count);
    log("\n");

    for (u32 i=0;i<object->header.section_count;i++) {
        BarfSection* section = &object->sections[i];
        log(" Section '%s'\n", section->name);
        log("   flags: ");
        if (section->flags & BARF_FLAG_WRITE) {
            log("WRITE ");
        }
        if (section->flags & BARF_FLAG_EXEC) {
            log("EXEC ");
        }
        if (section->flags & BARF_FLAG_ZEROED) {
            log("ZEROED ");
        }
        if (section->flags & BARF_FLAG_IGNORE) {
            log("IGNORE ");
        }
        log("\n");
        log("   align:  %hu\n", section->alignment);
        log("   offset: "FLU"\n", section->data_offset);
        log("   size:   "FLU"\n", section->data_size);
        log("   reloc_offset: "FLU"\n", section->relocation_offset);
        log("   reloc_count:  %u\n", section->relocation_count);

        for (u32 ri=0; ri < section->relocation_count; ri++) {
            BarfRelocation* relocation = &object->relocations[i][ri];
            BarfSymbol* symbol = &object->symbols[relocation->symbol_index];
            const char* name = object->strings + symbol->string_offset;
            if (symbol->type == BARF_SYMBOL_LOCAL) {
                log("     0x%x %s (local symbol)\n", relocation->offset, name);
            } else if (symbol->type == BARF_SYMBOL_GLOBAL) {
                if (symbol->section_index >= 0 && symbol->section_index < object->header.section_count) {
                    BarfSection* sym_section = &object->sections[symbol->section_index];
                    log("     0x%x %s (in %s)\n", relocation->offset, name, sym_section->name);
                } else {
                    log("     0x%x %s (in section index %d, bad index)\n", relocation->offset, name, symbol->section_index);
                }
            } else {
                log("     0x%x %s (external symbol)\n", relocation->offset, name);
            }
        }
    }
    for (u32 i=0;i<object->header.symbol_count;i++) {
        BarfSymbol* symbol = &object->symbols[i];
        const char* name = object->strings + symbol->string_offset;
        log(" Symbol '%s'\n", name);
        log("   type: ");
        if (symbol->type == BARF_SYMBOL_LOCAL) {
            log("LOCAL\n");
        } else if (symbol->type == BARF_SYMBOL_GLOBAL) {
            log("GLOBAL\n");
        } else if (symbol->type == BARF_SYMBOL_EXTERNAL) {
            log("EXTERNAL\n");
        }
        if (symbol->type != BARF_SYMBOL_EXTERNAL) {
            const char* section_name = object->sections[symbol->section_index].name;
            log("   section: %s\n", section_name);
            log("   offset: %u\n", symbol->offset);
        }
    }
}

bool barf_convert_from_coff(const char* path, const char* output) {
    BarfObject* object   = NULL;
    FILE*       file     = NULL;
    u8*         data     = NULL;
    u64         dataSize = 0;
    
    file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "barf: could not read '%s'\n", path);
        goto cleanup;
    }

    fseek(file, 0, SEEK_END);
    dataSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    data = malloc(COFF_File_Header_SIZE);

    // printf("filesize %d\n", (int)filesize);

    size_t read_bytes = fread(data, 1, COFF_File_Header_SIZE, file);
    // LOOP
    if(read_bytes != COFF_File_Header_SIZE) {
        // not COFF
        return false;
    }

    // Heuristic checks for COFF format
    COFF_File_Header* header = (COFF_File_Header*) data;
    if (
        (header->Machine != IMAGE_FILE_MACHINE_AMD64 &&
        header->Machine != IMAGE_FILE_MACHINE_ARM &&
        header->Machine != IMAGE_FILE_MACHINE_ARM64 &&
        header->Machine != IMAGE_FILE_MACHINE_I386) ||
        (header->NumberOfSections == 0 || header->NumberOfSections > 256) ||
        header->SizeOfOptionalHeader != 0 // we only handle object files and not images since they don't have relocations (dlls might have some but not really)
    ) {
        goto cleanup;
    }

    // Probably COFF, read rest of file at once (easiest)
    data = realloc(data, dataSize);
    read_bytes = fread(data + COFF_File_Header_SIZE, 1, dataSize - COFF_File_Header_SIZE, file);
    if(read_bytes != dataSize - COFF_File_Header_SIZE) {
        fprintf(stderr, "barf: could not read all bytes '%s'\n", path);
        goto cleanup;
    }

    fclose(file);
    file = NULL;
    
    header = (COFF_File_Header*) data; // set again after realloc
    
    object = malloc(sizeof(*object));
    memset(object, 0, sizeof(*object));

    object->header.magic = BARF_MAGIC;
    object->header.version = 1;
    object->header.flags = 0; // little endian

    switch (header->Machine) {
        case IMAGE_FILE_MACHINE_AMD64: strcpy(object->header.target, "x86_64");  break;
        case IMAGE_FILE_MACHINE_I386:  strcpy(object->header.target, "x86");     break;
        case IMAGE_FILE_MACHINE_ARM:   strcpy(object->header.target, "arm");     break;
        case IMAGE_FILE_MACHINE_ARM64: strcpy(object->header.target, "aarch64"); break;
        default: strcpy(object->header.target, "unknown");
    }

    object->sections = malloc(header->NumberOfSections * sizeof(*object->sections));
    memset(object->sections, 0, header->NumberOfSections * sizeof(*object->sections));

    
    u64 size_of_relocations_list = header->NumberOfSections * sizeof(*object->relocations);
    object->relocations = malloc(size_of_relocations_list);
    memset(object->relocations, 0, size_of_relocations_list);


    u64 offset_of_sections = COFF_File_Header_SIZE + header->SizeOfOptionalHeader;
    u64 offset_of_strings = header->PointerToSymbolTable + header->NumberOfSymbols * Symbol_Record_SIZE;
    u64 size_of_strings   = *(u32*)(data + offset_of_strings) + header->NumberOfSymbols * 9; // first 4 bytes of string table is the size of the table, the symbol names can be "inlined" in symbol records if they are small but in BARF that's not the case so we need to add symbol_len*9 (8 + 1 because length of shortname + null char)

    typedef struct {
        int section_index;
    } SectionInfo;

    SectionInfo* section_infos = malloc(sizeof(SectionInfo) * header->NumberOfSections);
    memset(section_infos, 0, sizeof(SectionInfo) * header->NumberOfSections);

    for (int i = 0; i < header->NumberOfSections; i++) {
        Section_Header* section = (Section_Header*)(data + offset_of_sections + i * Section_Header_SIZE);
        section_infos[i].section_index = -1;

        char _name[12];
        char* name = _name;
        memcpy(_name, section->Name, 8);
        _name[8] = '\0';
        if (section->Name[0] == '/') {
            char* endptr;
            uint32 name_offset = strtoul(_name+1, &endptr, 10);
            // ASSERT(endptr) corrupt COFF
            name = (char*)(data + offset_of_strings + name_offset);
        }

        // printf("%s\n", name);

        BarfSection* sec = &object->sections[object->header.section_count];

        int namelen = strlen(name);
        if (namelen >= sizeof(sec->name)) {
            fprintf(stderr, "  name to long, '%s'\n", name);
            continue;
        }
        if (section->SizeOfRawData == 0) {
            // skip empty sections
            continue;
        }

        section_infos[i].section_index = object->header.section_count;
        object->header.section_count++;
        
        strcpy(sec->name, name);
        sec->data_size = section->SizeOfRawData;
        sec->data_offset = section->PointerToRawData; // Temporarily set offset in COFF to section data, updated later
        
        if (section->Characteristics & IMAGE_SCN_MEM_EXECUTE) {
            sec->flags |= BARF_FLAG_EXEC;
        }
        if (section->Characteristics & IMAGE_SCN_MEM_WRITE) {
            sec->flags |= BARF_FLAG_WRITE;
        }
        if (section->Characteristics & IMAGE_SCN_LNK_INFO) {
            sec->flags |= BARF_FLAG_IGNORE;
        }
        if (section->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA) {
            sec->flags |= BARF_FLAG_ZEROED;
        }

        sec->alignment = 1 << (((section->Characteristics >> 20) & 0xF) - 1);
    }

    object->strings = malloc(size_of_strings);
    memset(object->strings, 0, size_of_strings);

    object->symbols = malloc(header->NumberOfSymbols * sizeof(*object->symbols));
    memset(object->symbols, 0, header->NumberOfSymbols * sizeof(*object->symbols));

    // Need a way to map Coff symbol numbers to barf symbol indexes, since aux symbols aren't carried over and number indexes don't map 1:1

    typedef struct {
        u32 symbol_index;
    } SymbolInfo;
    SymbolInfo* symbol_infos = malloc(sizeof(SymbolInfo) * header->NumberOfSymbols);
    memset(symbol_infos, 0, sizeof(SymbolInfo) * header->NumberOfSymbols);

    u64 next_string_offset = 0;

    for (int i=0;i<header->NumberOfSymbols;i++) {
        Symbol_Record* symbol = (Symbol_Record*)(data + header->PointerToSymbolTable + i * Symbol_Record_SIZE);
        BarfSymbol*    sym = &object->symbols[object->header.symbol_count];

        symbol_infos[i].symbol_index = -1;

        if (symbol->SectionNumber < 0) {
             if (symbol->NumberOfAuxSymbols) {
                i += symbol->NumberOfAuxSymbols;
            }
            continue;
        }

        char _name[12];
        memcpy(_name, symbol->Name.ShortName, 8);
        _name[8] = '\0';
        char* name = _name;
        if (symbol->Name.zero == 0) {
            name = (char*)(data + offset_of_strings + symbol->Name.offset);
        }
        int name_len = strlen(name);

        if (symbol->SectionNumber == 0) {
            sym->type = BARF_SYMBOL_EXTERNAL;
            sym->section_index = -1;
        } else {
            sym->section_index = section_infos[symbol->SectionNumber-1].section_index;
            if (symbol->StorageClass == IMAGE_SYM_CLASS_STATIC) {
                sym->type = BARF_SYMBOL_LOCAL;
            } else {
                sym->type = BARF_SYMBOL_GLOBAL;
            }
        }
        sym->string_offset = next_string_offset;

        sym->offset = symbol->Value;

        memcpy(object->strings + next_string_offset, name, name_len + 1);
        next_string_offset += name_len + 1;

        symbol_infos[i].symbol_index = object->header.symbol_count;
        object->header.symbol_count++;

        if (symbol->NumberOfAuxSymbols) {
            i += symbol->NumberOfAuxSymbols;
        }
    }

    object->header.string_size = next_string_offset;
    

    for (int si = 0; si < header->NumberOfSections; si++) {
        Section_Header* section = (Section_Header*)(data + offset_of_sections + si * Section_Header_SIZE);
        SectionInfo* section_info = &section_infos[si];

        if (section_info->section_index == -1) {
            continue;
        }
        BarfSection* sec = &object->sections[section_info->section_index];

        if (!section->NumberOfRelocations) {
            continue;
        }
        BarfRelocation* relocations = malloc(section->NumberOfRelocations * sizeof(BarfRelocation));
        object->relocations[si] = relocations;
        memset(relocations, 0, section->NumberOfRelocations * sizeof(BarfRelocation));
        sec->relocation_offset = section->PointerToRelocations;

        for (int ri=0;ri<section->NumberOfRelocations;ri++) {
            COFF_Relocation* relocation = (COFF_Relocation*)(data + section->PointerToRelocations + ri * COFF_Relocation_SIZE);
            BarfRelocation* rel = &relocations[sec->relocation_count];
            u32 symbol_index = symbol_infos[relocation->SymbolTableIndex].symbol_index;

            BarfSymbol* symbol = &object->symbols[symbol_index];
            const char* name = object->strings + symbol->string_offset;

            if (relocation->Type == IMAGE_REL_AMD64_REL32) {
                rel->type = BARF_RELOC_REL32;
                rel->symbol_index = symbol_index;
                rel->offset = relocation->VirtualAddress;
                sec->relocation_count++;
            } else {
                printf("barf: Unhandled coff reloc type %d, sym index %u\n", relocation->Type, symbol_index);
            }
        }
    }

    // barf_dump(object);

    file = fopen(output, "wb");
    if (!file) {
        fprintf(stderr, "barf: Could not open '%s'\n", output);
        goto cleanup;
    }

    u64 size_of_symbols = sizeof(*object->symbols) * object->header.symbol_count;
    u64 size_of_sections = sizeof(*object->sections) * object->header.section_count;
    
    object->header.section_offset = sizeof(object->header);
    object->header.symbol_offset  = sizeof(object->header) + size_of_sections;
    object->header.string_offset  = sizeof(object->header) + size_of_sections + size_of_symbols;
    
    u64 next_section_data_offset = sizeof(object->header) + size_of_sections + size_of_symbols + object->header.string_size;

    for (int i = 0; i < object->header.section_count; i++) {
        BarfSection* section = &object->sections[i];

        next_section_data_offset += (section->alignment - (next_section_data_offset % section->alignment)) % section->alignment;

        u64 new_offset = next_section_data_offset;

        fseek(file, next_section_data_offset, SEEK_SET);
        fwrite(data + section->data_offset, 1, section->data_size, file);

        next_section_data_offset += section->data_size;

        section->data_offset = new_offset;
        
        next_section_data_offset += (8 - (next_section_data_offset % 8)) % 8;

        new_offset = next_section_data_offset;

        fseek(file, next_section_data_offset, SEEK_SET);
        fwrite(object->relocations[i], sizeof(BarfRelocation), section->relocation_count, file);
        
        section->relocation_offset = new_offset;

        next_section_data_offset += section->relocation_count * sizeof(BarfRelocation);
    }
    
    object->header.total_size = ftell(file);

    fseek(file, 0, SEEK_SET);
    fwrite(&object->header, sizeof(object->header), 1, file);

    fwrite(object->sections, sizeof(*object->sections), object->header.section_count, file);
    
    fwrite(object->symbols, sizeof(*object->symbols), object->header.symbol_count, file);
    
    fwrite(object->strings, 1, object->header.string_size, file);
    fclose(file);
    free(object);
    return true;

cleanup:
    if (file)
        fclose(file);
    if (object)
        free(object);
    if (data)
        free(data);
    return false;
}
bool barf_convert_from_elf(const char* path, const char* output) {
    // Elf64_Ehdr* header = (Elf64_Ehdr*)data;
    // object->sections = malloc(header->e_shnum * sizeof(*object->sections));
    // memset(object->sections, 0, header->e_shnum * sizeof(*object->sections));

    // Elf64_Shdr* sections = (Elf64_Shdr*)(data + header->e_shoff);
    // // char* names = (char*)data + sections[header->e_shstrndx].sh_offset;
    // char* names = (char*)data + header->e_shstrndx;

    // for (int i = 0; i < header->e_shnum; i++) {
    //     Elf64_Shdr* section = (Elf64_Shdr*)(data + header->e_shoff);
    //     char* name =  names + section->sh_name;
    //     printf("%s\n", name);

    //     BarfSection* sec = &object->sections[object->header.section_count];

    //     int namelen = strlen(name);
    //     if (namelen >= sizeof(sec->name)) {
    //         printf("  name to long");
    //         continue;
    //     }

    //     object->header.section_count++;
        
    //     strcpy(sec->name, name);
    //     sec->size = section->sh_size;
    //     sec->offset = 0; // set later

    //     // sec->flags = 
    // }

    // return true;
    return false;
}

void transfer_bytes(FILE* input, u64 in_offset, FILE* output, u64 out_offset, u64 size) {
    #define TRANSFER_BUFFER_SIZE 4096
    char buffer[TRANSFER_BUFFER_SIZE];
    fseek(input, in_offset, SEEK_SET);
    fseek(output, out_offset, SEEK_SET);
    while (size > 0) {
        u64 chunk_size = size < TRANSFER_BUFFER_SIZE ? size : TRANSFER_BUFFER_SIZE;
        size_t read_bytes = fread(buffer, 1, chunk_size, input);
        ASSERT(read_bytes == chunk_size);
        size_t written_bytes = fwrite(buffer, 1, chunk_size, output);
        ASSERT(written_bytes == chunk_size);
        size -= chunk_size;
    }
}

bool barf_combine_to_artifact(int input_count, const char** input_files, const char* output) {
    // FILE*       file   = NULL;
    // u8*         data   = NULL;
    // BarfObject* object = NULL;

    if (input_count == 0)
        // Should we make empty BA? probably not right?
        return false;

    // convert non BA files to BA, this means convert elf and coff to BA
    // first find all ELF fiels, convert to BA
    // find all COFF files, convert to BA

    // We can be inefficient to begin with.


    // then combine all BA files into one

    int    ba_path_text_cap = 0x10000;
    int    ba_path_text_len = 0;
    char*  ba_path_text     = malloc(ba_path_text_cap);
    const char** ba_paths         = malloc(sizeof(char*) * input_count);

    for (int i=0;i<input_count;i++) {
        const char* input = input_files[i];
        const char* ba_path = ba_path_text + ba_path_text_len;
        ba_paths[i] = ba_path;
        int input_len = strlen(input);
        int dot = input_len-1;
        while (dot > 0 && input[dot] != '.') dot--;
        if (dot != -1)
            input_len -= input_len - dot;
        ba_path_text_len += 1 + snprintf(ba_path, ba_path_text_cap - ba_path_text_len, "%.*s.ba", input_len, input);

        // @TODO ba_path should be in temporary 'int' directory. Same directory
        //   as the object files will work for now.
        // @TODO If we want to be professional then we might want to handle
        //   duplicate input, collision with some elf file being named the same as a barf file.

        bool res;
        res = barf_convert_from_coff(input, ba_path);
        if (!res)
            res = barf_convert_from_elf(input, ba_path);
        if (!res)
            ba_paths[i] = input;
    }

    BarfObject** objects = malloc(sizeof(BarfObject*) * input_count);

    int estimated_symbol_count = 0;
    int estimated_section_count = 0;
    int estimated_string_size = 0;

    for (int i=0; i<input_count;i++) {
        objects[i] = barf_parse_header_from_file(ba_paths[i]);
        if (!objects[i]) {
            // already printed error
            goto cleanup;
        }

        if (i != 0) {
            if (strcmp(objects[0]->header.target, objects[i]->header.target)) {
                fprintf(stderr, "barf: Target architectures did not match, %s vs %s (%s and %s)\n", objects[0]->header.target, objects[i]->header.target, input_files[0], input_files[i]);
                goto cleanup;
            }
        }
        estimated_section_count += objects[i]->header.section_count;
        estimated_symbol_count  += objects[i]->header.symbol_count;
        estimated_string_size   += objects[i]->header.string_size;
    }

    BarfObject* merged = malloc(sizeof(BarfObject));
    memset(merged, 0, sizeof(*merged));
    merged->header.magic = BARF_MAGIC;
    merged->header.version = 1;
    memcpy(merged->header.target, objects[0]->header.target, sizeof(merged->header.target));
    
    merged->sections = malloc(sizeof(BarfSection) * estimated_section_count);
    memset(merged->sections, 0, sizeof(BarfSection) * estimated_section_count);
    // merged->header.section_count = estimated_section_count;
   
    merged->relocations = malloc(sizeof(BarfRelocation*) * estimated_section_count);
    memset(merged->relocations, 0, sizeof(BarfRelocation*) * estimated_section_count);

    merged->strings = malloc(estimated_string_size);

    // @TODO We have tons of memory leaks here. Fix 'em up

    // @TODO The same strings may exist in some artifacts, especially symbol names. (only symbol names use string data at the moment)
    //    We could create string data from symbols names anew.

    int* string_mapping = malloc(input_count * sizeof(int));
    int** section_mapping = malloc(input_count * sizeof(int*));
    for (int oi=0; oi<input_count;oi++) {
        BarfObject* object = objects[oi];

        // string_mapping[oi] = merged->header.string_size;
        // memcpy(merged->strings + merged->header.string_size, object->strings, object->header.string_size);
        // merged->header.string_size += object->header.string_size;

        section_mapping[oi] = malloc(object->header.section_count * sizeof(int));

        for (int si=0;si<object->header.section_count;si++) {
            BarfSection* section = &object->sections[si];
            
            // Map [object index, section index] to [merged section index]
            section_mapping[oi][si] = merged->header.section_count;

            BarfSection* merged_section = &merged->sections[merged->header.section_count];
            merged->header.section_count++;

            memcpy(merged_section->name, section->name, sizeof(section->name));
            merged_section->alignment = section->alignment;
            merged_section->flags = section->flags;
            merged_section->relocation_count = section->relocation_count;
            merged_section->data_size = section->data_size;

            merged_section->data_offset = section->data_offset;
            merged_section->relocation_offset = section->relocation_offset;
        }
    }

    // Create a map from [symbol index] to [merged symbol index]

    // Merge external and global symbols, symbols, merg
    merged->symbols = malloc(sizeof(BarfSymbol) * estimated_symbol_count);
    memset(merged->symbols, 0, sizeof(BarfSymbol) * estimated_symbol_count);

    int** symbol_mapping = malloc(input_count * sizeof(int*));

    // @TODO Consider separating local,external,global symbol lists so
    //  it's easier to merge, add and remove the different types.
    //    local are always added
    //    external may be removed if there is an equivalent global
    //    or merged if there is one
    //    globals are checked for duplicates

    for (int bi=0;bi<input_count;bi++) {
        BarfObject* object = objects[bi];
        symbol_mapping[bi] = malloc(object->header.symbol_count * sizeof(int));
        memset(symbol_mapping[bi], 0xDE, object->header.symbol_count * sizeof(int));

        
        for (int si = 0; si < object->header.symbol_count; si++) {
            BarfSymbol* symbol = &object->symbols[si];
            const char* name = object->strings + symbol->string_offset;

            if (symbol->type == BARF_SYMBOL_LOCAL) {
                // Local symbols never collide. The name is not relevant.
                
                symbol_mapping[bi][si] = merged->header.symbol_count;
                BarfSymbol* merged_symbol = &merged->symbols[merged->header.symbol_count];
                merged->header.symbol_count++;

                merged_symbol->type = BARF_SYMBOL_LOCAL;
                merged_symbol->offset = symbol->offset;
                merged_symbol->section_index = section_mapping[bi][symbol->section_index];
                merged_symbol->string_offset = merged->header.string_size;

                int len = strlen(name);
                memcpy(merged->strings + merged_symbol->string_offset, name, len+1);
                merged->header.string_size += len + 1;

                // debug("local %s\n", merged->strings + merged_symbol->string_offset);

                continue;
            } else if (symbol->type == BARF_SYMBOL_EXTERNAL) {
                // Duplicate of external symbols can be removed.
                // If there is a global symbol then external shall be removed in favour of the global.

                // Look for external name, don't add if we have it (fix symbol mapping)
                // Look for global name, don't add if we have it (fix symbol mapping)
                bool found = false;
                for (int i=0;i<merged->header.symbol_count;i++) {
                    BarfSymbol* merged_symbol = &merged->symbols[i];
                    char* merge_name = merged->strings + merged_symbol->string_offset;

                    if (merged_symbol->type != BARF_SYMBOL_EXTERNAL && merged_symbol->type != BARF_SYMBOL_GLOBAL)
                        continue;

                    if (!strcmp(name, merge_name)) {
                        symbol_mapping[bi][si] = i;
                        found = true;
                        break;
                    }
                }

                if (found)
                    continue;

                // Otherwise add external symbol.
                symbol_mapping[bi][si] = merged->header.symbol_count;
                BarfSymbol* merged_symbol = &merged->symbols[merged->header.symbol_count];
                merged->header.symbol_count++;

                merged_symbol->type = BARF_SYMBOL_EXTERNAL;
                merged_symbol->offset = symbol->offset;
                merged_symbol->section_index = -1;
                merged_symbol->string_offset = merged->header.string_size;

                int len = strlen(name);
                memcpy(merged->strings + merged_symbol->string_offset, name, len+1);
                merged->header.string_size += len + 1;
                // debug("external %d %s\n", merged->header.symbol_count-1, merged->strings + merged_symbol->string_offset);

            } else if (symbol->type == BARF_SYMBOL_GLOBAL) {
                // Duplicates are not allowed.

                // Check if we already added symbol name, error if we did
                // Check if we have external name with, if so replace it with global
                bool found = false;
                for (int i=0;i<merged->header.symbol_count;i++) {
                    BarfSymbol* merged_symbol = &merged->symbols[i];
                    char* merge_name = merged->strings + merged_symbol->string_offset;

                    if (merged_symbol->type == BARF_SYMBOL_GLOBAL) {
                        if (!strcmp(name, merge_name)) {
                            // @TODO Do a search in previous artifacts and find where the first symbol came from.
                            //    Error message is better if we show the two artifacts that have colliding symbols.
                            fprintf(stderr, "barf: Duplicate global symbol %s, cannot combine! (second here %s)\n", name, input_files[bi]);
                            goto cleanup;
                        }
                    } else if (merged_symbol->type == BARF_SYMBOL_EXTERNAL) {
                        if (!strcmp(name, merge_name)) {
                            merged_symbol->type = BARF_SYMBOL_GLOBAL;
                            merged_symbol->section_index = section_mapping[bi][symbol->section_index];
                            merged_symbol->offset = symbol->offset;
                            symbol_mapping[bi][si] = i;
                            found = true;
                            break;
                        }
                    }
                }

                if (found)
                    continue;

                // Otherwise add a new global symbol
                symbol_mapping[bi][si] = merged->header.symbol_count;
                BarfSymbol* merged_symbol = &merged->symbols[merged->header.symbol_count];
                merged->header.symbol_count++;

                merged_symbol->type = BARF_SYMBOL_GLOBAL;
                merged_symbol->offset = symbol->offset;
                merged_symbol->section_index = section_mapping[bi][symbol->section_index];
                merged_symbol->string_offset = merged->header.string_size;

                ASSERT(merged_symbol->section_index >= 0 && merged_symbol->section_index < merged->header.symbol_count);

                int len = strlen(name);
                memcpy(merged->strings + merged_symbol->string_offset, name, len+1);
                merged->header.string_size += len + 1;
                // debug("global %d %s\n", merged->header.symbol_count-1, merged->strings + merged_symbol->string_offset);
            } else {
                fprintf(stderr, "barf: Unhandled symbol %s in %s\n", name, ba_paths[bi]);
            }
        }
    }

    // Relocations
    for (int bi = 0; bi < input_count; bi++) {
        BarfObject* object = objects[bi];
        for (int si = 0; si < object->header.section_count; si++) {
            BarfSection* section = &object->sections[si];

            BarfSection* merged_section = &merged->sections[section_mapping[bi][si]];
            merged_section->relocation_count = section->relocation_count;
            BarfRelocation* relocations = malloc(sizeof(BarfRelocation) * section->relocation_count);
            merged->relocations[section_mapping[bi][si]] = relocations;
            memcpy(relocations, object->relocations[si], sizeof(BarfRelocation) * section->relocation_count);

            for (int ri = 0; ri < section->relocation_count; ri++) {
                BarfRelocation* rel = &relocations[ri];

                rel->symbol_index = symbol_mapping[bi][rel->symbol_index];
            }
        }
    }

    // We can either merge sections or we can add sections to artifact, this means multiple .text sections.
    // Human-wise it's hard to differentiate the sections, loader wise everything refers
    // to sections by ID so it doesn't matter much?.

    FILE* file;
    file = fopen(output, "wb");
    if (!file) {
        fprintf(stderr, "barf: Could not open '%s'\n", output);
        goto cleanup;
    }

    u64 size_of_symbols = sizeof(*merged->symbols) * merged->header.symbol_count;
    u64 size_of_sections = sizeof(*merged->sections) * merged->header.section_count;
    
    merged->header.section_offset = sizeof(merged->header);
    merged->header.symbol_offset  = sizeof(merged->header) + size_of_sections;
    merged->header.string_offset  = sizeof(merged->header) + size_of_sections + size_of_symbols;
    
    u64 next_section_data_offset = sizeof(merged->header) + size_of_sections + size_of_symbols + merged->header.string_size;

    for (int bi=0;bi<input_count;bi++) {
        BarfObject* prev_object = objects[bi];
        FILE* in_file = fopen(ba_paths[bi], "rb");
        for (int si=0;si<prev_object->header.section_count;si++) {
            BarfSection* prev_section = &prev_object->sections[si];
            BarfSection* section = &merged->sections[section_mapping[bi][si]];

            next_section_data_offset += (section->alignment - (next_section_data_offset % section->alignment)) % section->alignment;

            u64 new_offset = next_section_data_offset;


            transfer_bytes(in_file, prev_section->data_offset, file, next_section_data_offset, prev_section->data_size);

            // fseek(file, next_section_data_offset, SEEK_SET);
            // fwrite(data + section->data_offset, 1, section->data_size, file);

            next_section_data_offset += section->data_size;

            section->data_offset = new_offset;
            
            next_section_data_offset += (8 - (next_section_data_offset % 8)) % 8;

            new_offset = next_section_data_offset;

            // transfer_bytes(in_file, prev_section->data_offset, file, next_section_data_offset, prev_section->data_size);
            fseek(file, next_section_data_offset, SEEK_SET);
            size_t written_elements = fwrite(merged->relocations[section_mapping[bi][si]], sizeof(BarfRelocation), section->relocation_count, file);
            ASSERT(written_elements == section->relocation_count);
            section->relocation_offset = new_offset;

            next_section_data_offset += section->relocation_count * sizeof(BarfRelocation);
        }
        fclose(in_file);
    }
    
    merged->header.total_size = ftell(file);

    fseek(file, 0, SEEK_SET);
    fwrite(&merged->header, sizeof(merged->header), 1, file);

    fwrite(merged->sections, sizeof(*merged->sections), merged->header.section_count, file);
    
    fwrite(merged->symbols, sizeof(*merged->symbols), merged->header.symbol_count, file);
    
    fwrite(merged->strings, 1, merged->header.string_size, file);
    
    // @TODO Free input objects
    fclose(file);
    free(merged);

    return true;


cleanup:
    // if (file)
    //     fclose(file);
    // if (data)
    //     free(data);
    if (merged)
        free(merged);
    if (objects) {
        for (int i=0;i<input_count;i++) {
            if (objects[i]) {
                free(objects[i]);
            }
        }
        free(objects);
    }
    if (ba_path_text)
        free(ba_path_text);
    if (ba_paths)
        free(ba_paths);
    return false;
}



