#!/usr/bin/env bash

~/.local/bin/conan profile detect --force

~/.local/bin/conan install . \
  -of build/conan \
  --build=missing \
  -s build_type=Release \
  -s compiler.cppstd=gnu20
