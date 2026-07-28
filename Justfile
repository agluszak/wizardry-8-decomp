set dotenv-load := true
set shell := ["bash", "-eu", "-o", "pipefail", "-c"]

default:
    @just --list

prepare:
    uv run wiz8 prepare

test *args:
    uv run pytest {{args}}

check:
    uv run wiz8 check

build target="match" *args:
    uv run wiz8 build {{target}} {{args}}

run:
    uv run wiz8 run

compare target="WIZ8" *args:
    uv run wiz8 compare {{target}} {{args}}

verify *args:
    uv run wiz8 verify {{args}}

[positional-arguments]
ghidra *args:
    uv run wiz8 ghidra "$@"

wiz8 *args:
    uv run wiz8 {{args}}
