
# Current
- [ ] Pass user args from barf to Binary Artifact program.
- [ ] Linux support. ELF -> .ba and handle relocations, relocations in COFF add section offset to displacement in the code, relocations in ELF might to it differently.
- [ ] Combine two binary artifacts.

# Extra
- [ ] `barf --verify program.ba`, checks if the program can be executed. It can't if there are unresolved symbols. Are some allowed? functions that aren't called for example? hmmm... too complex to determine?
- [ ] Experiment with converting libc dll to .ba? where do things break?

# Done
- [x] Apply relocations to sections.
- [x] Add external function table for platform calls (platform.h/platform.c, can add linux, libc calls in the future)
