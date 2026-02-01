# Tests to make

- Compiling code resulting larger than 4GB object file. Then using BARF on it. can BARF handle it?
- Compiling many object files (10 MB) and combining them into one BARF file. Should become larger than 4 GB, can BARF handle it.
- Performance measurements on large files, combining many files, starting many programs. Make a program that runs 1000 small programs and how long it takes to load and run them.

# What do I deem acceptable in performance?

Any build or compile process should in total take less than 1 second.
BARF is an additional step at the end of the build process. This process already takes a long time because clang, gcc, msvc are slow. It is unacceptable for BARF to add an additional second.

For 100 object files 1 ms each, we add 100 ms to the build process. This seems acceptable. At the very least or goal is below 1 ms.

Something to consider as well is hotreloading. A 60 FPS game has ~16ms per frame. Ideally you would put the game code reload on a separate thread, let it do it's thing and when done switch out the tick_event function when it's done. But let's say you keep hotreloading a ton of stuff on the main thread then we want to keep loading below 1 ms as well. Preferably lower but reading from disc takes time as well.

To achieve these times we need batch loading. Few large memcopies (copy memory directly to where it needs to be if possible).

I believe build time (converting object file to BARF) can take a little longer than loading and running a BARF program.
You build once, you run the program many times.
