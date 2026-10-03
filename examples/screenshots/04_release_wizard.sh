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

$HUM style --foreground '#FF74C8' --bold --border rounded \
    --border-foreground '#FFE46B' --padding '0 2' --align center --width 64 \
    'hum release wizard'
printf '\n'

version=$($HUM input --header 'Release version' --prompt 'version › ' \
    --value 'v0.4.0' --width 40)

channel=$($HUM choose --header 'Choose a channel' --cursor '› ' \
    stable beta nightly)

notes=$($HUM write --header 'Release notes · Ctrl+D to continue' \
    --placeholder 'What changed?' --value 'Polished terminal tools for Haiku.' \
    --show-line-numbers --show-cursor-line --height 5 --max-lines 5)

if ! $HUM confirm --affirmative 'Ship it' --negative 'Not yet' \
    "Publish $version to $channel?"; then
    printf '\n'
    $HUM style --foreground '#FFE46B' --bold 'Release paused — nothing changed.'
    exit 0
fi

printf '\n'
$HUM spin --spinner globe --title "Packaging $version..." -- sh -c 'sleep 2'

if [ -t 1 ] && [ "${NO_CLEAR:-0}" != 1 ]; then
    printf '\033[2J\033[H'
fi

$HUM style --foreground '#8DE2A1' --bold --border double \
    --border-foreground '#8DE2A1' --padding '0 2' --align center --width 64 \
    '✓ RELEASE READY'
printf '\n'
printf 'Field,Value\nVersion,%s\nChannel,%s\nStatus,packaged\n' \
    "$version" "$channel" |
    $HUM table --print --border rounded --widths 16,42
printf '\n'
$HUM style --foreground '#79E8F2' --bold 'Release notes'
$HUM style --border rounded --border-foreground '#C8B4FF' --padding '1 2' \
    --width 58 "$notes"

