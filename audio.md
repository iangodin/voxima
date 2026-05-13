Voice ──► Noise Gate ──► Compressor ──► EQ (hi-pass 80Hz) ──┐
                                                            ├──► Summing Bus ──► Limiter ──► Output
Music ──► LUFS Normalize ──► Ducking (keyed by voice) ──────┤
                                                            │
SFX ───► Peak Limiter ──────────────────────────────────────┘

Good approach — rolling your own gives you full control. Here's the ranking:

---

### Difficulty Ranking (Easiest → Hardest)

| Rank | Operation | Difficulty |             |
|------|------------------------|-------------|
|   1  | **Summing**            | Trivial     |
|   2  | **Peak Limiter**       | Very Easy   |
|   3  | **EQ — Hi-Pass**       | Easy        |
|   4  | **Noise Gate**         | Easy–Medium |
|   5  | **Limiter**            | Medium      |
|   6  | **Compressor**         | Medium      |
|   7  | **Ducking**            | Medium      |
|   8  | **LUFS Normalization** | Hard        |
|------|------------------------|-------------|

---

### Notes on Each

**Summing** — Just addition with gain scaling. The only gotcha is clipping if you don't manage levels first.

**Peak Limiter** — Essentially a hard clip or simple `min/max` clamp on the sample value. No time constants needed.

**EQ — Hi-Pass** — A single biquad filter with a well-known coefficient formula (RBJ Audio EQ Cookbook gives you everything). ~10 lines of math.

**Noise Gate** — Conceptually simple (open/close based on level threshold), but attack/hold/release timing adds some state management.

**Limiter** (with lookahead) — Like a compressor at ∞:1 ratio, but the lookahead buffer (to catch transients before they happen) adds real complexity.

**Compressor** — The gain computation itself isn't bad, but getting natural-sounding attack/release envelopes and RMS detection right takes iteration.

**Ducking** — Conceptually easy, but it's really a compressor with an external sidechain signal (the voice channel), so it inherits compressor complexity plus the cross-channel routing.

**LUFS Normalization** — The hardest by far. ITU-R BS.1770 requires a specific two-stage filter (K-weighting), true RMS integration over gating windows, and multi-channel loudness weighting. It's well-documented but involves significant implementation work.

---

### Practical Suggestion

For LUFS specifically, since it's a measurement standard rather than a creative effect, it's worth using a small dedicated library like **libebur128** (MIT licensed) just for that one operation, and implementing everything else yourself.

