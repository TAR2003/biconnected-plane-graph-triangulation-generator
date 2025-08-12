#include <iostream>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <utility>
#include <set>
#include <fstream>

using namespace std;

// Custom comparator for sets of pairs
struct PairSetComparator
{
    bool operator()(const set<pair<int, int>> &a, const set<pair<int, int>> &b) const
    {
        return a < b;
    }
};

vector<pair<int, int>> findConflictingPairs(const vector<pair<int, int>> &seq1,
                                            const vector<pair<int, int>> &seq2)
{
    set<pair<int, int>> elements;
    vector<pair<int, int>> conflicts;

    // Add all elements from first sequence to a set
    for (const auto &p : seq1)
    {
        elements.insert(p);
    }

    // Check each pair in second sequence
    for (const auto &p : seq2)
    {
        if (elements.count(p) > 0)
        {
            conflicts.push_back(p);
        }
    }

    return conflicts;
}

void printSequence(const set<pair<int, int>> &seq)
{
    cout << "{ ";
    for (const auto &p : seq)
    {
        cout << "(" << p.first << "," << p.second << ") ";
    }
    cout << "}";
}

int main()
{
    int numSequences, pairsPerSequence;

    // Open the file a.txt for reading
    ifstream inputFile("a.txt");
    if (!inputFile.is_open()) {
        cout << "Error: Could not open file a.txt" << endl;
        return 1;
    }

    // Read number of sequences and pairs per sequence from file
    inputFile >> numSequences >> pairsPerSequence;
    cout << "Number of sequences: " << numSequences << ", Pairs per sequence: " << pairsPerSequence << endl;

    vector<vector<pair<int, int>>> sequences(numSequences);

    // Read all sequences from file
    cout << "\nReading sequences from a.txt...\n";
    for (int i = 0; i < numSequences; ++i)
    {
        vector<pair<int, int>> sequence;
        for (int j = 0; j < pairsPerSequence; ++j)
        {
            int a, b;
            inputFile >> a >> b;
            sequence.emplace_back(min(a, b), max(a, b));
        }
        sequences[i] = sequence;
    }

    inputFile.close();

    // Display all input sequences
    cout << "\nInput Sequences:\n";
    for (int i = 0; i < numSequences; ++i)
    {
        cout << "Sequence " << i + 1 << ": { ";
        for (const auto &p : sequences[i])
        {
            cout << "(" << p.first << "," << p.second << ") ";
        }
        cout << "}\n";
    }
    cout << endl;

    set<set<pair<int, int>>, PairSetComparator> allCompatibleSets;

    // For each sequence, find compatible sequences and build combined sets
    for (int i = 0; i < numSequences; ++i)
    {
        vector<int> compatible;

        for (int j = 0; j < numSequences; ++j)
        {
            if (i == j)
                continue;

            auto conflicts = findConflictingPairs(sequences[i], sequences[j]);
            if (conflicts.empty())
            {
                compatible.push_back(j + 1);

                // Create combined set of pairs from both sequences
                set<pair<int, int>> combinedSet(sequences[i].begin(), sequences[i].end());
                combinedSet.insert(sequences[j].begin(), sequences[j].end());
                allCompatibleSets.insert(combinedSet);
            }
        }

        // Output results for this sequence
        cout << "Results for Sequence " << i + 1 << ":\n";
        cout << "Compatible sequences (" << compatible.size() << "): ";
        for (int seq : compatible)
            cout << seq << " ";
        cout << "\n\n";
    }

    // Print all unique compatible sets
    cout << "\nAll Unique Compatible Sets (" << allCompatibleSets.size() << "):\n";
    int setNum = 1;
    for (const auto &s : allCompatibleSets)
    {
        cout << "Set " << setNum++ << ": ";
        printSequence(s);
        cout << endl;
    }

    return 0;
}