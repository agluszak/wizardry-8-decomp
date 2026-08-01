set dotenv-load := true
set shell := ["bash", "-eu", "-o", "pipefail", "-c"]

default:
    @just --list

prepare:
    uv run wiz8 prepare

test *args:
    uv run wiz8 analyze source-index
    uv run pytest tests/unit tests/repository {{args}}

check:
    uv run wiz8 check

build-lint-image:
    uv run wiz8 toolchain build vc6-sp5

lint:
    uv run wiz8 lint

diagnostics:
    uv run wiz8 diagnostics

build target="match" *args:
    uv run wiz8 build {{target}} {{args}}

run:
    uv run wiz8 run

runtime-test:
    uv run wiz8 runtime-test

recover address *args:
    uv run wiz8 recover function {{address}} {{args}}

compare *args:
    uv run wiz8 compare {{args}}

triage *args:
    uv run wiz8 triage {{args}}

vtable *args:
    uv run wiz8 vtable {{args}}

datacmp *args:
    uv run wiz8 datacmp {{args}}

addr *args:
    uv run wiz8 addr {{args}}

verify *args:
    uv run wiz8 verify {{args}}

context address program="wiz8" *args:
    uv run wiz8 report context {{address}} --program {{program}} {{args}}

wiz8 *args:
    uv run wiz8 {{args}}
