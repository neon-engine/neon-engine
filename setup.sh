#!/usr/bin/env bash

echo "Setting up ProjectNeon"

(
  echo "building containers"
  # shellcheck disable=SC2164
  cd containers;
  ./build-containers.sh
)

project_vol=$(pwd):/CLionProjects/ProjectNeon:z

# shellcheck disable=SC2046
podman run -i --rm -v "${project_vol}" neon-dev.linux-x64:latest << EOF
  (
    cd /CLionProjects/ProjectNeon/vcpkg

    ./bootstrap-vcpkg.sh -disableMetrics

    ./vcpkg x-update-baseline --add-initial-baseline
  )

  if [[ ! -d /CLionProjects/ProjectNeon/external/gl3w ]]; then
    git clone https://github.com/skaslev/gl3w
    cd gl3w
    python3 gl3w_gen.py --root /CLionProjects/ProjectNeon/external/gl3w
    cd ..
    rm -r gl3w
  fi
EOF
