#!/usr/bin/env python3
"""Generate 32 final 160x68 LVGL 1-bit Evangelion frames for nice!view.

The payload is produced from the approved 8x4 storyboard by detecting the
actual grid borders first. The source grid has non-uniform row/column sizes,
so frames are stored here already cropped/resized to their final framebuffer
size; no runtime row/column slicing or stretch step can bleed adjacent cells.
"""
from pathlib import Path
import base64
import sys
import zlib

WIDTH = 160
HEIGHT = 68
ROW_BYTES = WIDTH // 8
FRAME_BYTES = ROW_BYTES * HEIGHT
FRAME_COUNT = 32

PAYLOAD = "".join(
    (Path(__file__).with_name(f"eva_payload_{i}.txt").read_text().strip())
    for i in range(1, 7)
)


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: generate_art.py OUTPUT_C")

    out_path = Path(sys.argv[1])
    raw = zlib.decompress(base64.b85decode(PAYLOAD.encode("ascii")))
    expected = FRAME_COUNT * FRAME_BYTES
    if len(raw) != expected:
        raise SystemExit(f"bad payload size: {len(raw)} != {expected}")

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
