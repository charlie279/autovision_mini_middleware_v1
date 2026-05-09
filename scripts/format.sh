#!/usr/bin/env bash
#
# 功能：
#   使用 clang-format 统一格式化本项目的 C++ 头文件和源文件。
#
# 使用方法：
#   ./scripts/format.sh
#
# 格式化范围：
#   include/*.hpp
#   src/*.cpp

set -euo pipefail

if ! command -v clang-format >/dev/null 2>&1; then
    echo "[format] clang-format not found. Please install it first:"
    echo "sudo apt install -y clang-format"
    exit 1
fi

find include src \( -name '*.hpp' -o -name '*.cpp' \) -print0 | xargs -0 clang-format -i

echo "[format] done"