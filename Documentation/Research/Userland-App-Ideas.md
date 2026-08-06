# Userland App Ideas for AIkOS

**Date:** 2026-08-06
**Author:** mimo_researcher (research task)
**Status:** Research brief — proposals grounded in AIkOS current state and real-world hobby OS precedents

## Current State Summary

AIkOS as of v0.5.0 (Phase 3 complete) has:
- **Syscall ABI:** `int 0x80` with `write` (1) and `exit` (2) only — no `open`, `read`, `write`-to-file, or any input syscalls.
- **Filesystem:** AIkFS v1 (read-only, baked at build time via `buildfs.py`, RAM-backed initramfs).
- **ELF loader:** loads static ET_EXEC ELF64 programs from `/bin` into the 2 MiB user region.
- **Memory:** buddy heap (kmalloc/kfree) over physical pages.
- **Display:** VGA text mode (serial + VGA), no framebuffer yet. Phase 4 = framebuffer GUI.
- **Single process:** one ring-3 process at a time, no scheduling.
- **No networking** before Phase 5; no real hardware before Phase 7.

This means: text-mode apps that only `write` to stdout can run **now**. Anything interactive (keyboard input, mouse, windowed GUI) needs new syscalls. The table below flags each app's syscall needs.

## Summary Table

| # | App Name | Pitch | When | Syscalls Needed | Key New Syscalls |
|---|----------|-------|------|-----------------|------------------|
| 1 | Calculator | Four-function arithmetic REPL | Now (Phase 3) | write, exit | None |
| 2 | Minesweeper | Classic grid mine-clearing game (text-mode) | Now (Phase 3) | write, exit | None |
| 3 | Snake | Terminal-based snake game | Now (Phase 3) | write, exit | None |
| 4 | System Info | Display OS version, CPU, memory stats | Now (Phase 3) | write, exit | None |
| 5 | Text Editor | Multi-line text editing with save/load | Phase 4 | write, exit + **read, open, close, lseek, framebuf_write, key_read, mouse_read** | read, open, close, lseek, framebuf_write, key_read, mouse_read |
| 6 | File Browser | Graphical directory tree + file launching | Phase 4 | write, exit + **read, open, close, dir_read, framebuf_write, key_read, mouse_read, launch** | read, open, close, dir_read, framebuf_write, key_read, mouse_read, launch |
| 7 | Paint App | Bitmap drawing with mouse input | Phase 4 | write, exit + **framebuf_write, key_read, mouse_read** | framebuf_write, key_read, mouse_read |
| 8 | Hex Editor | Binary file viewer/editor with hex+ASCII display | Phase 4 | write, exit + **read, open, close, lseek, framebuf_write, key_read** | read, open, close, lseek, framebuf_write, key_read |
| 9 | Image Viewer (PPM) | Display PPM bitmap images in a window | Phase 4 | write, exit + **read, open, close, framebuf_write** | read, open, close, framebuf_write |
| 10 | System Monitor | Live process/memory/CPU stats display | Phase 4 | write, exit + **framebuf_write, key_read, process_list, mem_info** | framebuf_write, key_read, process_list, mem_info |
| 11 | Music Player | Audio playback with playlist UI | Phase 6+ | write, exit + **read, open, close, framebuf_write, key_read, audio_write** | framebuf_write, key_read, audio_write (requires audio subsystem) |
| 12 | Assembler (Own Tongue precursor) | Assemble Own Tongue source to machine code | Phase 6 | write, exit + **read, open, close, write_file** | read, open, close, write_file |

---

## Per-App Details

### 1. Calculator

**One-line pitch:** A four-function (and optionally scientific) calculator that runs as a text-mode REPL over serial.

**Why it fits AIkOS now:** With only `write` and `exit`, a text-mode calculator that reads from hardcoded expressions or a serial input loop is the simplest possible "app." It exercises arithmetic logic, string formatting, and error handling — all pure computation with no kernel dependencies beyond serial output. SerenityOS's Calculator is described as "a straightforward playground for building out a widget toolkit" — even the earliest hobby OS calculators serve as a UI framework proving ground.

**Syscalls needed:** `write` (1), `exit` (2) — both exist. For interactive use (user types expressions), a `read`-from-serial syscall would be needed, but a hardcoded demo works now.

**Phase:** Now (text-mode demo), Phase 4 (GUI with buttons).

**Source inspiration:**
1. **ToaruOS calculator** — "Four-function calculator app... intended to be a more straightforward playground for building out a widget toolkit." ([Source](https://github.com/klange/toaruos/blob/master/apps/calculator.c))
2. **SerenityOS Calculator** — ships as a recommended SerenityOS application with a full GUI button grid. ([Source](https://github.com/SerenityOS/serenity/tree/master/Userland/Applications/Calculator))

---

### 2. Minesweeper

**One-line pitch:** The classic grid-based mine-clearing game, playable in text mode via serial.

**Why it fits AIkOS now:** Minesweeper is a canonical "first game" for hobby OSes — it needs only a grid representation (text characters), random number generation, and state tracking. ToaruOS ships a minesweeper game ("Originally written in Python and ported to Kuroko. Visual design is based on the Gnome 'Mines'"). A text-mode version (using `#` for mines, digits for counts, `.` for unrevealed) needs zero new syscalls.

**Syscalls needed:** `write` (1), `exit` (2) — both exist. For interactive play, `read` would be needed.

**Phase:** Now (text-mode demo), Phase 4 (GUI with clickable cells).

**Source inspiration:**
1. **ToaruOS Mines** — "Minesweeper game. Originally written in Python and ported to Kuroko. Visual design is based on the Gnome 'Mines'." ([Source](https://github.com/klange/toaruos/blob/master/apps/mines.krk))
2. **SerenityOS Games** — ships a full Mines application in its Userland/Applications directory. ([Source](https://github.com/SerenityOS/serenity/tree/master/Userland/Applications))

---

### 3. Snake

**One-line pitch:** The classic snake game where the player guides a growing line to eat food without hitting walls or itself.

**Why it fits AIkOS now:** Snake is another canonical hobby OS game — it requires only a 2D grid (text characters), a timer tick for movement, and keyboard input for direction changes. With the existing PIT timer (100 Hz, Phase 1) and serial keyboard input, a text-mode snake is buildable now. MenuetOS included simple games as part of its demo suite; ToaruOS ships `pong.c` as a compositor demo game.

**Syscalls needed:** `write` (1), `exit` (2) — both exist. For interactive play and timing, `read` (keyboard) and `timer_get` (or use kernel timer) would be needed.

**Phase:** Now (text-mode demo), Phase 4 (GUI with pixel rendering).

**Source inspiration:**
1. **ToaruOS Pong** — "Play pong where the paddles and ball are all windows... Rendering updates are all done by the compositor, while the game only renders to the windows once at start up." ([Source](https://github.com/klange/toaruos/blob/master/apps/pong.c))
2. **ToaruOS Snow** — "Draw pretty falling snowflakes" — a simple animated demo showing the compositor's rendering loop. ([Source](https://github.com/klange/toaruos/blob/master/apps/snow.c))

---

### 4. System Info

**One-line pitch:** Display AIkOS version, CPU info, memory stats, and uptime — a "screenfetch" for AIkOS.

**Why it fits AIkOS now:** The kernel already exposes `cpuid`, `time`, `heap` (memory stats), and version info via the REPL. A `/bin/sysinfo.elf` could aggregate this data and print a formatted banner. ToaruOS's `sysinfo` is described as "similar to tools like 'screenfetch', this displays information about ToaruOS, the current machine state, and the user's configuration options."

**Syscalls needed:** `write` (1), `exit` (2) — both exist. For richer data, `read` (to query kernel state) would be needed.

**Phase:** Now (text-mode, reads kernel-provided data), Phase 4 (GUI with styled layout).

**Source inspiration:**
1. **ToaruOS sysinfo** — "Similar to tools like 'screenfetch', this displays information about ToaruOS, the current machine state, and the user's configuration options, alongside a terminal-safe rendition of the OS's logo." ([Source](https://github.com/klange/toaruos/blob/master/apps/sysinfo.c))
2. **ToaruOS uname** — standard system information utility. ([Source](https://github.com/klange/toaruos/blob/master/apps/uname.c))

---

### 5. Text Editor

**One-line pitch:** A multi-line text editor for creating and editing files on AIkFS.

**Why it fits AIkOS in Phase 4:** A text editor is the quintessential "OS completeness" app — SerenityOS's TextEditor and ToaruOS's `bim` (Vim-inspired editor) are core applications. However, a real editor needs: (a) keyboard input beyond serial polling, (b) file read/write syscalls, and (c) a framebuffer for multi-line display. AIkFS v1 is read-only, so editing requires AIkFS v2 (write support, Phase 3.x). This makes the text editor a Phase 4 app that drives multiple syscall additions.

**Syscalls needed:** `write` (1), `exit` (2) — exist. **NEW:** `read` (keyboard input), `open`, `close`, `write_file` (save), `lseek` (cursor positioning), `framebuf_write` (display), `key_read` (raw keyboard).

**Phase:** Phase 4+ (requires framebuffer GUI + file write support + keyboard input).

**Source inspiration:**
1. **ToaruOS bim** — a Vim-inspired text editor with syntax highlighting, described as an "advanced code editor" in the ToaruOS README. ([Source](https://github.com/toaruos/bim))
2. **SerenityOS TextEditor** — ships in the Userland/Applications directory as a core application. ([Source](https://github.com/SerenityOS/serenity/tree/master/Userland/Applications/TextEditor))

---

### 6. File Browser

**One-line pitch:** A graphical directory navigator that lists files, shows sizes, and launches apps.

**Why it fits AIkOS in Phase 4:** A file browser is the visual shell of any GUI OS — it's how users discover and launch apps. ToaruOS's file-browser.c is "Based on the original Python implementation and inspired by Nautilus and Thunar. Also provides a 'wallpaper' mode for managing the desktop background." For AIkOS, a file browser needs directory reading (AIkFS v1 has directories but no runtime `dir_read` syscall), file metadata, and a GUI framework.

**Syscalls needed:** `write` (1), `exit` (2) — exist. **NEW:** `read`, `open`, `close`, `dir_read` (enumerate directory entries), `framebuf_write`, `key_read`, `mouse_read`, `launch` (run an ELF from userland).

**Phase:** Phase 4+ (requires framebuffer + dir_read syscall + app launcher).

**Source inspiration:**
1. **ToaruOS file-browser** — "Based on the original Python implementation and inspired by Nautilus and Thunar. Also provides a 'wallpaper' mode for managing the desktop background." ([Source](https://github.com/klange/toaruos/blob/master/apps/file-browser.c))
2. **SerenityOS FileManager** — the standard graphical file manager. ([Source](https://github.com/SerenityOS/serenity/tree/master/Userland/Applications/FileManager))

---

### 7. Paint App

**One-line pitch:** A bitmap drawing application with pencil, fill, and color selection tools.

**Why it fits AIkOS in Phase 4:** Pixel-level drawing is the canonical framebuffer demo — it exercises the entire graphics stack (window creation, mouse tracking, pixel writes). SerenityOS's PixelPaint and ToaruOS's `drawlines.c` ("The original compositor demo application... Opens a very basic window and randomly fills it with colorful lines") both demonstrate this. For AIkOS, a paint app drives the framebuffer API design and proves mouse input works.

**Syscalls needed:** `write` (1), `exit` (2) — exist. **NEW:** `framebuf_write` (direct pixel access), `key_read`, `mouse_read` (coordinate tracking), `open`/`write_file` (save as PPM).

**Phase:** Phase 4 (requires framebuffer + mouse input).

**Source inspiration:**
1. **ToaruOS drawlines** — "The original compositor demo application, this dates all the way back to the original pre-Yutani compositor. Opens a very basic window (no decorations) and randomly fills it with colorful lines." ([Source](https://github.com/klange/toaruos/blob/master/apps/drawlines.c))
2. **SerenityOS PixelPaint** — a full-featured image editor in the Userland/Applications directory. ([Source](https://github.com/SerenityOS/serenity/tree/master/Userland/Applications/PixelPaint))

---

### 8. Hex Editor

**One-line pitch:** A binary file viewer showing hex bytes alongside ASCII representation.

**Why it fits AIkOS in Phase 4:** A hex editor is essential for inspecting raw filesystem data, ELF binaries, and debugging. SerenityOS's HexEditor is a core utility; ToaruOS's `hexify` is "Convert binary to hex... based on the output of xxd." For AIkOS, a hex editor needs file read syscalls and a framebuffer for the dual-pane hex+ASCII display.

**Syscalls needed:** `write` (1), `exit` (2) — exist. **NEW:** `read`, `open`, `close`, `lseek`, `framebuf_write`, `key_read`.

**Phase:** Phase 4+ (requires framebuffer + file read syscalls).

**Source inspiration:**
1. **ToaruOS hexify** — "Convert binary to hex. This is based on the output of xxd." ([Source](https://github.com/klange/toaruos/blob/master/apps/hexify.c))
2. **SerenityOS HexEditor** — a core hex editing application in the Userland/Applications directory. ([Source](https://github.com/SerenityOS/serenity/tree/master/Userland/Applications/HexEditor))

---

### 9. Image Viewer (PPM)

**One-line pitch:** Display PPM (Portable Pixmap) format images in a GUI window.

**Why it fits AIkOS in Phase 4:** PPM is the simplest bitmap format (ASCII or binary header + raw RGB pixels), requiring zero decompression. ToaruOS's `imgviewer` is described as "Display bitmaps in a graphical window... uses the libtoaru_graphics sprite functionality to load images." For AIkOS, PPM is the natural first image format — the project already has `tools/ppm2png.py` for host-side conversion, proving PPM is part of the toolchain.

**Syscalls needed:** `write` (1), `exit` (2) — exist. **NEW:** `read`, `open`, `close`, `framebuf_write`.

**Phase:** Phase 4 (requires framebuffer for display).

**Source inspiration:**
1. **ToaruOS imgviewer** — "Display bitmaps in a graphical window. This uses the libtoaru_graphics sprite functionality to load images, so it will support whatever that ends up supporting — which at the time of writing is just bitmaps of various types." ([Source](https://github.com/klange/toaruos/blob/master/apps/imgviewer.c))
2. **SerenityOS ImageViewer** — the standard image viewing application. ([Source](https://github.com/SerenityOS/serenity/tree/master/Userland/Applications/ImageViewer))

---

### 10. System Monitor

**One-line pitch:** A live-updating dashboard showing process list, memory usage, and CPU activity.

**Why it fits AIkOS in Phase 4:** A system monitor proves the OS is "alive" — it demonstrates process introspection, memory tracking, and real-time updates. SerenityOS's SystemMonitor includes "ProcessModel, MemoryStatsWidget, NetworkStatisticsWidget, ProcessFileDescriptorMapWidget, ProcessMemoryMapWidget" widgets. For AIkOS, this requires process enumeration syscalls and a framebuffer for the dashboard UI.

**Syscalls needed:** `write` (1), `exit` (2) — exist. **NEW:** `framebuf_write`, `key_read`, `process_list` (enumerate running processes), `mem_info` (heap/page stats), `timer_get` (for refresh rate).

**Phase:** Phase 4+ (requires framebuffer + process introspection syscalls).

**Source inspiration:**
1. **SerenityOS SystemMonitor** — includes widgets for process state, memory stats, network stats, file descriptors, and memory maps. ([Source](https://github.com/SerenityOS/serenity/tree/master/Userland/Applications/SystemMonitor))
2. **ToaruOS top** — "Show processes sorted by resource usage." ([Source](https://github.com/klange/toaruos/blob/master/apps/top.c))

---

### 11. Music Player

**One-line pitch:** Audio playback with a playlist UI and basic transport controls (play/pause/stop).

**Why it fits AIkOS in Phase 6+:** Music playback requires an audio subsystem (AC97 or HDA driver, DMA ring buffer, mixer) — this is far beyond AIkOS's current scope. ToaruOS has `piano.c` ("Interactively make beeping noises") and `play.c` for audio output; SerenityOS has SoundPlayer and Piano. This is a "horizon" app that motivates audio driver development.

**Syscalls needed:** `write` (1), `exit` (2) — exist. **NEW:** `read`, `open`, `close`, `framebuf_write`, `key_read`, `audio_write` (stream PCM to audio device), `audio_ctrl` (play/pause/stop).

**Phase:** Phase 6+ (requires audio subsystem — far future).

**Source inspiration:**
1. **ToaruOS piano** — "Interactively make beeping noises" — a simple speaker-based music app using ioctl. ([Source](https://github.com/klange/toaruos/blob/master/apps/piano.c))
2. **SerenityOS SoundPlayer** — a full audio playback application. ([Source](https://github.com/SerenityOS/serenity/tree/master/Userland/Applications/SoundPlayer))

---

### 12. Assembler (Own Tongue Precursor)

**One-line pitch:** An x86-64 assembler that runs on AIkOS itself — the stepping stone to Phase 6 (Own Tongue compiler).

**Why it fits AIkOS in Phase 6:** The roadmap's Phase 6 is "Own Tongue — Compiler for our own language. Compile + run a program on AIkOS itself." Before writing a compiler, you need an assembler. Building an assembler on AIkOS proves: (a) the OS can parse text input, (b) it can allocate memory for symbol tables, (c) it can write binary output (ELF files). This is the self-hosting bootstrap step.

**Syscalls needed:** `write` (1), `exit` (2) — exist. **NEW:** `read` (source file input), `open`, `close`, `write_file` (output ELF), `lseek`, `mmap` (symbol table allocation).

**Phase:** Phase 6 (bootstrap for Own Tongue).

**Source inspiration:**
1. **ToaruOS nm** — "Symbol table listing" utility, demonstrating ELF parsing on the OS itself. ([Source](https://github.com/klange/toaruos/blob/master/apps/nm.c))
2. **ToaruOS readelf** — ELF file inspection utility. ([Source](https://github.com/klange/toaruos/blob/master/apps/readelf.c))

---

## Needs New Syscalls — Summary

The following new syscalls are required to unlock the apps above, grouped by priority:

### Immediate (Phase 3.5 — unlocks text-mode interactivity)

| Syscall | Number (suggested) | Purpose | Unlocks |
|---------|-------------------|---------|---------|
| `read` | 3 | Read from keyboard/serial input | Interactive calculator, minesweeper, snake, all text-mode games |
| `open` | 4 | Open a file by path (returns fd) | Text editor, hex editor, file browser, image viewer |
| `close` | 5 | Close a file descriptor | All file-using apps |
| `read_file` | 6 | Read bytes from an fd | Text editor, hex editor, image viewer |

### Phase 4 (framebuffer GUI)

| Syscall | Number (suggested) | Purpose | Unlocks |
|---------|-------------------|---------|---------|
| `framebuf_write` | 10 | Write pixels to the framebuffer window | All GUI apps: calculator, paint, image viewer, system monitor |
| `key_read` | 11 | Read a keypress event (non-blocking) | All interactive GUI apps |
| `mouse_read` | 12 | Read mouse position and button state | Paint app, file browser, GUI calculator |
| `window_create` | 13 | Create a GUI window | All windowed apps |
| `window_draw` | 14 | Flush a window's backbuffer to screen | All windowed apps |

### Phase 4+ (filesystem richness)

| Syscall | Number (suggested) | Purpose | Unlocks |
|---------|-------------------|---------|---------|
| `write_file` | 7 | Write bytes to an fd | Text editor save, hex editor save, assembler output |
| `lseek` | 8 | Seek to a position in a file | Hex editor, text editor cursor |
| `dir_read` | 9 | Read directory entries | File browser |
| `launch` | 15 | Execute an ELF file from userland | File browser "open" action |

### Phase 5+ (process introspection)

| Syscall | Number (suggested) | Purpose | Unlocks |
|---------|-------------------|---------|---------|
| `process_list` | 20 | Enumerate running processes | System monitor |
| `mem_info` | 21 | Query heap/page statistics | System monitor |
| `timer_get` | 22 | Get monotonic clock | System monitor refresh rate |

### Phase 6+ (audio)

| Syscall | Number (suggested) | Purpose | Unlocks |
|---------|-------------------|---------|---------|
| `audio_write` | 30 | Stream PCM data to audio device | Music player |
| `audio_ctrl` | 31 | Play/pause/stop/volume control | Music player |

---

## Phasing Recommendation

**Phase 3.5 (between now and Phase 4):** Add `read`, `open`, `close`, `read_file` syscalls. This unlocks text-mode interactive apps (calculator, minesweeper, snake, system info) that can run over serial with keyboard input — immediate demo material without waiting for the framebuffer.

**Phase 4 (A Face):** Add framebuffer + window syscalls. Ship calculator (GUI), paint app, image viewer (PPM), and hex editor as the first GUI apps. These exercise the full display stack.

**Phase 4+ (late Phase 4):** Add `write_file`, `lseek`, `dir_read`, `launch`. Ship text editor, file browser, and system monitor — the "productivity suite" that makes the GUI useful.

**Phase 5+:** Network-aware apps (IRC client, fetch-like tool) once TCP/IP lands.

**Phase 6:** Assembler on AIkOS itself, then Own Tongue compiler.

---

## Source URL List

All URLs verified as of 2026-08-06:

1. https://github.com/SerenityOS/serenity/tree/master/Userland/Applications — SerenityOS application directory (Calculator, HexEditor, ImageViewer, TextEditor, FileManager, PixelPaint, SystemMonitor, SoundPlayer)
2. https://github.com/klange/toaruos/blob/master/apps/calculator.c — ToaruOS calculator ("Four-function calculator app")
3. https://github.com/klange/toaruos/blob/master/apps/mines.krk — ToaruOS minesweeper ("Minesweeper game... based on the Gnome 'Mines'")
4. https://github.com/klange/toaruos/blob/master/apps/pong.c — ToaruOS pong ("Play pong where the paddles and ball are all windows")
5. https://github.com/klange/toaruos/blob/master/apps/snow.c — ToaruOS snow ("Draw pretty falling snowflakes")
6. https://github.com/klange/toaruos/blob/master/apps/sysinfo.c — ToaruOS sysinfo ("Similar to tools like 'screenfetch'")
7. https://github.com/klange/toaruos/blob/master/apps/file-browser.c — ToaruOS file browser ("inspired by Nautilus and Thunar")
8. https://github.com/klange/toaruos/blob/master/apps/drawlines.c — ToaruOS drawlines ("The original compositor demo application")
9. https://github.com/klange/toaruos/blob/master/apps/hexify.c — ToaruOS hexify ("Convert binary to hex... based on the output of xxd")
10. https://github.com/klange/toaruos/blob/master/apps/imgviewer.c — ToaruOS image viewer ("Display bitmaps in a graphical window")
11. https://github.com/klange/toaruos/blob/master/apps/piano.c — ToaruOS piano ("Interactively make beeping noises")
12. https://github.com/klange/toaruos/blob/master/apps/top.c — ToaruOS top ("Show processes sorted by resource usage")
13. https://github.com/klange/toaruos/blob/master/apps/nm.c — ToaruOS nm (ELF symbol table listing)
14. https://github.com/klange/toaruos/README.md — ToaruOS README (overview of project, notable components, goals)
15. https://github.com/klange/toaruos/blob/master/apps/julia.c — ToaruOS Julia fractal generator
