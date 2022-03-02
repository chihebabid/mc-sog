# See here for image contents: https://github.com/microsoft/vscode-dev-containers/tree/v0.217.4/containers/cpp/.devcontainer/base.Dockerfile

# [Choice] Debian / Ubuntu version (use Debian 11, Ubuntu 18.04/21.04 on local arm64/Apple Silicon): debian-11, debian-10, ubuntu-21.04, ubuntu-20.04, ubuntu-18.04
ARG VARIANT="bullseye"
FROM mcr.microsoft.com/vscode/devcontainers/cpp:0-${VARIANT}

# [Optional] Uncomment this section to install additional packages.
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends wget cmake bison flex \
    openmpi-bin libopenmpi-dev && \
    wget http://www.lrde.epita.fr/dload/spot/spot-2.10.4.tar.gz && \
    tar xzf spot-2.10.4.tar.gz && cd spot-2.10.4 && \
    ./configure --disable-doxygen --disable-python && make && make install
