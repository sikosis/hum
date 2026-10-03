#!/bin/bash
set -e

HUM=${HUM:-hum}

if ! command -v "$HUM" >/dev/null 2>&1; then
    echo "hum was not found. Install it or run with HUM=/path/to/hum." >&2
    exit 1
fi

if [ -t 1 ] && [ "${NO_CLEAR:-0}" != 1 ]; then
    printf '\033[2J\033[H'
fi

title=$($HUM style --foreground '#FF74C8' --bold --align center --width 68 \
    'h u m')
tagline=$($HUM style --foreground '#79E8F2' --italic --align center --width 68 \
    'terminal tools that feel at home on Haiku')

left_panel=$($HUM style --border rounded --border-foreground '#FF74C8' \
    --padding '1 2' --width 29 \
    $'STYLE\ncolours · borders · layout\n\nPROMPT\ninput · choose · confirm')

right_panel=$($HUM style --border rounded --border-foreground '#79E8F2' \
    --padding '1 2' --width 29 \
    $'EXPLORE\nfilter · file · pager\n\nAUTOMATE\nspin · table · log')

printf '%s\n%s\n\n' "$title" "$tagline"
$HUM join --horizontal --align top "$left_panel" '  ' "$right_panel"
printf '\n\n'
$HUM style --foreground '#FFE46B' --bold --align center --width 68 \
    'dependency-free · C++17 · made for Haiku'
printf '\n'
$HUM style --foreground '#8DE2A1' --align center --width 68 \
    'github.com/sikosis/hum'

