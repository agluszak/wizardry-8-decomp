set dotenv-load := true
set positional-arguments := true

default:
    @just --list

check:
    uv run wiz8 check

build *args:
    uv run wiz8 build {{args}}

run *args:
    #!/usr/bin/env bash
    set -euo pipefail
    : "${WIZ8_WORK_DIR:?Set WIZ8_WORK_DIR in .env or the environment.}"
    game_dir="$WIZ8_WORK_DIR/variants/gog-base"
    exe="{{justfile_directory()}}/build/decomp/Wiz8Runtime.exe"
    if [[ ! -f "$exe" ]]; then
        echo "Missing $exe — run 'just build runtime' first." >&2
        exit 1
    fi
    if [[ ! -d "$game_dir/Data" ]]; then
        echo "Missing retail game data in $game_dir." >&2
        exit 1
    fi
    if [[ ! -f "$game_dir/3DVideo.CFG" ]]; then
        cp "{{justfile_directory()}}/config/runtime/3DVideo.CFG" "$game_dir/3DVideo.CFG"
    fi
    if [[ ! -f "$game_dir/Wiz8.CFG" ]]; then
        xxd -r -p "{{justfile_directory()}}/config/runtime/Wiz8.CFG.hex" "$game_dir/Wiz8.CFG"
    fi
    runtime_exe="$game_dir/Wiz8Runtime.exe"
    if [[ -e "$runtime_exe" && ! -L "$runtime_exe" ]]; then
        echo "Refusing to replace unmanaged $runtime_exe." >&2
        exit 1
    fi
    ln -sfn "$exe" "$runtime_exe"
    cd "$game_dir"
    exec wine ./Wiz8Runtime.exe /WINDOW "$@"

run-original *args:
    #!/usr/bin/env bash
    set -euo pipefail
    : "${WIZ8_WORK_DIR:?Set WIZ8_WORK_DIR in .env or the environment.}"
    game_dir="$WIZ8_WORK_DIR/variants/gog-base"
    exe="{{justfile_directory()}}/build/decomp/Wiz8Runtime.exe"
    if [[ ! -f "$exe" ]]; then
        echo "Missing $exe — run 'just build runtime' first." >&2
        exit 1
    fi
    if [[ ! -d "$game_dir/Data" ]]; then
        echo "Missing retail game data in $game_dir." >&2
        exit 1
    fi
    if [[ ! -f "$game_dir/3DVideo.CFG" ]]; then
        cp "{{justfile_directory()}}/config/runtime/3DVideo.CFG" "$game_dir/3DVideo.CFG"
    fi
    if [[ ! -f "$game_dir/Wiz8.CFG" ]]; then
        xxd -r -p "{{justfile_directory()}}/config/runtime/Wiz8.CFG.hex" "$game_dir/Wiz8.CFG"
    fi
    runtime_exe="$game_dir/Wiz8Runtime.exe"
    if [[ -e "$runtime_exe" && ! -L "$runtime_exe" ]]; then
        echo "Refusing to replace unmanaged $runtime_exe." >&2
        exit 1
    fi
    ln -sfn "$exe" "$runtime_exe"
    cd "$game_dir"
    exec wine ./Wiz8.exe /WINDOW "$@"

debug *args:
    #!/usr/bin/env bash
    set -euo pipefail
    : "${WIZ8_WORK_DIR:?Set WIZ8_WORK_DIR in .env or the environment.}"
    game_dir="$WIZ8_WORK_DIR/variants/gog-base"
    exe="{{justfile_directory()}}/build/decomp/Wiz8Runtime.exe"
    if [[ ! -f "$exe" ]]; then
        echo "Missing $exe — run 'just build runtime' first." >&2
        exit 1
    fi
    if [[ ! -d "$game_dir/Data" ]]; then
        echo "Missing retail game data in $game_dir." >&2
        exit 1
    fi
    if [[ ! -f "$game_dir/3DVideo.CFG" ]]; then
        cp "{{justfile_directory()}}/config/runtime/3DVideo.CFG" "$game_dir/3DVideo.CFG"
    fi
    if [[ ! -f "$game_dir/Wiz8.CFG" ]]; then
        xxd -r -p "{{justfile_directory()}}/config/runtime/Wiz8.CFG.hex" "$game_dir/Wiz8.CFG"
    fi
    runtime_exe="$game_dir/Wiz8Runtime.exe"
    if [[ -e "$runtime_exe" && ! -L "$runtime_exe" ]]; then
        echo "Refusing to replace unmanaged $runtime_exe." >&2
        exit 1
    fi
    ln -sfn "$exe" "$runtime_exe"
    wineserver -k 2>/dev/null || true
    sleep 1
    cd "$game_dir"
    script="${DEBUG_SCRIPT:-$'cont\nbt\nquit\n'}"
    printf '%s' "$script" | winedbg ./Wiz8Runtime.exe /WINDOW "$@"

runtime-test:
    uv run wiz8 runtime-test

recover address *args:
    uv run wiz8 recover function {{address}} {{args}}

compare *args:
    uv run wiz8 compare {{args}}

wiz8 *args:
    uv run wiz8 {{args}}
