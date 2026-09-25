FROM debian:trixie-20260824-slim@sha256:d7e12182ce18b85b93007c1dedf31f2d29e01ccf3182cc4017c709b6259bc132 AS toolchain

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
    WINEPATH="C:\\cmake\\bin;C:\\msvc\\Common\\MSDev98\\Bin;C:\\msvc\\VC98\\Bin" \
    WIZ8_REDUNDANT_CAST_LINES="*"

RUN wineboot --init || true

# Pin the complete compiler tree. The selected SP5 linker is 6.0.8447, the
# build recorded by the canonical Wizardry 8 executable's Rich header.
RUN git init /root/.wine/drive_c/msvc \
 && git -C /root/.wine/drive_c/msvc remote add origin "${MSVC_REPOSITORY}" \
 && git -C /root/.wine/drive_c/msvc fetch --depth 1 origin "${MSVC_REF}" \
 && git -C /root/.wine/drive_c/msvc checkout --detach FETCH_HEAD \
 && test "$(git -C /root/.wine/drive_c/msvc rev-parse HEAD)" = "${MSVC_REF}" \
 && rm -rf /root/.wine/drive_c/msvc/.git

# A native Windows CMake drives NMake/JOM and VC6 inside the Wine prefix.
RUN wget -O /tmp/cmake.zip \
      https://github.com/Kitware/CMake/releases/download/v3.26.6/cmake-3.26.6-windows-i386.zip \
 && echo "dc3f535d880d1ffa7cfa86a0eefdc1fc9bf8b9ede59d1deb7a2bb41f0a58d2d4  /tmp/cmake.zip" \
      | sha256sum -c - \
 && unzip /tmp/cmake.zip -d /root/.wine/drive_c \
 && mv /root/.wine/drive_c/cmake-3.26.6-windows-i386 /root/.wine/drive_c/cmake \
 && rm /tmp/cmake.zip

RUN wget -O /tmp/jom.zip \
      https://download.qt.io/official_releases/jom/jom_1_1_3.zip \
 && echo "128fdd846fe24f8594eed37d1d8929a0ea78df563537c0c1b1861a635013fff8  /tmp/jom.zip" \
      | sha256sum -c - \
 && mkdir -p /root/.wine/drive_c/jom \
 && unzip /tmp/jom.zip -d /root/.wine/drive_c/jom \
 && rm /tmp/jom.zip

# Product builds only need Wine plus the already-provisioned VC6/CMake/JOM
# prefix. Keep Linux analysis/build packages out of this image.
FROM debian:trixie-20260824-slim@sha256:d7e12182ce18b85b93007c1dedf31f2d29e01ccf3182cc4017c709b6259bc132 AS product

RUN dpkg --add-architecture i386 \
 && apt-get update \
 && apt-get install -y --no-install-recommends \
      ca-certificates \
      wine \
      wine32 \
 && rm -rf /var/lib/apt/lists/*

COPY --from=toolchain /root/.wine /root/.wine

ENV WINEARCH=win32 \
    WINEDEBUG=-all \
    WINEPREFIX=/root/.wine \
    INCLUDE="C:\\msvc\\VC98\\Include;C:\\msvc\\VC98\\MFC\\Include;C:\\msvc\\VC98\\ATL\\Include" \
    LIB="C:\\msvc\\VC98\\Lib;C:\\msvc\\VC98\\MFC\\Lib" \
    COMSPEC="C:\\windows\\system32\\cmd.exe" \
    TMP="Z:\\out\\tmp" \
    TEMP="Z:\\out\\tmp" \
    WINEPATH="C:\\cmake\\bin;C:\\msvc\\Common\\MSDev98\\Bin;C:\\msvc\\VC98\\Bin" \
    WIZ8_REDUNDANT_CAST_LINES="*"

ENTRYPOINT ["/usr/bin/wine"]

# Repository analysis adds native Clang/LLVM and the custom clang-tidy plugin.
FROM product AS analysis

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      clang \
      clang-tidy \
      cmake \
      libclang-dev \
      llvm-dev \
      ninja-build \
 && rm -rf /var/lib/apt/lists/* \
 && ln -sf clang /usr/bin/clang-cl

# VC6's archive stores many headers with uppercase filenames. Wine provides
# Windows' case-insensitive lookup to cl.exe, but native clang-cl needs a
# normalized lint-only mirror. Do not carry this duplicate into the product image.
RUN for source in VC98/Include VC98/MFC/Include VC98/ATL/Include; do \
      destination="/opt/msvc6-$(echo "${source}" | tr '[:upper:]/' '[:lower:]-')"; \
      cp -a "/root/.wine/drive_c/msvc/${source}" "${destination}"; \
      find "${destination}" -depth -mindepth 1 -exec bash -c 'for path do \
        directory=${path%/*}; name=${path##*/}; lower=$(echo "$name" | tr "[:upper:]" "[:lower:]"); \
        if [ "$name" != "$lower" ]; then mv "$path" "$directory/$lower"; fi; \
      done' bash {} +; \
    done

COPY clang-tidy-plugin /tmp/wiz8-clang-tidy-plugin
RUN cmake \
      -S /tmp/wiz8-clang-tidy-plugin \
      -B /tmp/wiz8-clang-tidy-plugin/build \
      -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/usr/local \
      -DLLVM_DIR=/usr/lib/llvm-19/lib/cmake/llvm \
      -DClang_DIR=/usr/lib/llvm-19/lib/cmake/clang \
 && cmake --build /tmp/wiz8-clang-tidy-plugin/build \
 && cmake --install /tmp/wiz8-clang-tidy-plugin/build \
 && install -m 0755 /tmp/wiz8-clang-tidy-plugin/clang-tidy-wrapper.py /usr/local/bin/clang-tidy \
 && test "$(command -v clang-tidy)" = /usr/local/bin/clang-tidy \
 && clang-tidy --wiz8-wrapper-self-test \
 && clang-tidy --checks=-*,wiz8-redundant-scalar-cast --list-checks \
      | grep -q 'wiz8-redundant-scalar-cast' \
 && clang-tidy --checks=-*,wiz8-bool-like-byte --list-checks \
      | grep -q 'wiz8-bool-like-byte' \
 && WIZ8_REDUNDANT_CAST_LINES='*' clang-tidy \
      --checks=-*,wiz8-redundant-scalar-cast \
      /tmp/wiz8-clang-tidy-plugin/test.cpp \
      -- --target=i686-pc-windows-msvc -std=c++17 \
      > /tmp/wiz8-clang-tidy-plugin/test.log 2>&1 \
 && test "$(grep -c '\[wiz8-redundant-scalar-cast\]' \
      /tmp/wiz8-clang-tidy-plugin/test.log)" -eq 7 \
 && WIZ8_REDUNDANT_CAST_LINES='/home/build/wizardry/include/wiz8/clang_tidy_record_fixture_types.h@1-200;/home/build/wizardry/include/wiz8/clang_tidy_record_fixture_macros.h@1-20;/home/build/wizardry/src/wiz8/clang_tidy_record_cast_fixture.cpp@1-500;/home/build/wizardry/src/wiz8/clang_tidy_record_cast_fixture_second.cpp@1-50' \
      clang-tidy \
      --checks=-*,wiz8-project-record-reinterpret-cast \
      --header-filter=.* \
      /tmp/wiz8-clang-tidy-plugin/record_cast_fixture.cpp \
      /tmp/wiz8-clang-tidy-plugin/record_cast_fixture_second.cpp \
      -- --target=i686-pc-windows-msvc -std=c++17 \
      > /tmp/wiz8-clang-tidy-plugin/record_cast_fixture.log 2>&1 \
 && record_cast_diagnostics="$(grep -c '\[wiz8-project-record-reinterpret-cast\]' \
      /tmp/wiz8-clang-tidy-plugin/record_cast_fixture.log)" \
 && if [ "$record_cast_diagnostics" -ne 18 ]; then \
      echo "expected 18 project-record cast diagnostics, got $record_cast_diagnostics"; \
      cat /tmp/wiz8-clang-tidy-plugin/record_cast_fixture.log; exit 1; \
    fi \
 && grep -q "repository record 'cast_fixture::A' to repository record 'cast_fixture::B'" \
      /tmp/wiz8-clang-tidy-plugin/record_cast_fixture.log \
 && grep -q "source record 'cast_fixture::A\*' is erased here" \
      /tmp/wiz8-clang-tidy-plugin/record_cast_fixture.log \
 && grep -q "repository record 'cast_fixture::Renderer::Pick'" \
      /tmp/wiz8-clang-tidy-plugin/record_cast_fixture.log \
 && grep -q 'source record provenance .* is unresolved' \
      /tmp/wiz8-clang-tidy-plugin/record_cast_fixture.log \
 && WIZ8_REDUNDANT_CAST_LINES='*' clang-tidy \
      --checks=-*,wiz8-project-record-reinterpret-cast \
      --header-filter=.* \
      /tmp/wiz8-clang-tidy-plugin/record_cast_fixture.cpp \
      /tmp/wiz8-clang-tidy-plugin/record_cast_fixture_second.cpp \
      /tmp/wiz8-clang-tidy-plugin/record_cast_fixture_empty.cpp \
      -- --target=i686-pc-windows-msvc -std=c++17 \
      -I /tmp/wiz8-clang-tidy-plugin \
      > /tmp/wiz8-clang-tidy-plugin/record_cast_audit.log 2>&1 \
 && test "$(grep -c '\[wiz8-project-record-reinterpret-cast\]' \
      /tmp/wiz8-clang-tidy-plugin/record_cast_audit.log)" -eq 18 \
 && grep -q 'explicit full-tree audit mode selected' \
      /tmp/wiz8-clang-tidy-plugin/record_cast_audit.log \
 && if WIZ8_REDUNDANT_CAST_LINES='/tmp/wiz8-clang-tidy-plugin/bool_test.cpp@1-240' clang-tidy \
      --checks=-*,wiz8-bool-like-byte \
      /tmp/wiz8-clang-tidy-plugin/bool_test.cpp \
      -- --target=i686-pc-windows-msvc -std=c++17 \
      > /tmp/wiz8-clang-tidy-plugin/bool_test.log 2>&1; then \
      cat /tmp/wiz8-clang-tidy-plugin/bool_test.log; exit 1; \
    fi \
 && bool_diagnostics="$(grep -c '\[wiz8-bool-like-byte\]' \
      /tmp/wiz8-clang-tidy-plugin/bool_test.log)" \
 && if [ "$bool_diagnostics" -ne 12 ]; then \
      echo "expected 12 bool-like diagnostics, got $bool_diagnostics"; \
      cat /tmp/wiz8-clang-tidy-plugin/bool_test.log; exit 1; \
    fi \
 && grep -q "warning: byte-returning predicate 'HasLineOfSightToBounds0046FD70'" \
      /tmp/wiz8-clang-tidy-plugin/bool_test.log \
 && rm -rf /tmp/wiz8-clang-tidy-plugin

ENTRYPOINT ["/usr/bin/wine"]
