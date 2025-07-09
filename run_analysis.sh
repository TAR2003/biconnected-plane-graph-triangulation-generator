#!/bin/bash

echo "=== Running Triangulation Analysis ==="
echo ""

echo "Step 1: Generating triangulations and writing to a.txt..."
python generateTriangulations.py

echo ""
echo "Step 2: Analyzing pair sequences from a.txt..."
python pairPerSequence.py > ans.txt

echo ""
echo "=== Analysis Complete ==="
