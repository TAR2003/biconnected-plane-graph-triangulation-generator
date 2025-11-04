#!/usr/bin/env python3
"""
Script to remove duplicated sections from the paper after failed edits.
Removes duplicate Space Complexity and Implementation sections.
"""

def clean_duplicates(input_file, output_file):
    with open(input_file, 'r', encoding='utf-8') as f:
        lines = f.readlines()
    
    # Find line numbers for key markers
    space_complexity_lines = []
    implementation_lines = []
    
    for i, line in enumerate(lines):
        if line.strip() == r'\subsection{Space Complexity}':
            space_complexity_lines.append(i)
        if line.strip() == r'\section{Implementation and Experimental Results}':
            implementation_lines.append(i)
    
    print(f"Found {len(space_complexity_lines)} Space Complexity sections at lines: {space_complexity_lines}")
    print(f"Found {len(implementation_lines)} Implementation sections at lines: {implementation_lines}")
    
    # If we have duplicates, keep the last occurrence and delete everything between first and last
    if len(space_complexity_lines) >= 2 and len(implementation_lines) >= 2:
        # Delete from first Space Complexity to just before second Space Complexity
        start_delete = space_complexity_lines[0]
        end_delete = space_complexity_lines[-1]
        
        print(f"\nDeleting lines {start_delete} to {end_delete-1} (first Space Complexity section)")
        
        # Keep lines before first duplicate and after first duplicate
        cleaned_lines = lines[:start_delete] + lines[end_delete:]
        
        print(f"Original file: {len(lines)} lines")
        print(f"Cleaned file: {len(cleaned_lines)} lines")
        print(f"Removed: {len(lines) - len(cleaned_lines)} lines")
        
        with open(output_file, 'w', encoding='utf-8') as f:
            f.writelines(cleaned_lines)
        
        return True
    else:
        print("No duplicates found or unexpected structure")
        return False

if __name__ == '__main__':
    input_file = r'c:\Users\TAWKIR\Documents\GitHub\ThesisGraph\paper\main.tex'
    output_file = r'c:\Users\TAWKIR\Documents\GitHub\ThesisGraph\paper\main_cleaned.tex'
    
    if clean_duplicates(input_file, output_file):
        print(f"\nCleaned file saved to: {output_file}")
        print("Please review the cleaned file and replace main.tex if it looks correct")
    else:
        print("\nNo changes made")
