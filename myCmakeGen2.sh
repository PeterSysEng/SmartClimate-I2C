#!/bin/bash
set -e  # Megáll, ha bármelyik parancs hibát dob

echo "--- Generate CMake project (build directory)... ---"

echo "1. command -> [PROJEKT_DIR=$(pwd)]... "
# A változóba elmentjük a projekt aktuális mappáját
PROJECT_DIR=$(pwd)

# -B build: létrehozza a mappát és oda generál
# -D...: átadjuk a toolchain fájlt (most a gyökérből nézve a relatív utat!)
# -DCMAKE_EXPORT_COMPILE_COMMANDS=ON: kell az LSP kódkiegészítéshez

echo "2. command 
->  build dir create: cmake -B build
-> -DCMAKE_TOOLCHAIN_FILE=$PROJECT_DIR/cmake/gcc-arm-none-eabi.cmake
-> -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
"
cmake -B build \
      -DCMAKE_TOOLCHAIN_FILE="$PROJECT_DIR/cmake/gcc-arm-none-eabi.cmake" \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

echo "--- Compile in progress...  (j$(nproc)) ---"
# --build build: a CMake maga hívja meg a make-et a build mappában
echo "3. command -> Compile : [cmake --build build -j$(nproc)]"
cmake --build build -j$(nproc)

echo "--- Indexing updating LSP... ---"
# -f: felülírja a régit, ha létezik (force)
echo "4. command -> Link create: [ln -sf build/compile_commands.json .]"
ln -sf build/compile_commands.json .

echo "--- Done! The generated file: $(find build -name "*.elf") ---"
