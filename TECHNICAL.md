# Technical notes

This document covers building, testing and maintaining hum. For installation
and everyday examples, start with the [README](README.md).

## Requirements

hum is written in C++17 and uses POSIX terminal facilities, including
`termios`, `poll` and ANSI escape sequences. It has no third-party runtime
dependencies.

Building requires either:

- a C++17 compiler and Make; or
- a C++17 compiler and CMake 3.16 or newer.

## Build with Make

```sh
make
```

The resulting executable is `./hum`.

To remove compiled files:

```sh
make clean
```

## Build with CMake

```sh
cmake -S . -B build
cmake --build build
```

To install into a chosen prefix:

```sh
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/chosen/prefix
cmake --build build
cmake --install build
```

## Tests

The complete Make-based test suite is:

```sh
make test
```

For a CMake build:

```sh
ctest --test-dir build --output-on-failure
```

The shell suite exercises public command behaviour. The C++ terminal suite
checks key decoding and terminal helpers. Interactive changes should also be
tested manually in Haiku Terminal because terminal emulators can deliver key
sequences differently.

## Terminal and pipeline behaviour

Interactive interfaces use the controlling terminal when appropriate, keeping
selected values on standard output for shell composition. Commands that accept
piped input continue to read stdin. In particular, piped `y` or `n` input to
`hum confirm` takes precedence over an available controlling terminal.

Logs use standard error by default. `NO_COLOR` disables generated ANSI colours
and text attributes.

## Project layout

| Path | Purpose |
| --- | --- |
| `src/` | Command implementations and shared terminal code |
| `include/hum/` | C++ declarations |
| `tests/` | CLI and terminal tests |
| `docs/` | Detailed command coverage and compatibility notes |
| `examples/` | Demonstration shell scripts |
| `packaging/` | HaikuPorts staging material |
| `website/` | Public project website |

## Versioning

The current version lives in the root `VERSION` file. Make and CMake both read
that file, so it must remain the single source of truth.

hum uses the repository's two-part version scheme: larger feature updates add
`0.1`, while smaller updates and fixes add `0.01`. Published tags and archives
are immutable and must never be replaced to reuse a version.

## HaikuPorts

The candidate recipe, release prerequisites and Haiku-side validation steps
are documented in [packaging/haikuports](packaging/haikuports/README.md).

## Compatibility scope

hum is inspired by Charmbracelet's Gum, but it is an independent C++
implementation rather than a byte-for-byte or source-compatible port. The
[command reference](docs/COMMANDS.md) records implemented behaviour and known
differences.
