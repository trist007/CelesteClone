#!/bin/bash

timestamp=$(date +%s)

defines="-DENGINE"
libs="-luser32 -lopengl32 -lgdi32 -lole32 -Lthird_party/lib -lfreetype.lib"
warnings="-Wall -Wextra -Werror -Wno-unused-variable -Wno-missing-field-initializers -Wreturn-type
          -Wno-unused-parameter -Wno-writable-strings -Wno-format-security -Wno-deprecated-declarations
          -Wno-switch -Wno-unused-value -Wno-missing-braces -Wno-unused-but-set-variable -Wno-sign-compare
          -Wno-char-subscripts"
includes="-Ithird_party -Ithird_party/Include"

clang++ -fsanitize=address $includes -g src/main.cpp -o schnitzel.exe $libs $warnings $defines

rm -f game_* # Remove old game_* files
clang++ -g "src/game.cpp" -shared -o game_$timestamp.dll $warnings $defines
mv game_$timestamp.dll game.dll