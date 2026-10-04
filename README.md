# hum

> hum ... it makes your Terminal sing.

hum makes shell scripts friendlier with colourful text, interactive prompts,
pickers, tables, progress indicators and other terminal building blocks made
for Haiku.

It is a single C++ program with no third-party runtime dependencies. Use one
command on its own, pipe information into another, or combine several commands
into a complete interactive script.

## What can hum do?

| Command | What it does |
| --- | --- |
| `style` | Add colours, borders, spacing and text styles |
| `confirm` | Ask a yes-or-no question |
| `input` | Prompt for a line of text or a password |
| `choose` | Pick one or more items from a list |
| `filter` | Search a list and select matching items |
| `file` | Browse and select files or directories |
| `spin` | Show progress while another command runs |
| `pager` | Read and navigate longer text |
| `table` | Display CSV or TSV data as a table |
| `write` | Enter and edit multiple lines of text |
| `join` | Arrange text blocks beside or above each other |
| `log` | Print readable or structured log messages |

Run `hum help` to see the full list, or `hum <command> --help` for help with a
particular command.

## Install on Haiku

Until hum is available through HaikuDepot, build it from source:

```sh
git clone https://github.com/sikosis/hum.git
cd hum
make
mkdir -p ~/config/non-packaged/bin
cp hum ~/config/non-packaged/bin/hum
```

Check the installation with:

```sh
hum --version
```

## Try it

Make a colourful greeting:

```sh
hum style --foreground 212 --bold "Hello from Haiku"
```

Create a bordered card:

```sh
hum style --border rounded --padding "1 3" --align center --width 32 \
    "Welcome to hum"
```

Ask before continuing:

```sh
if hum confirm "Install the update?"; then
    echo "Installing..."
fi
```

Collect information from a user:

```sh
name=$(hum input --placeholder "Your name")
colour=$(hum choose Red Green Blue)
hum style --bold "Hello $name — you chose $colour"
```

Search a list:

```sh
printf 'Deskbar\nTracker\nTerminal\nWebPositive\n' | hum filter
```

Show a command running:

```sh
hum spin --title "Preparing files..." -- sleep 2
```

## Useful tips

- Set `NO_COLOR=1` if you do not want ANSI colours or text attributes.
- Interactive selections are printed to standard output, so they can be saved
  in shell variables or passed to another command.
- `confirm` returns exit status `0` for yes and `1` for no, making it suitable
  for `if` statements.
- Many commands accept information from stdin, which makes them easy to add to
  existing shell pipelines.

## More examples and documentation

- [Screenshot-ready example scripts](examples/screenshots/README.md)
- [Complete command reference](docs/COMMANDS.md)
- [Building, testing and implementation details](TECHNICAL.md)
- [HaikuPorts submission work](packaging/haikuports/README.md)

## Inspiration

hum is an independent C++ project inspired by
[Charmbracelet's Gum](https://github.com/charmbracelet/gum). It brings a
familiar style of pleasant shell interaction to Haiku, but it is not an
official Charmbracelet project or a direct source port.

## License

hum is available under the [MIT License](LICENSE).
