#!/usr/bin/env bash
set -euo pipefail
project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
# Keep binaries outside the public source tree and remove only this newly created temporary directory.
test_dir="$(mktemp -d)"
trap 'rm -rf -- "$test_dir"' EXIT
compiler="${CXX:-g++}"
json_headers="$project_dir/../libraries/ArduinoJson/src"
# Power tests use the real model/renderer and a recording display, with no Arduino runtime.
"$compiler" -std=c++17 -Wall -Wextra -Werror "$project_dir/tests/test_power.cpp" -o "$test_dir/power"
"$test_dir/power"
# Parse real ArduinoJson data while injecting a deterministic color resolver.
"$compiler" -std=c++17 -Wall -Wextra -Werror -I "$json_headers" "$project_dir/tests/test_power_colors.cpp" -o "$test_dir/colors"
"$test_dir/colors"
# Ping tests mock only platform/network boundaries; no packets or serial ports are used.
"$compiler" -std=c++17 -Wall -Wextra -Werror -I "$project_dir/tests/stubs" -I "$json_headers" "$project_dir/tests/test_ping.cpp" -o "$test_dir/ping"
"$test_dir/ping"
