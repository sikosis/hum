# Hum

Hum brings pleasant, composable terminal prompts and formatting to Haiku.
It is an independent C++ implementation inspired by Charmbracelet's Gum,
designed to build without third-party runtime dependencies.

hum 0.31 provides:

- `hum style` for ANSI colours, text attributes, borders, alignment, sizing,
  margins and padding;
- `hum confirm` for keyboard-driven yes/no prompts and shell-friendly exit
  statuses;
- `hum join` for horizontal and vertical block layout;
- `hum log` for coloured text, structured, JSON and logfmt messages;
- `hum input` for UTF-8-aware single-line entry, including password input;
- `hum choose` for single and multiple selection from arguments or stdin;
- `hum spin` for running commands with animated progress, output policies and
  timeouts;
- `hum pager` for bordered, wrapping, line-numbered text navigation;
- `hum filter` for fuzzy single or multiple selection;
- `hum file` for POSIX file and directory browsing;
- `hum table` for quoted CSV/TSV rendering and row selection;
- `hum write` for UTF-8 multi-line editing and Ctrl+D submission;
- `hum help`, `hum version`, `--help`, and `--version`;
- a small POSIX terminal layer based on `termios` and `poll`.

See [docs/COMMANDS.md](docs/COMMANDS.md) for the complete command inventory
and phased implementation plan.

For polished demonstrations designed for Haiku Terminal screenshots, see
[examples/screenshots](examples/screenshots/README.md).

## Build

With Make:

```sh
make
make test
```

Or with CMake:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Hum requires a C++17 compiler. No external libraries are required.

For the candidate HaikuPorts recipe, Haiku-side package tests, and submission
steps, see [packaging/haikuports](packaging/haikuports/README.md).

## Examples

```sh
hum style --foreground 212 --bold "Hello from Haiku"
hum style --border rounded --padding "1 3" --align center --width 30 "Hum"
hum join --horizontal "Left" "Right"
hum log --structured --level info "Starting" port haiku

if hum confirm "Continue?"; then
    echo "Continuing"
fi

name=$(hum input --placeholder "Your name")
colour=$(hum choose Red Green Blue)
hum spin --title "Working..." -- sleep 2
printf 'alpha\nbeta\ngamma\n' | hum filter
hum file --all .
printf 'Name,Value\nAlpha,1\nBeta,2\n' | hum table
notes=$(hum write --header "Notes")
```

`confirm` returns status `0` for the affirmative action and `1` for the
negative action. Its interface is written to `/dev/tty` when available, so
standard output remains safe for shell composition.

Set `NO_COLOR` to disable ANSI colours and text attributes.

## Status

Phase 3 is implemented and ready for Haiku testing. Component-specific styling
flags and a few advanced upstream behaviours remain intentionally deferred;
see the command roadmap for the precise support boundary.

## License

MIT
