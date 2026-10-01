#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SKETCH="$SCRIPT_DIR"
BUILD="$SCRIPT_DIR/build_out"
FQBN="esp32:esp32:esp32"
DEFAULT_PORT="/dev/serial/by-id/usb-1a86_USB_Serial-if00-port0"
CONBEE_HINT="ConBee_II"
UPLOAD_LOCK="/tmp/cyd_dash001_upload.lock"

usage() {
    cat <<EOF
Usage:
  ./upload.sh [port]

Default ESP32 port:
  $DEFAULT_PORT

Safety:
  Never pass the ConBee II port. Check ports with:
  ls -l /dev/serial/by-id
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage
    exit 0
fi

if ! command -v arduino-cli >/dev/null 2>&1; then
    echo "[ERROR] arduino-cli not found in PATH." >&2
    exit 1
fi

PORT="${1:-$DEFAULT_PORT}"

if [[ "$PORT" == *"$CONBEE_HINT"* || "$PORT" == *"dresden"* || "$PORT" == "/dev/ttyACM0" ]]; then
    echo "[ERROR] Refusing to upload to the ConBee II port: $PORT" >&2
    exit 1
fi

if [[ ! -e "$PORT" ]]; then
    echo "[ERROR] Port not found: $PORT" >&2
    echo
    usage
    exit 1
fi

if [[ ! -d "$BUILD" ]]; then
    echo "[ERROR] Build folder not found: $BUILD" >&2
    echo "Run ./build.sh first." >&2
    exit 1
fi

echo
echo "Port : $PORT"
echo "Build: $BUILD"
echo "FQBN : $FQBN"
echo

cleanup_lock() {
    rm -f "$UPLOAD_LOCK"
}
trap cleanup_lock EXIT
printf '%s\n' "$$" > "$UPLOAD_LOCK"
# Give the serial monitor a short window to notice the lock and close the port.
sleep 1

arduino-cli upload \
    -p "$PORT" \
    --fqbn "$FQBN" \
    --build-path "$BUILD" \
    "$SKETCH"

echo
echo "Upload done."
