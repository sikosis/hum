<div align="center">
  <img src="assets/hum-icon.svg" alt="hum terminal icon" width="192" height="192">
  <h1>hum</h1>
  <p><strong>hum v0.34 ... it makes your Terminal sing.</strong></p>
  <p>
    <a href="https://github.com/sikosis/hum"><img alt="GitHub stars" src="https://img.shields.io/github/stars/sikosis/hum?style=for-the-badge&amp;color=ff74c8"></a>
    <img alt="Version 0.34" src="https://img.shields.io/badge/version-v0.34-79e8f2?style=for-the-badge">
    <img alt="Platform Haiku" src="https://img.shields.io/badge/platform-Haiku-ffe46b?style=for-the-badge">
    <img alt="C++17" src="https://img.shields.io/badge/C%2B%2B-17-79e8f2?style=for-the-badge">
    <a href="LICENSE"><img alt="MIT License" src="https://img.shields.io/github/license/sikosis/hum?style=for-the-badge&amp;color=8de2a1"></a>
  </p>
</div>

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

### Add a little colour

```sh
hum style --foreground '#79E8F2' --bold 'Haiku in colour'
hum style --foreground '#FF74C8' --italic 'A little terminal melody'
hum style --foreground '#FFE46B' --background '#262332' --bold 'Ready to go'
```

### Give text a frame

```sh
hum style --border rounded --border-foreground '#FF74C8' \
    --padding '1 3' --align center --width 34 'Hello from hum'
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

- [SSH stash transfer example](examples/stash_transfer.sh)
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
