#!/bin/bash
pandoc paper.md --pdf-engine=xelatex --filter pandoc-citeproc  -o paper.pdf
