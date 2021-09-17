#!/bin/bash

python3 -m venv venv
. ./venv/bin/activate
pip install --upgrade pip
pip install freetype-py

python3 fontconvert.py regular_font 14 ~/Downloads/Lato-Regular.ttf  --compress > ../src/Fonts/regular_font.h
python3 fontconvert.py bold_font 14 ~/Downloads/Lato-Bold.ttf  --compress > ../src/Fonts/bold_font.h
python3 fontconvert.py italic_font 14 ~/Downloads/Lato-Italic.ttf  --compress > ../src/Fonts/italic_font.h
python3 fontconvert.py bold_italic_font 14 ~/Downloads/Lato-BoldItalic.ttf  --compress > ../src/Fonts/bold_italic_font.h