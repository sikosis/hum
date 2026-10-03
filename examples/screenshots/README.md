# hum screenshot demos

These Bash scripts create colourful, screenshot-friendly demonstrations in
Haiku Terminal. They only use `hum` and standard shell utilities.

Build or install `hum`, then make the scripts executable:

```sh
chmod +x examples/screenshots/*.sh
```

Run any demo from the repository root:

```sh
examples/screenshots/01_welcome.sh
examples/screenshots/02_system_dashboard.sh
examples/screenshots/03_colour_lab.sh
examples/screenshots/04_release_wizard.sh
examples/screenshots/05_command_palette.sh
examples/screenshots/06_note_card.sh
```

If `hum` is not in `PATH`, point the demos to the executable:

```sh
HUM=./hum examples/screenshots/01_welcome.sh
```

Each script clears the terminal before drawing its scene. Set `NO_CLEAR=1` to
keep the existing terminal contents:

```sh
NO_CLEAR=1 examples/screenshots/03_colour_lab.sh
```

For the cleanest screenshots, use a terminal window around 90 columns wide,
hide unrelated windows, and let each script finish drawing before capturing.

## What each demo shows

- `01_welcome.sh`: `style` and `join` arranged as a colourful project card.
- `02_system_dashboard.sh`: live Haiku details rendered with `table` and `log`.
- `03_colour_lab.sh`: true-colour text, attributes and border styles.
- `04_release_wizard.sh`: `input`, `choose`, `write`, `confirm` and `spin`.
- `05_command_palette.sh`: an interactive fuzzy `filter` command palette.
- `06_note_card.sh`: a multi-line `write` editor followed by a shareable card.

