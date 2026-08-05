# How to run AIkOS in QEMU

> **Status:** placeholder — no bootable image exists yet (2026-08-05).

Will contain the exact QEMU invocation used for development, including:

- Headless flags (`-display none`, `-serial stdio`) for agent-driven testing
- The QEMU monitor (`-monitor stdio`) for register/memory inspection
- `screendump` for capturing the framebuffer as a PNG (so the GUI can be *looked at*)
- gdb stub (`-s -S`) for kernel debugging
- Network setup for Phase 5 (user-mode networking)

**Updates go here as soon as the first bootable image exists — this is a high-traffic page.**
