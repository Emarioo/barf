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

bool barf_combine_to_artifact(int input_count, const char** input_files, const char* output);


bool barf_load_file(const char* path, int argc, const char** argv);
