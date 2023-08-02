FROM --platform=linux/amd64 ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive
ENV CC=/usr/bin/clang
ENV CXX=/usr/bin/clang++

RUN apt-get update && apt-get -y install tzdata

# Install dev tools from apt, git, and pipx
RUN apt update \
    && apt install -y \
          python3 \
          git \
          clang \
          gdb \
          cmake \
          make \
          ninja-build \
          libgl1-mesa-dev \
          vulkan-tools \
          libvulkan-dev \
          vulkan-validationlayers-dev \
          spirv-tools \
          python3.8-venv \
          pipx \
    && apt clean \
    && git clone https://github.com/skaslev/gl3w \
    && cd gl3w \
    && python3 gl3w_gen.py --root /usr/local \
    && cd .. \
    && rm -r gl3w \
    && pipx install conan \
    && echo 'export PATH=/root/.local/bin:$PATH' >> ~/.bashrc \
    && source ~/.bashrc \
    && conan profile detect --force

ENTRYPOINT [ "/bin/bash" ]
