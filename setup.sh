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

project_vol=$(pwd):/CLionProjects/ProjectNeon:z
conan_vol=$(pwd)/conan/cache:/conan:z

# shellcheck disable=SC2046
podman run -i --rm -v "${project_vol}" -v "${conan_vol}" neon-dev.linux-x64:latest << EOF
  cd /CLionProjects/ProjectNeon

  scripts/restore-conan-deps.sh
EOF
