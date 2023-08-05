#!/usr/bin/env bash

echo "Setting up ProjectNeon"

echo "Removing the old conan directory"
rm -r build/conan

(
  echo "building containers"
  # shellcheck disable=SC2164
  cd containers;
  ./build-containers.sh
)

# shellcheck disable=SC2046
podman run -i --rm -v $(pwd):/CLionProjects/ProjectNeon:z neon-dev.linux-x64:latest << EOF
  cd /CLionProjects/ProjectNeon

  scripts/restore-conan-deps.sh

  cd /CLionProjects/ProjectNeon/build/conan;

  cmake /CLionProjects/ProjectNeon \
    -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -B /CLionProjects/ProjectNeon/build/Debug
EOF
