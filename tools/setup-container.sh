#!/usr/bin/env bash

echo "Setting up ProjectNeon development with containerization"

# Get the absolute path of the script
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Get the current directory
current_dir="$(pwd)"

# Check if the script is being executed from its own directory
if [ "${script_dir}" != "${current_dir}" ]; then
    echo "Please run this script from its own directory: ${current_dir}"
    exit 1
fi

git pull --recurse-submodules

(
  echo "building containers"
  # shellcheck disable=SC2164
  cd containers;
  ./build-containers.sh
)

container_dir=/CLionProjects/ProjectNeon
project_vol=${current_dir}:${container_dir}:z

# shellcheck disable=SC2046
podman run -i --rm -v "${project_vol}" -w ${container_dir} neon-dev.linux-x64:latest -e << EOF
  (
    cd vcpkg

    ./bootstrap-vcpkg.sh -disableMetrics

    ./vcpkg x-update-baseline --add-initial-baseline
  )

  if [[ ! -d external/gl3w ]]; then
    git clone https://github.com/skaslev/gl3w
    cd gl3w
    python3 gl3w_gen.py --root external/gl3w --ext
    cd ..
    rm -r gl3w
  else
    echo "external/gl3w already exists, delete and rerun the script to regenerate"
  fi
EOF
