
This project explores loaders and formats for executables, libraries, and object files.

*A Binary Artifact represents .lib, .dll, .o, .exe at the same time. Static and dynamic libraries are semantically the same and a function from an object file can simply be executed without linking.*

File extension for **Binary Artifact** is `.ba`.

# Usage

```c
// main.c
void log__printf(char* format, ...);

int ba_entry(const char* path, const char* data, int size) {
    log__printf("Hello from %s, %d bytes of data\n", path, size);
    log__printf("  %*.s\n", size, data);
    return 0;
}
```

```bash
# Compile Object File
gcc -c main.c
# Convert to Binary Artifact
barf -c -o app.ba main.o
# Run Binary Artifact
barf app.ba -- Here is data

> Hello from app.ba, 12 bytes of data
>   Here is data
```

The projects has
- BARF Library with C API headers
    - Runtime loader for Binary Artifacts
    - Object file to Binary Artifact converter
    - Binary Artifact dumping (similar objdump)
- BARF program compiled for Linux/Windows which provides command line arguments to use the library.


# Documentation

- [Binary Artifact Format](docs/ba_format.md)

# Building

This builds BARF as a native program (executable on Windows/Linux).

Supports Windows and will support Linux.

**Dependencies**
- Python (3.9+)
- GCC (use MinGW or WSL on windows)

```bash
# build
build.py

# run
releases/barf-0.0.1-dev-windows-x86_64/barf.exe
releases/barf-0.0.1-dev-linux-x86_64/barf
```

