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
    log="$(mktemp)"
    trap 'rm -f "$log"' EXIT
    # Wine's emergency printer cannot unwind the PE-header stub that
    # /FORCE:UNRESOLVED jumps into, so keep the process output and ask the
    # MAP symbolizer for the consumed return address after the process exits.
    set +e
    wine ./Wiz8Runtime.exe /WINDOW "$@" 2>&1 | tee "$log"
    status=${PIPESTATUS[0]}
    set -e
    # The in-process reporter writes the product markers, while a hard crash
    # leaves Wine's own report; winedbg sessions expose the Registers section.
    # Only symbolize one of those, never an empty log.
    if grep -qiE "WIZ8_RUNTIME_CRASH|Unhandled exception|Unhandled page fault|Register dump:" "$log"; then
        (
            cd "{{justfile_directory()}}"
            uv run wiz8 analyze crash --log "$log" \
                --map "$PWD/build/decomp/Wiz8Runtime.map" || true
        )
    fi
    exit "$status"

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
    # WineDbg consumes stdin one command per stop.  `cont` reaches the
    # exception, and the queued commands then run at that stop before `quit`
    # ends the session: Wine's own exception dump can be cut short when the
    # debugger is told to leave immediately, so ask for the register file,
    # the frame walk and enough raw stack words to recover return addresses
    # explicitly.  `info reg`/`bt`/`info stack` are WineDbg commands, not GDB
    # spellings; `info stack 96` prints the same `Stack dump:` format Wine's
    # automatic dump uses.
    script="${DEBUG_SCRIPT:-$'cont\ninfo reg\nbt\ninfo stack 96\nquit\n'}"
    log="$(mktemp)"
    trap 'rm -f "$log"' EXIT
    set +e
    printf '%s' "$script" | winedbg ./Wiz8Runtime.exe /WINDOW "$@" 2>&1 | tee "$log"
    # The pipeline is printf | winedbg | tee, so index 1 owns the debugger's
    # status; PIPESTATUS[0] only reports printf.
    status=${PIPESTATUS[1]}
    set -e
    # winedbg stops print "Exception c0000005" and only a DEBUG_SCRIPT that
    # asks for state leaves a Register dump; Wine's own hard-crash report says
    # "Unhandled page fault"/"Unhandled exception", and the product reporter
    # writes WIZ8_RUNTIME_CRASH. Symbolize any of those.
    if grep -qiE "WIZ8_RUNTIME_CRASH|Unhandled exception|Unhandled page fault|Register dump:" "$log"; then
        (
            cd "{{justfile_directory()}}"
            uv run wiz8 analyze crash --log "$log" \
                --map "$PWD/build/decomp/Wiz8Runtime.map" \
                --objects "$PWD/build/decomp/CMakeFiles/wiz8_recovered_objects.dir" || true
        )
    fi
    exit "$status"

runtime-test:
    uv run wiz8 runtime-test

recover address *args:
    uv run wiz8 recover function {{address}} {{args}}

compare *args:
    uv run wiz8 compare {{args}}

wiz8 *args:
    uv run wiz8 {{args}}
