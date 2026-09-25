"""Generate complete size series for the one-connected star and tree cases."""

from pathlib import Path


ROOT = Path(__file__).parent / "input" / "Oneconnected"


def write_case(path: Path, face: list[int]) -> None:
    path.write_text(f"1\n{len(face)}\n" + " ".join(map(str, face)) + "\n")


def star_face(vertex_count: int) -> list[int]:
    return [vertex for leaf in range(2, vertex_count + 1) for vertex in (1, leaf)]


def tree_face(vertex_count: int) -> list[int]:
    children = [[] for _ in range(vertex_count + 1)]
    for vertex in range(2, vertex_count + 1):
        parent = vertex // 2
        children[parent].append(vertex)

    face = [1]

    def walk(parent: int) -> None:
        for child in children[parent]:
            face.append(child)
            walk(child)
            face.append(parent)

    walk(1)
    face.pop()
    return face


def main() -> None:
    star_dir = ROOT / "02_star"
    tree_dir = ROOT / "03_tree"
    star_dir.mkdir(parents=True, exist_ok=True)
    tree_dir.mkdir(parents=True, exist_ok=True)

    for vertex_count in range(3, 61):
        star_path = star_dir / f"star_{vertex_count}.txt"
        if not star_path.exists():
            write_case(star_path, star_face(vertex_count))

        tree_path = tree_dir / f"tree_{vertex_count}.txt"
        if not tree_path.exists():
            write_case(tree_path, tree_face(vertex_count))


if __name__ == "__main__":
    main()