#include "barf/barf.h"

#include "barf/elf.h"
#include "barf/coff.h"

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
                BarfSection* sym_section = &object->sections[symbol->section_index];
                log("     0x%x %s (in %s)\n", relocation->offset, name, sym_section->name);
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

bool barf_convert_from_coff(u8* data, u64 size, BarfObject* object, const char* path, const char* output) {
    COFF_File_Header* header = (COFF_File_Header*)data;

    object->header.magic = BARF_MAGIC;
    object->header.version = 1;
    object->header.flags = 0; // little endian

    if (header->Machine == IMAGE_FILE_MACHINE_AMD64) {
        strcpy(object->header.target, "x86_64");
    } else {
        strcpy(object->header.target, "unknown");
    }

    object->sections = malloc(header->NumberOfSections * sizeof(*object->sections));
    memset(object->sections, 0, header->NumberOfSections * sizeof(*object->sections));

    
    u64 size_of_relocations_list = object->header.section_count * sizeof(*object->relocations);
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
            } else {
                printf("barf: Unhandled coff reloc type %d, sym index %u\n", relocation->Type, symbol_index);
            }
            rel->symbol_index = symbol_index;
            rel->offset = relocation->VirtualAddress;

            sec->relocation_count++;
        }
    }

    // barf_dump(object);

    FILE* file = fopen(output, "wb");
    if (!file) {
        fprintf(stderr, "barf: Could not open '%s'\n", output);
        return false;
    }

    u64 size_of_symbols = sizeof(*object->sections) * object->header.symbol_count;
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

    return true;
}
bool barf_convert_from_elf(u8* data, u64 size, BarfObject* object, const char* path, const char* output) {
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

    return true;
}

bool barf_convert_from_elf_file(const char* path, const char* output) {
    FILE*       file   = NULL;
    u8*         data   = NULL;
    BarfObject* object = NULL;
    
    file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "barf: could not read '%s'\n", path);
        goto cleanup;
    }

    fseek(file, 0, SEEK_END);
    size_t filesize = ftell(file);
    fseek(file, 0, SEEK_SET);

    data = malloc(filesize);

    // printf("filesize %d\n", (int)filesize);

    size_t read_bytes = fread(data, 1, filesize, file);
    // LOOP
    if(read_bytes != filesize) {
        fprintf(stderr, "barf: could not read all bytes '%s'\n", path);
        goto cleanup;
    }

    fclose(file);
    file = NULL;
    
    object = malloc(sizeof(*object));
    memset(object, 0, sizeof(*object));

    // @TODO Don't assume 64-bit ELF
    Elf64_Ehdr* elf_header = (Elf64_Ehdr*)data;
    // COFF_File_Header* coff_header = (COFF_File_Header*)data;

    bool res;
    if (!strncmp((char*)elf_header->e_ident, "\x7f""ELF", 4)) {
        res = barf_convert_from_elf(data, filesize, object, path, output);
        if (!res)
            goto cleanup;
    } else {
        // @TODO Somehow detect if file is COFF, COFF doesn't have an identifier or magic number. (for object files)
        res = barf_convert_from_coff(data, filesize, object, path, output);
        if (!res)
            goto cleanup;
    }

    free(data);
    free(object);

    return true;


cleanup:
    if (file)
        fclose(file);
    if (data)
        free(data);
    if (object)
        free(object);
    return false;
}
