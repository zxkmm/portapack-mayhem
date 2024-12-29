#!/bin/bash

# 确保生成 compile_commands.json
CMAKE_EXPORT_COMPILE_COMMANDS=ON

# 构建目录
BUILD_DIR="build"

# 创建构建目录
mkdir -p $BUILD_DIR
cd $BUILD_DIR

# 运行 cmake 生成 compile_commands.json
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..

# 回到项目根目录
cd ..

# 运行 clang-tidy
find firmware/common firmware/hackrf_usb host/libhackrf/src host/hackrf-tools/src \
     -name '*.c' -o -name '*.cpp' -o -name '*.h' -o -name '*.hpp' | \
while read -r file; do
    echo "Checking: $file"
    clang-tidy -p=$BUILD_DIR/compile_commands.json "$file"
done
