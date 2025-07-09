def find_conflicting_pairs(seq1, seq2):
    """Find pairs that appear in both sequences"""
    elements = set(seq1)
    conflicts = []
    
    # Check each pair in second sequence
    for p in seq2:
        if p in elements:
            conflicts.append(p)
    
    return conflicts

def print_sequence(seq):
    """Print a sequence of pairs in formatted way"""
    print("{ ", end="")
    for p in seq:
        print(f"({p[0]},{p[1]}) ", end="")
    print("}", end="")

def main():
    # Open the file a.txt for reading
    try:
        with open("a.txt", "r") as input_file:
            # Read number of sequences and pairs per sequence from file
            num_sequences, pairs_per_sequence = map(int, input_file.readline().split())
            print(f"Number of sequences: {num_sequences}, Pairs per sequence: {pairs_per_sequence}")
            
            sequences = []
            
            # Read all sequences from file
            print("\nReading sequences from a.txt...")
            for i in range(num_sequences):
                sequence = []
                for j in range(pairs_per_sequence):
                    a, b = map(int, input_file.readline().split())
                    sequence.append((min(a, b), max(a, b)))
                sequences.append(sequence)
    
    except FileNotFoundError:
        print("Error: Could not open file a.txt")
        return
    
    # Display all input sequences
    # print("\nInput Sequences:")
    # for i in range(num_sequences):
    #     print(f"Sequence {i + 1}: {{ ", end="")
    #     for p in sequences[i]:
    #         print(f"({p[0]},{p[1]}) ", end="")
    #     print("}")
    # print()
    
    all_compatible_sets = []
    sequence_sets = {}  # Dictionary to track which sets belong to which sequence
    
    # For each sequence, find compatible sequences and build combined sets
    for i in range(num_sequences):
        compatible = []
        sequence_sets[i + 1] = []  # Initialize list for this sequence
        
        # First, show all available sequences for this sequence
        print(f"Results for Sequence {i + 1}:")
        available_sequences = [j + 1 for j in range(num_sequences) if j != i]
        print(f"Available sequences: {available_sequences}")
        
        # Show the actual sets for available sequences
        # print("Available sets:")
        # for j in range(num_sequences):
        #     if j != i:
        #         print(f"  Sequence {j + 1}: {{ ", end="")
        #         for p in sequences[j]:
        #             print(f"({p[0]},{p[1]}) ", end="")
        #         print("}")
        
        for j in range(num_sequences):
            if i == j:
                continue
            
            conflicts = find_conflicting_pairs(sequences[i], sequences[j])
            if not conflicts:  # if conflicts is empty
                compatible.append(j + 1)
                
                # Create combined set of pairs from both sequences
                combined_set = set(sequences[i])
                combined_set.update(sequences[j])
                combined_list = sorted(list(combined_set))
                all_compatible_sets.append(combined_list)
                sequence_sets[i + 1].append(combined_list)  # Track which sequence this set belongs to
        
        # Then show the compatible sequences
        print(f"Compatible sequences ({len(compatible)}): ", end="")
        for seq in compatible:
            print(seq, end=" ")
        print()
        
        # Show the actual sets for compatible sequences
        if compatible:
            print("Compatible sets:")
            for seq_num in compatible:
                seq_idx = seq_num - 1  # Convert to 0-based index
                print(f"  Sequence {seq_num}: {{ ", end="")
                for p in sequences[seq_idx]:
                    print(f"({p[0]},{p[1]}) ", end="")
                print("}")
        
        print()
    
    # Remove duplicate sets while preserving which sequence they came from
    unique_sets_by_sequence = {}
    seen_sets = []
    
    for seq_num, sets_list in sequence_sets.items():
        unique_sets_by_sequence[seq_num] = []
        for s in sets_list:
            set_tuple = tuple(s)
            # if set_tuple not in seen_sets:
            seen_sets.append(set_tuple)
            unique_sets_by_sequence[seq_num].append(s)
    
    # Count total unique sets
    total_unique_sets = sum(len(sets) for sets in unique_sets_by_sequence.values())
    
    # Print all unique compatible sets grouped by sequence
    print(f"\nAll Unique Compatible Sets ({total_unique_sets}):")
    set_counter = 1
    for seq_num in sorted(unique_sets_by_sequence.keys()):
        if unique_sets_by_sequence[seq_num]:  # Only show sequences that have compatible sets
            print(f"For sequence {seq_num}:")
            for s in unique_sets_by_sequence[seq_num]:
                print(f"Set {set_counter}: ", end="")
                print_sequence(s)
                print()
                set_counter += 1
            print()  # Extra line between sequences

if __name__ == "__main__":
    main()
