#pragma once

#include "barf/ba_format.h"

// ###########################
//      LOADER TYPES
// ###########################

typedef struct {
    u8* address;
} BarfSegment;

#define JUMP_ENTRY_STRIDE 12

typedef struct BarfObject BarfObject;
typedef struct {
    BarfObject* objects;
    u32         object_count;

    void* external_segment;
    // @TODO Lookup table
    char** external_names;
    int external_names_len;
} BarfLoader;

// ############################
//         PARSER TYPES
// ############################



typedef struct BarfObject {
    BarfHeader       header;
    BarfSection*     sections;    // find counts in the header
    BarfSymbol*      symbols;
    BarfRelocation** relocations;
    char*            strings;
    char**           section_data;

    // used at runtime
    BarfSegment* segments;
} BarfObject;



// ##########################
//         FUNCTIONS
// ##########################

BarfObject* barf_parse_header_from_file(const char* path);
void barf_dump(BarfObject* object);

void barf_free_object(BarfObject* object);

// Returns false if it wasn't coff
bool barf_convert_from_coff(const char* path, const char* output);
// Returns false if it wasn't elf
bool barf_convert_from_elf(const char* path, const char* output);

bool barf_combine_to_artifact(int input_count, const char** input_files, const char* output);


bool barf_load_file(const char* path, int argc, const char** argv);
