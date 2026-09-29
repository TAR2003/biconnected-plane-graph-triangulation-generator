#!/usr/bin/env python3
"""Exhaustive small-n inputs via plantri (Brinkmann & McKay; compile it yourself).
    python plantri_import.py --plantri ./plantri --kind biconnected  --nmin 4 --nmax 10 --out inputs/plantri_bicon
    python plantri_import.py --plantri ./plantri --kind oneconnected --nmin 3 --nmax 9  --out inputs/plantri_onecon
    plantri -a ... | python plantri_import.py --stdin --kind biconnected --out X     (parse existing output)
plantri's ASCII line is:  <n> <nbrs of a>,<nbrs of b>,...   (vertices a,b,c,..; a-z then A-Z, n<=52),
each list being that vertex's rotation. Every graph goes through the same Euler/connectivity validation as
the random generators, so an orientation or format misunderstanding fails loudly instead of silently."""
import argparse, subprocess, sys, random
from pathlib import Path
from planar_common import validate, write_graph, mirror

LET = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
IDX = {c: i for i, c in enumerate(LET)}


def parse_line(line):
    n_s, rest = line.split(None, 1); n = int(n_s)
    lists = rest.strip().split(",")
    if len(lists) != n: raise ValueError("bad plantri line")
    return {i: [IDX[c] for c in s] for i, s in enumerate(lists)}


def convert(lines, kind, out, n, base, orientation, seed, max_files):
    rng = random.Random(seed); made = skipped = 0
    for line in lines:
        line = line.strip()
        if not line or not line[0].isdigit(): continue
        try:
            rot = parse_line(line); validate(rot, kind)
        except ValueError as e:
            skipped += 1          # e.g. -pc1 output includes graphs that are actually biconnected
            continue
        if orientation == "ccw" or (orientation == "mixed" and rng.random() < .5): rot = mirror(rot)
        d = out / f"n{len(rot)}"; d.mkdir(parents=True, exist_ok=True)
        write_graph(rot, d / f"plantri_n{len(rot)}_{made:07d}.txt", base); made += 1
        if made >= max_files: break
    return made, skipped


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--plantri", default="plantri"); ap.add_argument("--stdin", action="store_true")
    ap.add_argument("--kind", choices=["biconnected", "oneconnected"], required=True)
    ap.add_argument("--nmin", type=int, default=4); ap.add_argument("--nmax", type=int, default=10)
    ap.add_argument("--out", default="input/plantri"); ap.add_argument("--base", type=int, default=0, choices=[0, 1])
    ap.add_argument("--orientation", choices=["cw", "ccw", "mixed"], default="mixed")
    ap.add_argument("--seed", type=int, default=1); ap.add_argument("--max-files", type=int, default=10**9)
    a = ap.parse_args(); out = Path(a.out)
    if a.stdin:
        print(convert(sys.stdin, a.kind, out / "stdin", 0, a.base, a.orientation, a.seed, a.max_files)); sys.exit()
    flags = "-pc2m2a" if a.kind == "biconnected" else "-pc1m1a"     # -p planar, -c conn., -m min degree, -a ascii
    for n in range(a.nmin, a.nmax + 1):
        p = subprocess.run([a.plantri, flags, str(n)], capture_output=True, text=True)
        if p.returncode: sys.exit(f"plantri failed at n={n}: {p.stderr}")
        made, skipped = convert(p.stdout.splitlines(), a.kind, out, n, a.base, a.orientation, a.seed + n, a.max_files)
        print(f"n={n}: {made} files ({skipped} rejected by validation)")
