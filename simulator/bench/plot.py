#!/usr/bin/env python3

import sys
import os
import glob
import re
import math
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np

if len(sys.argv) > 1:
    src = sys.argv[1]
else:
    script_dir = os.path.dirname(os.path.abspath(__file__))
    files = sorted(glob.glob(os.path.join(script_dir, "results", "bench_*.txt")))
    if not files:
        sys.exit("No bench/results/bench_*.txt found. Run ./bench.sh first.")
    src = files[-1]

print(f"Reading: {src}")
text = open(src).read()

out_dir = os.path.dirname(src)

BLUE   = "#2563EB"
GREEN  = "#16A34A"
RED    = "#DC2626"
ORANGE = "#D97706"
PURPLE = "#7C3AED"
GRAY   = "#6B7280"
DARK   = "#111827"
BG     = "#F9FAFB"

plt.rcParams.update({
    "figure.facecolor":  BG,
    "axes.facecolor":    BG,
    "axes.edgecolor":    DARK,
    "axes.labelcolor":   DARK,
    "xtick.color":       DARK,
    "ytick.color":       DARK,
    "text.color":        DARK,
    "font.family":       "DejaVu Sans",
    "font.size":         11,
    "axes.titlesize":    13,
    "axes.titleweight":  "bold",
    "axes.spines.top":   False,
    "axes.spines.right": False,
    "figure.dpi":        150,
})

def section(title_fragment):
    lines = text.splitlines()
    inside = False
    result = []
    for line in lines:
        if title_fragment in line:
            inside = True
            continue
        is_full_sep = "────" in line and " " not in line and len(line) > 20
        if inside and is_full_sep and result:
            break
        if inside:
            result.append(line)
    return result

def parse_table(lines, n_cols):
    rows = []
    for line in lines:
        tokens = line.split()
        nums = []
        for t in tokens:
            t2 = t.rstrip("%x")
            try:
                nums.append(float(t2))
            except ValueError:
                pass
        if len(nums) >= n_cols:
            rows.append(nums[:n_cols])
    return rows

ev_lines = section("EV Comparison")
ev_data = []
strategy_order = [
    "double-first", "bullish", "surrender-first", "always-stand",
    "bearish", "mimic-dealer", "basic", "hi-lo"
]
label_map = {
    "always-stand":    "Always Stand",
    "bearish":         "Bearish",
    "mimic-dealer":    "Mimic Dealer",
    "bullish":         "Bullish",
    "double-first":    "Double First",
    "surrender-first": "Surrender First",
    "basic":           "Basic",
    "hi-lo":           "Hi-Lo",
}
for line in ev_lines:
    for key in strategy_order:
        if line.strip().startswith(key):
            parts = line.split()
            try:
                ev_val = float(parts[1])
                ev_data.append((key, ev_val))
            except (IndexError, ValueError):
                pass

ev_dict = dict(ev_data)
labels  = [label_map[s] for s in strategy_order]
values  = [ev_dict.get(s, 0) for s in strategy_order]
colors  = [GREEN if v >= 0 else RED for v in values]

fig, ax = plt.subplots(figsize=(9, 5))
bars = ax.barh(labels, values, color=colors, edgecolor="white", linewidth=0.5, height=0.65)

ax.axvline(0, color=DARK, linewidth=0.8)
for bar, val in zip(bars, values):
    pad = 0.5
    ha  = "left" if val >= 0 else "right"
    x   = val + pad if val >= 0 else val - pad
    ax.text(x, bar.get_y() + bar.get_height() / 2,
            f"{val:+.2f}¢", va="center", ha=ha, fontsize=9.5)

ax.set_xlabel("Expected Value per Hand (cents)")
ax.set_title("Strategy Comparison: Expected Value per Hand")
ax.set_xlim(min(values) * 1.15, max(values) * 1.35 + 5)

good  = mpatches.Patch(color=GREEN, label="Positive EV (player advantage)")
bad   = mpatches.Patch(color=RED,   label="Negative EV (house wins)")
ax.legend(handles=[good, bad], loc="lower right", fontsize=9)

fig.tight_layout()
p = f"{out_dir}/chart1_ev_comparison.png"
fig.savefig(p); print(f"Saved: {p}")
plt.close()

ss_lines = section("Strong Scaling")
ss_rows  = parse_table(ss_lines, 3)

threads_ss  = [int(r[0]) for r in ss_rows]
speedups_ss = [r[2] for r in ss_rows]

m = re.search(r"Serial fraction\s+f\s*[≈~=]+\s*([\d.]+)", text)
f_serial = float(m.group(1)) if m else 0.078

max_t = max(threads_ss)
t_range = np.linspace(1, max_t, 200)
amdahl  = 1.0 / (f_serial + (1 - f_serial) / t_range)

fig, ax = plt.subplots(figsize=(8, 5))
ax.plot(t_range, amdahl,  color=GRAY,   linewidth=1.5, linestyle="--",
        label=f"Amdahl's Law  (f={f_serial:.3f})")
ax.plot(t_range, t_range, color=BLUE,   linewidth=1,   linestyle=":",
        label="Ideal linear speedup")
ax.plot(threads_ss, speedups_ss, color=ORANGE, linewidth=2.5,
        marker="o", markersize=7, label="Measured speedup")

for t, s in zip(threads_ss, speedups_ss):
    ax.annotate(f"{s:.2f}×", (t, s), textcoords="offset points",
                xytext=(6, 4), fontsize=9)

ax.set_xlabel("Threads")
ax.set_ylabel("Speedup")
ax.set_title(f"Strong Scaling (Amdahl's Law)  —  serial fraction f ≈ {f_serial:.3f}")
ax.set_xticks(threads_ss)
ax.set_xlim(0.5, max_t + 0.5)
ax.set_ylim(0.5)
ax.legend(fontsize=10)
ax.grid(axis="y", alpha=0.3)

fig.tight_layout()
p = f"{out_dir}/chart2_strong_scaling.png"
fig.savefig(p); print(f"Saved: {p}")
plt.close()

ws_lines = section("Weak Scaling")
ws_rows  = parse_table(ws_lines, 3)

threads_ws = [int(r[0]) for r in ws_rows]
times_ws   = [r[2] for r in ws_rows]
ideal_time = times_ws[0]

fig, ax = plt.subplots(figsize=(8, 5))
ax.axhline(ideal_time, color=BLUE, linewidth=1.2, linestyle=":",
           label=f"Ideal (constant {ideal_time:.0f} ms)")
ax.plot(threads_ws, times_ws, color=ORANGE, linewidth=2.5,
        marker="s", markersize=7, label="Measured wall time")

for t, ms in zip(threads_ws, times_ws):
    ax.annotate(f"{ms:.0f}", (t, ms), textcoords="offset points",
                xytext=(5, 5), fontsize=9)

ax.set_xlabel("Threads  (work scales proportionally)")
ax.set_ylabel("Wall time (ms)")
ax.set_title("Weak Scaling (Gustafson's Law)  —  games ∝ threads")
ax.set_xticks(threads_ws)
ax.set_xlim(0.5, max(threads_ws) + 0.5)
ax.legend(fontsize=10)
ax.grid(axis="y", alpha=0.3)

fig.tight_layout()
p = f"{out_dir}/chart3_weak_scaling.png"
fig.savefig(p); print(f"Saved: {p}")
plt.close()

sp_lines = section("Speedup per Strategy")
sp_data  = {}
strat_keys = ["always-stand","bearish","mimic-dealer","bullish",
              "double-first","surrender-first","basic","hi-lo"]
for line in sp_lines:
    for key in strat_keys:
        if line.strip().startswith(key):
            parts = line.split()
            try:
                spd = float(parts[-1])
                sp_data[key] = spd
            except (IndexError, ValueError):
                pass

sp_labels  = [label_map[s] for s in strat_keys if s in sp_data]
sp_values  = [sp_data[s]   for s in strat_keys if s in sp_data]
sp_colors  = [GREEN if s in ("basic","hi-lo") else BLUE for s in strat_keys if s in sp_data]

fig, ax = plt.subplots(figsize=(9, 4.5))
bars = ax.bar(sp_labels, sp_values, color=sp_colors,
              edgecolor="white", linewidth=0.5, width=0.6)
ax.axhline(max(sp_values), color=GRAY, linewidth=0.8, linestyle="--", alpha=0.6)
for bar, val in zip(bars, sp_values):
    ax.text(bar.get_x() + bar.get_width() / 2, val + 0.05,
            f"{val:.2f}×", ha="center", va="bottom", fontsize=9.5)

ax.set_ylabel(f"Speedup  (1 → {max_t} threads)")
ax.set_title(f"Parallel Speedup per Strategy  ({max_t} threads)")
ax.set_ylim(0, max(sp_values) * 1.2)
plt.xticks(rotation=20, ha="right")
ax.grid(axis="y", alpha=0.3)

fig.tight_layout()
p = f"{out_dir}/chart4_speedup_per_strategy.png"
fig.savefig(p); print(f"Saved: {p}")
plt.close()

effs = [(r[0], (r[2] / r[0]) * 100) for r in ss_rows]
ts   = [int(e[0]) for e in effs]
eff_vals = [e[1] for e in effs]

fig, ax = plt.subplots(figsize=(8, 4.5))
ax.axhline(100, color=BLUE, linewidth=1, linestyle=":", label="Perfect efficiency")
ax.plot(ts, eff_vals, color=RED, linewidth=2.5, marker="^", markersize=7, label="Parallel efficiency")
for t, e in zip(ts, eff_vals):
    ax.annotate(f"{e:.0f}%", (t, e), textcoords="offset points",
                xytext=(5, 4), fontsize=9)

ax.set_xlabel("Threads")
ax.set_ylabel("Parallel Efficiency  (Speedup / N × 100%)")
ax.set_title("Parallel Efficiency vs Thread Count")
ax.set_xticks(ts)
ax.set_xlim(0.5, max(ts) + 0.5)
ax.set_ylim(0, 115)
ax.legend(fontsize=10)
ax.grid(axis="y", alpha=0.3)

fig.tight_layout()
p = f"{out_dir}/chart5_efficiency.png"
fig.savefig(p); print(f"Saved: {p}")
plt.close()

print(f"\nAll charts saved to {out_dir}/")
print("Files:")
for f in sorted(glob.glob(f"{out_dir}/chart*.png")):
    print(f"  {f}")
