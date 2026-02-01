#!/usr/bin/env python3

import os, subprocess, shlex, platform

ROOT = os.path.dirname(__file__)

def main():
    compile()

def compile():
    INT = f"{ROOT}/int"
    RELEASES = f"{ROOT}/releases"
    VERSION = f"0.0.1-dev"
    TARGET = f"{platform.system().lower()}-x86_64"
    PACKAGE = f"{RELEASES}/barf-{VERSION}-{TARGET}"
    os.makedirs(INT, exist_ok=True)
    os.makedirs(RELEASES, exist_ok=True)
    os.makedirs(PACKAGE, exist_ok=True)

    FLAGS = f"-g -Wall -I{ROOT}/src -I{ROOT}/include"
    FILES = f"{ROOT}/src/barf/main.c {ROOT}/src/barf/format.c {ROOT}/src/barf/barf.c"
    EXE   = f"{PACKAGE}/barf{'.exe' if platform.system()=="Windows" else ''}"
    cmd(f"gcc {FLAGS} {FILES} -o {EXE}")

    os.system("gcc -c -mavx2 examples/wa.c -o wa.o")

    os.system(f"{EXE} -c wa.ba wa.o")
    os.system(f"{EXE} wa.ba")

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

