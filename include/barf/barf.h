#pragma once

#include "barf/types.h"

// ################################
//          FORMAT TYPES
// ################################


#define BARF_MAGIC 0x46524142u // BARF



typedef enum BarfHeaderFlag {
    BARF_FLAG_BIG_ENDIAN = 0x1, // format is little endian if flag is not present
} BarfHeaderFlag;
typedef u32 BarfHeaderFlags;



typedef struct BarfHeader {
    u32              magic;          // "BARF"
    u32              version;
    BarfHeaderFlags  flags;          // u32
    u32              _reserved;
    u64              total_size;     // total size of whole format including header (the file size)
    char             target[32];     // null-terminated, 31 max length
    u32              section_count;
    u32              symbol_count;
    u32              string_size;
    u64              section_offset;
    u64              symbol_offset;
    u64              string_offset;
} BarfHeader;



typedef enum BarfSectionFlag {
    // BARF_FLAG_READ   = 0x1, // all sections are readable
    BARF_FLAG_WRITE  = 0x2,
    BARF_FLAG_EXEC   = 0x4,
    BARF_FLAG_ZEROED = 0x8,
    BARF_FLAG_IGNORE = 0x10,
} BarfSectionFlag;
typedef u16 BarfSectionFlags;


typedef struct BarfSection {
    char              name[32];  // null-terminated, 31 max characters for section name
    BarfSectionFlags  flags;     // u16
    // @TODO We probably want to specify alignment. defaulting to 16 bytes is fine but
    //   what if you use SIMD on variables in .data section and you assume it's 64-byte aligned when it's just 16 byte aligned.
    u16    alignment;            // needed when using SIMD instructions (some need 32-byte alignment)
    u32    _reserved;
    u32    relocation_count;     // offset from start of format
    u64    data_size;
    u64    relocation_offset;
    u64    data_offset;          // offset from start of format
} BarfSection;

typedef enum {
    BARF_SYMBOL_LOCAL,
    BARF_SYMBOL_GLOBAL,
    BARF_SYMBOL_EXTERNAL,
} _BarfSymbolType;
typedef u8 BarfSymbolType;

typedef struct BarfSymbol {
    BarfSymbolType  type; // u8
    u8              _reserved[3];
    u32             string_offset;
    u32             section_index;
    u32             offset; // offset into section where object symbol refers to is located
} BarfSymbol;



typedef enum {
    BARF_RELOC_REL32,
} _BarfRelocationType;
typedef u8 BarfRelocationType;

typedef struct {
    BarfRelocationType type; // u8
    u8                 _reserved[3];
    u32                symbol_index;
} BarfRelocation;


// ###########################
//      LOADER TYPES
// ###########################

typedef struct {
    void* address;
} BarfSegment;

typedef struct BarfObject BarfObject;
typedef struct {
    BarfObject* objects;
    u32         object_count;
} BarfLoader;

// ############################
//         PARSER TYPES
// ############################



typedef struct BarfObject {
    BarfHeader       header;
    BarfSection*     sections;    // find section count in header
    BarfSymbol*      symbols;     // find symbol count in header
    BarfRelocation** relocations; // find relocation count in sections
    char*            strings;

    // used at runtime
    BarfSegment* segments;
} BarfObject;



// ##########################
//         FUNCTIONS
// ##########################

BarfObject* barf_parse_header_from_file(const char* path);
void barf_dump(BarfObject* object);

void barf_free_object(BarfObject* object);

bool barf_convert_from_elf_file(const char* path, const char* output);


bool barf_load_file(const char* path);
