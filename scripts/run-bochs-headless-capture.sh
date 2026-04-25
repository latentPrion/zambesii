#!/bin/bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BOCHS_BIN="${BOCHS_BIN:-/home/latentprion/bin/bin/bochs}"
SRC_CFG="${BOCHS_CFG:-$ROOT_DIR/bochsrc.txt}"
TMP_CFG="/tmp/zambesii-bochsrc.nogui.txt"
SERIAL_OUT="${BOCHS_SERIAL_OUT:-$ROOT_DIR/b/bochs.out.tmp}"
EMU_OUT="${BOCHS_EMU_OUT:-$ROOT_DIR/b/bochs.emu.out.tmp}"
SECONDS_TO_RUN="${BOCHS_SECONDS:-15}"
CPU_MODEL_OVERRIDE="${BOCHS_CPU_MODEL:-p4_willamette}"
ROMIMAGE_OVERRIDE="${BOCHS_ROMIMAGE:-}"

mkdir -p "$(dirname "$SERIAL_OUT")"
mkdir -p "$(dirname "$EMU_OUT")"

python3 - "$SRC_CFG" "$TMP_CFG" "$SERIAL_OUT" "$CPU_MODEL_OVERRIDE" "$ROMIMAGE_OVERRIDE" <<'PY'
from pathlib import Path
import sys

src_cfg = Path(sys.argv[1]).read_text()
tmp_cfg = Path(sys.argv[2])
serial_out = sys.argv[3]
cpu_model_override = sys.argv[4]
romimage_override = sys.argv[5]

lines = []
for line in src_cfg.splitlines():
    stripped = line.strip()

    if stripped.startswith("display_library:"):
        lines.append("display_library: nogui")
        continue

    if stripped.startswith("com1:"):
        lines.append(f'com1: enabled=true, mode=file, dev="{serial_out}"')
        continue

    if cpu_model_override and stripped.startswith("cpu:"):
        parts = [part.strip() for part in stripped.split(",")]
        parts[0] = parts[0].split("model=")[0].rstrip()
        rewritten = []
        model_replaced = False
        for part in parts:
            if part.startswith("cpu:"):
                rewritten.append(part)
                continue
            if part.startswith("model="):
                rewritten.append(f"model={cpu_model_override}")
                model_replaced = True
            else:
                rewritten.append(part)
        if not model_replaced:
            rewritten.append(f"model={cpu_model_override}")
        lines.append(", ".join(rewritten))
        continue

    if romimage_override and stripped.startswith("romimage:"):
        lines.append(f'romimage: file="{romimage_override}"')
        continue

    lines.append(line)

tmp_cfg.write_text("\n".join(lines) + "\n")
PY

: > "$SERIAL_OUT"
: > "$EMU_OUT"

cd "$ROOT_DIR"
timeout "${SECONDS_TO_RUN}s" "$BOCHS_BIN" -q -f "$TMP_CFG" >"$EMU_OUT" 2>&1 || true

echo "Bochs emulator log: $EMU_OUT"
echo "Bochs serial log:   $SERIAL_OUT"
