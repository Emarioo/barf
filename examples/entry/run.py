#!/usr/bin/env python3

import os

ROOT = os.path.dirname(__file__)

prev_cwd = os.getcwd()
if ROOT != prev_cwd:
    os.chdir(ROOT)

os.system(f"gcc -c -o app.o app.c")
os.system(f"barf -c -o app.ba app.o")
os.system(f"barf app.ba -- Here is data")

if ROOT != prev_cwd:
    os.chdir(prev_cwd)
