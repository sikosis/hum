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

system_name=$(uname -s)
release_name=$(uname -r)
machine_name=$(uname -m)
host_name=$(hostname 2>/dev/null || printf 'haiku')
clock_value=$(date '+%Y-%m-%d  %H:%M')
hum_version=$($HUM --version)

$HUM style --foreground '#FF74C8' --bold --border double \
    --border-foreground '#79E8F2' --padding '0 2' --align center --width 70 \
    'HAIKU CONTROL CENTRE'
printf '\n'

printf 'Item,Value\nSystem,%s %s\nArchitecture,%s\nHost,%s\nTime,%s\nToolkit,%s\n' \
    "$system_name" "$release_name" "$machine_name" "$host_name" \
    "$clock_value" "$hum_version" |
    $HUM table --print --border rounded --widths 18,46

printf '\n'
$HUM log --level info --prefix SYSTEM 'Terminal services ready'
$HUM log --level info --prefix NETWORK 'Shell environment online'
$HUM log --level info --prefix HUM 'Prompt toolkit loaded'
printf '\n'
$HUM style --foreground '#8DE2A1' --bold --align center --width 70 \
    '● ALL SYSTEMS NOMINAL'

