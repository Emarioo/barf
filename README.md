
This project explores loaders and formats for executables, libraries, and object files.

The general idea is:

*Binary Artifacts represent .lib, .dll, .o, .exe at the same time. Static and dynamic libraries are not different things. A library and an object file are the same thing.*

.ba = Binary Artifact file extension

# Building

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

