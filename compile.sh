#!/bin/bash
# Compile ESPHome with memory-safe settings for WSL
# Limits SCons parallel jobs to prevent OOM kills
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Limit PlatformIO/SCons to 2 parallel compile jobs
# Default is nproc (8 on this system), each cc1plus uses ~300MB → OOM
export PLATFORMIO_RUN_JOBS=2

source .venv/bin/activate

echo "Compiling with max 2 parallel jobs (PLATFORMIO_RUN_JOBS=2)..."
esphome compile "${1:-config/esp32-mqtt-nibe.yaml}" "${@:2}"
