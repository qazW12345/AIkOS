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
