#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/.."

BIN="./build-release/blackjack_simulator"
MAX_THREADS=$(nproc)
RESULTS_DIR="bench/results"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
OUT="$RESULTS_DIR/bench_$TIMESTAMP.txt"

GAMES_STRONG=50000
ROUNDS=100
GAMES_PER_THREAD=5000

DO_PERF=true
DO_FLAMEGRAPH=false

for arg in "$@"; do
    case $arg in
        --quick)      GAMES_STRONG=10000; GAMES_PER_THREAD=1000 ;;
        --no-perf)    DO_PERF=false ;;
        --flamegraph) DO_FLAMEGRAPH=true ;;
    esac
done

sep()  { printf '%s\n' "────────────────────────────────────────────────────────" | tee -a "$OUT"; }
hdr()  { echo; sep; printf "  %s\n" "$1" | tee -a "$OUT"; sep; }
log()  { printf '%s\n' "$@" | tee -a "$OUT"; }

wall_1t()  { echo "$1" | grep "Wall time:"  | head -1 | awk '{print $(NF-1)}'; }
wall_nt()  { echo "$1" | grep "Wall time:"  | tail -1 | awk '{print $(NF-1)}'; }
ev_hand()  { echo "$1" | grep "EV per hand:" | head -1 | awk '{print $NF}'; }

mkdir -p "$RESULTS_DIR"
: > "$OUT"

log "Blackjack Simulator Benchmark Suite"
log "Date:    $(date)"
log "Host:    $(hostname)"
log "CPU:     $(grep 'model name' /proc/cpuinfo | head -1 | cut -d: -f2 | xargs)"
log "Cores:   $MAX_THREADS"
log "Binary:  $BIN"
log ""

echo "Building release binary..."
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_CXX_FLAGS="-O3 -march=native -g" \
      > /dev/null 2>&1
cmake --build build-release --target blackjack_simulator > /dev/null 2>&1
log "Build: OK  (Release, -O3 -march=native -g)"

hdr "1. EV Comparison — All Strategies (${GAMES_STRONG} games × ${ROUNDS} rounds, 1 thread)"
log ""
log "Strategy           EV/hand (cents)   1T wall (ms)"
log "─────────────────  ───────────────   ────────────"

for s in always-stand bearish mimic-dealer bullish double-first surrender-first basic hi-lo; do
    raw=$("$BIN" --games "$GAMES_STRONG" --rounds "$ROUNDS" --threads 1 --strategy "$s" 2>/dev/null)
    ev=$(ev_hand  "$raw")
    ms=$(wall_1t  "$raw")
    printf "%-18s  %15s   %s\n" "$s" "$ev" "$ms" | tee -a "$OUT"
done

hdr "2. Strong Scaling — basic strategy (${GAMES_STRONG} games × ${ROUNDS} rounds, fixed work)"
log ""
log "Threads   Wall time (ms)   Speedup    Efficiency"
log "───────   ──────────────   ───────    ──────────"

t1_ms=""
ms_max=""
for t in 1 2 3 4 6 8 10 12; do
    [ "$t" -gt "$MAX_THREADS" ] && continue
    raw=$("$BIN" --games "$GAMES_STRONG" --rounds "$ROUNDS" --threads "$t" --strategy basic 2>/dev/null)
    ms=$(wall_nt "$raw")
    if [ -z "$t1_ms" ]; then
        t1_ms="$ms"
        speedup="1.000"
        eff="100.0%"
    else
        speedup=$(awk "BEGIN {printf \"%.3f\", $t1_ms / $ms}")
        eff=$(awk     "BEGIN {printf \"%.1f%%\", ($t1_ms / $ms) / $t * 100}")
    fi
    ms_max="$ms"
    printf "%-9s %-16s %-10s %s\n" "$t" "$ms" "$speedup" "$eff" | tee -a "$OUT"
done

serial_frac=$(awk "BEGIN {
    s = $t1_ms / $ms_max
    n = $MAX_THREADS
    f = (1/s - 1/n) / (1 - 1/n)
    if (f < 0) f = 0
    printf \"%.4f\", f
}")
theo_max=$(awk "BEGIN { printf \"%.1f\", 1 / $serial_frac }")
log ""
log "  Serial fraction  f ≈ $serial_frac"
log "  Theoretical max speedup (Amdahl): 1/f = ${theo_max}x"

hdr "3. Weak Scaling — basic strategy (${GAMES_PER_THREAD} games/thread × ${ROUNDS} rounds)"
log ""
log "Threads   Total games   Wall time (ms)   Gustafson efficiency"
log "───────   ───────────   ──────────────   ────────────────────"

t1_weak=""
for t in 1 2 3 4 6 8 10 12; do
    [ "$t" -gt "$MAX_THREADS" ] && continue
    games=$((t * GAMES_PER_THREAD))
    raw=$("$BIN" --games "$games" --rounds "$ROUNDS" --threads "$t" --strategy basic 2>/dev/null)
    ms=$(wall_nt "$raw")
    if [ -z "$t1_weak" ]; then
        t1_weak="$ms"
        eff_g="100.0%"
    else
        eff_g=$(awk "BEGIN {printf \"%.1f%%\", $t1_weak / ($t * $ms) * 100}")
    fi
    printf "%-9s %-13s %-16s %s\n" "$t" "$games" "$ms" "$eff_g" | tee -a "$OUT"
done

hdr "4. Speedup per Strategy — ${GAMES_STRONG} games × ${ROUNDS} rounds"
log ""
log "Strategy           1T (ms)   ${MAX_THREADS}T (ms)   Speedup"
log "─────────────────  ───────   ──────────   ───────"

for s in always-stand bearish mimic-dealer bullish double-first surrender-first basic hi-lo; do
    rawN=$("$BIN" --games "$GAMES_STRONG" --rounds "$ROUNDS" --threads "$MAX_THREADS" --strategy "$s" 2>/dev/null)
    ms1=$(wall_1t "$rawN")
    msN=$(wall_nt "$rawN")
    spd=$(awk "BEGIN {printf \"%.3f\", $ms1 / $msN}")
    printf "%-18s %-9s %-12s %s\n" "$s" "$ms1" "$msN" "$spd" | tee -a "$OUT"
done

if $DO_PERF; then
    if ! command -v perf &>/dev/null; then
        hdr "5. Hardware Counters (perf stat)"
        log "[SKIP] perf not found"
    else
        hdr "5. Hardware Counters (perf stat) — basic, ${GAMES_STRONG} games"
        log ""
        log "--- 1 thread ---"
        perf stat -e cycles,instructions,cache-misses,cache-references,task-clock \
            "$BIN" --games "$GAMES_STRONG" --rounds "$ROUNDS" --threads 1 --strategy basic \
            2>&1 | grep -E "cycles|instructions|cache-misses|cache-references|task-clock|IPC" \
                 | tee -a "$OUT"
        log ""
        log "--- $MAX_THREADS threads ---"
        perf stat -e cycles,instructions,cache-misses,cache-references,task-clock \
            "$BIN" --games "$GAMES_STRONG" --rounds "$ROUNDS" --threads "$MAX_THREADS" --strategy basic \
            2>&1 | grep -E "cycles|instructions|cache-misses|cache-references|task-clock|IPC" \
                 | tee -a "$OUT"
    fi
fi

if $DO_FLAMEGRAPH; then
    hdr "6. Flamegraph"
    FG_DIR=""
    for d in /tmp/FlameGraph ~/FlameGraph; do
        [ -f "$d/flamegraph.pl" ] && FG_DIR="$d" && break
    done
    if [ -z "$FG_DIR" ]; then
        log "Cloning FlameGraph to /tmp/FlameGraph..."
        git clone --depth 1 https://github.com/brendangregg/FlameGraph /tmp/FlameGraph
        FG_DIR="/tmp/FlameGraph"
    fi

    SVG="$RESULTS_DIR/flamegraph_$TIMESTAMP.svg"
    PROFDATA="$RESULTS_DIR/perf_$TIMESTAMP.data"

    log "Recording perf data..."
    perf record -g --call-graph fp -o "$PROFDATA" \
        "$BIN" --games "$GAMES_STRONG" --rounds "$ROUNDS" --threads "$MAX_THREADS" --strategy basic \
        2>/dev/null

    perf script -i "$PROFDATA" 2>/dev/null \
        | "$FG_DIR/stackcollapse-perf.pl" \
        | "$FG_DIR/flamegraph.pl" > "$SVG"

    log "Flamegraph saved: $SVG  (open in a browser)"
fi

hdr "Done"
log "Results saved to: $OUT"
log ""
log "Presentation data guide:"
log "  Section 1  →  bar chart: EV per hand across strategies"
log "  Section 2  →  line chart: speedup vs threads (Amdahl curve)"
log "                quote f=$serial_frac, theo max=${theo_max}x"
log "  Section 3  →  line chart: time vs threads at scaled work (Gustafson)"
log "  Section 4  →  table: does parallelism benefit all strategies equally?"
log "  Section 5  →  table: IPC and cache-miss rate 1T vs ${MAX_THREADS}T"
log "  --flamegraph  visual hotspot SVG"
