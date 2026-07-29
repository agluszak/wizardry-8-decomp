FROM debian:bookworm-20260623-slim@sha256:60eac759739651111db372c07be67863818726f754804b8707c90979bda511df

ARG MSVC_REPOSITORY=https://github.com/archaic-msvc/msvc600_sp5.git
ARG MSVC_REF=b0b07e29108e2695eb0274c2a377a7b7d7326150

RUN dpkg --add-architecture i386 \
 && apt-get update \
 && apt-get install -y --no-install-recommends \
      ca-certificates \
      clang \
      cmake \
      libclang-dev \
      llvm-dev \
      git \
      ninja-build \
      unzip \
      wget \
      wine \
      wine32 \
 && rm -rf /var/lib/apt/lists/*

# Debian ships clang's CL-compatible driver mode without the unversioned
# clang-cl launcher. The executable name selects that mode.
RUN ln -sf clang /usr/bin/clang-cl

ENV WINEARCH=win32 \
    WINEDEBUG=-all \
    WINEPREFIX=/root/.wine \
    INCLUDE="C:\\msvc\\VC98\\Include;C:\\msvc\\VC98\\MFC\\Include;C:\\msvc\\VC98\\ATL\\Include" \
    LIB="C:\\msvc\\VC98\\Lib;C:\\msvc\\VC98\\MFC\\Lib" \
    COMSPEC="C:\\windows\\system32\\cmd.exe" \
    TMP="Z:\\out\\tmp" \
    TEMP="Z:\\out\\tmp" \
    WINEPATH="C:\\cmake\\bin;C:\\msvc\\Common\\MSDev98\\Bin;C:\\msvc\\VC98\\Bin"

RUN wineboot --init || true

# Pin the complete compiler tree.  The selected SP5 linker is 6.0.8447, the
# build recorded by the canonical Wizardry 8 executable's Rich header.  The
# repository/ref remain build arguments so other VC6 service-pack hypotheses
# can be tested without changing the container logic.
RUN git init /root/.wine/drive_c/msvc \
 && git -C /root/.wine/drive_c/msvc remote add origin "${MSVC_REPOSITORY}" \
 && git -C /root/.wine/drive_c/msvc fetch --depth 1 origin "${MSVC_REF}" \
 && git -C /root/.wine/drive_c/msvc checkout --detach FETCH_HEAD \
 && test "$(git -C /root/.wine/drive_c/msvc rev-parse HEAD)" = "${MSVC_REF}"

# VC6's archive stores many headers with uppercase filenames. Wine provides
# Windows' case-insensitive lookup to cl.exe, but native clang-cl needs a
# normalized lint-only mirror. Keep the canonical compiler tree untouched.
RUN for source in VC98/Include VC98/MFC/Include VC98/ATL/Include; do \
      destination="/opt/msvc6-$(echo "${source}" | tr '[:upper:]/' '[:lower:]-')"; \
      cp -a "/root/.wine/drive_c/msvc/${source}" "${destination}"; \
      find "${destination}" -depth -mindepth 1 -exec bash -c 'for path do \
        directory=${path%/*}; name=${path##*/}; lower=$(echo "$name" | tr "[:upper:]" "[:lower:]"); \
        if [ "$name" != "$lower" ]; then mv "$path" "$directory/$lower"; fi; \
      done' bash {} +; \
    done

# A native Windows CMake drives NMake and VC6 inside the same Wine prefix.
RUN wget -O /tmp/cmake.zip \
      https://github.com/Kitware/CMake/releases/download/v3.26.6/cmake-3.26.6-windows-i386.zip \
 && echo "dc3f535d880d1ffa7cfa86a0eefdc1fc9bf8b9ede59d1deb7a2bb41f0a58d2d4  /tmp/cmake.zip" \
      | sha256sum -c - \
 && unzip /tmp/cmake.zip -d /root/.wine/drive_c \
 && mv /root/.wine/drive_c/cmake-3.26.6-windows-i386 /root/.wine/drive_c/cmake \
 && rm /tmp/cmake.zip

# VC6 has no /MP, so compile parallelism belongs at the make layer. JOM 1.1.3
# is the last 32-bit release and runs inside this image's win32 Wine prefix.
RUN wget -O /tmp/jom.zip \
      https://download.qt.io/official_releases/jom/jom_1_1_3.zip \
 && echo "128fdd846fe24f8594eed37d1d8929a0ea78df563537c0c1b1861a635013fff8  /tmp/jom.zip" \
      | sha256sum -c - \
 && mkdir -p /root/.wine/drive_c/jom \
 && unzip /tmp/jom.zip -d /root/.wine/drive_c/jom \
 && rm /tmp/jom.zip

ENTRYPOINT ["/usr/bin/wine"]
