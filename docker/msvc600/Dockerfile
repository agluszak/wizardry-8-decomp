FROM debian:bookworm-slim

ARG MSVC_REPOSITORY=https://github.com/archaic-msvc/msvc600_sp5.git
ARG MSVC_REF=b0b07e29108e2695eb0274c2a377a7b7d7326150

RUN dpkg --add-architecture i386 \
 && apt-get update \
 && apt-get install -y --no-install-recommends \
      ca-certificates \
      git \
      unzip \
      wget \
      wine \
      wine32 \
 && rm -rf /var/lib/apt/lists/*

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

# A native Windows CMake drives NMake and VC6 inside the same Wine prefix.
RUN wget -O /tmp/cmake.zip \
      https://github.com/Kitware/CMake/releases/download/v3.26.6/cmake-3.26.6-windows-i386.zip \
 && echo "dc3f535d880d1ffa7cfa86a0eefdc1fc9bf8b9ede59d1deb7a2bb41f0a58d2d4  /tmp/cmake.zip" \
      | sha256sum -c - \
 && unzip /tmp/cmake.zip -d /root/.wine/drive_c \
 && mv /root/.wine/drive_c/cmake-3.26.6-windows-i386 /root/.wine/drive_c/cmake \
 && rm /tmp/cmake.zip

ENTRYPOINT ["/usr/bin/wine"]
