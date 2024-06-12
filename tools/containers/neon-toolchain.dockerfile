ARG ARCH=amd64
ARG OS_VERSION=8.10
FROM $ARCH/almalinux:8.10

RUN mkdir /sysroot

ARG GCC_VERSION=13.2.0
ARG GDB_VERSION=14.1

RUN dnf update -y &&  \
    dnf install -y 'dnf-command(config-manager)' && \
    dnf group install -y "Development Tools" && \
    dnf config-manager --set-enabled powertools && \
    dnf install -y \
        wget \
        tar \
        gmp-devel \
        mpfr-devel \
        libmpc-devel \
    && \
    echo $OS_VERSION > /etc/version_id
RUN dnf install --installroot=/sysroot --releasever=$(cat /etc/version_id) -y \
    glibc \
    cmake \
    ninja-build \
    libXcursor-devel \
    libXi-devel \
    libXinerama-devel \
    libXrandr-devel \
    pulseaudio-libs-devel \
    alsa-lib-devel

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


