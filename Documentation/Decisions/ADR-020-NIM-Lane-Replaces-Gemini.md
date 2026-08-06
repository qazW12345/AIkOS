# ADR-020: NIM implementer lane replaces Gemini

- **Status:** Accepted (2026-08-06)
- **Supersedes:** the implementer-roster and provider-governance clauses of ADR-019 (worker team)
- **Superseded by:** nothing

## Context

ADR-019 fixed the implementer roster at two interchangeable profiles:
`nemotron_implementer` (nemotron-3-ultra-550b-a55b:free via OpenRouter) and
`gemini_implementer` (gemini-3.5-flash-lite via Google AI Studio, paid).
Gemini's paid lane proved operationally fragile for agent work: it hits a
hard **token cap** (250K input tokens/min shared bucket, throttled under
sustained agent loops) that made long kernel tasks unreliable, and it carried
a monthly spend-cap failure mode that required manual top-ups. Meanwhile
NVIDIA NIM offers the **same top model** as the OpenRouter lane
(`nvidia/nemotron-3-ultra-550b-a55b`) as a direct, free endpoint with an
independent rate bucket. Question: what does the implementer roster become
when the paid lane is retired?

## Decision

1. **Gemini is removed from the team.** Profile `gemini_implementer` deleted,
   its budget-governor proxy (`gemini_budget_proxy.py`, localhost:8787) and
   keep-alive cron (`e7a0b4e562f3`) retired to `.gemini_retired/`. The model
   remains usable for one-shot tasks via Google AI Studio if ever needed, but
   it is no longer a kanban assignee.
2. **New second implementer: `nvidia_implementer`.** Same model as the
   OpenRouter lane (`nvidia/nemotron-3-ultra-550b-a55b`) served directly from
   `https://integrate.api.nvidia.com/v1` (NIM free tier, ~40 RPM per key).
   Two implementer lanes, same model, independent buckets — parallelism comes
   from *provider lanes*, not from model spread.
3. **Implementer roster stays at two** — the ADR-019 ceiling ("no third
   implementer at this codebase size") is unchanged, because this is a
   lane-for-lane replacement, not an expansion.
4. **Provider governance.** NVIDIA NIM is a free endpoint: traffic is logged
   (no credentials/PATs/personal data in cards or briefs — same rule as the
   OpenRouter free lane). No budget proxy is needed for NIM; `model.rate_limit_delay`
   remains inert in this Hermes build. OpenRouter free tier daily cap was
   permanently raised 50 → 1000 req/day by a one-time $10 credit purchase
   (all-time threshold, per OpenRouter docs).

## Consequences

**Positive:** implementer throughput rises (OpenRouter 20 RPM + NIM ~40 RPM on
the same proven model); zero paid spend on implementers; lower latency than the
OpenRouter hop; the ADR-019 "two implementers" ceiling survives unchanged.
**Negative:** both implementer lanes now run the *same* model — a Nemotron-family
regression would hit both lanes simultaneously; NIM free capacity is
community-contributed and can be temporarily unavailable.
**Neutral:** profile re-pointing proved trivial (ADR-019 anticipated this); the
gemini proxy scripts are preserved in `.gemini_retired/` if the lane is ever
re-activated.
