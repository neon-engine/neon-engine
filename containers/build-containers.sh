#!/usr/bin/env bash

echo "Building containers"

# Make sure that containers are built with default settings
export CONTAINERS_CONF=/usr/share/containers/containers.conf

timestamp=$(date +%Y%m%d%H%M%S)

podman build \
  -t "neon-dev.linux-x64:${timestamp}" \
  -f neon-dev.linux-x64.Dockerfile
