#!/usr/bin/env bash

conan install ./conan \
  -of build \
  --profile:build=conan/profiles/default.jinja \
  --profile:build=conan/profiles/compiler-linux-clang.jinja \
  --build=missing \
  -s build_type=Release
