#!/usr/bin/env bash
set -euo pipefail
# Resolve paths from the script location so invocation works from another directory.
project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
cd -- "$project_dir"
board="${1:-$(<board.txt)}"
port="${2:-}"
# A profile is the entire hardware contract: numeric macros, MCU and memory options.
# Normalize aliases before choosing artifact paths or writing metadata.
board="${board,,}"
case "$board" in
  esp32_ili9341_xpt2046|amarela2.8)
    board=esp32_ili9341_xpt2046; board_id=1; fqbn='esp32:esp32:esp32' ;;
  esp32_ili9341_cap_touch|pink)
    board=esp32_ili9341_cap_touch; board_id=2; fqbn='esp32:esp32:esp32' ;;
  esp32s3_axs15231b_qspi|black|preta)
    board=esp32s3_axs15231b_qspi; board_id=3; fqbn='esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,CDCOnBoot=cdc,PartitionScheme=app3M_fat9M_16MB' ;;
  esp32_st7789_parallel_cst820|amarela2.2)
    board=esp32_st7789_parallel_cst820; board_id=4; fqbn='esp32:esp32:esp32' ;;
  *) echo 'Unknown profile. Use a documented hardware ID or personal alias.' >&2; exit 1 ;;
esac
# Fail before incrementing counters or producing artifacts when local config is missing.
if [[ ! -f secrets.h ]]; then
  echo 'Copy secrets_template.h to secrets.h and configure it before building.' >&2
  exit 1
fi
cli="${ARD_CLI:-arduino-cli}"
libraries="$(cd -- ../libraries && pwd)"
# Count compile attempts, including failed builds; this is not a device identity.
number=$(( $(<build_nr.txt) + 1 ))
printf '%s\n' "$number" > build_nr.txt
printf '#pragma once\n#define BUILD_NR %s\n' "$number" > build.h
output="$project_dir/build_out/$board"
# Pass the local library root once and force the complete TFT setup into all C++ units.
"$cli" compile --fqbn "$fqbn" --libraries "$libraries" \
  --output-dir "$output" --build-path "$project_dir/build/$board" \
  --build-property "compiler.cpp.extra_flags=-DDASH_BOARD=$board_id -include \"$project_dir/tft_setup.h\"" \
  --build-property "compiler.c.extra_flags=-DDASH_BOARD=$board_id" "$project_dir"
# Write profile metadata only after compile succeeds; shell arguments avoid JSON quoting errors.
python3 - "$output/board-build.json" "$board" "$fqbn" "$number" <<'PY'
import datetime,json,sys
from pathlib import Path
Path(sys.argv[1]).write_text(json.dumps({'Board':sys.argv[2],'Fqbn':sys.argv[3],
 'Build':int(sys.argv[4]),'Date':datetime.datetime.now().astimezone().isoformat()},indent=2)+'\n')
PY
printf '%s\n' "$board" > board.txt
# Upload is opt-in through the explicit second argument. Confirm the physical port/MAC first.
if [[ -n "$port" ]]; then
  "$cli" upload --fqbn "$fqbn" --port "$port" --input-dir "$output" "$project_dir"
fi
