#!/usr/bin/env bash
set -euo pipefail
# No compiler or library modifications: only the sketch-local profile files.
script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
exec python3 "$script_dir/select_board.py" "$@"
