

Binary Aritfact are files that contain code and data.

BARF is the loader that asks OS/Kernel for executable and read/write memory.

BARF runs on Windows/Linux or is embedded into an operating system (my experimental OS for my own learning)

An artifact can be completely independent and execute code but somewhere it has to interact with the OS to
create files and render to the screen.

As such there is a platform layer somewhere.

The OS may be constructed with Binary Artifacts for PCI code, USB, Ethernet, storage device driver code, scheduler. How are these integrated into the system, do they also use BARF loader?

This is OS specific rather than BARF but for the use case of BARF this matters a lot.

The OS starts with EFI or some kind of boot. It will load a minimal CORE KERNEL into memory so the rest of the kernel and OS can start up. Perhaps boot loads in the whole Kernel.

The OS is C code compiled into object files then converted to Binary Artifact.

The Kernel has BARF loader embedded into it. It can be used to load artifacts (code and data) into memory and solve relocations to code and data sections.

When running `barf main.ba` you may get unresolved symbol because you didn't add `game.ba`, `math.ba` and everything else.

BARF loader is smart and runs a fast hash on the contents of read only sections. If you load many artifacts in the system that has the same sections then it will reuse them. Mostly done for common libraries like libc, libm.




ed_create()
