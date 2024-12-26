#!/bin/bash

libs="-luser32 -lopengl32 -lgdi32"
warnings="-Wall -Wextra -Werror -Wno-unused-variable -Wno-missing-field-initializers -Wreturn-type
          -Wno-unused-parameter -Wno-writable-strings -Wno-format-security -Wno-deprecated-declarations"
includes="-Ithird_party -Ithird_party/Include"

clang++ -fsanitize=address $includes -g src/main.cpp -o hweg.exe $libs $warnings