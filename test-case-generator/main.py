"""
Main driver: generates valid test-case graphs for each of the 6
categories, subject to constraints, and writes them to disk in the
required input format, sorted ascending by (#faces, max face size).

CONSTRAINTS CAN BE SET GLOBALLY *AND* OVERRIDDEN PER CATEGORY.

There are two ways to configure this:

  1. A JSON config file (see config.json for a template). Each category
     can override any of: count, max_faces, max_vertices_in_face,
     max_vertices, min_vertices, seed. Any field left out of a category's
     entry falls back to the "defaults" block. This is the recommended
     way to tune things category-by-category (e.g. category 3 usually
     needs a larger max_vertices_in_face than the others, since
     subdividing 3x stretches every face boundary to 4x its original
     length).

  2. Command-line flags, which set the GLOBAL defaults for a quick
     one-off run without needing to edit a file. These are overridden by
     anything explicitly set in the config file's per-category section.

Usage:
    python3 main.py                              # uses config.json if present, else built-in defaults
    python3 main.py --config myconfig.json       # use a specific config file
    python3 main.py --max-faces 10 --count 50    # quick global override, no config file needed
"""

import os
import json
import argparse
import warnings

warnings.filterwarnings("ignore", message="The hashes produced for graphs")

from common import Constraints, save_sorted_instances
from gen_triangulation_subdivide import generate_category as gen_subdiv
from gen_halin import generate_category as gen_halin
from gen_cycles import generate_category_cycles, generate_category_union


CATEGORY_ORDER = [
    "1_triangulation_subdivide_1",
    "2_triangulation_subdivide_2",
    "3_triangulation_subdivide_3",
    "4_halin",
    "5_cycles",
    "6_cycle_union",
]

CATEGORY_LABELS = {
    "1_triangulation_subdivide_1": "Category 1: Delaunay triangulation, subdivide each edge once",
    "2_triangulation_subdivide_2": "Category 2: Delaunay triangulation, subdivide each edge twice",
    "3_triangulation_subdivide_3": "Category 3: Delaunay triangulation, subdivide each edge three times",
    "4_halin": "Category 4: Halin graphs (random tree + leaf cycle)",
    "5_cycles": "Category 5: Cycles C_n, n = 3..15",
    "6_cycle_union": "Category 6: Union of k cycles (k=2..4, sizes 4..12)",
}


def load_config(config_path, cli_defaults):
    """
    Load a JSON config file (if it exists) and merge it with CLI-supplied
    global defaults. Returns (defaults_dict, per_category_overrides_dict).

    If no config file is found, every category simply uses cli_defaults.
    """
    defaults = dict(cli_defaults)
    categories = {name: {} for name in CATEGORY_ORDER}

    if config_path and os.path.exists(config_path):
        with open(config_path) as f:
            cfg = json.load(f)

        # CLI flags are the starting point; config file "defaults" block
        # overrides them (config file is the more explicit, intentional
        # source of truth when present).
        defaults.update(cfg.get("defaults", {}))

        for name, overrides in cfg.get("categories", {}).items():
            if name not in categories:
                print(f"  [warning] config.json references unknown category '{name}', ignoring.")
                continue
            # strip any "_comment*" keys, which are just documentation
            clean = {k: v for k, v in overrides.items() if not k.startswith("_comment")}
            categories[name] = clean

    return defaults, categories


def resolve_category_settings(name, defaults, categories):
    """Merge global defaults with this category's specific overrides."""
    settings = dict(defaults)
    settings.update(categories.get(name, {}))
    return settings


def build_constraints(settings):
    return Constraints(
        max_faces=settings.get("max_faces"),
        max_vertices_in_face=settings.get("max_vertices_in_face"),
        max_vertices=settings.get("max_vertices"),
        min_vertices=settings.get("min_vertices", 3),
    )


def run_category(name, settings, out_root):
    constraints = build_constraints(settings)
    count = settings.get("count", 100)
    seed = settings.get("seed", 42)

    print(f"=== {CATEGORY_LABELS[name]} ===")
    print(
        f"  settings: count={count} max_faces={constraints.max_faces} "
        f"max_vertices_in_face={constraints.max_vertices_in_face} "
        f"max_vertices={constraints.max_vertices} seed={seed}"
    )

    if name == "1_triangulation_subdivide_1":
        inst = gen_subdiv(k=1, count=count, constraints=constraints, seed=seed)
    elif name == "2_triangulation_subdivide_2":
        inst = gen_subdiv(k=2, count=count, constraints=constraints, seed=seed)
    elif name == "3_triangulation_subdivide_3":
        inst = gen_subdiv(k=3, count=count, constraints=constraints, seed=seed)
    elif name == "4_halin":
        inst = gen_halin(count=count, constraints=constraints, seed=seed)
    elif name == "5_cycles":
        inst = generate_category_cycles(count=count, constraints=constraints, seed=seed)
    elif name == "6_cycle_union":
        inst = generate_category_union(count=count, constraints=constraints, seed=seed)
    else:
        raise ValueError(f"Unknown category: {name}")

    save_sorted_instances(inst, os.path.join(out_root, name), "case")
    print(f"  saved {len(inst)} graphs (requested {count})\n")
    return len(inst)


def main():
    parser = argparse.ArgumentParser(
        description="Biconnected planar graph test-case generator "
                     "(supports per-category constraint overrides via config.json)"
    )
    parser.add_argument("--config", type=str, default="config.json",
                         help="path to JSON config file with per-category overrides "
                              "(default: config.json in the current directory; "
                              "if missing, falls back to CLI flags / built-in defaults)")
    parser.add_argument("--count", type=int, default=100, help="global default: graphs per category")
    parser.add_argument("--out", type=str, default="input", help="input root folder")
    parser.add_argument("--max-faces", type=int, default=15, help="global default: max faces incl. outer face")
    parser.add_argument("--max-face-vertices", type=int, default=40, help="global default: max vertices in any single face")
    parser.add_argument("--max-vertices", type=int, default=200, help="global default: max total vertices in graph")
    parser.add_argument("--seed", type=int, default=42, help="global default: base random seed")
    args = parser.parse_args()

    cli_defaults = {
        "count": args.count,
        "max_faces": args.max_faces,
        "max_vertices_in_face": args.max_face_vertices,
        "max_vertices": args.max_vertices,
        "min_vertices": 3,
        "seed": args.seed,
    }

    defaults, categories = load_config(args.config, cli_defaults)

    if os.path.exists(args.config):
        print(f"Loaded config from: {os.path.abspath(args.config)}")
    else:
        print(f"No config file found at '{args.config}' -- using CLI flags / built-in defaults for all categories.")
    print(f"Global defaults: {defaults}\n")

    out_root = args.out
    os.makedirs(out_root, exist_ok=True)

    results = {}
    for name in CATEGORY_ORDER:
        settings = resolve_category_settings(name, defaults, categories)
        # give each category a distinct effective seed derived from its
        # base seed, so categories don't accidentally share a random
        # stream even if the user set the same "seed" everywhere
        settings = dict(settings)
        settings["seed"] = settings.get("seed", 42) + CATEGORY_ORDER.index(name)
        results[name] = run_category(name, settings, out_root)

    print("All categories complete. Output written under:", os.path.abspath(out_root))
    print("\nSummary:")
    for name in CATEGORY_ORDER:
        print(f"  {name}: {results[name]} graphs")


if __name__ == "__main__":
    main()
