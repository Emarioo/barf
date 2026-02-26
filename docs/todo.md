
# Current
- [ ] Linux support. ELF -> .ba and handle relocations, relocations in COFF add section offset to displacement in the code, relocations in ELF might to it differently.
- [ ] Command arguments to print external symbols. Implement objdump flags? -h -t -s -x
- [ ] Add tests. 1-3 is fine, just something to run.
- [ ] JIT debug info for GDB


# Extra
- [ ] `barf --verify program.ba`, checks if the program can be executed. It can't if there are unresolved symbols. Are some allowed? functions that aren't called for example? hmmm... too complex to determine?
- [ ] Experiment with converting libc dll to .ba? where do things break?
- [ ] Optimization option where BARF detects frequent runs of an artifact and keeps an image with relocations applied on disc or in memory. Only local relocations (to readonly section) are applied, we still need to apply external ones. This is what an executable is but the BARF system treats it as a behind the scenes thing. Do we need to hash sections and compare with image incase .ba file changes so we don't use old image?
- [ ] We can't run artifact made on Windows on Linux because ABI (calling convention). We would need a wrapper layer to convert the convention when artifact calls platform layer. Since we don't know number or types of arguments i'm not sure how this is possible without debug info or we specify it when adding platform layer functions in the loader. What if we hook in a library like Kernel32, Vulkan? Manually specifying argument types is not viable, debug info then?

# Done
- [x] Run BARF artifact inside BARF
- [x] Combine two binary artifacts.
- [x] Pass user args from barf to Binary Artifact program.
- [x] Apply relocations to sections.
- [x] Add external function table for platform calls (platform.h/platform.c, can add linux, libc calls in the future)
