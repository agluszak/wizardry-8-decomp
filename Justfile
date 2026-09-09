default:
    @just --list

check:
    uv run wiz8 check

build *args:
    uv run wiz8 build {{args}}

run:
    uv run wiz8 run

runtime-test:
    uv run wiz8 runtime-test

recover address *args:
    uv run wiz8 recover function {{address}} {{args}}

compare *args:
    uv run wiz8 compare {{args}}

wiz8 *args:
    uv run wiz8 {{args}}
