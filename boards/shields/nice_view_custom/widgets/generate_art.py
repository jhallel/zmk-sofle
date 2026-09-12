#!/usr/bin/env python3
"""Generate the final 75-frame 160x68 LVGL 1-bit Evangelion reel for nice!view.

The approved 57-frame reel stays in eva_payload_1..6 unchanged.
A separate 18-frame polished EMERGENCY sequence is stored in
eva_warning_payload.txt and spliced immediately before the original
24-frame EVA-01 awakening.
"""
from pathlib import Path
import base64
import sys
import zlib

WIDTH = 160
HEIGHT = 68
ROW_BYTES = WIDTH // 8
FRAME_BYTES = ROW_BYTES * HEIGHT
BASE_FRAME_COUNT = 57
WARNING_FRAME_COUNT = 18
FRAME_COUNT = BASE_FRAME_COUNT + WARNING_FRAME_COUNT
WARNING_INSERT_INDEX = 31  # before original frame 32

BASE_PAYLOAD = "".join(
    Path(__file__).with_name(f"eva_payload_{i}.txt").read_text().strip()
    for i in range(1, 7)
)
WARNING_PAYLOAD = Path(__file__).with_name("eva_warning_payload.txt").read_text().strip()


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: generate_art.py OUTPUT_C")

    out_path = Path(sys.argv[1])
    base_raw = zlib.decompress(base64.b85decode(BASE_PAYLOAD.encode("ascii")))
    warning_raw = zlib.decompress(base64.b85decode(WARNING_PAYLOAD.encode("ascii")))

    expected_base = BASE_FRAME_COUNT * FRAME_BYTES
    expected_warning = WARNING_FRAME_COUNT * FRAME_BYTES
    if len(base_raw) != expected_base:
        raise SystemExit(f"bad base payload size: {len(base_raw)} != {expected_base}")
    if len(warning_raw) != expected_warning:
        raise SystemExit(f"bad warning payload size: {len(warning_raw)} != {expected_warning}")

    cut = WARNING_INSERT_INDEX * FRAME_BYTES
    raw = base_raw[:cut] + warning_raw + base_raw[cut:]
    expected = FRAME_COUNT * FRAME_BYTES
    if len(raw) != expected:
        raise SystemExit(f"bad final payload size: {len(raw)} != {expected}")

    lines = [
        "#include <lvgl.h>",
        "",
        "#ifndef LV_ATTRIBUTE_MEM_ALIGN",
        "#define LV_ATTRIBUTE_MEM_ALIGN",
        "#endif",
        "",
    ]

    for idx in range(FRAME_COUNT):
        name = f"eva{idx + 1:02d}"
        start = idx * FRAME_BYTES
        data = raw[start:start + FRAME_BYTES]
        lines += [
            f"const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST uint8_t {name}_map[] = {{",
            "#if CONFIG_NICE_VIEW_WIDGET_INVERTED",
            "    0xff, 0xff, 0xff, 0xff,",
            "    0x00, 0x00, 0x00, 0xff,",
            "#else",
            "    0x00, 0x00, 0x00, 0xff,",
            "    0xff, 0xff, 0xff, 0xff,",
            "#endif",
        ]
        for off in range(0, len(data), ROW_BYTES):
            chunk = data[off:off + ROW_BYTES]
            lines.append("    " + ", ".join(f"0x{b:02x}" for b in chunk) + ",")
        lines += [
            "};",
            "",
            f"const lv_img_dsc_t {name} = {{",
            "    .header.cf = LV_IMG_CF_INDEXED_1BIT,",
            "    .header.always_zero = 0,",
            "    .header.reserved = 0,",
            f"    .header.w = {WIDTH},",
            f"    .header.h = {HEIGHT},",
            f"    .data_size = {FRAME_BYTES + 8},",
            f"    .data = {name}_map,",
            "};",
            "",
        ]

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text("\n".join(lines))


if __name__ == "__main__":
    main()
