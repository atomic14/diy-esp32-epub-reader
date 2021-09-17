#!/bin/bash

python3 -m venv venv
. ./venv/bin/activate
pip install --upgrade pip
pip install freetype-py

python3 fontconvert.py regular_font 18 ~/Downloads/Lato-Regular.ttf  --compress > ../lib/Fonts/regular_font.h
python3 fontconvert.py bold_font 18 ~/Downloads/Lato-Bold.ttf  --compress > ../lib/Fonts/bold_font.h
python3 fontconvert.py italic_font 18 ~/Downloads/Lato-Italic.ttf  --compress > ../lib/Fonts/italic_font.h
python3 fontconvert.py bold_italic_font 18 ~/Downloads/Lato-BoldItalic.ttf  --compress > ../lib/Fonts/bold_italic_font.h