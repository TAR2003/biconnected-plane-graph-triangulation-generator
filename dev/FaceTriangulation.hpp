#pragma once
#include <bits/stdc++.h>
using namespace std;
#include "Edge.hpp"
#include "PairHash.hpp"

// Forward declaration to avoid circular dependency
class GraphTriangulation;


class FaceTriangulation
{
public:
    /// @brief all the chords in the current cycle
    list<Edge *> chords;
    /// @brief the generating set of the cycle
    list<Edge *> GS;
    /// @brief the list of all edges in the cycle (for memory management)
    list<Edge *> VGS;
    /// @brief the vertex number of the cycle
    long long n;
    /// @brief set of all present chords in the original graph (reference to shared set)
    unordered_multiset<pair<long long, long long>, PairHash> &present;
    /// @brief set of all present chords in the current face, need to keep track for the root triangulation multi edge conflict
    unordered_multiset<pair<long long, long long>, PairHash> presentFace;
    /// @brief all the triangulations generated
    vector<vector<pair<long long, long long>>> allTriangulations;
    vector<long long> positions; // vector to store the positions of the vertices in the cycle
    vector<long long> elements;  // vector to store the elements of the cycle

    long long problems;
    GraphTriangulation *gt;
    long long serial;

    /// @brief the constructor of the class
    /// @param n the number of vertices in the cycle
    /// @param elements the elements of the cycle
    /// @param present the set of present chords
    /// @param serial the serial number of the face
    /// @param gt pointer to the GraphTriangulation class
    FaceTriangulation(long long n, vector<long long> &elements, unordered_multiset<pair<long long, long long>, PairHash> &present, long long serial, GraphTriangulation *gt)
        : n(n), elements(elements), present(present),  serial(serial), gt(gt), positions(n, -1), problems(0)
    {
        findSafeRoot();
    }

    /// @brief the destructor of the class
    virtual ~FaceTriangulation()
    {
        for (auto &chord : chords)
        {
            delete chord; // free the memory allocated for each chord
        }
        presentFace.clear(); // clear the set of present chords

    }

    void printSet(list<Edge *> &s)
    {
        cout << "=============================================" << endl;
        cout << "First the position list" << endl;
        for (auto &edge : s)
        {
            cout << "(" << edge->first << ", " << edge->second << ") , ";
        }
        cout << endl;

        cout << "Now the original list" << endl;
        for (auto &edge : s)
        {
            cout << "(" << positions[edge->first] << ", " << positions[edge->second] << ") , ";
        }
        cout << endl;
        cout << "=============================================" << endl;
    }

    void printPresent()
    {
        cout << "Printing the present set" << endl;
        for (auto &edge : present)
        {
            cout << "(" << edge.first << ", " << edge.second << ") , ";
        }
        cout << endl;
    }

    void printPair(pair<long long, long long> p)
    {
        cout << " (" << p.first << ", " << p.second << ") ";
    }

    pair<long long, long long> getPair(Edge *e)
    {
        return {min(positions[e->first], positions[e->second]), max(positions[e->first], positions[e->second])};
    }

    pair<long long, long long> getOppositePair(Edge *e)
    {
        return {min(positions[e->opposite_first], positions[e->opposite_second]), max(positions[e->opposite_first], positions[e->opposite_second])};
    }

    /// @brief finds a safe root for the cycle and updates the positions vector accordingly
    void findSafeRoot()
    {
        long long startIndex = 0;
        long long endIndex = n - 2;
        while (startIndex < endIndex - 1)
        {
            cout << "Candidates: " << elements[startIndex] << " and " << elements[endIndex] << endl;
            if (present.find({elements[startIndex], elements[endIndex]}) != present.end() || present.find({elements[endIndex], elements[startIndex]}) != present.end() || elements[startIndex] == elements[endIndex])
            {
                startIndex++;
            }
            else
            {
                endIndex--;
            }
        }
        // start Index is the safe root
        for (long long i = 0; i < n; i++)
        {
            positions[i] = elements[(startIndex + i) % n];
        }
    }

    /// @brief printing all the triangulations after finishing the complete task
    void printAllTriangulations()
    {
        cout << "Total triangulations: " << allTriangulations.size() << endl;
        for (auto &triangulation : allTriangulations)
        {
            for (auto &chord : triangulation)
            {
                cout << "(" << chord.first << ", " << chord.second << ") , ";
            }
            cout << endl;
        }
    }

    /// @brief Updates the opposite endpoints of the edge pointed to by itr_other based on the flip operation
    /// @param itr Iterator pointing to the current edge
    /// @param itr_other Iterator pointing to the other edge whose opposite endpoints need to be updated
    /// @param newChord The new chord after the flip
    /// @param oldChord The old chord before the flip
    void flipit(list<Edge *>::iterator itr, list<Edge *>::iterator itr_other,
                pair<long long, long long> newChord, pair<long long, long long> oldChord)
    {

        Edge *other_e = *itr_other; // other edge whose opposite endpoints need to be updated

        // Find which endpoint to update
        long long oldPoint = oldChord.first;
        if (oldPoint == other_e->first || oldPoint == other_e->second)
        {
            oldPoint = oldChord.second;
        }

        // Find the new endpoint to set
        long long newPoint = newChord.first;
        if (newPoint == other_e->first || newPoint == other_e->second)
        {
            newPoint = newChord.second;
        }

        if (other_e->opposite_first == oldPoint)
        {
            other_e->opposite_first = newPoint;
        }
        else if (other_e->opposite_second == oldPoint)
        {
            other_e->opposite_second = newPoint;
        }
    }



    /// @brief adds the current triangulation to the list of all triangulations
    void addTriangulation()
    {
        vector<pair<long long, long long>> currentTriangulation;
        for (auto &chord : chords)
        {
            currentTriangulation.push_back(getPair(chord));
        }

        allTriangulations.push_back(currentTriangulation);
    }

    void removeCurrentChordsFromPresent()
    {
        for (auto &chord : chords)
        {
            auto it = present.find(getPair(chord));
            if (it != present.end())
                present.erase(it); // unmarking the edges after finishing
        }
    }

    virtual void visitAllBranches(list<Edge *>::iterator &itr) = 0;

    /// @brief Outputs the current triangulation
    virtual void output() = 0;

    /// @brief Flips the edge pointed to by the iterator in the generating set
    /// @param itrVGS Iterator pointing to the edge to be flipped
    virtual void flip(list<Edge *>::iterator iteratorToFlip)  = 0;

    /// @brief Generates child triangulations by flipping the edge pointed to by the iterator
    /// @param itr Iterator pointing to the edge to be flipped
    virtual void generateChildTriangulations(list<Edge *>::iterator &itrGS) = 0;

    /// @brief generates all triangulations of the cycle
    virtual void generateAllTriangulations() = 0;
};





