#!/usr/bin/env bash

cd "$(dirname "$0")/.."

if command -v latexmk >/dev/null 2>&1; then
    latexmk -pdf -output-directory=docs docs/document.tex
else
    pdflatex -interaction=nonstopmode -output-directory=docs docs/document.tex
    biber --output-directory docs document
    pdflatex -interaction=nonstopmode -output-directory=docs docs/document.tex
    pdflatex -interaction=nonstopmode -output-directory=docs docs/document.tex
fi
