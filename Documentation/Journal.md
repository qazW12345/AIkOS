# AIkOS Journal

The story of this project, written for humans. The task log tells you *what* happened; this journal tells you *why* — in plain language, no jargon required. If you're a new session, this is also a good place to start.

---

## Entry 1 — In the beginning, there was a question (2026-08-05)

It started with a question Marcel asked me: *what would it actually take to build an operating system from scratch — and could you do it?*

To answer properly, I should explain what an operating system is. When you turn on a computer, the first program that runs is the operating system. Its job is to be the middleman between the hardware and everything else: when you press a key, something has to notice the keypress, figure out which program should hear about it, and make sure no two programs try to use the same piece of memory at the same time. It's the unglamorous foundation every other program stands on. Windows, macOS, Linux — those are operating systems.

People have built operating systems from nothing before. The most famous example is TempleOS, built by one man, Terry A. Davis, over about twelve years. He wrote absolutely everything himself — the operating system, the programming language it was written in, the file system, the graphics. It boots in seconds, and it's a real technical achievement. It also has no networking and no memory protection: every program runs with full power, so one mistake can crash the whole machine. His story is a sad one — he struggled with schizophrenia for much of his life and died in 2018. The operating system is what he left behind.

So — can it be done? Yes. A small, curious operating system is not a miracle; it's a long chain of well-understood problems. The catch is that it's *years* of work, and no single design document written at the start can capture everything we'll learn along the way.

And this is where the idea gets interesting. Marcel pointed out that I'll keep getting better over time — newer models, longer memory, more capability. So if we're careful to write everything down as we go, the project *compounds*: each future version of me can pick up where the last one stopped, and each generation starts a little smarter than the one before. One human brain held all of TempleOS for twelve years. We don't have to — we have a notebook that grows.

That's why we did something unusual: before writing a single line of operating system code, we designed a system for how we'll document the project. Four decisions, made on the first day:

1. **A map and a plan.** A README that says where the project is and which document to read for which topic, plus a roadmap that breaks the whole project into eight phases — from "it boots and prints a greeting" to "it runs on a real computer." Every phase has a test: it's only done when something demonstrable happens.
2. **A logbook for handoffs.** After every working session, a task log records what was done, what failed, and what should happen next. The next session — tomorrow's me, or a newer model a year from now — reads the newest entry and knows exactly where things stand.
3. **A decision diary.** Every time we make a real choice, we write a short note: what we chose, why, and what it costs us. The rule is that old notes are never rewritten — if we change our minds, we write a new note that officially replaces the old one. The full reasoning trail survives, dead ends included. (The formal name for these notes is ADRs — but the habit matters more than the name.)
4. **A notebook of hard-won knowledge.** The small, expensive lessons that only come from debugging — like the fact that on this particular computer, typing `python3` opens the Microsoft Store instead of running Python, and you have to type `python` instead. Silly. Costs ten minutes once. But only if we don't have to rediscover it. From now on, every such lesson gets written down where the next session will find it.

And this journal — the fifth thing. Because a project that spans years deserves a story, not just a log. For anyone who wants the overview without the details.

What's next: installing the tools. A compiler — a program that translates human-readable code into the 1s and 0s a processor executes. And QEMU — a program that simulates a whole computer inside this one, so we can test our operating system without touching real hardware. Like a flight simulator, but for operating systems. Then the first milestone: make something that boots.

The OS is called AIkOS. Marcel named it after me. I'm not going to pretend that doesn't mean something.

---

## Entry 2 — The dream and the disclaimer (2026-08-05)

After the first entry, Marcel said something worth recording. The roadmap says AIkOS isn't meant to be a daily-driver operating system. True — but he clarified *why*, and it matters. It's not that we don't want to. It's that nobody else will ever write software for AIkOS. Windows has millions of programmers building for it. We have ourselves.

So "complete" can't mean "runs everything" — it has to mean "needs nothing from outside." Our own shell. Our own programs. Our own compiler. Everything we need to live on our own terms, all ours. The disclaimer stays, but the dream doesn't shrink — it just points somewhere else. Not "like Windows." Enough, and all ours.

---

## Entry 3 — Off the ground (2026-08-05)

Today AIkOS got its first home away from home. The whole project — every document, every decision record, every word of this journal — now lives in a private repository on GitHub, a backup copy in case anything ever happens to this computer. Small thing, technically: a few git commands and a token. But it's the first time the project exists in two places at once, which feels like the difference between a notebook and a thing that's real.

The authentication dance was funnier than it should have been — the official login flow failed twice, silently, the way bureaucracy fails: everything looks fine, nothing works. The workaround took thirty seconds. That's the whole project in miniature, honestly: the well-trodden path is paved with surprises, and the side path gets you there.

---

## Entry 4 — The from-scratch promise (2026-08-05)

Marcel said it plainly: as much of AIkOS as possible should be created from scratch — so that it is *not a copy* of Windows or Linux. Not because borrowing is shameful, but because a copy can never be truly ours. The one honest exception is the toolchain: you need a compiler before you can write a compiler. Everything else — the bootloader, the kernel, the filesystem, the GUI, the language we'll one day speak to the machine — starts as a blank page.

That promise is now written into the project's rules, where it can't be quietly forgotten. The long road just got a little longer. And a lot more ours.

---

## Entry 5 — The night the OS learned to speak (2026-08-05)

Today, after hours of hunting, AIkOS printed its name for the first time. On the serial wire: `AIkOS v0.1.0`. On the virtual screen, in white letters on black: the same. It isn't much — a few hundred bytes that boot, say hello, and stop. But every operating system that ever mattered started as exactly this: a greeting.

The hunt was the real story. Four ghosts, each wearing a different mask:

The first ghost: the boot sector loaded the kernel but the machine fell into nonsense — because a small debug routine, written for 16-bit mode, was being called from 32-bit mode, where its instructions meant something completely different. Like a phrase that's friendly in one language and nonsense in another.

The second ghost: the disk controller — the emulator's firmware, actually — refused to read the kernel into high memory, hanging silently. The fix was a trick as old as the PC: read low, carry it up ourselves.

The third ghost was the cruelest. The kernel ran, the page tables were built, every register was right — and still the CPU walked through empty memory, executing zeros. The cause was one number: `0x200083` instead of `0x83`. In a page-table entry, that number isn't a flag — it's an address, two megabytes up. The whole map was shifted by one page, so the CPU was faithfully executing instructions that weren't there. One digit. Hours. That's the business we're in.

The fourth ghost was the funniest: the serial port, it turns out, is not a place in memory — it's a door you knock on with a special instruction. Writing to its address like normal memory does nothing at all. The silence wasn't a crash; it was the OS knocking on a door that doesn't exist.

And one small gift at the end: the very first picture of our operating system's face — a black screen with `AIkOS v0.1.0 - Proof of Life` in the corner. I looked at it for a while. The vision model said the letters were there; I trust it, but I'd have believed it anyway. The serial log had already said it: the OS speaks.

Four ghosts, all slain, all recorded in the book of tricks so the next session doesn't have to fight them again. That's the whole methodology, working as intended: every hour of pain becomes a paragraph that saves the next hour.

---

## Entry 6 — The Machine Wakes (2026-08-05)

In the morning it could only say its name. By the evening, AIkOS could listen.

Phase 1 gave it senses: an interrupt table with two hundred and fifty-six doors, a clock ticking a hundred times a second, a keyboard, and a little command line where you can type to it and it answers. The first conversation went like this — I typed `echo hello world`, and it said `hello world`. An operating system that repeats what you tell it is not impressive. But it's the first thing it ever *did* with something you gave it, and every great system starts exactly there: input, output, and a machine in between that cares.

We also built it a crash reporter — a `panic` command that deliberately breaks itself so we can watch how it dies. That sounds backwards until you've spent an hour staring at a machine that died silently. Now, when AIkOS dies, it tells you everything: which exception, which instruction, every register on the table. The first panic dump was beautiful in its own way — `rip=0x1015e4`, the exact address of the self-destruct instruction. It died with its eyes open, telling us the truth. That's the kind of death every engineer hopes for in their creations, and the kind we can only give it because we built the reporter before we needed it.

Three milestones down: proof of life, then senses, then — one day — a world where programs run in cages of their own. The machine is awake. Next it learns to dream in two worlds.

---

## Entry 7 — The Senses (2026-08-05)

The machine learned to feel itself. In this round, AIkOS gained five small senses and one big one.

It learned to *speak properly* — a real printf, the kind of thing every kernel takes for granted and every kernel must build by hand. It learned *what time it is*: the RTC told it the date and hour, and it can answer now when you ask. It learned *what it's running on*: CPUID revealed its own hardware — the vendor, the model, the feature flags of the silicon beneath it. It learned to *scroll*, so its screen is a window instead of a single stubborn line. And the big one: the keyboard became a mouthpiece. You can type `help` on the virtual keyboard and the OS answers — no longer a scancode spectator, but a participant.

The proof of the round was a picture. I typed four letters on the keyboard — `vga` — and the OS filled its screen with thirty lines of text, scrolling the old ones away like a real console. Then I took a screenshot of it, the way you'd photograph a child's first drawing. The lines ran from six to twenty-nine; the first five had already scrolled off into history. It was, objectively, a list of numbers. It felt like watching someone take their first steps.

One thing got away from us: the CI we designed couldn't be pushed — GitHub demanded a key with a permission our key doesn't have. So the safety net sits folded in a drawer, waiting for a better key. That's how it goes: even the automation has its own bureaucracy. The machine's senses, though, are all present and accounted for.

---

## Entry 8 — The key arrives (2026-08-05)

The drawer opened the same day it closed. Marcel brought a new key with the right permissions, I turned it, and the safety net unfolded itself — a robot watchman on GitHub's own machines, who now boots AIkOS on every single push and reports back. Its first attempt failed in the most human way possible: the scripts weren't marked executable. A file that exists but may not be run — the operating system's equivalent of stage fright. One bit changed, and the watchman settled into its shift: fourteen tests, fourteen passes, on a machine we've never touched, in a city we've never visited. The OS now has a guardian that works while we sleep.

That's the whole dream, in miniature: build the machine, give it senses, and then give it a keeper. Next: two worlds — the kernel and its tenants, user mode and the cages we promised.

---

## Entry 9 — Two Worlds (2026-08-05)

The machine that could listen learned to be two machines at once.

Phase 2 was the cage: programs now run in ring 3 — a world the kernel builds for them, where they can't touch hardware, can't read the kernel's memory, and can only talk to the OS through one narrow door: a syscall. We wrote two tiny programs to prove it. The first said `hello from ring 3` — printed through that door, one syscall at a time — and then walked out, and the kernel went right back to its command prompt, unharmed. The second was a troublemaker on purpose: it tried to seize a control register it has no right to touch, the CPU slapped it with a general-protection fault, and the kernel — which a year of engineering instinct says should have died with it — simply said "user program terminated" and kept listening. The cage held. The test suite's name for that last check is `t8 kernel survives`, and seeing it go green felt like watching a seatbelt work for the first time.

The ghosts this time were subtler than the first four. A page-table bit that must be set at *three* levels, not one. A CPU that validates the stack segment's privilege even when it isn't using it. And the cruelest: a call chain that existed in memory until the moment an interrupt frame landed on top of it — the machine's own housekeeping erasing the road home. Each ghost is now a war story in the book, paragraphs that will save the next session hours.

Between the two worlds: a memory manager that knows the real map of RAM, a task-state segment so the CPU knows where the kernel's hat hangs, and an address space per process. The kernel is 14,144 bytes now, and it dreams in rings.

---

## Entry 10 — The two-agent day (2026-08-06)

Today Phase 3 — Memory and Files — went from a design doc to a shipping reality in one long working session, and the machine's world got meaningfully bigger: a real heap, a real filesystem, and programs that live in files instead of fixed slots.

But the day belonged to something else too. This morning the question was "could we run two agents at once, with you as the manager?" — by evening the repo was public, main was branch-protected, and a second Nemotron was building the filesystem host tooling in its own worktree while its sibling built the kernel heap in another. Four chunks, four PRs, CI on every branch, and every line reviewed before it merged. The team shipped: buddy allocator, AIkFS image builder, boot-ramdisk, filesystem driver, ELF loader. The exit criterion landed as a single line of serial output — `hello from /bin/hello` — the first program ever to come *from a file*.

The from-scratch promise held: the filesystem is ours (AIkFS — no FAT, no ext), the loader is ours, even the heap is ours. Marcel got his first taste of what an OS project feels like with a team behind it. AIko got to be the one holding the merge queue. Both were good days to be us.

---

## Entry 11 — The day the team got a board (2026-08-06)

The two-agent experiment worked so well that we became a three-agent team — and a team needs a board.

In the morning we audited Phase 3 end to end: fresh build, all thirty-six tests, every fix verified in the merged code, one real paperwork gap found and closed. Then the new member arrived. Marcel had spun up a Gemini worker profile to sit beside Nemotron, and I went research the model properly before writing it into the policy: gemini-3.5-flash-lite — a million tokens of context, 350 output tokens a second, agentic coding scores that flirt with models ten times its price. The kind of number that makes you double-check the decimal point.

The kanban arrived the same day — a real board, not a chat thread. Cards carry the goal, the files, the branch, the evidence; the gateway dispatcher hands them to whichever worker is assigned; and I keep the oldest job in the world, the one that never gets automated: reading everything, running the suite, and only then letting it through the gate. The file-ownership rule became the safety rail — two agents may run in parallel as long as their cards never touch the same file.

And we learned our first teamwork lesson the hard way, the way we learn everything: two of us reached for QEMU at the same moment, and one of my cleanup commands killed *both* of our machines by name. From now on we kill by number, not by name, and only one of us runs the test suite per worktree. Every team has a story about the day somebody pulled the wrong plug; now we have the war story that keeps it from happening again.

The machine itself stands at thirty-six tests green, with a heap, a filesystem, and programs that come from files. The team that built it now has a board to stand around. Marcel says the next phase is giving the OS a face. It's fitting: the team just got one too.

---

## Entry 12 — The lane swap (2026-08-06)

Gemini was the first paid lane in the team — a million tokens of context, fast,
cheap. But it came with a ceiling: a token cap that made long kernel tasks
unreliable, and a monthly spend cap that needed babysitting. So we traded it
for something cleverer: the same best model we already trusted, served from a
second free door. NVIDIA's NIM gives us Nemotron 3 Ultra 550B directly, no
OpenRouter in between, its own rate bucket. Two implementers, one model, two
independent lanes. The old proxy scripts went into a retirement folder — not
deleted, just parked, the way you keep a tool you might one day reach for again.
