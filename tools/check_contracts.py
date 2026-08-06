#!/usr/bin/env python
# ADR-014 contract validator — checks every kernel source file for
# Component/Provides/Depends on/Owns header block.
# KISS: stdlib only, no argparse, matches tools/buildfs.py style.

import sys
import os
import glob

REQUIRED_MARKERS = [
    "Component:",
    "Provides:",
    "Depends on:",
    "Owns:",
]

MAX_HEADER_LINES = 12


def get_source_files():
    """Return list of source files to check, deduplicated."""
    patterns = [
        "src/kernel/*.c",
        "src/kernel/*.asm",
        "src/boot/*.asm",
    ]
    files = []
    seen = set()
    for pat in patterns:
        for f in glob.glob(pat):
            # Normalize path for Windows
            norm = os.path.normpath(f)
            if norm not in seen:
                seen.add(norm)
                files.append(norm)
    return sorted(files)


def check_file(filepath):
    """Check a single file for all required markers in first 12 lines.
    Returns (ok, missing_markers_list)."""
    try:
        with open(filepath, "r", encoding="utf-8") as f:
            lines = f.readlines()
    except Exception as e:
        return False, [f"read error: {e}"]

    # Check first MAX_HEADER_LINES lines
    header_text = "".join(lines[:MAX_HEADER_LINES])

    missing = []
    for marker in REQUIRED_MARKERS:
        if marker not in header_text:
            missing.append(marker)

    return len(missing) == 0, missing


def main():
    files = get_source_files()

    if not files:
        print("ERROR: no source files found", file=sys.stderr)
        sys.exit(1)

    violations = []
    for f in files:
        ok, missing = check_file(f)
        if not ok:
            violations.append((f, missing))

    if violations:
        for filepath, missing in violations:
            print(f"{filepath}: missing {', '.join(missing)}")
        sys.exit(1)
    else:
        print(f"contracts OK: {len(files)} files")
        sys.exit(0)


if __name__ == "__main__":
    main()