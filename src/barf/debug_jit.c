/*

    Some importants notes on getting JIT debug info to work.

    The provided in-memory ELF file cannot have multiple debug sections.
    They must be merged.

    To merge debug sections we'll find the first one for each debug section type and at it as "primary".
    As we find more debug sections we will append them to the primary sections and appending and update relocations as we go.
    In advance we will find out how large each debug section is and reserve that amount when incrementing data_head.


*/

#include "barf/debug_jit.h"


#include "barf/barf.h"
#include "platform/platform.h"

#include "barf/elf.h"
#include "barf/dwarf.h"

// https://sourceware.org/gdb/current/onlinedocs/gdb.html/Declarations.html#Declarations
typedef enum {
  JIT_NOACTION = 0,
  JIT_REGISTER_FN,
  JIT_UNREGISTER_FN
} jit_actions_t;

typedef struct jit_code_entry {
  struct jit_code_entry *next_entry;
  struct jit_code_entry *prev_entry;
  char *symfile_addr;
  uint64_t symfile_size;
} jit_code_entry;

typedef struct jit_descriptor {
  uint32_t version;
  /* This type should be jit_actions_t, but we use uint32_t
     to be explicit about the bitwidth.  */
  uint32_t action_flag;
  jit_code_entry *relevant_entry;
  jit_code_entry *first_entry;
} jit_descriptor;

/* GDB puts a breakpoint in this function.  */
void __jit_debug_register_code() { }

/* Make sure to specify the version statically, because the
   debugger may check the version before we can set it.  */
jit_descriptor __jit_debug_descriptor = { 1, 0, 0, 0 };

#define MAX_JIT_CODE_ENTRIES 100
static jit_code_entry g_jit_code_entries[MAX_JIT_CODE_ENTRIES];
static int g_next_jit_code_entry_index;

#define debug(...) log__printf(__VA_ARGS__)
// #define debug(...)


int register_jit_debug(void* obj_blob, int obj_bob_size) {
    ASSERT(g_next_jit_code_entry_index < MAX_JIT_CODE_ENTRIES);

    int entry_id = g_next_jit_code_entry_index;
    jit_code_entry* entry = &g_jit_code_entries[g_next_jit_code_entry_index];
    g_next_jit_code_entry_index++;

    memset(entry, 0, sizeof(*entry));
    entry->symfile_addr = obj_blob;
    entry->symfile_size = obj_bob_size;

    __jit_debug_descriptor.action_flag = JIT_REGISTER_FN;
    __jit_debug_descriptor.relevant_entry = entry;

    // Insert entry into linked list
    if (!__jit_debug_descriptor.first_entry) {
        __jit_debug_descriptor.first_entry = entry;
    } else {
        jit_code_entry* head = __jit_debug_descriptor.first_entry;

        while (true) {
            if (!head->next_entry) {
                head->next_entry = entry;
                entry->prev_entry = head;
                break;
            }
            head = head->next_entry;
        }
    }

    __jit_debug_register_code();

    return entry_id;
}



void unregister_jit_debug(int entry_id) {
    __jit_debug_descriptor.action_flag = JIT_UNREGISTER_FN;

    jit_code_entry* entry = &g_jit_code_entries[entry_id];
    
    __jit_debug_descriptor.relevant_entry = entry;
    // Remove entry from linked list
    if (entry == __jit_debug_descriptor.first_entry) {
        __jit_debug_descriptor.first_entry = entry->next_entry;
    } else {
        entry->prev_entry = entry->next_entry;
    }
    
    __jit_debug_register_code();

    mem__free(entry->symfile_addr);
}



void* create_memory_obj_from_artifact(BarfObject* object, int* out_obj_size) {

    u8* data = mem__malloc(0x100000);

    int data_head = 0;
    
    // Elf64_Shdr

    Elf64_Ehdr* header = (Elf64_Ehdr*)(data + data_head);
    data_head += sizeof(*header);

    memset(header, 0, sizeof(*header));
    strcpy((char*)header->e_ident, ELFMAG);
    header->e_ident[EI_CLASS] = ELFCLASS64;
    header->e_ident[EI_DATA] = ELFDATA2LSB;
    header->e_ident[EI_VERSION] = EV_CURRENT;
    header->e_ident[EI_OSABI] = ELFOSABI_SYSV;
    header->e_ident[EI_ABIVERSION] = 1;
    header->e_type = ET_REL;
    header->e_machine = EM_X86_64;

    header->e_version = EV_CURRENT;
    header->e_ehsize = sizeof(*header);
    header->e_shentsize = sizeof(Elf64_Shdr);
    // header->e_shnum = 0; // set later
    // header->e_shstrndx = ; // set later

    #define GET_BARF_SECTION_DATA(INDEX) (object->segments[INDEX].address)

    header->e_shoff = data_head;

    {
        Elf64_Shdr* section_header = (Elf64_Shdr*)(data + data_head);
        data_head += sizeof(*section_header);
        header->e_shnum++;
        memset(section_header, 0, sizeof(*section_header));
    }

    typedef struct SectionInfo {
        int elf_section_index;
        int elf_rela_section_index;
        int sections_to_merge_cap;
        int sections_to_merge_len;
        BarfSection** sections_to_merge;
        int section_data_offset;
        int section_index_merged_into;
    } SectionInfo;

    // @NOCHECKIN MEMORY LEAK
    SectionInfo* section_infos = mem__malloc(sizeof(SectionInfo) * object->header.section_count);
    memset(section_infos, 0, sizeof(SectionInfo) * object->header.section_count);

    #define BARF_TO_ELF_SECTION_INDEX(INDEX) section_infos[INDEX].elf_section_index;
    
    typedef struct MergeSection {
        int index;
        const char* name;
    } MergeSection;

    MergeSection merge_sections[] = {
        { -1, ".debug_info" },
        { -1, ".debug_abbrev" },
    };

    // Elf64_Shdr* debug_info = NULL;
    // Elf64_Shdr* debug_abbrev = NULL;
    // Elf64_Shdr* debug_aranges = NULL;
    // Elf64_Shdr* debug_line = NULL;
    // Elf64_Shdr* debug_str = NULL;
    // Elf64_Shdr* debug_line_str = NULL;


    // int _max = sizeof(BarfSection*) * object->header.section_count;
    // int debug_sections_to_merge_len = 0;
    // BarfSection** debug_sections_to_merge = mem__malloc(_max);
    // memset(debug_sections_to_merge, 0, _max);

    for (int i=0;i<object->header.section_count;i++) {
        BarfSection* bsection = &object->sections[i];
        section_infos[i].elf_section_index = -1;
        section_infos[i].elf_rela_section_index = -1;

        if (bsection->flags & BARF_FLAG_IGNORE) {
            // Don't need .note and .comment sections
            // But we do need debug sections.
            // @TODO Consider adding flag for debug.
            // @TODO Consider having IGNORE which is purely extra note, comment info like compiler and flags used to compile.
            //     Then METADATA which is meant for data not used for execution. It can be stripped such as debug info.
            continue;
        }


        Elf64_Shdr* section_header = (Elf64_Shdr*)(data + data_head);

 
        #define ARRAY_LENGTH(ARR) (sizeof(ARR)/sizeof(*ARR))

        for (int mi=0;mi<ARRAY_LENGTH(merge_sections);mi++) {
            if (!strcmp(bsection->name, merge_sections[mi].name)) {
                if (merge_sections[mi].index != -1) {
                    SectionInfo* info = &section_infos[merge_sections[mi].index];
                    int new_data_offset;
                    if (!info->sections_to_merge) {
                        int cap = sizeof(BarfSection*) * object->header.section_count;
                        info->sections_to_merge_cap = cap;
                        info->sections_to_merge = mem__malloc(cap);
                        new_data_offset = object->sections[merge_sections[mi].index].data_size;
                    } else {
                        int prev_bindex = info->sections_to_merge[info->sections_to_merge_len] - object->sections;
                        new_data_offset = section_infos[prev_bindex].section_data_offset + object->sections[prev_bindex].data_size;
                    }
                    section_infos[i].section_data_offset = new_data_offset;
                    section_infos[i].section_index_merged_into = merge_sections[mi].index;
                    info->sections_to_merge[info->sections_to_merge_len] = bsection;
                    info->sections_to_merge_len++;

                    // Elf64_Shdr* debug_info_rela = (Elf64_Shdr*)(data + header->e_shoff + header->e_shentsize * section_infos[i].elf_rela_section_index);
                    // @NOCHECKIN This is an estimated size!
                    //   it should probably have -sizeof(DebugInfoHeader)
                    // debug_info->sh_size += bsection->data_size;
                    // @NOCHECKIN One of these relocations refer to header. We already do that and should skip that relocation.
                    // debug_info_rela->sh_size += sizeof(Elf64_Rela) * bsection->relocation_count;
                    continue;
                }
                merge_sections[mi].index = i;
            }
        }


        data_head += sizeof(*header);
        section_infos[i].elf_section_index = header->e_shnum;
        header->e_shnum++;
        memset(section_header, 0, sizeof(*section_header));

        section_header->sh_addr = (Elf64_Addr)object->segments[i].address;
        section_header->sh_addralign = bsection->alignment;
        section_header->sh_size = bsection->data_size;

        if (!strcmp(bsection->name, ".debug_str") || !strcmp(bsection->name, ".debug_line_str")) {
            section_header->sh_flags |= SHF_MERGE | SHF_STRINGS;
        } else if (!strncmp(bsection->name, ".debug_", 7)) {
            // no flags
        } else {
            section_header->sh_flags |= SHF_ALLOC;
        }

        if (bsection->flags & BARF_FLAG_ZEROED) {
            section_header->sh_type = SHT_NOBITS;
        } else {
            section_header->sh_type = SHT_PROGBITS;
        }
        if (bsection->flags & BARF_FLAG_EXEC) {
            section_header->sh_flags |= SHF_EXECINSTR;
        }
        if (bsection->flags & BARF_FLAG_WRITE) {
            section_header->sh_flags |= SHF_WRITE;
        }

        if (bsection->relocation_count) {
            Elf64_Shdr* section_header = (Elf64_Shdr*)(data + data_head);
            data_head += sizeof(Elf64_Shdr);
            section_infos[i].elf_rela_section_index = header->e_shnum;
            header->e_shnum++;
            memset(section_header, 0, sizeof(Elf64_Shdr));

            section_header->sh_addralign = 1;
            section_header->sh_entsize = sizeof(Elf64_Rela);
            section_header->sh_flags = SHF_INFO_LINK;
            section_header->sh_type = SHT_RELA;
            section_header->sh_info = section_infos[i].elf_section_index;
            // section_header->sh_link // set later when we know index to symbol table, we could create symbol table first.
            section_header->sh_size = sizeof(Elf64_Rela) * bsection->relocation_count;
        }
    }

    int index_of_symbol_table = header->e_shnum;
    Elf64_Shdr* symbol_header = (Elf64_Shdr*)(data + data_head);
    memset(symbol_header, 0, sizeof(Elf64_Shdr));
    data_head += sizeof(Elf64_Shdr);
    header->e_shnum++;
    symbol_header->sh_type = SHT_SYMTAB;
    symbol_header->sh_entsize = sizeof(Elf64_Sym);
    // symbol_header->sh_info = ; // index to first non local symbol in table

    Elf64_Shdr* symbol_string_header = (Elf64_Shdr*)(data + data_head);
    memset(symbol_string_header, 0, sizeof(Elf64_Shdr));
    data_head += sizeof(Elf64_Shdr);
    symbol_header->sh_link = header->e_shnum;
    header->e_shnum++;
    symbol_string_header->sh_type = SHT_STRTAB;
    
    Elf64_Shdr* section_string_header = (Elf64_Shdr*)(data + data_head);
    data_head += sizeof(Elf64_Shdr);
    header->e_shstrndx = header->e_shnum;
    header->e_shnum++;
    memset(section_string_header, 0, sizeof(Elf64_Shdr));
    section_string_header->sh_type = SHT_STRTAB;

    #define BARF_TO_ELF_SYMBOL_INDEX(INDEX) (symbol_infos[INDEX].elf_index)

    typedef struct SymbolInfo {
        int elf_index;
    } SymbolInfo;


    SymbolInfo* symbol_infos = mem__malloc(sizeof(SymbolInfo) * object->header.symbol_count);
    memset(symbol_infos, 0, sizeof(SymbolInfo) * object->header.symbol_count);
    int current_symbol_count = 0;

    symbol_header->sh_offset = data_head;
    {
        // NULL symbol
        Elf64_Sym* sym = (Elf64_Sym*)(data + data_head);
        data_head += sizeof(Elf64_Sym);
        memset(sym, 0, sizeof(Elf64_Sym));
        current_symbol_count++;
    }

    // In ELF symbol table all local symbols should appear first
    for (int i=0;i<object->header.symbol_count;i++) {
        BarfSymbol* bsymbol = &object->symbols[i];
        symbol_infos[i].elf_index = -1;

        if (bsymbol->type != BARF_SYMBOL_LOCAL) {
            continue;
        }

        symbol_infos[i].elf_index = current_symbol_count;
        current_symbol_count++;

        Elf64_Sym* sym = (Elf64_Sym*)(data + data_head);
        data_head += sizeof(Elf64_Sym);
        memset(sym, 0, sizeof(Elf64_Sym));

        sym->st_info = ELF64_ST_INFO(STB_LOCAL, STT_NOTYPE);
        sym->st_shndx = BARF_TO_ELF_SECTION_INDEX(bsymbol->section_index);
        sym->st_value = bsymbol->offset;
        // sym->st_size = 0; // we don't have size of symbol

        symbol_header->sh_size += sizeof(Elf64_Sym);
    }
    
    symbol_header->sh_info = current_symbol_count; // index to first non local symbol in table

    for (int i=0;i<object->header.symbol_count;i++) {
        BarfSymbol* bsymbol = &object->symbols[i];

        if (bsymbol->type == BARF_SYMBOL_LOCAL) {
            continue;
        }

        symbol_infos[i].elf_index = current_symbol_count;
        current_symbol_count++;

        Elf64_Sym* sym = (Elf64_Sym*)(data + data_head);
        data_head += sizeof(Elf64_Sym);
        memset(sym, 0, sizeof(Elf64_Sym));

        switch (bsymbol->type) {
            case BARF_SYMBOL_LOCAL: ASSERT(false);
            case BARF_SYMBOL_EXTERNAL: {
                sym->st_info = ELF64_ST_INFO(STB_GLOBAL, STT_NOTYPE);
            } break;
            case BARF_SYMBOL_GLOBAL: {
                sym->st_info = ELF64_ST_INFO(STB_GLOBAL, STT_NOTYPE);
                sym->st_shndx = BARF_TO_ELF_SECTION_INDEX(bsymbol->section_index);
            } break;
        }
        sym->st_value = bsymbol->offset;
        // sym->st_size = 0; // we don't have size of symbol

        symbol_header->sh_size += sizeof(Elf64_Sym);
    }

    for (int i=0;i<object->header.section_count;i++) {
        BarfSection* bsection = &object->sections[i];
        
        if (bsection->flags & BARF_FLAG_IGNORE) {
            // Don't need .note and .comment sections
            continue;
        }

        if (section_infos[i].elf_section_index == -1) {
            // Only debug sections should be skipped here at the moment.
            // debug("Skipping %s\n", bsection->name);
            continue;
        }

        Elf64_Shdr* section_header = (Elf64_Shdr*)(data + header->e_shoff + header->e_shentsize * section_infos[i].elf_section_index);
        section_header->sh_offset = data_head;
        u8* section_data = (data + data_head);
        data_head += bsection->data_size;
        memcpy(section_data, GET_BARF_SECTION_DATA(i), bsection->data_size);
        section_header->sh_size = bsection->data_size;

        SectionInfo* info = &section_infos[i];

        int total_reloc_count = bsection->relocation_count;

        // Before this point we have calculated the offset which all sections to be merged should be placed at.
        // This means relocations to any section know if the section they refer to will be merged and which new section
        // and which offset it is placed at. We append the "merge section"'s data to the section
        for (int j=0;j<info->sections_to_merge_len;j++) {
            BarfSection* msection = info->sections_to_merge[j];
            int mindex = msection - object->sections;
            SectionInfo* merge_section_info = &section_infos[mindex];

            ASSERT(section_header->sh_size == merge_section_info->section_data_offset);

            memcpy(section_data + section_header->sh_size, GET_BARF_SECTION_DATA(mindex), msection->data_size);
            section_header->sh_size += msection->data_size;
            data_head += msection->data_size;


            total_reloc_count += msection->relocation_count;
        }


        if (total_reloc_count) {
            Elf64_Shdr* section_header = (Elf64_Shdr*)(data + header->e_shoff + header->e_shentsize * section_infos[i].elf_rela_section_index);
            section_header->sh_link = index_of_symbol_table;
            // section_header->sh_info = ; // set earlier
            section_header->sh_offset = data_head;
            for (int ri=0;ri<bsection->relocation_count;ri++) {
                BarfRelocation* brelocation = &object->relocations[i][ri];
                Elf64_Rela* rel = (Elf64_Rela*)(data + data_head);
                data_head += section_header->sh_entsize;

                // @NOCHECKIN If relocation refers to 
                rel->r_offset = brelocation->offset;
                if (brelocation->type == BARF_RELOC_REL32) {
                    int sym_index = BARF_TO_ELF_SYMBOL_INDEX(brelocation->symbol_index);
                    rel->r_info = ELF64_R_INFO(sym_index, R_X86_64_PC32);
                    rel->r_addend = *(i32*)(GET_BARF_SECTION_DATA(i) + brelocation->offset) - 4;
                } else ASSERT(false);
            }
            for (int j=0;j<info->sections_to_merge_len;j++) {
                BarfSection* msection = info->sections_to_merge[j];
                int mindex = msection - object->sections;
                SectionInfo* merge_section_info = &section_infos[mindex];

                ASSERT(section_header->sh_size == merge_section_info->section_data_offset);

                memcpy(section_data + section_header->sh_size, GET_BARF_SECTION_DATA(mindex), msection->data_size);
                section_header->sh_size += msection->data_size;
                data_head += msection->data_size;


                total_reloc_count += msection->relocation_count;
            }
        }
    }

    // for (int di=0;di<debug_sections_to_merge_len;di++) {
    //     BarfSection* bsection = debug_sections_to_merge[di];
    //     int bindex = bsection - object->sections;

    //     char* data = GET_BARF_SECTION_DATA(bindex);

    //     DW5_32_CompilationUnitHeader* unit = (DW5_32_CompilationUnitHeader*)data;

    //     debug("debug %s\n", bsection->name);
    // }


    section_string_header->sh_offset = data_head;
    (data + data_head)[0] = '\0'; // first character is null
    data_head++;

    {
        symbol_header->sh_name = data_head -  section_string_header->sh_offset;
        
        const char* symbol_section_name = ".symtab";
        int len = strlen(symbol_section_name);
        memcpy(data + data_head, symbol_section_name, len);
        (data + data_head + len)[0] = '\0';
        data_head += len + 1;

        symbol_string_header->sh_name = data_head -  section_string_header->sh_offset;
        const char* symbol_strtab_name = ".strtab";
        len = strlen(symbol_strtab_name);
        memcpy(data + data_head, symbol_strtab_name, len);
        (data + data_head + len)[0] = '\0';
        data_head += len + 1;
        
        section_string_header->sh_name = data_head -  section_string_header->sh_offset;
        const char* section_strtab_name = ".shstrtab";
         len = strlen(section_strtab_name);
        memcpy(data + data_head, section_strtab_name, len);
        (data + data_head + len)[0] = '\0';
        data_head += len + 1;
    }

    for (int i=0;i<object->header.section_count;i++) {
        BarfSection* bsection = &object->sections[i];
        
        if (bsection->flags & BARF_FLAG_IGNORE) {
            // Don't need .note and .comment sections
            continue;
        }

        Elf64_Shdr* section_header = (Elf64_Shdr*)(data + header->e_shoff + header->e_shentsize * section_infos[i].elf_section_index);
        section_header->sh_name = data_head - section_string_header->sh_offset;

        int len = strlen(bsection->name);
        memcpy(data + data_head, bsection->name, len);
        (data + data_head + len)[0] = '\0';
        data_head += len + 1;

        if (bsection->relocation_count) {
            Elf64_Shdr* section_header = (Elf64_Shdr*)(data + header->e_shoff + header->e_shentsize * section_infos[i].elf_rela_section_index);
            section_header->sh_name = data_head - section_string_header->sh_offset;
            
            strcpy((char*)data + data_head, ".rela");
            data_head += 5;
            memcpy(data + data_head, bsection->name, len);
            (data + data_head + len)[0] = '\0';
            data_head += len + 1;
        }
    }
    section_string_header->sh_size = data_head - section_string_header->sh_offset;

    
    symbol_string_header->sh_offset = data_head;
    (data + data_head)[0] = '\0'; // first character is null
    data_head++;

    for (int i=0;i<object->header.symbol_count;i++) {
        BarfSymbol* bsymbol = &object->symbols[i];

        int sym_index = BARF_TO_ELF_SYMBOL_INDEX(i);
        Elf64_Sym* sym = (Elf64_Sym*)(data + symbol_header->sh_offset + symbol_header->sh_entsize * sym_index);

        
        const char* sym_name = object->strings + bsymbol->string_offset;
        int len = strlen(sym_name);
        sym->st_name = data_head - symbol_string_header->sh_offset;
        memcpy(data + data_head, sym_name, len);
        (data + data_head + len)[0] = '\0';
        data_head += len + 1;
    }
    symbol_string_header->sh_size = data_head - symbol_string_header->sh_offset;



    *out_obj_size = data_head;
    return data;

}