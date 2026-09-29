#!/usr/bin/env python3
"""One-connected plane-graph generator: connected, >=1 cut vertex (n>=3).
Trees, cacti, and block-cut-tree gluing of random biconnected blocks / bridges
at random corners (so the cut vertex's rotation and the face hosting each block are random).
Usage: python gen_oneconnected.py --out inputs/oneconnected --sizes 10 100 1000 --count 20
"""
import random
from planar_common import run
from gen_biconnected import BICONNECTED, g_cycle

K2 = {0: [1], 1: [0]}


def glue(rot, v, blk, u, rng):
    """Identify vertex u of block blk with vertex v of rot; blk is drawn inside a random
    face at v (random cyclic shift) inserted at a random corner of v's rotation."""
    c, m = len(rot), {}
    for b in sorted(blk):
        if b == u: m[b] = v
        else: m[b] = c; c += 1
    ins = [m[w] for w in blk[u]]
    k = rng.randrange(len(ins)); ins = ins[k:] + ins[:k]
    p = rng.randint(0, len(rot[v]))
    rot[v][p:p] = ins
    for b, r in blk.items():
        if b != u: rot[m[b]] = [m[w] for w in r]
    return [m[b] for b in blk if b != u]


def random_block(size, rng, types):
    if size == 2: return K2
    names = list(types); rng.shuffle(names)
    for nm in names:
        try:
            b = BICONNECTED[nm](size, rng)
        except Exception:
            b = None
        if b is not None and len(b) == size: return b
    return g_cycle(size, rng)


def build(n, rng, attach, types, k2_prob=None, big_first=False, cactus=False):
    smax = rng.choice([3, 4, 6, 10, 25, max(3, n // 4), max(3, n // 2)])
    p2 = rng.random() if k2_prob is None else k2_prob
    types = ["cycle"] if cactus else types

    def draw():
        return 2 if rng.random() < p2 else rng.randint(3, max(3, smax))
    s0 = min(n - 1, max(3, n // 2) if big_first else draw())
    if s0 >= 3 and cactus: s0 = min(s0, n - 1)
    rot = {v: list(r) for v, r in random_block(max(2, s0), rng, types).items()}
    last, hub = list(rot), rng.choice(list(rot))
    pool = [v for v in rot for _ in range(len(rot[v]))]
    while len(rot) < n:
        rem = n - len(rot)
        s = min(2 if big_first else draw(), rem + 1)
        if cactus and s == 2: s = min(3, rem + 1)
        if s == 2 and cactus: s = 2
        blk = random_block(s, rng, types)
        u = rng.choice(list(blk))
        v = {"random": lambda: rng.randrange(len(rot)), "chain": lambda: rng.choice(last),
             "star": lambda: hub, "pref": lambda: rng.choice(pool)}[attach]()
        last = glue(rot, v, blk, u, rng)
        pool += [v] * len(blk[u]) + [w for w in last for _ in range(len(rot[w]))]
    return rot


# ------------------------------------------------------------ trees
def _tree(n, parent):
    rot = {i: [] for i in range(n)}
    for i in range(1, n):
        p = parent(i); rot[i].append(p); rot[p].append(i)
    return rot

def _tree_family(kind):
    def f(n, rng):
        if n < 3: return None
        if kind == "path": rot = _tree(n, lambda i: i - 1)
        elif kind == "star": rot = _tree(n, lambda i: 0)
        elif kind == "binary": rot = _tree(n, lambda i: (i - 1) // 2)
        elif kind == "random": rot = _tree(n, lambda i: rng.randrange(i))
        elif kind == "deep_random": rot = _tree(n, lambda i: rng.randint(max(0, i - 3), i - 1))
        elif kind == "caterpillar":
            L = rng.randint(2, n - 1); rot = _tree(n, lambda i: i - 1 if i < L else rng.randrange(L))
        elif kind == "spider":
            k = rng.randint(2, max(2, min(n - 1, 12))); rot = _tree(n, lambda i: 0 if i <= k else i - k)
        for r in rot.values(): rng.shuffle(r)      # random planar embedding
        return rot
    return f


def _mixed(attach, **kw):
    return lambda n, rng: None if n < 3 else build(n, rng, attach, list(BICONNECTED), **kw)

# ------------------------------------------------------------ asymmetric block trees
def g_comet(n, rng):
    """One dense stacked-triangulation core (~90% of vertices) + a long chain tail of
    small blocks (bridges / triangles)."""
    if n < 4: return None
    k = min(n - 1, max(3, int(0.9 * n)))
    rot = {v: list(r) for v, r in BICONNECTED["stacked_triangulation"](k, rng).items()}
    last = [rng.randrange(k)]
    while len(rot) < n:
        rem = n - len(rot)
        s = 2 if (rem == 1 or rng.random() < 0.5) else 3
        blk = random_block(s, rng, ["cycle"])
        last = glue(rot, rng.choice(last), blk, rng.choice(list(blk)), rng)
    return rot

def g_hub_asymmetric(n, rng):
    """One cut vertex shared by 1 huge block (~n/2 vertices) and O(n) tiny K3 / K2 blocks."""
    if n < 4: return None
    k = min(n - 1, max(3, n // 2))
    rot = {v: list(r) for v, r in random_block(k, rng, list(BICONNECTED)).items()}
    hub = rng.randrange(k)
    while len(rot) < n:
        blk = random_block(3 if n - len(rot) >= 2 else 2, rng, ["cycle"])
        glue(rot, hub, blk, rng.choice(list(blk)), rng)
    return rot

ONECONNECTED = {f"tree_{k}": _tree_family(k) for k in
                ["path", "star", "binary", "random", "deep_random", "caterpillar", "spider"]}
ONECONNECTED.update({
    "cactus_random": lambda n, rng: None if n < 3 else build(n, rng, "random", [], cactus=True),
    "cactus_chain":  lambda n, rng: None if n < 3 else build(n, rng, "chain", [], cactus=True),
    "cactus_star":   lambda n, rng: None if n < 3 else build(n, rng, "star", [], cactus=True),
    "blocks_random": _mixed("random"), "blocks_chain": _mixed("chain"),
    "blocks_star": _mixed("star"),     "blocks_pref": _mixed("pref"),
    "blocks_bridges": _mixed("random", k2_prob=0.5),
    "core_pendants": _mixed("random", big_first=True),
    "comet": g_comet, "hub_asymmetric": g_hub_asymmetric,
})

if __name__ == "__main__":
    run("oneconnected", ONECONNECTED)
