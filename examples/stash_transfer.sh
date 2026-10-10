#!/bin/bash
set -u

HUM=${HUM:-hum}
SSH_SPIKE=${SSH_SPIKE:-./ssh_spike}

if ! command -v "$HUM" >/dev/null 2>&1; then
    echo "hum was not found. Install it or run with HUM=/path/to/hum." >&2
    exit 1
fi

if [ ! -x "$SSH_SPIKE" ]; then
    echo "ssh_spike is not executable: $SSH_SPIKE" >&2
    echo "Set SSH_SPIKE=/path/to/ssh_spike if it is elsewhere." >&2
    exit 1
fi

"$HUM" style --foreground '#79E8F2' --bold --border double \
    --border-foreground '#FF74C8' --padding '0 2' --align center --width 62 \
    'STASH TRANSFER'
printf '\n'

if SELECTED_FILE=$("$HUM" file . --header 'Choose a file to send'); then
    :
else
    printf '\n'
    "$HUM" style --foreground '#FFE46B' --bold 'No file selected — transfer cancelled.'
    exit 0
fi

REMOTE_FILE="/boot/home/${SELECTED_FILE##*/}"

"$HUM" style --foreground '#FF74C8' --bold "You selected: $SELECTED_FILE"
"$HUM" style --foreground '#79E8F2' "Remote path: $REMOTE_FILE"
printf '\n'

printf 'Setting,Value\nHost,192.168.64.1\nPort,22\nUser,sikosis\nAuthentication,Password prompt\n' |
    "$HUM" table --print --border rounded --widths 18,38
printf '\n'

if ! "$HUM" confirm --affirmative 'Send it' --negative 'Cancel' \
    "Send $SELECTED_FILE to sikosis@192.168.64.1:$REMOTE_FILE?"; then
    printf '\n'
    "$HUM" style --foreground '#FFE46B' --bold 'Transfer cancelled.'
    exit 0
fi

printf '\n'
"$HUM" log --level info --prefix STASH 'Connecting to Haiku'
if PASSWORD=$("$HUM" input --password --header 'SSH password' \
    --prompt 'Password › ' --placeholder 'Enter password'); then
    :
else
    printf '\n'
    "$HUM" style --foreground '#FFE46B' --bold 'Password entry cancelled.'
    exit 0
fi

if printf '%s\n' "$PASSWORD" | "$SSH_SPIKE" \
    --host 192.168.64.1 \
    --port 22 \
    --user sikosis \
    --known-hosts ./new_stash_known_hosts \
    --local "$SELECTED_FILE" \
    --remote "$REMOTE_FILE" \
    --password; then
    unset PASSWORD
    printf '\n'
    "$HUM" style --foreground '#8DE2A1' --bold --border rounded \
        --border-foreground '#8DE2A1' --padding '0 2' --align center --width 52 \
        '✓ STASH TRANSFER COMPLETE'
else
    result=$?
    unset PASSWORD
    printf '\n'
    "$HUM" log --level error --prefix STASH "Transfer failed (exit $result)"
    exit "$result"
fi
