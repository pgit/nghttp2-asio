#
# https://github.com/microsoft/vscode-dev-containers/tree/main/containers/debian
#
FROM docker.io/psedoc/cpp-devcontainer:0.15

# ==================================================================================================

ARG NGHTTP2_VERSION=v1.65.0
RUN git clone --recursive --depth 1 --branch ${NGHTTP2_VERSION} https://github.com/nghttp2/nghttp2.git && \
    cd nghttp2 && \
    autoreconf -i && \
    ./configure --prefix=/usr/local && \
    make -j$(nproc) && make install && \
    cd .. && rm -rf nghttp2

# ==================================================================================================
