#!/usr/bin/env python3
"""Remove verbose 'Proof Strategy' paragraphs from main.tex"""

def main():
    with open('paper/main.tex', 'r', encoding='utf-8') as f:
        lines = f.readlines()
    
    new_lines = []
    skip_next = False
    removed_count = 0
    
    for i, line in enumerate(lines):
        # Check if this line contains "Proof Strategy:"
        if '\\textbf{Proof Strategy' in line:
            # Skip this line and the next blank line
            skip_next = True
            removed_count += 1
            print(f"Removed Proof Strategy at line {i+1}: {line.strip()[:80]}")
            continue
        
        # Skip one blank line after Proof Strategy
        if skip_next and line.strip() == '':
            skip_next = False
            continue
        
        new_lines.append(line)
    
    with open('paper/main_cleaned.tex', 'w', encoding='utf-8') as f:
        f.writelines(new_lines)
    
    print(f"\nRemoved {removed_count} Proof Strategy paragraphs")
    print(f"Original: {len(lines)} lines")
    print(f"Cleaned: {len(new_lines)} lines")
    print(f"Reduction: {len(lines) - len(new_lines)} lines")

if __name__ == '__main__':
    main()
