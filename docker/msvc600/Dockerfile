# syntax=docker/dockerfile:1
FROM debian:trixie-20260824-slim@sha256:d7e12182ce18b85b93007c1dedf31f2d29e01ccf3182cc4017c709b6259bc132 AS toolchain

# Use QEMU only on hosts that cannot execute Linux i386 ELF binaries.
ARG WIZ8_EMULATE_I386=0
COPY wine-runner.sh /usr/local/bin/wiz8-wine
COPY qemu-wine-preloader.sh /usr/local/libexec/qemu-wine-preloader

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
 && if [ "${WIZ8_EMULATE_I386}" = 1 ]; then \
      apt-get install -y --no-install-recommends qemu-user wine64 wine32-preloader; \
      mv /usr/lib/wine/wine-preloader /usr/lib/wine/wine-preloader.real; \
      install -m 0755 /usr/local/libexec/qemu-wine-preloader /usr/lib/wine/wine-preloader; \
    fi \
 && chmod +x /usr/local/bin/wiz8-wine \
 && rm -rf /var/lib/apt/lists/*

ENV WIZ8_EMULATE_I386=${WIZ8_EMULATE_I386} \
    WINEARCH=win32 \
    WINEDEBUG=-all \
    WINEPREFIX=/root/.wine \
    INCLUDE="C:\\msvc\\VC98\\Include;C:\\msvc\\VC98\\MFC\\Include;C:\\msvc\\VC98\\ATL\\Include" \
    LIB="C:\\msvc\\VC98\\Lib;C:\\msvc\\VC98\\MFC\\Lib" \
    COMSPEC="C:\\windows\\system32\\cmd.exe" \
    TMP="Z:\\out\\tmp" \
    TEMP="Z:\\out\\tmp" \
    WINEPATH="C:\\cmake\\bin;C:\\msvc\\Common\\MSDev98\\Bin;C:\\msvc\\VC98\\Bin"

# Stop the server and remove its Unix sockets before Docker snapshots the layer.
# VFS cannot copy those sockets into the next build step.
RUN /usr/local/bin/wiz8-wine wineboot --init \
 && if [ "${WIZ8_EMULATE_I386}" = 1 ]; then \
      /usr/lib/wine/wineserver64 -k; /usr/lib/wine/wineserver64 -w; \
    else \
      wineserver -k; wineserver -w; \
    fi \
 && rm -rf /tmp/.wine-0

# Pin the complete compiler tree. The selected SP5 linker is 6.0.8447, the
# build recorded by the canonical Wizardry 8 executable's Rich header.
RUN --mount=type=secret,id=proxy_ca,env=WIZ8_PROXY_CA \
    if [ -n "${WIZ8_PROXY_CA:-}" ]; then printf "%s\n" "$WIZ8_PROXY_CA" > /tmp/proxy-ca.pem; export GIT_SSL_CAINFO=/tmp/proxy-ca.pem; fi \
 && git init /root/.wine/drive_c/msvc \
 && git -C /root/.wine/drive_c/msvc remote add origin "${MSVC_REPOSITORY}" \
 && git -C /root/.wine/drive_c/msvc fetch --depth 1 origin "${MSVC_REF}" \
 && git -C /root/.wine/drive_c/msvc checkout --detach FETCH_HEAD \
 && test "$(git -C /root/.wine/drive_c/msvc rev-parse HEAD)" = "${MSVC_REF}" \
 && rm -rf /root/.wine/drive_c/msvc/.git \
 && rm -f /tmp/proxy-ca.pem

# A native Windows CMake drives NMake/JOM and VC6 inside the Wine prefix.
RUN --mount=type=secret,id=proxy_ca,env=WIZ8_PROXY_CA \
    if [ -n "${WIZ8_PROXY_CA:-}" ]; then printf "%s\n" "$WIZ8_PROXY_CA" > /tmp/proxy-ca.pem; export WGETRC=/tmp/wgetrc; printf "ca_certificate = /tmp/proxy-ca.pem\n" > "$WGETRC"; fi \
 && wget -O /tmp/cmake.zip \
      https://github.com/Kitware/CMake/releases/download/v3.26.6/cmake-3.26.6-windows-i386.zip \
 && echo "dc3f535d880d1ffa7cfa86a0eefdc1fc9bf8b9ede59d1deb7a2bb41f0a58d2d4  /tmp/cmake.zip" \
      | sha256sum -c - \
 && unzip /tmp/cmake.zip -d /root/.wine/drive_c \
 && mv /root/.wine/drive_c/cmake-3.26.6-windows-i386 /root/.wine/drive_c/cmake \
 && rm -f /tmp/cmake.zip /tmp/wgetrc /tmp/proxy-ca.pem

RUN --mount=type=secret,id=proxy_ca,env=WIZ8_PROXY_CA \
    if [ -n "${WIZ8_PROXY_CA:-}" ]; then printf "%s\n" "$WIZ8_PROXY_CA" > /tmp/proxy-ca.pem; export WGETRC=/tmp/wgetrc; printf "ca_certificate = /tmp/proxy-ca.pem\n" > "$WGETRC"; fi \
 && wget -O /tmp/jom.zip \
      https://download.qt.io/official_releases/jom/jom_1_1_3.zip \
 && echo "128fdd846fe24f8594eed37d1d8929a0ea78df563537c0c1b1861a635013fff8  /tmp/jom.zip" \
      | sha256sum -c - \
 && mkdir -p /root/.wine/drive_c/jom \
 && unzip /tmp/jom.zip -d /root/.wine/drive_c/jom \
 && rm -f /tmp/jom.zip /tmp/wgetrc /tmp/proxy-ca.pem

# Product builds only need Wine plus the already-provisioned VC6/CMake/JOM
# prefix. Keep Linux analysis/build packages out of this image.
FROM debian:trixie-20260824-slim@sha256:d7e12182ce18b85b93007c1dedf31f2d29e01ccf3182cc4017c709b6259bc132 AS product

ARG WIZ8_EMULATE_I386=0
COPY wine-runner.sh /usr/local/bin/wiz8-wine
COPY qemu-wine-preloader.sh /usr/local/libexec/qemu-wine-preloader

RUN dpkg --add-architecture i386 \
 && apt-get update \
 && apt-get install -y --no-install-recommends \
      ca-certificates \
      wine \
      wine32 \
 && if [ "${WIZ8_EMULATE_I386}" = 1 ]; then \
      apt-get install -y --no-install-recommends qemu-user wine64 wine32-preloader; \
      mv /usr/lib/wine/wine-preloader /usr/lib/wine/wine-preloader.real; \
      install -m 0755 /usr/local/libexec/qemu-wine-preloader /usr/lib/wine/wine-preloader; \
    fi \
 && chmod +x /usr/local/bin/wiz8-wine \
 && rm -rf /var/lib/apt/lists/*

COPY --from=toolchain /root/.wine /root/.wine

ENV WIZ8_EMULATE_I386=${WIZ8_EMULATE_I386} \
    WINEARCH=win32 \
    WINEDEBUG=-all \
    WINEPREFIX=/root/.wine \
    INCLUDE="C:\\msvc\\VC98\\Include;C:\\msvc\\VC98\\MFC\\Include;C:\\msvc\\VC98\\ATL\\Include" \
    LIB="C:\\msvc\\VC98\\Lib;C:\\msvc\\VC98\\MFC\\Lib" \
    COMSPEC="C:\\windows\\system32\\cmd.exe" \
    TMP="Z:\\out\\tmp" \
    TEMP="Z:\\out\\tmp" \
    WINEPATH="C:\\cmake\\bin;C:\\msvc\\Common\\MSDev98\\Bin;C:\\msvc\\VC98\\Bin"

ENTRYPOINT ["/usr/local/bin/wiz8-wine"]

# Repository analysis adds native Clang/LLVM and the custom clang-tidy plugin.
FROM product AS analysis

# Keep provisioning in one layer: VFS copies the complete parent filesystem
# for every RUN/COPY. The retained VC6 headers get a lowercase lint-only mirror;
# LLVM 21 and the plugin build dependencies stay out of the product image.
# Runtime indexer builds use the retained headers and shared libraries; discard
# static LLVM/Clang archives after the plugin tests to keep VFS runs within disk.
COPY clang-tidy-plugin /tmp/wiz8-clang-tidy-plugin
RUN echo "deb http://deb.debian.org/debian trixie-backports main" \
      > /etc/apt/sources.list.d/backports.list \
 && apt-get update \
 && apt-get install -y --no-install-recommends \
      cmake \
      ninja-build \
 && apt-get install -y --no-install-recommends -t trixie-backports \
      clang-21 \
      clang-tidy-21 \
      libclang-21-dev \
      libclang-cpp21-dev \
      llvm-21-dev \
 && rm -rf /var/lib/apt/lists/* \
 && ln -sf clang-21 /usr/bin/clang \
 && ln -sf clang++-21 /usr/bin/clang++ \
 && ln -sf clang-21 /usr/bin/clang-cl \
 && for source in VC98/Include VC98/MFC/Include VC98/ATL/Include; do \
      destination="/opt/msvc6-$(echo "${source}" | tr '[:upper:]/' '[:lower:]-')"; \
      cp -a "/root/.wine/drive_c/msvc/${source}" "${destination}"; \
      find "${destination}" -depth -mindepth 1 -exec bash -c 'for path do \
        directory=${path%/*}; name=${path##*/}; lower=$(echo "$name" | tr "[:upper:]" "[:lower:]"); \
        if [ "$name" != "$lower" ]; then mv "$path" "$directory/$lower"; fi; \
      done' bash {} +; \
    done \
 && cmake \
      -S /tmp/wiz8-clang-tidy-plugin \
      -B /tmp/wiz8-clang-tidy-plugin/build \
      -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/usr/local \
      -DLLVM_DIR=/usr/lib/llvm-21/lib/cmake/llvm \
      -DClang_DIR=/usr/lib/llvm-21/lib/cmake/clang \
 && cmake --build /tmp/wiz8-clang-tidy-plugin/build \
 && cmake --install /tmp/wiz8-clang-tidy-plugin/build \
 && install -m 0755 /tmp/wiz8-clang-tidy-plugin/clang-tidy-wrapper.py /usr/local/bin/clang-tidy \
 && install -m 0644 /tmp/wiz8-clang-tidy-plugin/scalar_facts.py /usr/local/bin/scalar_facts.py \
 && test "$(command -v clang-tidy)" = /usr/local/bin/clang-tidy \
 && clang-tidy --wiz8-wrapper-self-test \
 && clang-tidy --checks=-*,wiz8-redundant-scalar-cast --list-checks \
      | grep -q 'wiz8-redundant-scalar-cast' \
 && clang-tidy --checks=-*,wiz8-bool-like-byte --list-checks \
      | grep -q 'wiz8-bool-like-byte' \
 && clang-tidy --checks=-*,wiz8-scalar-facts --list-checks \
      | grep -q 'wiz8-scalar-facts' \
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
 && WIZ8_REDUNDANT_CAST_LINES='/tmp/wiz8-clang-tidy-plugin/bool_test.cpp@1-300' clang-tidy \
      --checks=-*,wiz8-bool-like-byte --warnings-as-errors=* \
      /tmp/wiz8-clang-tidy-plugin/bool_test.cpp \
      -- --target=i686-pc-windows-msvc -std=c++17 \
      > /tmp/wiz8-clang-tidy-plugin/bool_test.log 2>&1 \
 && test "$(grep -c 'warning:.*\[wiz8-bool-like-byte\]' \
      /tmp/wiz8-clang-tidy-plugin/bool_test.log)" -eq 17 \
 && for candidate in HasEntries Both HasLineOfSightToBounds0046FD70 m_dirty \
      selection_settled settled transition_active left_ready right_ready both_ready \
      fCombatMode talking execution_active_000 initialized display_active \
      SetPanelVisible::visible handed_ready; do \
      grep -q "'$candidate'" /tmp/wiz8-clang-tidy-plugin/bool_test.log || exit 1; \
    done \
 && ! grep -Eq 'error:|use bool|bool-byte-ok' /tmp/wiz8-clang-tidy-plugin/bool_test.log \
 && ! grep -Eq 'SetPanelLayered|SetCallbackEnabled|g_menu_active|has_group|monster_bound|nested_active' \
      /tmp/wiz8-clang-tidy-plugin/bool_test.log \
 && if WIZ8_REDUNDANT_CAST_LINES='/tmp/wiz8-clang-tidy-plugin/bool_test.cpp@1-300' clang-tidy \
      --checks=-*,wiz8-bool-like-byte \
      /tmp/wiz8-clang-tidy-plugin/bool_test.cpp \
      -- --target=i686-pc-windows-msvc -std=c++17 -DWIZ8_BOOL_TEST_INVALID \
      > /tmp/wiz8-clang-tidy-plugin/bool_test_invalid.log 2>&1; then \
      cat /tmp/wiz8-clang-tidy-plugin/bool_test_invalid.log; exit 1; \
    fi \
 && grep -q '\[clang-diagnostic-error\]' /tmp/wiz8-clang-tidy-plugin/bool_test_invalid.log \
 && WIZ8_SCALAR_FACTS_DIR=/tmp/wiz8-clang-tidy-plugin/scalar-facts clang-tidy \
      --checks=-*,wiz8-bool-like-byte,wiz8-scalar-facts \
      /tmp/wiz8-clang-tidy-plugin/scalar_test.cpp \
      /tmp/wiz8-clang-tidy-plugin/scalar_test_second.cpp \
      -- --target=i686-pc-windows-msvc -std=c++17 \
 && python3 /tmp/wiz8-clang-tidy-plugin/scalar_test.py \
      /tmp/wiz8-clang-tidy-plugin/scalar-facts \
 && rm -rf /tmp/wiz8-clang-tidy-plugin \
 && find /usr/lib/llvm-21/lib -maxdepth 1 -type f -name '*.a' -delete

ENTRYPOINT ["/usr/local/bin/wiz8-wine"]
