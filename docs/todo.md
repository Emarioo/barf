
# Current
- [ ] Combine two binary artifacts.
- [ ] Linux support. ELF -> .ba and handle relocations, relocations in COFF add section offset to displacement in the code, relocations in ELF might to it differently.
- [ ] JIT debug info for GDB

# Extra
- [ ] `barf --verify program.ba`, checks if the program can be executed. It can't if there are unresolved symbols. Are some allowed? functions that aren't called for example? hmmm... too complex to determine?
- [ ] Experiment with converting libc dll to .ba? where do things break?
- [ ] Optimization option where BARF detects frequent runs of an artifact and keeps an image with relocations applied on disc or in memory. Only local relocations (to readonly section) are applied, we still need to apply external ones. This is what an executable is but the BARF system treats it as a behind the scenes thing. Do we need to hash sections and compare with image incase .ba file changes so we don't use old image?

# Done
- [x] Pass user args from barf to Binary Artifact program.
- [x] Apply relocations to sections.
- [x] Add external function table for platform calls (platform.h/platform.c, can add linux, libc calls in the future)
