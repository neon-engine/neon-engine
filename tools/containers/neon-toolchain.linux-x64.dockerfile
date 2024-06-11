ARG ARCH=amd64
FROM $ARCH/almalinux:8

RUN mkdir /sysroot

ARG GCC_VERSION=13.2.0
ARG GDB_VERSION=14.1

RUN grep '^VERSION_ID=' /etc/os-release | cut -d '=' -f 2 | tr -d '"' > /etc/version_id && \
    echo "VERSION_ID=$(cat /etc/version_id)"

RUN dnf update -y
RUN dnf install -y 'dnf-command(config-manager)'
RUN dnf config-manager --set-enabled powertools
RUN dnf install --installroot=/sysroot --releasever=$(cat /etc/version_id) -y \
    glibc \
    libXcursor-devel \
    libXi-devel \
    libXinerama-devel \
    libXrandr-devel \
    pulseaudio-libs-devel \
    alsa-lib-devel

RUN dnf group install -y "Development Tools"
RUN dnf install -y \
    wget \
    tar \
    libmpc-devel

RUN wget http://gnu.mirror.constant.com/gcc/gcc-$GCC_VERSION/gcc-$GCC_VERSION.tar.gz
RUN tar zxf gcc-$GCC_VERSION.tar.gz
RUN mkdir gcc-build
RUN cd gcc-build
RUN ../gcc-$GCC_VERSION/configure \
  --enable-languages=c,c++ \
  --disable-multilib \
  --prefix=/sysroot
RUN make -j4
RUN make install


