def generate_triangulations(n):
    """
    Generate all triangulations of a cycle with n vertices.
    Returns a list of triangulations, where each triangulation is represented
    as a set of chords (each chord is a tuple of endpoints, sorted).
    """
    if n < 3:
        return []
    if n == 3:
        return [set()]  # Only the triangle itself, no chords needed
    
    def _generate(i, j, memo):
        if (i, j) in memo:
            return memo[(i, j)]
        if j - i <= 1:
            return [set()]
        
        triangulations = []
        for k in range(i + 1, j):
            left_triangs = _generate(i, k, memo)
            right_triangs = _generate(k, j, memo)
            chord = tuple(sorted((i, j)))
            
            for left in left_triangs:
                for right in right_triangs:
                    new_triang = set()
                    new_triang.update(left)
                    new_triang.update(right)
                    new_triang.add(chord)
                    triangulations.append(new_triang)
        
        memo[(i, j)] = triangulations
        return triangulations
    
    memo = {}
    full_triangulations = _generate(0, n-1, memo)
    
    # Remove duplicates that might occur due to different splitting orders
    unique_triangulations = []
    seen = set()
    
    for triang in full_triangulations:
        sorted_triang = tuple(sorted(triang))
        if sorted_triang not in seen:
            seen.add(sorted_triang)
            unique_triangulations.append(triang)
    
    return unique_triangulations

def print_triangulations(n):
    triangulations = generate_triangulations(n)
    print(f"All triangulations of a cycle with {n} vertices:")
    
    # Prepare data for file writing
    file_data = []
    
    for i, triang in enumerate(triangulations, 1):
        # Filter out normal edges (consecutive vertices in the cycle)
        internal_chords = []
        for chord in triang:
            a, b = chord
            # Check if it's not a normal edge (consecutive vertices or wrap-around edge)
            if not ((b - a == 1) or (a == 0 and b == n - 1)):
                internal_chords.append(chord)
        
        # Console output (convert to 1-based indexing)
        console_chords = [(chord[0] + 1, chord[1] + 1) for chord in sorted(internal_chords)]
        print(f"Triangulation {i}: {console_chords}")
        
        # Store data for file writing
        file_data.append(sorted(internal_chords))
    
    print(f"Total number of triangulations: {len(triangulations)}")
    
    # Write to file a.txt
    with open("a.txt", "w") as f:
        total_sequences = len(triangulations)
        # Calculate pairs per sequence (should be same for all sequences in a triangulation)
        pairs_per_sequence = len(file_data[0]) if file_data else 0
        
        # Write header once at the beginning
        f.write(f"{total_sequences} {pairs_per_sequence}\n")
        
        # Write all pairs for all sequences
        for internal_chords in file_data:
            for chord in internal_chords:
                f.write(f"{chord[0] + 1} {chord[1] + 1}\n")  # Convert to 1-based indexing
    
    print(f"Data written to a.txt")

# Example usage:
n = 5 # You can change this to any n ≥ 3
print_triangulations(n)