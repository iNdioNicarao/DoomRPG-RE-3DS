# Toolchain image for building DoomRPG-RE-3DS .3dsx + .cia.
# Mirrors the dxx-3ds Dockerfile CIA steps (makerom from source, bannertool
# from release). DoomRPG has no PHYSFS/picaGL dependency, so those are dropped.
#
FROM devkitpro/devkitarm:latest

RUN apt-get update && apt-get install -y gcc g++ zip cmake git wget unzip

# makerom (built from source)
RUN cd / && git clone --recursive https://github.com/profi200/Project_CTR
RUN cd /Project_CTR/makerom && make deps && make
ENV PATH="/Project_CTR/makerom/bin:${PATH}"

# bannertool: the upstream release is a prebuilt linux-x86_64 binary. On an
# aarch64 host (colima on Apple Silicon) run it under qemu by adding the amd64
# dynamic loader + libc. This mirrors how the dxx-3ds pipeline used the same
# prebuilt bannertool on x86_64 hosts.
RUN dpkg --add-architecture amd64 && \
    apt-get update && \
    apt-get install -y --no-install-recommends libc6:amd64 libstdc++6:amd64 qemu-user && \
    cd /tmp && \
    wget -q https://github.com/Epicpkmn11/bannertool/releases/download/v1.2.2/bannertool.zip && \
    unzip -o bannertool.zip && \
    mv linux-x86_64/bannertool /usr/local/bin/ && \
    chmod +x /usr/local/bin/bannertool && \
    rm -f bannertool.zip

# Default command: build the game + CIA when the repo is mounted at /src
# and this cia/ dir is mounted at /cia. Override with your own command.
WORKDIR /src
CMD set -e && \
    rm -rf build && mkdir build && cd build && \
    cmake .. -DCMAKE_TOOLCHAIN_FILE=/opt/devkitpro/cmake/3DS.cmake && \
    cmake --build . -j4 && \
    cp build/src/DoomRPG.elf /cia/DoomRPG.elf && \
    cd /cia && ./make_cia.sh /cia/DoomRPG.elf
