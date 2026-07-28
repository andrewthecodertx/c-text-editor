# ErwinText

ErwinText is a simple, lightweight text editor built in C using the ncurses
library. It aims to provide a classic terminal-based editing experience with
essential features for developers and writers.

## Features

* **Syntax Highlighting:** Supports C, C++, Shell Scripts, JavaScript, HTML,
  CSS, and XML.
* **File Management:** Create, open, and save files.
* **Basic Editing:** Insert, delete, and modify text.
* **Search:** Find text within a file.
* **Undo/Redo:** Revert recent changes (1000-level undo history).
* **Clipboard Integration:** Paste from the system clipboard (requires `xclip`
  or `wl-paste`).
* **Mouse Support:** Click to position the cursor and use the scroll wheel.
* **Select All:** Select all text for quick deletion.
* **Text Selection:** Shift+Arrow keys to select text (groundwork for copy/cut).

## Building

To build ErwinText, you'll need `ncurses` installed.

Then, simply run `make` in the project root directory:

```bash
make
```

This will create an executable named `erwintext`.

### Installation

To install ErwinText system-wide, run:

```bash
sudo make install
```

This will copy the `erwintext` executable to `/usr/local/bin`.

To uninstall, run:

```bash
sudo make uninstall
```

## Running

To run ErwinText, execute the following command:

```bash
./erwintext [filename]
```

Replace `[filename]` with the path to the file you want to open or create. If
no filename is provided, ErwinText will start with an empty buffer.

## Keybindings

| Keybinding        | Action                  |
| ----------------- | ----------------------- |
| `Ctrl+Q` / `Ctrl+C` | Quit                    |
| `Ctrl+S`          | Save File               |
| `Ctrl+F`          | Find (Search)           |
| `Ctrl+A`          | Select All              |
| `Ctrl+V`          | Paste from Clipboard    |
| `Ctrl+Z`          | Undo                    |
| Arrow Keys        | Move Cursor             |
| `Shift+Arrows`    | Select Text             |
| `Home` / `End`      | Go to Start/End of Line |
| `Page Up` / `Page Down` | Move Page Up/Down       |
| `Backspace` / `Del` | Delete Character        |
| Mouse Click       | Position Cursor         |
| Mouse Wheel       | Scroll Up/Down          |

## Version History

### v0.2.0 (2026-07-27)
- **Growable prompt buffer** — removed 128-byte limit on search/save-as prompts (PR #22)
- **Text selection** — Shift+Arrow selection model with `editor_resolve_selection()` (PR #26)
- **Undo action refactor** — `editor_action_free()` for proper memory management, `MAX_UNDO_STATES` 20→1000 (PR #23 + #25)
- **Code formatting** — `.clang-format` + CI check via GitHub Actions (PR #24)

### v0.1.0
- Initial release with basic editing, syntax highlighting, search, undo, clipboard, and mouse support.

## Contributors

Thanks to the following contributors for their work on v0.2.0:

- [Erdem Karaahmet](https://github.com/ErdemKaraahmet) — Code formatting & CI (PR #24)
- [Sushant Kataria](https://github.com/sushant-kataria) — Growable prompt buffer (PR #22), Undo memory fixes & capacity increase (PR #25)
- [Abhishek Krishna A M](https://github.com/Abhishek-Krishna-A-M) — Undo action management in `editor_actions.c` (PR #23)
- [Paulo Ferlin](https://github.com/paulorf0) — Text selection with Shift+Arrow keys (PR #26)

## License

MIT, see [LICENSE](LICENSE).

## Contributing

PRs welcome. Please open an issue first for major changes.