#!/usr/bin/env python3

import os, subprocess, shlex, platform

ROOT = os.path.dirname(__file__)
 
BARF_FILES = [
    f"{ROOT}/src/barf/main.c",
    f"{ROOT}/src/barf/format.c",
    f"{ROOT}/src/barf/barf.c"
]
LIBC_FILES = [
    f"{ROOT}/src/libc/libc.c",
    f"{ROOT}/src/libc/libm.c",
]

WARN_FLAGS = "-Wall -Wno-unused-variable -Wno-unused-value"
NOLIB_FLAGS = "-fno-builtin -static -fPIC -fpie -nostdlib -ffreestanding -nostartfiles -mavx2"

def main():
    compile()

def compile():
    RELEASES = f"{ROOT}/releases"
    VERSION = f"0.0.1-dev"
    TARGET = f"{platform.system().lower()}-x86_64"
    PACKAGE = f"{RELEASES}/barf-{VERSION}-{TARGET}"

    compile_native_barf(PACKAGE)

    
    artifact = "barf.ba"
    artifact_flags = f"-I{ROOT}/include -I{ROOT}/src {WARN_FLAGS} {NOLIB_FLAGS}"
    compile_artifact(artifact, BARF_FILES + LIBC_FILES, artifact_flags)

    compile_artifact("pro.ba", ["examples/wa.c","examples/util.c"] + LIBC_FILES, artifact_flags)
    # compile_artifact("pro.ba", ["examples/wa.c","examples/util.c"], artifact_flags)

    # os.system("barf -d barf.ba")

    # os.system("barf barf.ba -- wa.ba")


    os.system(f"barf pro.ba")

    # os.system(f"{EXE} -d pro.ba")
    # os.system(f"{EXE} -c wa.ba wa.o -- ok")
    # os.system(f"{EXE} wa.ba")

def compile_native_barf(output_path):
    INT = f"{ROOT}/int"
    os.makedirs(INT, exist_ok=True)
    os.makedirs(output_path, exist_ok=True)

    COMMON_FLAGS = f"-g -I{ROOT}/include -I{ROOT}/src"
    EXE = f"{output_path}/barf{'.exe' if platform.system()=="Windows" else ''}"

    OBJECTS = [ INT + "/" + os.path.basename(f).replace('.c', '.o') for f in BARF_FILES ]

    if platform.system() == "Windows":
        COMMON_FLAGS += " -DOS_WINDOWS"
    if platform.system() == "Linux":
        COMMON_FLAGS += " -DOS_LINUX"

    for obj, src in zip(OBJECTS, BARF_FILES):
        cmd(f"gcc -c {COMMON_FLAGS} {WARN_FLAGS} {src} -o {obj}")

    cmd(f"gcc {COMMON_FLAGS} {WARN_FLAGS} -o {EXE} {ROOT}/src/platform/platform.c {' '.join(OBJECTS)}")
    
    
def compile_artifact(output_file, files, flags):
    INT = f"{ROOT}/int"
    os.makedirs(INT, exist_ok=True)
    os.makedirs(os.path.dirname(os.path.abspath(output_file)), exist_ok=True)
    OBJECTS = [ INT + "/" + os.path.basename(f).replace('.c', '.o') for f in files ]

    for obj, src in zip(OBJECTS, files):
        cmd(f"gcc -c {flags} {src} -o {obj}")
    
    cmd(f"barf -c -o {output_file} {' '.join(OBJECTS)}")

def cmd(c: str, silent: bool = False):
    c = c.replace("\\", "/")
    
    # print(c)
    res = os.system(c)
    if res != 0:
        print(c)
        exit(1)
    # proc = subprocess.run(shlex.split(c), shell=True, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    # if (proc.returncode != 0 or len(proc.stdout) > 0) and not silent:
    #     print(c)
    #     print(proc.stdout, end="")
    # return proc.returncode

if __name__ == "__main__":
    main()

