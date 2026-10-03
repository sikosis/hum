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

$HUM style --foreground '#FFE46B' --bold --align center --width 74 \
    'hum colour laboratory'
$HUM style --foreground '#A9A1B5' --align center --width 74 \
    'true colour + ANSI attributes + border styles'
printf '\n'

colours=(
    '#FF74C8|bubblegum'
    '#79E8F2|terminal cyan'
    '#FFE46B|deskbar yellow'
    '#8DE2A1|status green'
    '#C8B4FF|soft violet'
    '#FF9F68|warm orange'
)

for item in "${colours[@]}"; do
    colour=${item%%|*}
    label=${item#*|}
    $HUM style --foreground "$colour" --bold --width 35 \
        "$label  ████████████████  $colour"
done

printf '\n'
normal=$($HUM style --border normal --border-foreground '#FF74C8' \
    --padding '0 2' 'normal')
rounded=$($HUM style --border rounded --border-foreground '#79E8F2' \
    --padding '0 2' 'rounded')
double=$($HUM style --border double --border-foreground '#FFE46B' \
    --padding '0 2' 'double')
$HUM join --horizontal --align middle "$normal" '  ' "$rounded" '  ' "$double"

printf '\n\n'
$HUM style --bold 'bold  ' | tr -d '\n'
$HUM style --italic 'italic  ' | tr -d '\n'
$HUM style --underline 'underline  ' | tr -d '\n'
$HUM style --strikethrough 'strikethrough'
