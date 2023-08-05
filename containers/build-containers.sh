#!/usr/bin/env bash

echo "Building containers"

# Make sure that containers are built with default settings
export CONTAINERS_CONF=/usr/share/containers/containers.conf

podman build \
  -t "neon-dev.linux-x64" \
  -f neon-dev.linux-x64.Dockerfile
