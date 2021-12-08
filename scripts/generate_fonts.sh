#!/bin/bash

python3 -m venv venv
. ./venv/bin/activate
pip install --upgrade pip
pip install freetype-py

curl https://raw.githubusercontent.com/google/fonts/main/ofl/sourcesanspro/SourceSansPro-Regular.ttf -o SourceSansPro-Regular.ttf
curl https://raw.githubusercontent.com/google/fonts/main/ofl/sourcesanspro/SourceSansPro-Bold.ttf -o SourceSansPro-Bold.ttf
curl https://raw.githubusercontent.com/google/fonts/main/ofl/sourcesanspro/SourceSansPro-Italic.ttf -o SourceSansPro-Italic.ttf
curl https://raw.githubusercontent.com/google/fonts/main/ofl/sourcesanspro/SourceSansPro-BoldItalic.ttf -o SourceSansPro-BoldItalic.ttf

curl https://raw.githubusercontent.com/googlefonts/opensans/ebedbda589fe5bd861b02325aca98c86ad845251/fonts/ttf/OpenSans-Regular.ttf -o OpenSans-Regular.ttf
curl https://raw.githubusercontent.com/googlefonts/opensans/ebedbda589fe5bd861b02325aca98c86ad845251/fonts/ttf/OpenSans-Bold.ttf -o OpenSans-Bold.ttf
curl https://raw.githubusercontent.com/googlefonts/opensans/ebedbda589fe5bd861b02325aca98c86ad845251/fonts/ttf/OpenSans-Italic.ttf -o OpenSans-Italic.ttf
curl https://raw.githubusercontent.com/googlefonts/opensans/ebedbda589fe5bd861b02325aca98c86ad845251/fonts/ttf/OpenSans-BoldItalic.ttf -o OpenSans-BoldItalic.ttf


python3 fontconvert.py regular_font 18 SourceSansPro-Regular.ttf OpenSans-Regular.ttf --two-color --compress > ../lib/Fonts/regular_font.h
python3 fontconvert.py bold_font 18 SourceSansPro-Bold.ttf OpenSans-Bold.ttf --two-color --compress > ../lib/Fonts/bold_font.h
python3 fontconvert.py italic_font 18 SourceSansPro-Italic.ttf OpenSans-Italic.ttf --two-color --compress > ../lib/Fonts/italic_font.h
python3 fontconvert.py bold_italic_font 18 SourceSansPro-BoldItalic.ttf OpenSans-BoldItalic.ttf --two-color --compress > ../lib/Fonts/bold_italic_font.h