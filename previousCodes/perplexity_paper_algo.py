from functools import reduce
import operator

def total_triangulations(all_triangulations):
    counts = [len(triangs) for triangs in all_triangulations]
    return reduce(operator.mul, counts, 1)
from collections import deque

def generate_all_triangulations(n):
    """
    Generate all triangulations of a labeled cycle C with n vertices.
    Vertices are numbered 1..n in counterclockwise order.
    Each triangulation is represented as a list of chords (edges not on the cycle).
    """
    result = []

    # Each triangulation state consists of:
    # - L: list of chords in this triangulation (tuples: (u, v), u < v)
    # - GS: list of generating chords (incident to v1) for this triangulation
    # - O: list of opposite pairs for GS (each: ((a, b), (c, d)))

    # Initialize: Root triangulation - all chords incident to vertex 1
    root_L = []
    root_GS = []
    root_O = []

    for j in range(n - 1, 2, -1):
        root_L.append((1, j))
        root_GS.append((1, j))
        root_O.append((j-1, (j % n) + 1))  # opposite pair (vj-1, vj+1) with index wrap

    def flip(tri, idx):
        """
        Given a triangulation 'tri', generate its child by flipping the generating chord at GS[idx].
        Returns a new state (L, GS, O).
        """
        L, GS, O = tri
        L_new = L.copy()
        GS_new = GS.copy()
        O_new = O.copy()

        flip_chord = GS[idx]
        opp_pair = O[idx]

        # Remove the flipped chord from L and GS, add the new blocking chord
        try:
            L_new.remove(flip_chord)
        except ValueError:
            # Should always be present
            pass
        GS_new.pop(idx)
        O_new.pop(idx)

        new_blocking_chord = tuple(sorted(opp_pair))
        L_new.append(new_blocking_chord)

        # Now, update GS and O for the child triangulation
        # Find blocking chords to update GS accordingly
        # We'll scan GS; new generating chords are those still incident to 1 and j >= b,
        # where (b, b') is the leftmost blocking chord

        # Find leftmost blocking chord (not incident to vertex 1)
        blocking = [e for e in L_new if 1 not in e]
        if blocking:
            leftmost_blocking = min(blocking, key=lambda chord: max(chord[0], chord[1]))  # select per algorithm
            b = min(leftmost_blocking)
        else:
            leftmost_blocking = None
            b = 2

        # New GS: all chords (1, j) with j >= b # chord order: (1, j) with 3 <= j <= n-1
        GS_new = []
        O_new = []

        for e in L_new:
            if e[0] == 1 and e[1] >= b:
                GS_new.append(e)
                # need to infer new opposite pair; reconstruct subpolygon for this chord to find its quad
                # For simplicity, approximate with earlier method
                j = e[1]
                opp = (j-1 if j > 1 else n, (j+1 if j < n else 1))
                if opp[0] == 1:
                    opp = ((j-2) if (j-2) >= 1 else n+(j-2), opp[1])
                if opp[1] == 1:
                    opp = (opp[0], (j+2) if (j+2) <= n else (j+2)%n)
                O_new.append(opp)

        return (list(sorted(L_new)), GS_new, O_new)

    def dfs(L, GS, O):
        """
        Recursively explore the tree of triangulations starting from the current state.
        """
        # Save a new triangulation representation (copy)
        result.append(list(sorted(L)))
        if not GS:
            return
        # For generating chords in GS (iterate right to left as in original), recurse
        for idx in reversed(range(len(GS))):
            child = flip((L, GS, O), idx)
            dfs(*child)

    # Start the recursion from the root
    dfs(root_L, root_GS, root_O)

    return result

# # Example usage: print all triangulations of a hexagon (n=6)
# if __name__ == "__main__":
#     for n in range(4, 13):
#         triangulations = generate_all_triangulations(n)
#         print(f"All triangulations of a labeled {n}-cycle:")
#         # for i, triangulation in enumerate(triangulations, 1):
#         #     print(f"{i}: {triangulation}")
#         print(f"Number of triangulations: {len(triangulations)}\n")

def findtriangulationforaface(face):
    triangulations = generate_all_triangulations(len(face))
    print(triangulations)
    mapped_triangulations = []
    for triangulation in triangulations:
        mapped = []
        for edge in triangulation:
            mapped_edge = (face[edge[0] - 1], face[edge[1] - 1])
            mapped.append(mapped_edge)
        mapped_triangulations.append(mapped)
    return mapped_triangulations

def findAll(faces):
    results = []
    for face in faces:
        triangulation = findtriangulationforaface(face)
        if triangulation:
            results.append(triangulation)
    return results


from functools import reduce
import operator

def total_triangulations(all_triangulations):
    counts = [len(triangs) for triangs in all_triangulations]
    return reduce(operator.mul, counts, 1)

if __name__ == "__main__":
    faces = [[1,2,3,4,5]]
    results = findAll(faces)
    print("Triangulations found:")
    for triangulation in results:
        print(triangulation)
    total = total_triangulations(results)
    print(f"Total cross-multiplied triangulations: {total}")

    # Print all combinations of triangulations (cross-multiplied)
    from itertools import product
    print("\nAll triangulation combinations:")
    all = product(*results)
    tempall = []
    for combo in all:
        # print(combo)
        tempall.append(combo)
    all = tempall
    temp = []
    for a in all:
        temparr = []
        for b in a:
            for c in b:
                temparr.append(c)
        temp.append(temparr)
    # print(temp)
    # print(len(temp))
    for i in temp:
        print(i)
    all = []
    for i in temp:
        s = set()
        for j in i:
            s.add(j)
        print("i: " , i)
        print("s: ", s)
        if(len(i) == len(s)):
            all.append(i)
            print('another one added ')
        # all.append(s)
    print("\nUnique triangulations:")
    for s in all:
        print(s)