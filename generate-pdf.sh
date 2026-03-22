#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

pandoc tutorial.md \
  -o tutorial.pdf \
  --pdf-engine=xelatex \
  -V geometry:margin=2.5cm \
  -V fontsize=11pt \
  -V colorlinks=true \
  -V linkcolor=blue \
  -V urlcolor=blue \
  -V monofont="DejaVu Sans Mono" \
  --highlight-style=tango \
  --toc \
  --toc-depth=3 \
  -V toc-title="Table of Contents"

echo "Generated tutorial.pdf"
