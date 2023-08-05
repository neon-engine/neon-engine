#!/usr/bin/env bash

~/.local/bin/conan install ./conan \
  -of build/conan \
  --profile:build=conan/profiles/default.jinja \
  --profile:build=conan/profiles/compiler-linux-clang.jinja \
  --build=missing \
  -s build_type=Release
