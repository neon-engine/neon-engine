ARG ARCH=amd64
ARG OS_VERSION=8.10
FROM ${ARCH}/almalinux:${OS_VERSION}

RUN dnf update -y \
    && dnf install -y 'dnf-command(config-manager)' \
    && dnf group install -y "Development Tools" \
    && dnf config-manager --set-enabled powertools \
    && dnf install -y epel-release \
    && dnf install -y \
        wget \
        tar \
        gmp-devel \
        mpfr-devel \
        libmpc-devel \
        texinfo \
        rsync \
        ncurses-devel \
        perl \
        fileutils \
        bc \
        python3 \
        patch \
        perl-ExtUtils-MakeMaker \
        perl-IPC-Cmd \
    && dnf clean all \
    && echo ${OS_VERSION} > /etc/version_id

ENV PATH="/sdk/bin:${PATH}"
ENV MENUCONFIG_COLOR=blackbg

ENTRYPOINT [ "/bin/bash" ]
