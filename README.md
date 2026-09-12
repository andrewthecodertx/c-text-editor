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

To build ErwinText, you'll need a C99 compiler, `make`, and `ncurses`
installed. Linux distributions usually provide ncurses through a package such
as `libncurses-dev`; macOS includes a compatible system ncurses library.

The default build is a debug build with AddressSanitizer and
UndefinedBehaviorSanitizer enabled:

```bash
make
```

This will create an executable named `erwintext`.

Additional build targets are available:

```bash
make debug    # -O0, debug symbols, ASan and UBSan
make release  # optimized production binary
make tsan     # ThreadSanitizer build
make check    # static analysis with cppcheck
make format-check
```

Build artifacts are kept under `build/`, with the selected executable copied
to the project root.

### Installation

To install ErwinText system-wide, run:

```bash
sudo make install
```

This will copy the `erwintext` executable to `/usr/local/bin`.

The installation prefix and staging root are configurable:

```bash
make install PREFIX="$HOME/.local"
make install DESTDIR=/tmp/package-root PREFIX=/usr
```

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
| `Ctrl+Q`          | Quit                    |
| `Ctrl+S`          | Save File               |
| `Ctrl+F`          | Find (Search)           |
| `Ctrl+A`          | Select All              |
| `Ctrl+C`          | Copy Selection          |
| `Ctrl+V`          | Paste from Clipboard    |
| `Ctrl+Z`          | Undo                    |
| `Ctrl+Y`          | Redo                    |
| Arrow Keys        | Move Cursor             |
| `Shift` + `Arrows` / `Home` / `End` / `PgUp` / `PgDn` | Select Text |
| `Home` / `End`      | Go to Start/End of Line |
| `Page Up` / `Page Down` | Move Page Up/Down       |
| `Backspace` / `Del` | Delete Character        |
| Mouse Click       | Position Cursor         |
| Mouse Wheel       | Scroll Up/Down          |

## Version History

### v0.3.0 (2026-08-20)
- **Portable builds & macOS support** — `make debug` (ASan+UBSan) / `release` / `tsan` targets, per-config `build/` dirs, header dependency tracking, pkg-config ncurses detection, configurable `PREFIX`/`BINDIR`/`DESTDIR`, `KEY_RESIZE` handling, expanded CI (Linux+macOS, static analysis) (PR #29)
- **Non-fatal error handling** — OOM/file errors no longer call `exit(1)`; editor stays open with a critical error bar, allowing Ctrl+S to save and Ctrl+Q to quit safely (PR #27, fixes #4)
- **Clipboard copy** — `Ctrl+C` copies the active selection (or select-all) to the system clipboard via `wl-copy` (Wayland) or `xclip` (X11); selection extended to `Shift`+`Home`/`End`/`PgUp`/`PgDn` (PR #30)

### v0.2.0 (2026-07-27)
- **Growable prompt buffer** — removed 128-byte limit on search/save-as prompts (PR #22)
- **Text selection** — Shift+Arrow selection model with `editor_resolve_selection()` (PR #26)
- **Undo action refactor** — `editor_action_free()` for proper memory management, `MAX_UNDO_STATES` 20→1000 (PR #23 + #25)
- **Code formatting** — `.clang-format` + CI check via GitHub Actions (PR #24)

### v0.1.0
- Initial release with basic editing, syntax highlighting, search, undo, clipboard, and mouse support.

## Contributors

Thanks to the following contributors for their work on v0.3.0:

- [Enzo Gagarin](https://github.com/enzogagarin) — Portable build targets & macOS support (PR #29)
- [Kampito Jha](https://github.com/kampitojha) — Non-fatal error handling (PR #27)
- [Paulo Ferlin](https://github.com/paulorf0) — Clipboard copy & extended selection (PR #30)

Thanks to the following contributors for their work on v0.2.0:

- [Erdem Karaahmet](https://github.com/ErdemKaraahmet) — Code formatting & CI (PR #24)
- [Sushant Kataria](https://github.com/sushant-kataria) — Growable prompt buffer (PR #22), Undo memory fixes & capacity increase (PR #25)
- [Abhishek Krishna A M](https://github.com/Abhishek-Krishna-A-M) — Undo action management in `editor_actions.c` (PR #23)
- [Paulo Ferlin](https://github.com/paulorf0) — Text selection with Shift+Arrow keys (PR #26)

## License

MIT, see [LICENSE](LICENSE).

## Contributing

PRs welcome. Please open an issue first for major changes.
