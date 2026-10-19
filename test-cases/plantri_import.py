#!/usr/bin/env python3
"""Exhaustive small-n inputs via plantri (Brinkmann & McKay).

    python plantri_import.py --plantri ./plantri --kind biconnected  --nmin 3 --nmax 7 --out input/Biconnected/Biconnected_plantri
    python plantri_import.py --plantri ./plantri --kind oneconnected --nmin 3 --nmax 7 --out input/Oneconnected/Oneconnected_plantri

plantri -p enumerates simple plane graphs. For connectivity below 3 its
isomorphism is of the *embedding*: the same abstract graph can be written
more than once, and -o would split mirror images as well. This importer
keeps one embedding per abstract isomorphism class (OEIS A021103 for
2-connected graphs; connected graphs A003094 minus those, for cut-vertex
graphs) and refuses to finish if the class count disagrees. That is the
difference between, at 7 vertices,

    5     maximal triangulations          (plantri 7)
    27    disk triangulations             (plantri -P 7)
    294   abstract 2-connected simple     (what --kind biconnected writes)
    564   embedded 2-connected maps       (plantri -p -c2 7, before this filter)
"""
import argparse, subprocess, sys, random
from pathlib import Path
import networkx as nx
from planar_common import validate, write_graph, mirror

LET = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
IDX = {c: i for i, c in enumerate(LET)}

# Abstract isomorphism classes, not embedded maps and not O-P classes.
# biconnected: OEIS A021103. oneconnected: A003094(n) - A021103(n).
EXPECTED = {
    "biconnected": {3: 1, 4: 3, 5: 9, 6: 44, 7: 294, 8: 2893},
    "oneconnected": {3: 1, 4: 3, 5: 11, 6: 55, 7: 352, 8: 3081},
}


def plantri_flags(kind, n):
    """Switches for one abstract class of simple plane graphs.

    -c1x asks for connectivity exactly 1. For n <= 3 plantri's polytope
    generator overwrites that exact-connectivity target with 3 before
    writing the star, so -c1x emits nothing and the 3-vertex path is lost.
    -c1 still writes the path, together with the triangle; the triangle is
    biconnected and is dropped while reading.
    """
    if kind == "biconnected":
        return ["-p", "-c2", "-a"]
    if n <= 3:
        return ["-p", "-c1", "-a"]
    return ["-p", "-c1x", "-a"]


def parse_line(line):
    n_s, rest = line.split(None, 1)
    n = int(n_s)
    lists = [s for s in rest.strip().split(",") if s != ""]
    if len(lists) != n:
        raise ValueError(f"bad plantri line (got {len(lists)} lists for n={n})")
    if n > len(LET):
        raise ValueError(f"ascii plantri format only covers n<={len(LET)}")
    rot = {}
    for i, s in enumerate(lists):
        try:
            rot[i] = [IDX[c] for c in s]
        except KeyError as e:
            raise ValueError(f"bad vertex letter {e} in plantri line") from e
    return rot


def _graph_of(rot):
    G = nx.Graph()
    G.add_nodes_from(rot)
    G.add_edges_from((v, w) for v, nbrs in rot.items() for w in nbrs if v < w)
    return G


class _IsoFilter:
    """One representative per abstract isomorphism class."""

    def __init__(self):
        self._buckets = {}

    def keep(self, G):
        degree_seq = tuple(sorted(d for _, d in G.degree()))
        sig = (G.number_of_nodes(), G.number_of_edges(), degree_seq,
               nx.weisfeiler_lehman_graph_hash(G, iterations=4))
        bucket = self._buckets.setdefault(sig, [])
        if any(nx.is_isomorphic(G, old) for old in bucket):
            return False
        bucket.append(G)
        return True


def classes_from_lines(lines, kind):
    """Parse plantri ascii lines into one rotation per abstract isomorphism class.

    A biconnected graph in a one-connected run is skipped (n <= 3 is asked
    for with -c1, which also writes the triangle). Any other validation
    failure is raised: dropping those lines is what used to move the file
    count off the census.
    """
    raw = skipped = 0
    kept = []
    iso = _IsoFilter()
    for line in lines:
        line = line.strip()
        if not line or not line[0].isdigit():
            continue
        raw += 1
        rot = parse_line(line)
        try:
            validate(rot, kind)
        except ValueError as e:
            if kind == "oneconnected" and str(e) == "has no cut vertex":
                skipped += 1
                continue
            raise
        if iso.keep(_graph_of(rot)):
            kept.append(rot)
    return kept, raw, skipped


def convert(lines, kind, out, base, orientation, seed, max_files):
    rng = random.Random(seed)
    kept, raw, skipped = classes_from_lines(lines, kind)
    out.mkdir(parents=True, exist_ok=True)
    made = 0
    for rot in kept:
        if orientation == "ccw" or (orientation == "mixed" and rng.random() < .5):
            rot = mirror(rot)
        write_graph(rot, out / f"plantri_n{len(rot)}_{made:07d}.txt", base)
        made += 1
        if made >= max_files:
            break
    return made, raw, len(kept), skipped


def _check_census(kind, n, n_classes):
    expected = EXPECTED.get(kind, {}).get(n)
    if expected is None:
        return
    if n_classes != expected:
        sys.exit(
            f"n={n} {kind}: kept {n_classes} abstract isomorphism classes, "
            f"census expects {expected}."
        )


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--plantri", default="plantri")
    ap.add_argument("--stdin", action="store_true")
    ap.add_argument("--kind", choices=["biconnected", "oneconnected"], required=True)
    ap.add_argument("--nmin", type=int, default=3)
    ap.add_argument("--nmax", type=int, default=7)
    ap.add_argument("--out", default="input/plantri")
    ap.add_argument("--base", type=int, default=0, choices=[0, 1])
    ap.add_argument("--orientation", choices=["cw", "ccw", "mixed"], default="mixed")
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--max-files", type=int, default=10**9)
    a = ap.parse_args()
    out = Path(a.out)
    category_prefix = f"{a.kind.capitalize()}_"
    if not out.name.startswith(category_prefix):
        out = out.parent / f"{category_prefix}{out.name}"
    if a.stdin:
        try:
            made, raw, n_classes, skipped = convert(
                sys.stdin, a.kind, out, a.base, a.orientation, a.seed, a.max_files)
        except ValueError as e:
            sys.exit(f"plantri line rejected: {e}")
        print(f"stdin: wrote {made} files from {n_classes} isomorphism classes "
              f"({raw} plantri lines, {skipped} biconnected ignored)")
        sys.exit()
    for n in range(a.nmin, a.nmax + 1):
        flags = plantri_flags(a.kind, n)
        p = subprocess.run([a.plantri, *flags, str(n)], capture_output=True, text=True)
        if p.returncode:
            sys.exit(f"plantri failed at n={n}: {p.stderr}")
        try:
            made, raw, n_classes, skipped = convert(
                p.stdout.splitlines(), a.kind, out, a.base, a.orientation, a.seed + n, a.max_files)
        except ValueError as e:
            sys.exit(f"n={n}: plantri output rejected ({e})")
        _check_census(a.kind, n, n_classes)
        note = p.stderr.strip().splitlines()
        tail = note[-1] if note else ""
        extra = f", {skipped} biconnected ignored" if skipped else ""
        print(f"n={n}: {n_classes} isomorphism classes, {made} files "
              f"({raw} plantri maps{extra}) {tail}")
