#!/usr/bin/env bash

llvm_version=20
export CC="/opt/homebrew/opt/llvm@${llvm_version}/bin/clang"
export CXX="/opt/homebrew/opt/llvm@${llvm_version}/bin/clang++"
