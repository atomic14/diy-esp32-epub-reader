import freetype
import argparse

parser = argparse.ArgumentParser(
    description="Output the charactare intervals from a font"
)
parser.add_argument("font", help="The font to extract the intervals from")
args = parser.parse_args()

face = freetype.Face(args.font)

start = -1
end = -1
for c in range(0, 65536):
    glyph_index = face.get_char_index(c)
    success = False
    if glyph_index != 0:
        try:
            face.load_glyph(glyph_index, freetype.FT_LOAD_DEFAULT)
            success = True
        except:
            pass
    if success:
        if start == -1:
            start = c
        end = c
    else:
        if start != -1:
            print("(0x{:x},0x{:x}),".format(start, end))
            start = -1
            end = -1
