ARG ARCH=amd64
ARG OS_VERSION=8.10
FROM $ARCH/almalinux:8.10

RUN mkdir /sdk

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
