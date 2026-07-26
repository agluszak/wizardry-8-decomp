set dotenv-load := true
set shell := ["bash", "-eu", "-o", "pipefail", "-c"]

repo := justfile_directory()
vc6_image := "wizardry8-msvc600:sp5"
jpeg_target := "SREXT_JPEGIMPORTER"

default:
    @just --list

# Run the Python test suite directly through uv.
test *args:
    uv run pytest {{args}}

# Build the active recovered binary through its real CMake graph.
build target=jpeg_target: configure
    docker run --rm --network none \
        --volume "{{repo}}:/repo:ro" \
        --volume "$WIZ8_WORK_DIR/fid/sources/unpacked/ijg-jpeg-6/jpeg-6:/jpeg:ro" \
        --volume "$WIZ8_WORK_DIR/fid/sources/unpacked/zlib-1.0.4/zlib-1.0.4:/zlib:ro" \
        --volume "$WIZ8_WORK_DIR/fid/sources/unpacked/infozip-unzip-5.4:/infozip:ro" \
        --volume "{{repo}}/build/decomp:/out" \
        {{vc6_image}} \
        cmd /c "set TEMP=Z:\out\tmp&& set TMP=Z:\out\tmp&& C:\cmake\bin\cmake.exe --build Z:/out --target {{target}}"

# Refuse a build directory configured by a different checkout. Two checkouts
# sharing WIZ8_WORK_DIR overwrite each other's CMake cache and linked
# Wiz8.exe/Wiz8.pdb, and the symptom is a wrong comparison rather than an error.
_check-build-dir:
    @output=$(uv run wiz8 check-build-dir) || { printf '%s\n' "$output" >&2; exit 1; }

# Configure the active VC6 CMake build and reccmp's machine-local original path.
configure: _jpeg-sources _check-build-dir
    mkdir -p "{{repo}}/build/decomp/tmp"
    uv run reccmp-project detect \
        --search-path "$WIZ8_WORK_DIR/variants/gog-base" \
                      "$WIZ8_WORK_DIR/variants/gog-base/Dll" \
        --what original
    docker run --rm --network none \
        --volume "{{repo}}:/repo:ro" \
        --volume "$WIZ8_WORK_DIR/fid/sources/unpacked/ijg-jpeg-6/jpeg-6:/jpeg:ro" \
        --volume "$WIZ8_WORK_DIR/fid/sources/unpacked/zlib-1.0.4/zlib-1.0.4:/zlib:ro" \
        --volume "$WIZ8_WORK_DIR/fid/sources/unpacked/infozip-unzip-5.4:/infozip:ro" \
        --volume "{{repo}}/build/decomp:/out" \
        {{vc6_image}} \
        'C:\cmake\bin\cmake.exe' \
        -S Z:/repo -B Z:/out -G 'NMake Makefiles' \
        -DIJG_JPEG_SOURCE=Z:/jpeg \
        -DZLIB_SOURCE=Z:/zlib \
        -DINFOZIP_SOURCE=Z:/infozip \
        -DSGP_SOURCE=Z:/repo/third_party/sfi-sgp/sgp \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DRECCMP_PROJECT_DIR_HOST={{repo}} \
        -DRECCMP_BUILD_DIR_HOST="{{repo}}/build/decomp"

# Compare through reccmp itself; extra arguments are passed unchanged.
compare target=jpeg_target *args: _check-build-dir
    cd "{{repo}}/build/decomp" && \
        uv run --project {{repo}} reccmp-reccmp \
        --target {{target}} --no-color {{args}}

# Check reviewed bodies against the original by relocation-masked comparison.
# This is the matching criterion; `compare` measures the linked image, where our
# globals sit at other addresses, and scores byte-exact bodies well under 100%.
verify-boundaries *args: _check-build-dir
    uv run --project {{repo}} wiz8 verify-boundaries {{args}}

# Build the pinned VC6 SP5 image used by the active matching target.
build-image:
    docker build --pull --network host \
        --build-arg MSVC_REPOSITORY=https://github.com/archaic-msvc/msvc600_sp5.git \
        --build-arg MSVC_REF=b0b07e29108e2695eb0274c2a377a7b7d7326150 \
        --tag {{vc6_image}} docker/msvc600

# Forward Ghidra-specific arguments to the domain CLI.
[positional-arguments]
ghidra *args:
    uv run wiz8 ghidra "$@"

# Forward arbitrary corpus/analysis arguments to the domain CLI.
wiz8 *args:
    uv run wiz8 {{args}}

# Prepare verified source trees used by CMake and FID.
_jpeg-sources:
    uv run wiz8 ghidra fid fetch-sources
