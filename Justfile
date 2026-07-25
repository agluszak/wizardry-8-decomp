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
        --volume "$WIZ8_WORK_DIR/decomp/srext-jpegimporter:/out" \
        {{vc6_image}} \
        'C:\cmake\bin\cmake.exe' --build Z:/out --target {{target}}

# Configure the active VC6 CMake build and reccmp's machine-local original path.
configure: _jpeg-sources
    mkdir -p "$WIZ8_WORK_DIR/decomp/srext-jpegimporter/tmp"
    uv run reccmp-project detect \
        --search-path "$WIZ8_WORK_DIR/variants/gog-base/Dll" \
        --what original
    docker run --rm --network none \
        --volume "{{repo}}:/repo:ro" \
        --volume "$WIZ8_WORK_DIR/fid/sources/unpacked/ijg-jpeg-6/jpeg-6:/jpeg:ro" \
        --volume "$WIZ8_WORK_DIR/decomp/srext-jpegimporter:/out" \
        {{vc6_image}} \
        'C:\cmake\bin\cmake.exe' \
        -S Z:/repo -B Z:/out -G 'NMake Makefiles' \
        -DIJG_JPEG_SOURCE=Z:/jpeg \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DRECCMP_PROJECT_DIR_HOST={{repo}} \
        -DRECCMP_BUILD_DIR_HOST="$WIZ8_WORK_DIR/decomp/srext-jpegimporter"

# Compare through reccmp itself; extra arguments are passed unchanged.
compare *args:
    cd "$WIZ8_WORK_DIR/decomp/srext-jpegimporter" && \
        uv run --project {{repo}} reccmp-reccmp \
        --target {{jpeg_target}} --no-color {{args}}

# Build the pinned VC6 SP5 image used by the active matching target.
build-image:
    docker build --pull --network host \
        --build-arg MSVC_REPOSITORY=https://github.com/archaic-msvc/msvc600_sp5.git \
        --build-arg MSVC_REF=b0b07e29108e2695eb0274c2a377a7b7d7326150 \
        --tag {{vc6_image}} docker/msvc600

# Forward Ghidra-specific arguments to the domain CLI.
ghidra *args:
    uv run wiz8 ghidra {{args}}

# Forward arbitrary corpus/analysis arguments to the domain CLI.
wiz8 *args:
    uv run wiz8 {{args}}

# Prepare verified source trees used by CMake and FID.
_jpeg-sources:
    uv run wiz8 ghidra fid fetch-sources
