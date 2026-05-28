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
* **Undo:** Revert recent changes.
* **Clipboard Integration:** Paste from the system clipboard (requires `xclip`
or `wl-paste`).
* **Mouse Support:** Click to position the cursor and use the scroll wheel.
* **Select All:** Select all text for quick deletion.

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
| `Home` / `End`      | Go to Start/End of Line |
| `Page Up` / `Page Down` | Move Page Up/Down       |
| `Backspace` / `Del` | Delete Character        |
| Mouse Click       | Position Cursor         |
| Mouse Wheel       | Scroll Up/Down          |

## License

MIT, see [LICENSE](LICENSE).

## Contributing

PRs welcome. Please open an issue first for major changes.
