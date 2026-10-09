#!/usr/bin/env python3
"""
Sweep SLEEP_AFTER_TICKS, P_BIG and P_SMALL in YOUR scheduler.cpp and rank every
combination by energy x delay (E*D). No other source file is needed.

Requirements in scheduler.cpp:
  * three lines of the form  "#define SLEEP_AFTER_TICKS 16", "#define P_BIG P3",
    "#define P_SMALL P3"  (each on its own line, starting with #define)
  * SimulationComplete prints either
        RAW energy=<number> raw_time=<number>        (preferred, more precise)
    or the original "Run stopped at HH:MM:SS after consuming X kWh" line.

For each combination it rewrites those #define lines, rebuilds, runs, parses the result.
Your scheduler.cpp is restored when the script ends (also saved to scheduler.cpp.sweep_bak).
Core order (small-first / big-first) is NOT swept; change the loops by hand if you want that.
(LLM-assisted creation of this test script)

Example:
  python3 sweep.py --run "./simulator" --sleep 8,16 --p-big P2,P3 --p-small P3
"""
import argparse, csv, itertools, re, shutil, subprocess, sys

RAW = re.compile(r"RAW energy=(\S+) raw_time=(\S+)")
STOP = re.compile(r"Run stopped at (\d+):(\d+):(\d+) after consuming (\S+) kWh")

def set_define(text, name, value):
    pat = re.compile(rf"^[ \t]*#define[ \t]+{name}\b.*$", re.M)
    if not pat.search(text):
        sys.exit(f"scheduler.cpp has no '#define {name} ...' line; add one and rerun.")
    return pat.sub(f"#define {name} {value}", text, count=1)

def get_define(text, name):
    m = re.search(rf"^[ \t]*#define[ \t]+{name}[ \t]+(\S+)", text, re.M)
    return m.group(1) if m else None

def parse(output):
    m = RAW.search(output)
    if m:
        e, t = float(m.group(1)), float(m.group(2))
        return e, t, e * t, "raw"
    m = STOP.search(output)
    if m:
        h, mi, s = int(m.group(1)), int(m.group(2)), int(m.group(3))
        sec = h * 3600 + mi * 60 + s
        e = float(m.group(4))
        return e, sec, e * sec, "kWh*s"
    return None

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--target", default="scheduler.cpp")
    ap.add_argument("--build", default="make", help="build command (check the README)")
    ap.add_argument("--run", required=True, help="command that runs the simulator (check the README)")
    ap.add_argument("--timeout", type=int, default=600, help="seconds before a run counts as hung")
    ap.add_argument("--sleep", default="0,8,16,32")
    ap.add_argument("--p-big", default="P2,P3,P4")
    ap.add_argument("--p-small", default="P2,P3,P4")
    ap.add_argument("--out", default="sweep_results.csv")
    a = ap.parse_args()

    original = open(a.target).read()
    shutil.copy(a.target, a.target + ".sweep_bak")      # extra safety copy (overwritten each run)
    mine = (get_define(original, "SLEEP_AFTER_TICKS"), get_define(original, "P_BIG"),
            get_define(original, "P_SMALL"))

    combos = list(itertools.product(a.sleep.split(","), a.p_big.split(","), a.p_small.split(",")))
    print(f"{len(combos)} combinations")
    rows = []
    try:
        for i, (sl, pb, ps) in enumerate(combos, 1):
            text = set_define(original, "SLEEP_AFTER_TICKS", sl)
            text = set_define(text, "P_BIG", pb)
            text = set_define(text, "P_SMALL", ps)
            with open(a.target, "w") as f:
                f.write(text)                            # rewriting updates mtime so make rebuilds
            b = subprocess.run(a.build, shell=True, capture_output=True, text=True)
            e = t = edp = unit = ""
            if b.returncode != 0:
                print("BUILD FAILED:\n" + (b.stderr or b.stdout)[:2000])
                if not rows:
                    sys.exit(1)                          # first build failing = setup problem
                status = "BUILD_FAIL"
            else:
                try:
                    r = subprocess.run(a.run, shell=True, capture_output=True, text=True,
                                       timeout=a.timeout)
                    p = parse(r.stdout + r.stderr)
                    if p:
                        status, (e, t, edp, unit) = "ok", p
                    else:
                        status = "NO_RESULT_LINE"
                except subprocess.TimeoutExpired:
                    status = "TIMEOUT"
            rows.append(dict(sleep=sl, p_big=pb, p_small=ps, status=status,
                             energy=e, time=t, edp=edp, unit=unit))
            print(f"[{i}/{len(combos)}] sleep={sl} Pbig={pb} Psmall={ps} -> {status} {edp}")
    finally:
        with open(a.target, "w") as f:
            f.write(original)                            # always restore your file

    with open(a.out, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        w.writeheader(); w.writerows(rows)

    ok = sorted((r for r in rows if r["status"] == "ok"), key=lambda r: float(r["edp"]))
    print("\nBest 10 by E*D (lower is better):")
    for r in ok[:10]:
        print(r)
    mine_row = next((r for r in ok if (r["sleep"], r["p_big"], r["p_small"]) == mine), None)
    if mine_row:
        print("\nYour file's current settings:", mine_row)
    print(f"\nAll results saved to {a.out}")
    print("Note: E*D is only comparable between rows of the same unit column.")

if __name__ == "__main__":
    main()