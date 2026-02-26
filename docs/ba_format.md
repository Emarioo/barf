**This is work in progress, you might be better off reading the C headers and implementation**

This document describes the **Binary Artifact Format**.

See the [C header](../include/barf/ba_format.h) for data structures and [reader/writer implemenation](../src/barf/format.c) for details.


# Binary Artifact Format

**Format** refers to the layout specification.

**Artifact** refers to an instance of the format.

Any chunk of bytes is able to represent the format.
The format typically resides in a file but any chunk of bytes could represent a Binary Artifact.

The format contains a header, sections, symbols, string data, section data, and relocations.

The format is intentionally very simple.

All structures should follow C padding but usually there are explicit `_reserved` fields to indicate where padding is. These fields should be zero.

## Header

The header declares:
- Format version
- Size of the artifact (the whole artifact including the whole header)
- Target architecture (must be null terminated)
- Offset to section table and number of sections
- Offset to symbol table and number of symbols
- Offset to string table and size of the table

All offsets are relative to the start of the artifact.

Here is an example layout:

|Offset|Part|Description|
|-|-|-|
|0|Header|Tells you where to find the section, symbol and string tables|
|96|Section Table|Array of sections|
|...|Symbol Table|Array of symbols|
|...|String Table|Array of sections|
|...|Rest of data|Section data and their relocations|

## Sections

A section declares:
- Name
- Flags (writable, executable, zeroed/uninitialized, ignore on load)
- Alignment (important for aligned SIMD instructions that access data in `.data`, `.rodata` sections)
- Offset to relocations and number of relocations
- Offset to section data and size of section data

All sections are assumed to be readable.

There is no restriction on multiple sections having the same name. It lets you merge artifacts without merging sections.

|Flags|Typical section name|
|-|-|
|No flags|`.rodata`|
|Writable|`.data`|
|Executable|`.text`|
|Zeroed/unitialized|`.bss`|
|Ignore|`.note`|


## Symbols

A symbol declares:
- Type (local, global, external)
- Offset in string table where to find the name of the symbol
- Section index describing which section the symbol is defined in
- Offset into section where symbol is located

The name of local symbols cannot collide with each other.

The name of global symbols CAN collide with each other.

If symbol is external then it cannot be located in this artifact. The section index and offset are meaningless.

## String table

The string table is a chunk of characters. Symbols refer to strings in the string table by an offset. The end of the string is determined by a NULL character.

@TODO Consider using length-prefixed strings:
```c
u8 length    = string_table[symbol.offset-1];
char* string = string_table[symbol.offset];
ASSERT('\0' == string_table[symbol.offset + length])
```

## Relocations

Each section has an array of relocations

A relocation declares:
- Type (x86_64 uses REL32)
- Symbol index describing the target to relocate to.
- Offset into section where to apply relocation.

The symbol index may refer to a local or global symbol in which case the relocation
can be applied once sections have been loaded into memory.

If symbol index refers to an external symbol then you cannot relocate unless you merge artifacts or the runtime loader provides the external symbols.

Describe addend semantics. Depends on architecture and operating system.
