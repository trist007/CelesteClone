#!/bin/bash

libs="-luser32 -lopengl32 -lgdi32"
warnings="-Wno-writable-strings -Wno-format-security"
includes="-Ithird_party -Ithird_party/Include"

clang++ $includes -g src/main.cpp -o hweg.exe $libs $warnings