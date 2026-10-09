"""Shared helpers: rotation systems, validation, file output, CLI driver.

Internal representation: rot = {v: [neighbours in CLOCKWISE cyclic order]}, v in 0..n-1.
Output format (one file per graph):
    N
    deg_0 a a a ...      (whole file uses one orientation: cw or ccw)
    ...
"""
import argparse, csv, math, random, sys
from pathlib import Path
import networkx as nx


# ---------------------------------------------------------------- embeddings
def rot_from_graph(G):
    """Planar embedding (cw) from networkx's LR-planarity test."""
    ok, emb = nx.check_planarity(G)
    if not ok:
        raise ValueError("graph not planar")
    return {v: list(emb.neighbors_cw_order(v)) for v in G.nodes}


def rot_from_points(G, pts):
    """Exact cw rotation of a straight-line drawing (angles sorted decreasing)."""
    rot = {}
    for v in G.nodes:
        x0, y0 = pts[v]
        rot[v] = sorted(G[v], key=lambda w: -math.atan2(pts[w][1] - y0, pts[w][0] - x0))
    return rot


def filter_rot(rot, G):
    """Restrict a rotation system to the edges of subgraph G (still a valid embedding)."""
    return {v: [w for w in rot[v] if G.has_edge(v, w)] for v in G.nodes}


def faces(rot):
    pos = {v: {w: i for i, w in enumerate(r)} for v, r in rot.items()}
    seen, out = set(), []
    for u in rot:
        for v in rot[u]:
            if (u, v) in seen:
                continue
            f, a, b = [], u, v
            while (a, b) not in seen:
                seen.add((a, b)); f.append((a, b))
                r = rot[b]
                a, b = b, r[(pos[b][a] + 1) % len(r)]
            out.append(f)
    return out


def dual_rot(rot):
    fs = faces(rot)
    fid = {d: i for i, f in enumerate(fs) for d in f}
    return {i: [fid[(b, a)] for (a, b) in f] for i, f in enumerate(fs)}


def relabel(rot, rng):
    p = list(rot); rng.shuffle(p)
    return {p[v]: [p[w] for w in r] for v, r in rot.items()}


def mirror(rot):
    return {v: r[::-1] for v, r in rot.items()}


# ---------------------------------------------------------------- validation
def validate(rot, kind):
    """Raises ValueError unless rot is a simple, connected plane graph (Euler-checked)
    that is biconnected ('biconnected') or connected-with-a-cut-vertex ('oneconnected')."""
    n = len(rot)
    if sorted(rot) != list(range(n)):
        raise ValueError("labels must be 0..n-1")
    adj = {v: set(r) for v, r in rot.items()}
    m2 = 0
    for v, r in rot.items():
        if len(adj[v]) != len(r) or v in adj[v]:
            raise ValueError("loop or multi-edge")
        if any(v not in adj[w] for w in r):
            raise ValueError("asymmetric adjacency")
        m2 += len(r)
    m = m2 // 2
    G = nx.Graph(); G.add_nodes_from(rot)
    G.add_edges_from((v, w) for v, r in rot.items() for w in r)
    if not nx.is_connected(G):
        raise ValueError("disconnected")
    if n >= 2 and n - m + len(faces(rot)) != 2:
        raise ValueError("Euler check failed: rotation system is not planar")
    if kind == "biconnected" and not (n >= 3 and nx.is_biconnected(G)):
        raise ValueError("not biconnected")
    if kind == "oneconnected" and not (n >= 3 and not nx.is_biconnected(G)):
        raise ValueError("has no cut vertex")
    return m


# ---------------------------------------------------------------- output
def write_graph(rot, path, base=0):
    n = len(rot)
    with open(path, "w") as f:
        f.write(f"{n}\n")
        for v in range(n):
            r = rot[v]
            f.write(" ".join([str(len(r))] + [str(w + base) for w in r]) + "\n")


def run(kind, gens, argv=None, defaults=None):
    defaults = defaults or {}
    ap = argparse.ArgumentParser(description=f"Generate {kind} plane graphs (rotation-system txt files)")
    ap.add_argument("--out", default=defaults.get("out", f"inputs/{kind}"))
    ap.add_argument("--sizes", type=int, nargs="+",
                    default=defaults.get("sizes", [3, 4, 5, 6, 7, 8, 10, 12, 16, 20, 30, 50, 100, 200, 500, 1000]))
    ap.add_argument("--count", type=int, default=defaults.get("count", 10), help="graphs per (family,size)")
    ap.add_argument("--seed", type=int, default=defaults.get("seed", 12345))
    ap.add_argument("--families", nargs="+", default=defaults.get("families"))
    ap.add_argument("--orientation", choices=["cw", "ccw", "mixed"], default="mixed",
                    help="mixed = random per FILE (never within a file)")
    ap.add_argument("--no-relabel", action="store_true", help="keep generator's vertex labels")
    ap.add_argument("--base", type=int, choices=[0, 1], default=0, help="first vertex label")
    ap.add_argument("--retries", type=int, default=25)
    ap.add_argument("--list", action="store_true")
    a = ap.parse_args(argv)

    if a.list:
        print("\n".join(gens)); return
    fams = a.families or list(gens)
    bad = [f for f in fams if f not in gens]
    if bad:
        sys.exit(f"unknown families: {bad}\navailable: {list(gens)}")

    out = Path(a.out); out.mkdir(parents=True, exist_ok=True)
    rows, total = [], 0
    for fam in fams:
        for n in a.sizes:
            category = f"{kind.capitalize()}_{fam}"
            d = out / category
            made = 0
            for i in range(a.count):
                rng = random.Random(f"{a.seed}/{fam}/{n}/{i}")
                rot, unsupported, err = None, False, None
                for _ in range(a.retries):
                    try:
                        cand = gens[fam](n, rng)
                        if cand is None:
                            unsupported = True; break
                        if len(cand) != n:
                            raise ValueError(f"generator returned {len(cand)} vertices, wanted {n}")
                        m = validate(cand, kind); rot = cand; break
                    except Exception as e:
                        err = e
                if unsupported:
                    break
                if rot is None:
                    print(f"  [fail] {fam} n={n} #{i}: {err}", file=sys.stderr); continue
                if not a.no_relabel:
                    rot = relabel(rot, rng)
                ori = a.orientation if a.orientation != "mixed" else rng.choice(["cw", "ccw"])
                if ori == "ccw":
                    rot = mirror(rot)
                d.mkdir(parents=True, exist_ok=True)
                p = d / f"{fam}_n{n}_{i:04d}.txt"
                write_graph(rot, p, a.base)
                degs = [len(r) for r in rot.values()]
                rows.append([str(p.relative_to(out)), category, n, m, 2 - n + m, min(degs), max(degs), ori, a.seed])
                made += 1
            total += made
            print(f"{fam:24s} n={n:<6d} {'skipped (size unsupported)' if made == 0 else f'{made} files'}")
    with open(out / "manifest.csv", "a", newline="") as f:
        w = csv.writer(f)
        if f.tell() == 0:
            w.writerow(["file", "family", "n", "m", "faces", "minDeg", "maxDeg", "orientation", "seed"])
        w.writerows(rows)
    print(f"done: {total} graphs -> {out}")
