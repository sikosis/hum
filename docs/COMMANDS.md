# Command roadmap

Hum is an independent project rather than a source port. Gum's current v2
command surface is used as a feature checklist, while Hum uses its own name,
documentation, implementation and `HUM_` environment variable namespace.

## Upstream command inventory

| Command | Purpose | Hum phase |
| --- | --- | --- |
| `style` | Apply colours, attributes, borders and spacing | Phase 1 |
| `confirm` | Ask a yes/no question using the exit status | Phase 1 |
| `join` | Join multi-line blocks horizontally or vertically | Phase 2 ✓ |
| `log` | Print styled structured log messages | Phase 2 ✓ |
| `input` | Prompt for a single line of input | Phase 2 ✓ |
| `choose` | Select one or more items from a list | Phase 2 ✓ |
| `spin` | Show progress while running a command | Phase 2 ✓ |
| `filter` | Fuzzy-filter and select list items | Phase 3 ✓ |
| `file` | Select a path from a file tree | Phase 3 ✓ |
| `pager` | Interactively view long text | Phase 3 ✓ |
| `table` | Render and select rows from tabular data | Phase 3 ✓ |
| `write` | Enter multi-line text | Phase 3 ✓ |
| `format` | Render Markdown, code, templates and emoji | Phase 4 |
| `version-check` | Test the program version against a constraint | Phase 4 |
| `completion` | Generate shell completions (hidden upstream) | Phase 4 |
| `man` | Generate manual pages (hidden upstream) | Phase 4 |

Global help and version output are infrastructure rather than subcommands and
are included in Phase 1.

## Phase 1: foundation

This repository currently implements the following foundation:

- dependency-free C++17 build using either CMake or Make;
- `hum style` with 16/256-colour values, `#RRGGBB` true colour, foreground and
  background colours, five text attributes, six border styles, alignment,
  width, height, margin, padding, trimming and optional ANSI stripping;
- `hum confirm` with custom action labels, selectable default, arrow/tab and
  direct `y`/`n` controls, timeouts, optional result output, and piped-input
  fallback for automated use;
- terminal raw-mode restoration through RAII;
- `NO_COLOR` support;
- command-line smoke tests.

Before Phase 2, Phase 1 needs testing in Haiku Terminal for escape-sequence
handling, colours, Unicode border width, resizing behaviour and terminal-state
restoration after interruption.

## Phase 2: everyday scripting

Phase 2 is implemented in Hum 0.2. It adds `join`, `log`, `input`, `choose`, and
`spin`. These commands reuse the Phase 1 renderer and terminal layer while
adding list selection, UTF-8 text entry, subprocess execution and richer key
decoding.

The implemented support boundary is:

- `join`: horizontal/vertical layout with edge and centre alignment;
- `log`: levels, minimum level filtering, prefixes, timestamps, files,
  structured fields, JSON, logfmt and `%s` formatting;
- `input`: initial/piped values, prompts, placeholders, headers, character
  limits, display width, password masking, timeout, UTF-8 cursor editing, and
  cursor/prompt/placeholder foreground colours, plus `HUM_INPUT_*` environment
  configuration with command-line flag precedence;
- `choose`: arguments or delimited stdin, single/multiple selection, initial
  values, ordered output, label/value splitting, scrolling and timeouts;
- `spin`: all named spinner sets, left/right alignment, output selection,
  error-only output, subprocess status propagation and timeout termination.

Most component-specific style flags, cursor animation modes, general printf verbs
and upstream environment-variable aliases are not part of 0.2. Hum currently
uses `HUM_LOG_LEVEL` and `NO_COLOR`; further `HUM_` configuration will be added
as the interfaces settle.

## Phase 3: interactive data

Phase 3 is implemented in hum 0.3. It adds `filter`, `file`, `pager`, `table`,
and `write`, plus a shared live-region viewport and terminal-size detection.

The implemented support boundary is:

- `pager`: keyboard scrolling, page navigation, line numbers, soft wrapping,
  configurable dimensions and idle timeout;
- `filter`: fuzzy or word-prefix matching, score sorting, initial queries,
  strict/free-text output, single/multiple selection and custom delimiters;
- `file`: hidden entries, permissions, human-readable sizes, symlink and
  directory traversal, and separate file/directory selection policies;
- `table`: quoted CSV/TSV parsing, explicit columns and widths, six border
  styles, static printing, selectable rows and single-column output;
- `write`: UTF-8 insertion and deletion, multi-line navigation, line joining,
  character/line limits, line numbers, active-line highlighting, piped initial
  values and Ctrl+D submission.

Search highlighting in `pager`, the upstream default recursive file list in
`filter`, extensive component style flags, dynamic resize redraws, and full
`HUM_` environment coverage remain follow-up compatibility work.

## Phase 4: rich text and distribution

Phase 4 will add `format` (Markdown, code, templates and named emoji), semantic
`version-check`, shell completion generation and manual-page generation.
Markdown rendering and syntax highlighting will be evaluated carefully to
avoid turning large third-party dependencies into a requirement.

This phase also includes a HaikuPorter recipe and `.hpkg` packaging once the
supported command surface is stable.

## Compatibility policy

Hum aims for familiar shell semantics, not byte-for-byte Gum compatibility.
Where compatible behaviour is sensible, Hum will retain familiar command and
flag names. Differences will be documented, and Hum-specific environment
variables will use `HUM_` rather than `GUM_`.
