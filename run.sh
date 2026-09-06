#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
if [[ ! -d .venv ]]; then
  python3 -m venv .venv
  .venv/bin/pip install -r requirements.txt
fi
# Prefer pulse/dummy if no hardware audio (CI / xvfb)
if [[ -z "${SDL_AUDIODRIVER:-}" ]]; then
  export SDL_AUDIODRIVER="${SDL_AUDIODRIVER:-pulse}"
fi
exec .venv/bin/python -m cinder "$@"
