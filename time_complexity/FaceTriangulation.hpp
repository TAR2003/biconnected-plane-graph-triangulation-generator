#include <bits/stdc++.h>
using namespace std;
#pragma once
#include "Edge.hpp"
#include "pairHash.hpp"

// Forward declaration to avoid circular dependency
class biconnected;

class FaceTriangulation
{
public:
    /// @brief all the chords in the current cycle
    list<Edge *> chords;
    /// @brief the generating set of the cycle
    list<Edge *> GS;
    /// @brief the vertex number of the cycle
    int n;
    /// @brief set of all present chords in the original graph (reference to shared set)
    unordered_set<pair<int, int>, PairHash> &present;
    /// @brief all the triangulations generated
    vector<vector<pair<int, int>>> allTriangulations;
    vector<int> positions;
    vector<int> elements;
    biconnected *bc;
    int serial;

    /// @brief the constructor of the class
    /// @param n the number of vertices in the cycle
    /// @param elements the elements of the cycle
    /// @param present the set of present chords
    /// @param serial the serial number of the face
    /// @param bc pointer to the biconnected class
    FaceTriangulation(int n, vector<int> &elements, unordered_set<pair<int, int>, PairHash> &present, int serial, biconnected *bc)
        : n(n), present(present), elements(elements), serial(serial), bc(bc), positions(n, -1)
    {
        findSafeRoot();
    }


    /// @brief the destructor of the class
    ~FaceTriangulation()
    {
        for (auto &chord : chords)
        {
            delete chord; // free the memory allocated for each chord
        }
    }

    pair<int, int> getPair(Edge *e)
    {
        return {min(positions[e->first], positions[e->second]), max(positions[e->first], positions[e->second])};
    }

    pair<int, int> getOppositePair(Edge *e)
    {
        return {min(positions[e->opposite_first], positions[e->opposite_second]), max(positions[e->opposite_first], positions[e->opposite_second])};
    }

    /// @brief finds a safe root for the cycle and updates the positions vector accordingly
    void findSafeRoot()
    {
        int startIndex = n - 1;
        int endIndex = 1;
        while (startIndex > endIndex + 1)
        {
            if (present.find({elements[startIndex], elements[endIndex]}) != present.end() || present.find({elements[endIndex], elements[startIndex]}) != present.end())
            {
                startIndex--;
            }
            else
            {
                endIndex++;
            }
        }
        // start Index is the safe root
        for (int i = 0; i < n; i++)
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
                pair<int, int> newChord, pair<int, int> oldChord)
    {

        Edge *other_e = *itr_other; // other edge whose opposite endpoints need to be updated

        // Find which endpoint to update
        int oldPoint = oldChord.first;
        if (oldPoint == other_e->first || oldPoint == other_e->second)
        {
            oldPoint = oldChord.second;
        }

        // Find the new endpoint to set
        int newPoint = newChord.first;
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

    /// @brief Flips the edge pointed to by the iterator in the generating set
    /// @param itr Iterator pointing to the edge to be flipped
    void flip(list<Edge *>::iterator itr)
    {
        Edge *e = *itr; // Edge to be flipped

        // Store the values BEFORE flipping
        pair<int, int> newChord = make_pair(e->opposite_first, e->opposite_second); // New chord after flip
        pair<int, int> oldChord = make_pair(e->first, e->second);                   // Old chord before flip

        // Update neighbors with the stored values
        if (next(itr) != GS.end())
            flipit(itr, next(itr), newChord, oldChord); // if it is not the last edge, update the next edge
        if (itr != GS.begin())
            flipit(itr, prev(itr), newChord, oldChord); // if it is not the first edge, update the previous edge

        // Now flip the edge
        present.erase(getPair(e));
        e->flip();
        present.insert(getPair(e));
    }

    /// @brief adds the current triangulation to the list of all triangulations
    void addTriangulation()
    {
        vector<pair<int, int>> currentTriangulation;
        for (auto &chord : chords)
        {
            currentTriangulation.push_back(getPair(chord));
        }

        allTriangulations.push_back(currentTriangulation);
    }

    void output();

    /// @brief Generates child triangulations by flipping the edge pointed to by the iterator
    /// @param itr Iterator pointing to the edge to be flipped
    void generateChildTriangulations(list<Edge *>::iterator &itr)
    {
        flip(itr);              // Flip the edge at the current iterator, and update neighbors accordingly
        bool lastChord = false; // Flag to check if the current edge is the last in the generating set
        Edge *next_chord;       // Pointer to the next chord in the generating set
        if (next(itr) == GS.end())
        {
            lastChord = true; // If it is the last edge, set the flag to true
        }
        else
        {
            next_chord = *next(itr); // Get the next chord
        }
        Edge *c = *itr;                 // Current chord to be processed
        list<Edge *>::iterator itrloop; // Iterator for looping through the generating set
        if (itr == GS.begin())
        {
            itrloop = next(itr); // if it is the first edge, start from the next edge
        }
        else
        {
            itrloop = prev(itr); // else start from the previous edge
        }

        GS.erase(itr); // Remove the current edge from the generating set
        auto itr_begin = GS.begin();

        // Determine the leftmost blocking endpoint
        int leftmost_blocking_b = min(c->first, c->second);

        // addTriangulation(); 
        output();

        for (; itrloop != GS.end(); itrloop++)
        {
            // Recursively generate child triangulations for edges that can block the current edge
            if (present.find(getOppositePair(*itrloop)) == present.end())
            {

                generateChildTriangulations(itrloop);
            }
        }
        if (lastChord) // If the current edge was the last in the generating set
        {
            // cout << "last chord" << endl;
            GS.push_back(c);
            itr = prev(GS.end());
            c->chordItr = itr; // Update the iterator of the chord
        }
        else
        { 
            // If there are more edges in the generating set
            itr = GS.insert(next_chord->chordItr, c);
            c->chordItr = itr; // Update the iterator of the chord
        }
        flip(itr); // Flip back the edge to restore the original state
    }

    /// @brief generates all triangulations of the cycle
    void generateAllTriangulations()
    {
        for (int i = 2; i < n - 1; i++)
        {
            Edge *e = new Edge(0, i, i - 1, (i + 1) % n); // creating a new edge object
            GS.push_back(e);                              // adding the edge to the generating set
            chords.push_back(e);                          // adding the edge to the list of all chords
            present.insert(getPair(e));                   // marking the edge as present in the original graph
            auto itr = prev(GS.end());
            e->chordItr = itr;                            // setting the iterator of the chord
        }
        // addTriangulation(); // adding the initial root triangulation
        output();
        for (auto itr = GS.begin(); itr != GS.end(); itr++)
        {
            if (present.find(getOppositePair(*itr)) == present.end())
            {
                generateChildTriangulations(itr); // generating child triangulations recursively
            }
        }

        for (auto &chord : chords)
        {
            // cout << "erasing the chords" << endl;
            present.erase(getPair(chord)); // unmarking the edges after finishing
            // cout << "we are done erasing the chords" << endl;
        }
    }
};

// Include biconnected.hpp after class declaration to resolve circular dependency
#include "biconnected.hpp"

// Define output() method after including biconnected.hpp
inline void FaceTriangulation::output()
{
    bc->output(serial);
}
