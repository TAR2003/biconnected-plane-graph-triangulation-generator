#!/usr/bin/env python3
"""Clean up duplicate Future Work sections in main.tex"""

def main():
    with open('paper/main.tex', 'r', encoding='utf-8') as f:
        lines = f.readlines()
    
    # Find the first bibliographystyle
    first_bib = -1
    second_bib = -1
    
    for i, line in enumerate(lines):
        if '\\bibliographystyle{plain}' in line:
            if first_bib == -1:
                first_bib = i
                print(f"First bibliographystyle at line {i+1}")
            elif second_bib == -1:
                second_bib = i
                print(f"Second bibliographystyle at line {i+1}")
                break
    
    if first_bib != -1 and second_bib != -1:
        # Keep everything up to and including first bibliographystyle
        # Then skip to second bibliographystyle
        new_lines = lines[:first_bib+1] + lines[second_bib:]
        
        with open('paper/main_cleaned.tex', 'w', encoding='utf-8') as f:
            f.writelines(new_lines)
        
        print(f"Removed lines {first_bib+2} to {second_bib} ({second_bib - first_bib - 1} lines)")
        print(f"Original: {len(lines)} lines")
        print(f"Cleaned: {len(new_lines)} lines")
        print(f"Saved to paper/main_cleaned.tex")
    else:
        print("Could not find two bibliographystyle commands!")

if __name__ == '__main__':
    main()
