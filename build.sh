#!/bin/bash

timestamp=$(date +%s)

libs="-luser32 -lopengl32 -lgdi32"
warnings="-Wall -Wextra -Werror -Wno-unused-variable -Wno-missing-field-initializers -Wreturn-type
          -Wno-unused-parameter -Wno-writable-strings -Wno-format-security -Wno-deprecated-declarations
          -Wno-switch"
includes="-Ithird_party -Ithird_party/Include"

clang++ -fsanitize=address $includes -g src/main.cpp -o schnitzel.exe $libs $warnings

rm -f game_* # Remove old game_* files
clang++ -g "src/game.cpp" -shared -o game_$timestamp.dll $warnings
mv game_$timestamp.dll game.dll