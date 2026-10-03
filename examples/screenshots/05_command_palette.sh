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

$HUM style --foreground '#79E8F2' --bold --align center --width 70 \
    'COMMAND PALETTE'
$HUM style --foreground '#A9A1B5' --align center --width 70 \
    'type to fuzzy-search · arrows to move · enter to select'
printf '\n'

selection=$(printf '%s\n' \
    'style   — colours, borders and layout' \
    'confirm — ask before taking action' \
    'input   — capture a single line' \
    'choose  — select from a list' \
    'filter  — fuzzy-search values' \
    'spin    — show animated progress' \
    'table   — render CSV and TSV data' \
    'file    — browse files and folders' \
    'pager   — navigate long text' \
    'write   — edit multiple lines' \
    'join    — compose terminal blocks' \
    'log     — emit structured messages' |
    $HUM filter --header 'Find a hum command' --prompt 'search › ' \
        --placeholder 'start typing...' --height 8)

command_name=${selection%% *}

printf '\n'
$HUM style --foreground '#FF74C8' --bold --border rounded \
    --border-foreground '#FFE46B' --padding '1 3' --align center --width 62 \
    "hum $command_name"
printf '\n'
$HUM log --level info --prefix PALETTE "Selected: $selection"
$HUM style --foreground '#8DE2A1' --align center --width 70 \
    "Try it now: hum $command_name --help"

