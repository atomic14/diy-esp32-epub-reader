#!python3
import freetype
import zlib
import sys
import re
import math
import argparse
from collections import namedtuple

parser = argparse.ArgumentParser(
    description="Generate a header file from a font to be used with epdiy."
)
parser.add_argument("name", action="store", help="name of the font.")
parser.add_argument("size", type=int, help="font size to use.")
parser.add_argument(
    "fontstack",
    action="store",
    nargs="+",
    help="list of font files, ordered by descending priority.",
)
parser.add_argument(
    "--compress", dest="compress", action="store_true", help="compress glyph bitmaps."
)
parser.add_argument(
    "--additional-intervals",
    dest="additional_intervals",
    action="append",
    help="Additional code point intervals to export as min,max. This argument can be repeated.",
)
parser.add_argument(
    "--two-color",
    dest="two_color",
    action="store_true",
    help="Ouput the bitmaps with only two colors - this lets you update with MODE_DU",
)
args = parser.parse_args()

GlyphProps = namedtuple(
    "GlyphProps",
    [
        "width",
        "height",
        "advance_x",
        "left",
        "top",
        "compressed_size",
        "data_offset",
        "code_point",
    ],
)

font_stack = [freetype.Face(f) for f in args.fontstack]
compress = args.compress
size = args.size
font_name = args.name
two_color = args.two_color

# inclusive unicode code point intervals
# must not overlap and be in ascending order
intervals = [
    (0x20, 0x7E),
    (0xA0, 0x17E),
    (0x180, 0x180),
    (0x18F, 0x18F),
    (0x192, 0x193),
    (0x1A0, 0x1A1),
    (0x1AF, 0x1B0),
    (0x1C2, 0x1C3),
    (0x1CD, 0x1DC),
    (0x1E2, 0x1E3),
    (0x1E6, 0x1E7),
    (0x1EA, 0x1EB),
    (0x1F4, 0x1F5),
    (0x1F8, 0x1F9),
    (0x1FC, 0x1FD),
    (0x218, 0x21B),
    (0x237, 0x237),
    (0x243, 0x243),
    (0x250, 0x25C),
    (0x25E, 0x27B),
    (0x27D, 0x27E),
    (0x280, 0x284),
    (0x288, 0x292),
    (0x294, 0x295),
    (0x298, 0x299),
    (0x29C, 0x29D),
    (0x29F, 0x29F),
    (0x2A1, 0x2A2),
    (0x2A4, 0x2A4),
    (0x2A7, 0x2A7),
    (0x400, 0x486),  # https://en.wikipedia.org/wiki/List_of_Unicode_characters#Cyrillic
    (0x489, 0x4FF),  # https://en.wikipedia.org/wiki/List_of_Unicode_characters#Cyrillic
    (0x1E02, 0x1E03),
    (0x1E06, 0x1E07),
    (0x1E0A, 0x1E0F),
    (0x1E16, 0x1E17),
    (0x1E1E, 0x1E21),
    (0x1E24, 0x1E25),
    (0x1E2A, 0x1E2B),
    (0x1E32, 0x1E3B),
    (0x1E3E, 0x1E49),
    (0x1E52, 0x1E53),
    (0x1E56, 0x1E63),
    (0x1E6A, 0x1E6F),
    (0x1E80, 0x1E85),
    (0x1E8E, 0x1E8F),
    (0x1E92, 0x1E97),
    (0x1E9E, 0x1E9E),
    (0x1EA0, 0x1EF9),
    (0x2007, 0x2007),
    (0x2009, 0x200B),
    (0x2010, 0x2016),
    (0x2018, 0x201A),
    (0x201C, 0x201E),
    (0x2020, 0x2022),
    (0x2026, 0x2026),
    (0x202F, 0x2030),
    (0x2032, 0x2033),
    (0x2035, 0x2035),
    (0x2039, 0x203A),
    (0x203C, 0x203F),
    (0x2044, 0x2044),
    (0x2047, 0x2049),
    (0x2070, 0x2071),
    (0x2074, 0x208E),
    (0x2094, 0x2094),
    (0x20A1, 0x20A1),
    (0x20A4, 0x20A4),
    (0x20A6, 0x20A7),
    (0x20A9, 0x20A9),
    (0x20AB, 0x20AC),
    (0x20AE, 0x20AE),
    (0x20B1, 0x20B2),
    (0x20B4, 0x20B5),
    (0x20B8, 0x20BA),
    (0x20BD, 0x20BD),
    (0x2E22, 0x2E25),
    (0x2E3A, 0x2E3B),
    (0xA7B5, 0xA7B5),
]

add_ints = []
if args.additional_intervals:
    add_ints = [
        tuple([int(n, base=0) for n in i.split(",")]) for i in args.additional_intervals
    ]

intervals = sorted(intervals + add_ints)


def norm_floor(val):
    return int(math.floor(val / (1 << 6)))


def norm_ceil(val):
    return int(math.ceil(val / (1 << 6)))


for face in font_stack:
    # shift by 6 bytes, because sizes are given as 6-bit fractions
    # the display has about 150 dpi.
    face.set_char_size(size << 6, size << 6, 150, 150)


def chunks(l, n):
    for i in range(0, len(l), n):
        yield l[i : i + n]


total_size = 0
total_packed = 0
all_glyphs = []


def load_glyph(code_point):
    face_index = 0
    while face_index < len(font_stack):
        face = font_stack[face_index]
        glyph_index = face.get_char_index(code_point)
        if glyph_index > 0:
            face.load_glyph(glyph_index, freetype.FT_LOAD_RENDER)
            return face
            break
        face_index += 1
        print(
            f"falling back to font {face_index} for {chr(code_point)}.", file=sys.stderr
        )
    raise ValueError(f"code point {code_point:#04x} not found in font stack!")


for i_start, i_end in intervals:
    for code_point in range(i_start, i_end + 1):
        face = load_glyph(code_point)
        bitmap = face.glyph.bitmap
        pixels = []
        px = 0
        for i, v in enumerate(bitmap.buffer):
            y = i / bitmap.width
            x = i % bitmap.width
            if x % 2 == 0:
                if two_color:
                    px = 0xF if v > 128 else 0
                else:
                    px = v >> 4
            else:
                if two_color:
                    px = px | (0xF0 if v > 128 else 0)
                else:
                    px = px | (v & 0xF0)
                pixels.append(px)
                px = 0
            # eol
            if x == bitmap.width - 1 and bitmap.width % 2 > 0:
                pixels.append(px)
                px = 0

        packed = bytes(pixels)
        total_packed += len(packed)
        compressed = packed
        if compress:
            compressed = zlib.compress(packed)

        glyph = GlyphProps(
            width=bitmap.width,
            height=bitmap.rows,
            advance_x=norm_floor(face.glyph.advance.x),
            left=face.glyph.bitmap_left,
            top=face.glyph.bitmap_top,
            compressed_size=len(compressed),
            data_offset=total_size,
            code_point=code_point,
        )
        total_size += len(compressed)
        all_glyphs.append((glyph, compressed))

# pipe seems to be a good heuristic for the "real" descender
face = load_glyph(ord("|"))

glyph_data = []
glyph_props = []
for index, glyph in enumerate(all_glyphs):
    props, compressed = glyph
    glyph_data.extend([b for b in compressed])
    glyph_props.append(props)

print("total", total_packed, file=sys.stderr)
print("compressed", total_size, file=sys.stderr)

print("#pragma once")
print('#include "epd_driver.h"')
print(f"const uint8_t {font_name}Bitmaps[{len(glyph_data)}] = {{")
for c in chunks(glyph_data, 16):
    print("    " + " ".join(f"0x{b:02X}," for b in c))
print("};")

print(f"const EpdGlyph {font_name}Glyphs[] = {{")
for i, g in enumerate(glyph_props):
    print(
        "    { " + ", ".join([f"{a}" for a in list(g[:-1])]),
        "},",
        f"// {chr(g.code_point) if g.code_point != 92 else '<backslash>'}",
    )
print("};")

print(f"const EpdUnicodeInterval {font_name}Intervals[] = {{")
offset = 0
for i_start, i_end in intervals:
    print(f"    {{ 0x{i_start:X}, 0x{i_end:X}, 0x{offset:X} }},")
    offset += i_end - i_start + 1
print("};")

print(f"const EpdFont {font_name} = {{")
print(f"    {font_name}Bitmaps,")
print(f"    {font_name}Glyphs,")
print(f"    {font_name}Intervals,")
print(f"    {len(intervals)},")
print(f"    {1 if compress else 0},")
print(f"    {norm_ceil(face.size.height)},")
print(f"    {norm_ceil(face.size.ascender)},")
print(f"    {norm_floor(face.size.descender)},")
print("};")
