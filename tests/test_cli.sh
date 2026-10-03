#!/bin/sh
set -eu

HUM=${1:-./hum}

fail()
{
    echo "FAIL: $1" >&2
    exit 1
}

assert_eq()
{
    expected=$1
    actual=$2
    description=$3
    [ "$expected" = "$actual" ] || fail "$description: expected '$expected', got '$actual'"
}

assert_eq "hum 0.31" "$($HUM --version)" "version output"
assert_eq "hum v0.31 — pleasant terminal tools for Haiku" \
    "$($HUM --help | sed -n '1p')" "help heading version"
assert_eq "hello" "$(NO_COLOR=1 $HUM style hello)" "plain style"
assert_eq "hello
world" "$(printf 'hello\nworld\n' | NO_COLOR=1 $HUM style)" "stdin style"
assert_eq "╭──────╮
│  Hi  │
╰──────╯" "$(NO_COLOR=1 $HUM style --border rounded --padding '0 2' Hi)" "border layout"

if printf 'y\n' | NO_COLOR=1 $HUM confirm --no-show-help "Proceed?" >/dev/null 2>&1; then
    :
else
    fail "confirm yes should return zero"
fi

if printf 'n\n' | NO_COLOR=1 $HUM confirm --no-show-help "Proceed?" >/dev/null 2>&1; then
    fail "confirm no should return one"
fi

output=$(printf 'y\n' | NO_COLOR=1 $HUM confirm --show-output "Proceed?" 2>/dev/null)
assert_eq "Proceed? Yes" "$output" "confirm output"

assert_eq "A oneB two" "$(NO_COLOR=1 $HUM join --horizontal 'A one' 'B two')" "horizontal join"
assert_eq " A 
BBB" "$(NO_COLOR=1 $HUM join --vertical --align center A BBB)" "vertical join"

assert_eq "INFO Ready port=haiku" \
    "$(NO_COLOR=1 $HUM log --structured --level info Ready port haiku 2>&1)" \
    "structured log"
assert_eq '{"level":"info","message":"Ready","port":"haiku"}' \
    "$(NO_COLOR=1 $HUM log --structured --formatter json --level info Ready port haiku 2>&1)" \
    "JSON log"

assert_eq "seed value" "$(printf 'seed value\n' | NO_COLOR=1 $HUM input)" "piped input"
assert_eq "Not much, hby?" \
    "$(NO_COLOR=1 $HUM input --cursor.foreground '#FF0' --prompt.foreground '#0FF' \
        --placeholder "What's up?" --prompt '* ' --width 80 --value 'Not much, hby?')" \
    "styled input example"
assert_eq "from environment" \
    "$(HUM_INPUT_VALUE='from environment' HUM_INPUT_WIDTH=80 NO_COLOR=1 $HUM input </dev/null)" \
    "input environment value"
assert_eq "flag wins" \
    "$(HUM_INPUT_VALUE='environment loses' HUM_INPUT_PLACEHOLDER='Env placeholder' \
        HUM_INPUT_PROMPT='env> ' HUM_INPUT_CURSOR_FOREGROUND='#FF0' \
        HUM_INPUT_PROMPT_FOREGROUND='#0FF' HUM_INPUT_WIDTH=20 NO_COLOR=1 \
        $HUM input --value 'flag wins' --width 80 </dev/null)" \
    "input flags override environment"
assert_eq "only" "$(NO_COLOR=1 $HUM choose --select-if-one only)" "single choice"
assert_eq "42" "$(NO_COLOR=1 $HUM choose --select-if-one --label-delimiter : 'Answer:42')" \
    "choice label value"
assert_eq "done" "$(NO_COLOR=1 $HUM spin --show-stdout -- sh -c 'printf done')" "spinner output"

if NO_COLOR=1 $HUM spin -- sh -c 'exit 7'; then
    fail "spin should preserve a failing command status"
else
    status=$?
    assert_eq "7" "$status" "spinner exit status"
fi

assert_eq "problem" \
    "$(NO_COLOR=1 $HUM spin --show-error -- sh -c 'printf problem >&2; exit 3' 2>&1)" \
    "spinner error output"

assert_eq "one
two" "$(printf 'one\ntwo\n' | NO_COLOR=1 $HUM pager)" "non-interactive pager"
assert_eq "strawberry" \
    "$(printf 'apple\nstrawberry\npear\n' | NO_COLOR=1 $HUM filter --value straw)" \
    "non-interactive unique filter"
assert_eq "first line
second line" \
    "$(printf 'first line\nsecond line\n' | NO_COLOR=1 $HUM write)" \
    "piped write"

table_output=$(printf 'Name,Value\nAlpha,1\nBeta,2\n' | NO_COLOR=1 $HUM table --print --border none)
case "$table_output" in
    *Name*Value*Alpha*1*Beta*2*) : ;;
    *) fail "static table output is missing expected cells" ;;
esac

quoted_table=$(printf 'Name,Age\n"Doe, Jane",30\n' | NO_COLOR=1 $HUM table --print --border none)
case "$quoted_table" in
    *'Doe, Jane'*30*) : ;;
    *) fail "quoted CSV field was not preserved" ;;
esac

echo "All CLI tests passed"
