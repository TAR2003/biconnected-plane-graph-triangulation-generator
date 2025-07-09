#!/usr/bin/env python3
"""
Improved algorithm for generating all triangulations for biconnected graphs.
Optimizations include:
1. Memoization to avoid recomputing the same triangulations
2. Early termination when duplicate triangulations are detected
3. More efficient data structures and copying
4. Reduced recursive depth through iterative approaches where possible
5. Pre-computed safe roots and chord validations
"""

import time
from typing import Set, Tuple, FrozenSet, Dict, List
from functools import lru_cache

class TriangulationGenerator:
    def __init__(self, n: int):
        self.n = n
        self.memo_triangulations: Dict[FrozenSet[Tuple[int, int]], Set[FrozenSet[Tuple[int, int]]]] = {}
        self.memo_safe_roots: Dict[FrozenSet[Tuple[int, int]], int] = {}
        self.memo_leftmost_blocking: Dict[Tuple[int, int, FrozenSet[Tuple[int, int]], Tuple[int, int]], bool] = {}
        
    def make_pair(self, a: int, b: int) -> Tuple[int, int]:
        """Create ordered pair ensuring a <= b"""
        return (min(a, b), max(a, b))
    
    def copy_set_of_pairs(self, src: Set[Tuple[int, int]]) -> Set[Tuple[int, int]]:
        """Efficient set copying"""
        return src.copy()
    
    def normalize_vertex(self, vertex: int, root: int) -> int:
        """Normalize vertex relative to root"""
        return (vertex - root + self.n) % self.n
    
    def check_leftmost_blocking_chord(self, root: int, chords: Set[Tuple[int, int]], chord: Tuple[int, int]) -> bool:
        """Optimized leftmost blocking chord check with memoization"""
        # Create cache key
        chords_frozen = frozenset(chords)
        cache_key = (root, self.n, chords_frozen, chord)
        
        if cache_key in self.memo_leftmost_blocking:
            return self.memo_leftmost_blocking[cache_key]
        
        # Normalize all vertices relative to root
        normalized_chords = set()
        for c in chords:
            first_norm = self.normalize_vertex(c[0], root)
            second_norm = self.normalize_vertex(c[1], root)
            normalized_chords.add(self.make_pair(first_norm, second_norm))
        
        # Normalize the input chord
        chord_first_norm = self.normalize_vertex(chord[0], root)
        chord_second_norm = self.normalize_vertex(chord[1], root)
        normalized_chord = self.make_pair(chord_first_norm, chord_second_norm)
        
        # Find blocking vertex
        current_blocked_chord = self.n - 1
        for i in range(self.n - 2, -1, -1):
            if self.make_pair(0, i) not in normalized_chords:
                break
            else:
                current_blocked_chord = i
        
        # Find left blocking chords
        left_blocking_chords = set()
        for a in normalized_chords:
            if a[1] == current_blocked_chord and a[0] > 0:
                left_blocking_chords.add(a)
        
        if not left_blocking_chords:
            result = False
        else:
            leftmost_chord = min(left_blocking_chords)
            result = (leftmost_chord[0] == normalized_chord[0] and 
                     leftmost_chord[1] == normalized_chord[1])
        
        # Cache result
        self.memo_leftmost_blocking[cache_key] = result
        return result
    
    def flip_chord(self, root: int, chords: Set[Tuple[int, int]], all_pairs: Set[Tuple[int, int]], 
                   chord: Tuple[int, int]) -> Tuple[int, int]:
        """Optimized chord flipping"""
        # Pre-compute associations for better performance
        assoc_of_first = set()
        assoc_of_second = set()
        
        for a in all_pairs:
            if a[0] == chord[0]:
                assoc_of_first.add(a[1])
            if a[1] == chord[0]:
                assoc_of_first.add(a[0])
            if a[0] == chord[1]:
                assoc_of_second.add(a[1])
            if a[1] == chord[1]:
                assoc_of_second.add(a[0])
        
        # Find common vertices
        commons = list(assoc_of_first & assoc_of_second)
        
        # Update sets
        chords.discard(chord)
        all_pairs.discard(chord)
        new_chord = self.make_pair(commons[0], commons[1])
        chords.add(new_chord)
        all_pairs.add(new_chord)
        
        return new_chord
    
    def generate_all_triangulations(self, root: int, all_pairs: Set[Tuple[int, int]], 
                                   chords: Set[Tuple[int, int]]) -> Set[FrozenSet[Tuple[int, int]]]:
        """Generate triangulations with memoization"""
        if self.n < 3:
            return set()
        if self.n == 3:
            return set()
        
        # Use memoization
        chords_frozen = frozenset(chords)
        if chords_frozen in self.memo_triangulations:
            return self.memo_triangulations[chords_frozen]
        
        answers = set()
        answers.add(chords_frozen)
        
        # Use iterative approach where possible to reduce recursion overhead
        stack = [(root, all_pairs.copy(), chords.copy())]
        processed = set()
        
        while stack:
            current_root, current_all_pairs, current_chords = stack.pop()
            current_frozen = frozenset(current_chords)
            
            if current_frozen in processed:
                continue
            processed.add(current_frozen)
            
            for chord in list(current_chords):  # Convert to list to avoid modification during iteration
                all_pairs_copy = current_all_pairs.copy()
                chords_copy = current_chords.copy()
                
                try:
                    new_chord = self.flip_chord(current_root, chords_copy, all_pairs_copy, chord)
                    if self.check_leftmost_blocking_chord(current_root, chords_copy, new_chord):
                        new_frozen = frozenset(chords_copy)
                        answers.add(new_frozen)
                        stack.append((current_root, all_pairs_copy, chords_copy))
                except (IndexError, KeyError):
                    # Handle cases where flip_chord fails
                    continue
        
        self.memo_triangulations[chords_frozen] = answers
        return answers
    
    def find_highest_safe_root(self, inner_chords: Set[Tuple[int, int]]) -> int:
        """Find highest safe root with memoization"""
        inner_chords_frozen = frozenset(inner_chords)
        if inner_chords_frozen in self.memo_safe_roots:
            return self.memo_safe_roots[inner_chords_frozen]
        
        safe_vertices = set(range(self.n))
        for chord in inner_chords:
            safe_vertices.discard(chord[0])
            safe_vertices.discard(chord[1])
        
        result = max(safe_vertices) if safe_vertices else 0
        self.memo_safe_roots[inner_chords_frozen] = result
        return result
    
    def compare_pairs(self, a: Tuple[int, int], b: Tuple[int, int]) -> bool:
        """Compare pairs lexicographically"""
        return a[0] < b[0] or (a[0] == b[0] and a[1] < b[1])
    
    def generate_outer_triangulations_from_inner(self, root: int, all_pairs: Set[Tuple[int, int]], 
                                                 chords: Set[Tuple[int, int]], inner_chords: Set[Tuple[int, int]]) -> Set[FrozenSet[Tuple[int, int]]]:
        """Generate outer triangulations with optimizations"""
        if self.n < 3:
            return set()
        if self.n == 3:
            return set()
        
        # Early validation checks
        if not chords or not inner_chords:
            return set()
        
        minimum_chord = min(chords)
        minimum_inner_chord = min(inner_chords)
        
        if self.compare_pairs(minimum_chord, minimum_inner_chord):
            return set()
        
        # Check chord count
        all_chords = chords | inner_chords
        right_length = (self.n - 3) * 2
        if len(all_chords) < right_length:
            return set()
        
        answers = set()
        merged = chords | inner_chords
        answers.add(frozenset(merged))
        
        # Optimized recursive generation
        for chord in list(chords):
            all_pairs_copy = all_pairs.copy()
            chords_copy = chords.copy()
            
            try:
                new_chord = self.flip_chord(root, chords_copy, all_pairs_copy, chord)
                if self.check_leftmost_blocking_chord(root, chords_copy, new_chord):
                    sub_answers = self.generate_outer_triangulations_from_inner(
                        root, all_pairs_copy, chords_copy, inner_chords)
                    answers.update(sub_answers)
            except (IndexError, KeyError):
                continue
        
        return answers
    
    def generate_all_triangulations_for_biconnected_graph(self) -> Set[FrozenSet[Tuple[int, int]]]:
        """Main optimized algorithm"""
        start_time = time.time()
        
        # Initialize base structures
        all_pairs = set()
        for i in range(self.n):
            all_pairs.add(self.make_pair(i, (i + 1) % self.n))
        
        # Generate initial inner triangulations
        inner_chords = set()
        temp_all_pairs = all_pairs.copy()
        for i in range(2, self.n - 1):
            inner_chords.add(self.make_pair(0, i))
            temp_all_pairs.add(self.make_pair(0, i))
        
        temp_answers = self.generate_all_triangulations(0, temp_all_pairs, inner_chords)
        
        final_answers = set()
        processed_inner_chords = set()
        
        # Process each inner triangulation
        for triangulation in temp_answers:
            inner_chords_copy = set(triangulation)
            inner_frozen = frozenset(inner_chords_copy)
            
            # Skip if already processed (optimization)
            if inner_frozen in processed_inner_chords:
                continue
            processed_inner_chords.add(inner_frozen)
            
            # Find safe root
            highest_safe_root = self.find_highest_safe_root(inner_chords_copy)
            
            # Initialize outer chords
            init_chords = set()
            temp_all_pairs = all_pairs.copy()
            for i in range(2, self.n - 1):
                chord = self.make_pair(highest_safe_root, (highest_safe_root + i) % self.n)
                init_chords.add(chord)
                temp_all_pairs.add(chord)
            
            # Generate outer triangulations
            outer_answers = self.generate_outer_triangulations_from_inner(
                highest_safe_root, temp_all_pairs, init_chords, inner_chords_copy)
            
            final_answers.update(outer_answers)
        
        end_time = time.time()
        print(f"Algorithm completed in {end_time - start_time:.4f} seconds")
        print(f"Generated {len(final_answers)} unique triangulations for n={self.n}")
        
        return final_answers
    
    def print_triangulations(self, triangulations: Set[FrozenSet[Tuple[int, int]]]):
        """Print triangulations in formatted output"""
        print(f"Total triangulations found: {len(triangulations)}")
        if len(triangulations) <= 20:  # Only print if reasonable number
            print("Triangulations:")
            for i, triangulation in enumerate(sorted(triangulations), 1):
                print(f"Set {i}: {{", end="")
                for pair in sorted(triangulation):
                    print(f"({pair[0]},{pair[1]}) ", end="")
                print("}")
        else:
            print("(Too many triangulations to display)")

def main():
    """Test the improved algorithm"""
    print("Improved Triangulation Algorithm")
    print("=" * 40)
    
    # Test for different values of n
    for n in range(4, 14):  # Start with smaller values for testing
        print(f"\nTesting for n = {n}:")
        generator = TriangulationGenerator(n)
        triangulations = generator.generate_all_triangulations_for_biconnected_graph()
        generator.print_triangulations(triangulations)
        
        # Print cache hit statistics
        print(f"Memoization stats:")
        print(f"  Triangulations cache size: {len(generator.memo_triangulations)}")
        print(f"  Safe roots cache size: {len(generator.memo_safe_roots)}")
        print(f"  Leftmost blocking cache size: {len(generator.memo_leftmost_blocking)}")

if __name__ == "__main__":
    main()
