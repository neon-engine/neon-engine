#!/usr/bin/env bash

conan install ./conan \
  -of build \
  --profile conan/profiles/default.jinja \
  --profile conan/profiles/compiler-linux-clang.jinja \
  --build=missing \
  -s build_type=Release
