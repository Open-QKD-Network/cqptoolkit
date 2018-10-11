#!/bin/bash
pandoc paper.md --pdf-engine=xelatex --variable urlcolor=cyan --filter pandoc-citeproc  -o paper.pdf
