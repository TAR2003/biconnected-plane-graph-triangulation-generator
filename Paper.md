# Generating All Triangulations of Plane Graphs

**Authors:** Mohammad Tanvir Parvez, Md. Saidur Rahman, Shin-ichi Nakano

**Published:** 2011

## Abstract

This paper presents algorithms for generating all triangulations of plane graphs, focusing on both triconnected and biconnected plane graphs. The authors develop efficient enumeration algorithms based on reverse search techniques and canonical forms to systematically generate all possible triangulations without repetition.

## 1. Introduction

### Problem Definition
A **triangulation** of a plane graph is a maximal planar graph where every face (including the outer face) is bounded by exactly three edges. The problem addressed in this paper is to enumerate all possible triangulations of a given plane graph efficiently.

### Key Contributions
1. **Algorithm for triconnected plane graphs**: An efficient method to generate all triangulations of triconnected plane graphs
2. **Algorithm for biconnected plane graphs**: Extension to handle more general biconnected plane graphs
3. **Canonical representation**: Development of canonical forms to avoid generating duplicate triangulations
4. **Time complexity analysis**: Theoretical analysis of the computational efficiency

### Applications
- Graph theory research
- Computational geometry
- Mesh generation
- Network design
- Combinatorial optimization

## 2. Preliminaries and Background

### Graph Theory Foundations

#### Basic Definitions
- **Plane Graph**: A planar graph with a fixed embedding in the plane
- **Triangulation**: A maximal planar graph where all faces are triangles
- **Triconnected Graph**: A graph that remains connected after removing any two vertices
- **Biconnected Graph**: A graph that remains connected after removing any single vertex

#### Key Properties
- Every triangulation of an n-vertex plane graph has exactly 3n-6 edges
- The outer face of a triangulation must also be a triangle
- Triangulations have exactly 2n-4 faces (including the outer face)

### Reverse Search Technique
The paper employs **reverse search**, a powerful enumeration technique that:
- Defines a parent-child relationship among solutions
- Ensures each solution has exactly one parent (except the root)
- Generates solutions by traversing a tree structure
- Guarantees no duplicates and complete enumeration

## 3. Triangulations of Triconnected Plane Graphs

### 3.1 Finding the Root

The algorithm begins by finding a **canonical triangulation** that serves as the root of the enumeration tree. For triconnected plane graphs:

1. **Root Selection**: Choose a specific triangulation as the starting point
2. **Canonical Form**: Define a unique representation to avoid symmetries
3. **Validation**: Ensure the root satisfies all necessary properties

### 3.2 The Algorithm

The core algorithm for triconnected graphs follows these steps:

```
Algorithm: GenerateTriconnectedTriangulations
Input: Triconnected plane graph G
Output: All triangulations of G

1. Find canonical root triangulation T_root
2. Initialize queue Q with T_root
3. While Q is not empty:
   a. Remove triangulation T from Q
   b. Output T
   c. Generate all children of T
   d. Add valid children to Q
4. Return all generated triangulations
```

#### Parent-Child Relationship
- **Parent Operation**: Remove a specific edge to create a quadrilateral face
- **Child Operation**: Add an edge to triangulate a quadrilateral face
- **Canonicality Check**: Ensure generated triangulations maintain canonical form

## 4. Labeled Triangulations of a Cycle

### 4.1 Child-Parent Relationship

For cycle-based triangulations, the paper defines:
- **Parent**: A triangulation with one fewer internal edge
- **Child**: A triangulation obtained by adding one internal edge
- **Generation Rule**: Systematic method to generate all children from a parent

### 4.2 Generating Children

The child generation process involves:

1. **Identify Candidate Positions**: Find all valid locations for new edges
2. **Add Edges**: Insert edges that maintain planarity
3. **Check Validity**: Verify the resulting graph is still a valid triangulation
4. **Canonicality**: Ensure the child triangulation is in canonical form

### 4.3 Representation

Each triangulation is represented using:
- **Adjacency Lists**: Efficient storage of graph connectivity
- **Face Information**: Track triangular faces and their boundaries
- **Canonical Labeling**: Unique vertex and edge labeling scheme

### 4.4 The Algorithm

```
Algorithm: GenerateCycleTriangulations
Input: Cycle C with n vertices
Output: All triangulations of C

1. Start with empty triangulation (only cycle edges)
2. Apply reverse search:
   a. For each current triangulation T
   b. Generate all possible children by adding edges
   c. Recursively process each child
3. Output all generated triangulations
```

## 5. Generating Unlabeled Triangulations

### 5.1 Removing Rotational Repetitions

To handle unlabeled triangulations (where vertex labels don't matter):

1. **Rotation Equivalence**: Identify triangulations that are rotationally equivalent
2. **Canonical Representative**: Choose one representative from each equivalence class
3. **Rotation Detection**: Algorithm to detect and eliminate rotational duplicates

#### Algorithm for Rotation Removal
```
For each generated triangulation T:
1. Generate all rotations of T
2. Select the lexicographically smallest rotation
3. If current T is the canonical rotation, keep it
4. Otherwise, discard T as a duplicate
```

### 5.2 Avoiding Mirror Repetitions

Similarly, for mirror symmetries:

1. **Reflection Equivalence**: Identify triangulations that are mirror images
2. **Mirror Detection**: Algorithm to detect reflectional symmetry
3. **Canonical Selection**: Choose canonical representative from mirror pairs

#### Mirror Symmetry Handling
```
For each triangulation T:
1. Generate mirror image T'
2. Compare T and T' lexicographically
3. Keep only the lexicographically smaller one
4. Mark the other as duplicate
```

## 6. Theoretical Analysis

### Time Complexity

#### Triconnected Graphs
- **Per Triangulation**: O(n) time to generate each triangulation
- **Total Triangulations**: Exponential in the worst case
- **Overall Complexity**: O(n × number of triangulations)

#### Space Complexity
- **Storage**: O(n) space per triangulation
- **Working Space**: O(n) for algorithm operations

### Correctness Proof

The paper provides formal proofs that:
1. **Completeness**: All triangulations are generated
2. **No Duplicates**: Each triangulation is generated exactly once
3. **Termination**: The algorithm always terminates

## 7. Implementation Details

### Data Structures

1. **Graph Representation**: Adjacency lists for efficient edge operations
2. **Face Tracking**: Maintain list of triangular faces
3. **Canonical Form**: Efficient comparison and canonicalization

### Optimization Techniques

1. **Early Pruning**: Eliminate invalid branches early
2. **Memoization**: Cache results to avoid recomputation
3. **Efficient Canonicalization**: Fast methods to compute canonical forms

## 8. Experimental Results

The paper includes experimental validation showing:
- **Scalability**: Performance on graphs of various sizes
- **Accuracy**: Verification against known theoretical bounds
- **Efficiency**: Comparison with naive enumeration methods

## 9. Applications and Extensions

### Practical Applications
1. **Mesh Generation**: Creating triangular meshes for finite element analysis
2. **Network Design**: Designing robust triangulated networks
3. **Computer Graphics**: Triangulation for 3D rendering
4. **Combinatorial Enumeration**: Counting triangulations for research

### Possible Extensions
1. **Higher Genus**: Extension to graphs on surfaces of higher genus
2. **Constrained Triangulations**: Handling additional constraints
3. **Parallel Algorithms**: Parallelization of the enumeration process

## 10. Conclusion

### Key Achievements

The paper successfully presents:
1. **Efficient algorithms** for triangulation enumeration
2. **Theoretical guarantees** of completeness and correctness
3. **Practical implementations** with reasonable performance
4. **Extension to unlabeled cases** handling symmetries

### Significance

This work contributes to:
- **Theoretical computer science**: Advanced enumeration algorithms
- **Computational geometry**: Fundamental triangulation problems
- **Practical applications**: Real-world mesh generation and network design

### Future Directions

Potential areas for further research:
1. **Optimization**: Further improvements to time and space complexity
2. **Generalization**: Extension to broader classes of graphs
3. **Applications**: Development of specialized applications
4. **Parallel Processing**: Efficient parallel implementations

## Technical Summary

### Algorithm Characteristics
- **Technique**: Reverse search with canonical forms
- **Input**: Plane graphs (triconnected or biconnected)
- **Output**: All possible triangulations
- **Guarantee**: Complete enumeration without duplicates

### Performance
- **Time**: O(n × T) where T is the number of triangulations
- **Space**: O(n) per triangulation
- **Scalability**: Suitable for moderate-sized graphs

### Innovation
The paper's main innovation lies in the systematic application of reverse search techniques to triangulation enumeration, combined with sophisticated canonical form definitions to handle symmetries and avoid duplicates.

This comprehensive approach makes the work a significant contribution to both theoretical computer science and practical computational geometry applications.
