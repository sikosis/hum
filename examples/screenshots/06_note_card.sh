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

$HUM style --foreground '#FF74C8' --bold --align center --width 66 \
    'A LITTLE NOTE FROM HAIKU'
$HUM style --foreground '#A9A1B5' --align center --width 66 \
    'write your message, then press Ctrl+D'
printf '\n'

message=$($HUM write --header 'Compose note' --prompt '│ ' \
    --placeholder 'Something worth sharing...' --show-line-numbers \
    --show-cursor-line --height 7 --max-lines 7)

author=$($HUM input --header 'Sign your note' --prompt 'from › ' \
    --placeholder 'Haiku user' --width 36)

if [ -t 1 ] && [ "${NO_CLEAR:-0}" != 1 ]; then
    printf '\033[2J\033[H'
fi

heading=$($HUM style --foreground '#FFE46B' --bold \
    'POSTCARD FROM HAIKU')
body=$($HUM style --foreground '#F8F5FB' "$message")
signature=$($HUM style --foreground '#79E8F2' --italic "— $author")

$HUM style --border double --border-foreground '#FF74C8' \
    --no-strip-ansi --padding '1 3' --width 58 "$heading

$body

$signature"
printf '\n'
$HUM style --foreground '#8DE2A1' --align center --width 66 \
    'made with hum · github.com/sikosis/hum'
