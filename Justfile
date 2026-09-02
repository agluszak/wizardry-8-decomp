set dotenv-load := true
set shell := ["bash", "-eu", "-o", "pipefail", "-c"]

default:
    @just --list

prepare:
    uv run wiz8 prepare

test *args:
    uv run wiz8 analyze source-index
    uv run pytest -q tests/unit tests/repository {{args}}

check:
    uv run wiz8 check

build-lint-image:
    uv run wiz8 toolchain build vc6-sp5

lint:
    uv run wiz8 lint

diagnostics:
    uv run wiz8 diagnostics

build *args:
    uv run wiz8 build {{args}}

run:
    uv run wiz8 run

runtime-test:
    uv run wiz8 runtime-test

lifecycle-fixture:
    uv run wiz8 recover self-test

recover address *args:
    uv run wiz8 recover function {{address}} {{args}}

compare *args:
    uv run wiz8 compare {{args}}

vtable *args:
    uv run wiz8 vtable {{args}}

datacmp *args:
    uv run wiz8 datacmp {{args}}

addr *args:
    uv run wiz8 addr {{args}}

verify *args:
    uv run wiz8 verify {{args}}

context selector *args:
    uv run wiz8 report context {{selector}} {{args}}

recover-explain selector *args:
    uv run wiz8 recover explain {{selector}} {{args}}

wiz8 *args:
    uv run wiz8 {{args}}
