#!/bin/bash

python3 -m venv venv
. ./venv/bin/activate
pip install --upgrade pip
pip install pillow

python3 imgconvert.py
