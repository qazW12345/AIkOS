# ADR-014: Component Contract Methodology

**Status:** Accepted — 2026-08-05

## Context

AIkOS grows: 16 kernel C files + 3 assembly components today, with Phase 3 (filesystem, ELF loader, first userland apps) and beyond ahead. Two forces collide:

1. **Context is finite.** Neither the main model nor a subagent can keep the whole tree in mind as it grows. The hexdump delegation (2026-08-05) proved that a complete feature can be built by reading ~5 files — but only because the seams were small. As components grow, "which files are relevant" becomes the hard question, and implicit contracts (magic addresses like E820@0x5000, cross-file symbols like `gdt64`) force "read everything" — the failure mode to avoid.
2. **Userland will hit the same wall.** Phase 3+ apps (editors, file managers, eventually complex software) will outgrow whole-program reading too. The discipline must be established at the kernel layer now so it's inherited, not retrofitted.

The delegation policy (Guides/How-to-delegate-to-subagents.md) already requires self-contained briefs; this ADR gives those briefs a reliable anchor: **the component contract.**

## Decision

1. **Every component file carries a contract header block.** Format, immediately after the file's existing description comment:

   ```
   // Component: <name>
   // Provides: <public API — functions, globals, symbols exported>
   // Depends on: <components + specific contracts it relies on>
   // Owns: <fixed addresses, ports, IRQs, register ranges, magic numbers>
   ```

   Assembly files use `;` comments, same fields. `kernel.h` remains the component index and states this role in its header.

2. **The contract is the unit of delegation and analysis.** A brief for work on component X cites X's contract block + the contracts of X's *Depends on* list — never "read the tree." A contract that is wrong or missing is a review blocker.

3. **New components require zero build plumbing.** `build.sh` derives its kernel source list from the filesystem (glob over `src/kernel/*.c`); adding a file is just adding the file. `entry.o`/`interrupt.o` (assembly) stay explicit, entry pinned first.

4. **Userland (Phase 3+, intent recorded now):** each app gets its own design doc + contract blocks; shared code becomes static-linked library blobs before copy-paste can spread; apps remain separate blobs (ADR-013).

## Consequences

- **Positive:** components self-describe; delegation briefs shrink to "read these 3 contracts"; new files stop touching build.sh; Phase 3's filesystem/ELF work is born modular.
- **Negative:** contract blocks can drift from the code — mitigated by the review step in every delegation and by the coding rule that a contract change rides with the change it describes.
- **Neutral:** existing components were retrofitted on 2026-08-05 (16 C files, 3 asm files, kernel.h). Supersedes nothing; a new convention.
