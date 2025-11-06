# Triangulation Generation Algorithm

## Implementation of Parvez-Rahman-Nakano 2011 Paper

This is a complete C++ implementation of the algorithm presented in:

**"Generating All Triangulations of Plane Graphs"**  
by Mohammad Tanvir Parvez, Md. Saidur Rahman, and Shin-ichi Nakano  
*Journal of Graph Algorithms and Applications*, vol. 15, no. 3, pp. 457–482 (2011)

## Algorithm Overview

The algorithm generates all triangulations of a labeled cycle (or triconnected plane graph) without duplications using a genealogical tree structure.

### Key Features

- **Time Complexity**: O(1) per triangulation
- **Space Complexity**: O(n)
- **No Duplications**: Unique parent-child relationship ensures each triangulation is generated exactly once
- **No Omissions**: Every triangulation is reachable from the root via a unique path

### Core Concepts

1. **Triangulation**: A decomposition of a cycle into triangles using non-intersecting chords
2. **Root Vertex (v1)**: The reference vertex; all chords in root triangulation are incident to v1
3. **Blocking Chord**: A chord (vb, vb') where both endpoints are adjacent to v1 (blocks v1's vision)
4. **Leftmost Blocking Chord**: The blocking chord with the highest vb' value
5. **Generating Chord**: A chord (v1, vj) that can be flipped to generate a child triangulation
6. **Flipping**: Edge flip operation transforming one triangulation into another
7. **Genealogical Tree**: Tree structure where nodes are triangulations, edges are flip operations

## Data Structures (Section 4.3)

For each triangulation T, we maintain:

- **L**: List of all chords (n-3 chords for a cycle of n vertices)
- **GS**: Generating set - chords that can be flipped to create children
- **O**: Opposite pairs - for each generating chord (v1,vj), stores (vo, vo') where <v1, vo, vj, vo'> forms a quadrilateral

## Algorithm Description

### Root Triangulation (Section 4.3)

The root triangulation has all chords incident to vertex v1:
- Chords: (v1,v3), (v1,v4), ..., (v1,vn-1)
- All chords are generating chords
- For chord (v1,vj), opposite pair is (vj-1, vj+1)

### Generating Children (Section 4.2)

To generate children of triangulation T:
1. Find the leftmost blocking chord (vb, vb') of T
2. For each generating chord (v1,vj) where vj ≥ vb:
   - Flip (v1,vj) to create child T'
   - This removes (v1,vj) and adds a new blocking chord
3. Recursively generate children of each T'

### Flip Operation (Section 4.2)

Flipping chord (v1,vj):
1. Get opposite pair (vo, vo') for (v1,vj)
2. Remove chord (v1,vj) from T
3. Add chord (vo, vo') to T
4. Update generating set based on new leftmost blocking chord
5. Recompute opposite pairs

## Correctness Guarantees

### No Duplications (Lemma 6)
- Each triangulation has exactly one parent
- Parent is found by flipping the leftmost blocking chord
- Child-to-parent relationship is unique

### No Omissions (Lemma 3)
- Every triangulation has a unique path from the root
- The leftmost blocking chord uniquely determines the parent
- By flipping all generating chords, all children are generated

### Correct Count (Theorem 2)
- For a cycle of n vertices, generates exactly C(n-2) triangulations
- Where C(k) is the k-th Catalan number: C(k) = (2k)! / ((k+1)! × k!)

## Catalan Numbers

The number of triangulations equals the Catalan numbers:
- n=4 (triangle): C(2) = 2
- n=5 (quadrilateral): C(3) = 5
- n=6 (pentagon): C(4) = 14 ✓
- n=7 (hexagon): C(5) = 42 ✓
- n=8 (heptagon): C(6) = 132

## Compilation and Usage

### Compile

```bash
g++ -std=c++17 -O2 -o triangulation triangulation.cpp
```

### Run

```bash
./triangulation
```

### Output

The program generates:
1. All triangulations for n=6 (should output 14 triangulations)
2. All triangulations for n=7 (should output 42 triangulations)
3. Verification that the count matches the expected Catalan number

## Test Results

### Test Case: n=6
- **Expected**: 14 triangulations (Catalan number C(4))
- **Result**: ✓ CORRECT - Generated exactly 14 unique triangulations

### Test Case: n=7
- **Expected**: 42 triangulations (Catalan number C(5))
- **Result**: ✓ CORRECT - Generated exactly 42 unique triangulations

## Implementation Notes

### Key Implementation Details

1. **Edge Convention**: Edges are stored with u < v for consistency
2. **Vertex Numbering**: Vertices are numbered 1 to n (not 0 to n-1) as per the paper
3. **Root Vertex**: Always vertex 1 (v1)
4. **Cycle Boundary**: Edges (v1,v2) and (vn,v1) are on the cycle boundary, not chords

### Generating Set Computation

For a triangulation T with leftmost blocking chord (vb, vb'):
- If no blocking chord: All chords incident to v1 are generating
- Otherwise: Chords (v1,vj) where vj ≥ vb are generating

### Opposite Pair Computation

For generating chord (v1,vj), find (vo, vo') such that:
- <v1, vo, vj, vo'> forms a quadrilateral in the triangulation
- vo < vj < vo'
- This can be computed in O(1) time with proper data structures

## Extensions

The paper also describes:
1. **Triconnected Plane Graphs** (Section 3): Each face is treated as a cycle
2. **Unlabeled Triangulations** (Section 5): Avoid rotational and mirror repetitions (O(n²) per triangulation)

This implementation focuses on the labeled cycle case (Section 4), which is the foundation for the other algorithms.

## References

Paper: https://doi.org/10.7155/jgaa.00229

Key Lemmas and Theorems:
- **Lemma 2**: Non-root triangulations have at least one blocking chord
- **Lemma 3**: Every triangulation has a unique path from root
- **Lemma 5**: Flipping a generating chord creates the leftmost blocking chord of the child
- **Lemma 6**: Parent-child relationship via generating chords is unique
- **Theorem 2**: Algorithm generates all triangulations in O(1) time each with O(n) space

## Author

Implementation based on the research paper by Parvez, Rahman, and Nakano (2011)

## License

This implementation is provided for educational and research purposes.
