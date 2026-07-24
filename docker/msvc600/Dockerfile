FROM debian:bookworm-slim

ARG MSVC_REPOSITORY=https://github.com/archaic-msvc/msvc600_sp5.git
ARG MSVC_REF=b0b07e29108e2695eb0274c2a377a7b7d7326150

RUN dpkg --add-architecture i386 \
 && apt-get update \
 && apt-get install -y --no-install-recommends \
      ca-certificates \
      git \
      python3 \
      wine \
      wine32 \
 && rm -rf /var/lib/apt/lists/*

ENV WINEARCH=win32 \
    WINEDEBUG=-all \
    WINEPREFIX=/root/.wine

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

COPY entrypoint.py /entrypoint.py

ENTRYPOINT ["python3", "/entrypoint.py"]
