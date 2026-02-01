

You want to run a function from a Binary Artifact, like `main`.
This will do that:
```bash
barf program.ba
```

You have a SHA-256 library you want to use in your program. This is how
to compile, create and run the artifacts.
```bash
# Compile main program that use SHA-256
gcc main.c -o main.o -Isha256/include

# Produce SHA-256 binary artifact from static lib (if developer of library doesn't provide .ba)
barf --combine sha256.ba sha256.lib

# Combine into final program
barf --combine program.ba main.ba sha256.ba

# Run it
barf program.ba
```

To reload code at runtime (hotreloading) you have dynamic libraries.
With BARF there is no separation.
```bash
# Compile main program that has game loop
gcc main.c -o main.o -Ibarf/include
barf --combine program.ba main.o

# Compile game code
gcc game.c -o game.o
barf --combine game.ba game.o

# Run it
barf program.ba
```

The main program has this to dynamically load game code:

```c
TickFN func = NULL;
Artifact* artifact = NULL;
while true {
    if (needs_reload()) {
        if (artifact)
            barf_unload(artifact)
        artifact = barf_load("game.ba")
        func = barf_get_pointer(artifact, "tick_event")
    }

    func(game_state)
}
// NOTE: For smooth hotreload the code would look a little different.
```

You do not need to combine barf.ba library. The BARF loader implicitly does it.



# Merging artifacts

Static variables belong to one artifact and cannot be referenced by other artifacts.

When merging artifacts this remains true. Static variables have symbols and relocations refer to them by symbol index.
The symbol also has names. These names can be the same for local variables but they cannot collide since relocations use symbol indexes.

Non-static variables (GLOBAL) can collide.

External variables are merged.