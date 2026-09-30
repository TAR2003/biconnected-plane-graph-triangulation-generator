#pragma once
#include <bits/stdc++.h>
#include "FaceTriangulation.hpp"
#include "GraphTriangulation.hpp"
using namespace std;

class FaceTriangulationBiconnectedWithoutVGS : public FaceTriangulationBiconnected
{
public:
    FaceTriangulationBiconnectedWithoutVGS(long long n, vector<long long> &elements, unordered_multiset<pair<long long, long long>, PairHash> &present, long long serial, GraphTriangulation *gt)
        : FaceTriangulationBiconnected(n, elements, present, serial, gt) {}

    void flip(list<Edge *>::iterator iteratorToFlip)
    {
        auto itrGS = iteratorToFlip; // Use the provided iterator directly
        Edge *e = *itrGS;            // Edge to be flipped

        // Store the values BEFORE flipping
        pair<long long, long long> newChord = make_pair(e->opposite_first, e->opposite_second); // New chord after flip
        pair<long long, long long> oldChord = make_pair(e->first, e->second);                   // Old chord before flip
        auto itr = e->chordItrGS;                                                               // Corresponding iterator in the generating set
        // Update neighbors with the stored values
        if (next(itr) != GS.end())
        {
            flipit(itr, next(itr), newChord, oldChord);
        }
        if (itr != GS.begin())
        {
            flipit(itr, prev(itr), newChord, oldChord); 
        }
        // Now flip the edge
        present.erase(getPair(e));
        e->flip();
        present.insert(getPair(e));
    }

    void visitAllBranches(list<Edge *>::iterator &itr)
    {
        for (; itr != GS.end();)
        {
            // Recursively generate child triangulations for edges that can block the current edge
            if (gt->crossedLimits())
            {
                return;
            }
            Edge *child = *itr;
            generateChildTriangulations(itr);
            itr = next(child->chordItrGS);
        }
    }

    /// @brief Generates child triangulations by flipping the edge pointed to by the iterator
    /// @param itr Iterator pointing to the edge to be flipped
    void generateChildTriangulations(list<Edge *>::iterator &iteratorToFlip)
    {
        auto itrGS = iteratorToFlip;
        gt->totalChecks++;
        auto oppositePair = getOppositePair(*itrGS);
        if (present.find(oppositePair) != present.end())
        {
            return;
        }
        gt->successfulChecks++;

        flip(itrGS); // Flip the edge at the current iterator, and update neighbors accordingly

        bool lastChordGS = false;           // Flag to check if the current edge is the last in the generating setlist of all edges
        Edge *next_chord_gs;                // Pointer to the next chord in the generating set set
        if (next(itrGS) == GS.end())
        {
            lastChordGS = true; // If it is the last edge, set the flag to true
        }
        else
        {
            next_chord_gs = *next(itrGS); // Get the next chord
        }

        Edge *c = *itrGS;              // Current chord to be processed
        list<Edge *>::iterator itrloop; // Iterator for looping through the generating set

        GS.erase(itrGS);   // Remove the current edge from the generating set

        output();

        visitAllBranches(itrloop); // Visit all branches recursively

        if (lastChordGS) // If the current edge was the last in the generating set
        {
            // cout << "last chord" << endl;
            GS.push_back(c);
            itrGS = prev(GS.end());
            c->chordItrGS = itrGS; // Update the iterator of the chord
        }
        else
        {
            // If there are more edges in the generating set
            itrGS = GS.insert(next_chord_gs->chordItrGS, c);
            c->chordItrGS = itrGS; // Update the iterator of the chord
        }

        flip(itrGS); // Flip back the edge to restore the original state
    }

    /// @brief generates all triangulations of the cycle
    void generateAllTriangulations()
    {
        for (long long i = 2; i < n - 1; i++)
        {
            Edge *e = new Edge(0, i, i - 1, (i + 1) % n); // creating a new edge object
            GS.push_back(e);                              // adding the edge to the generating set
            chords.push_back(e);                          // adding the edge to the list of all chords
            present.insert(getPair(e));                   // marking the edge as present in the original graph
            auto itrGS = prev(GS.end());
            e->chordItrGS = itrGS; // setting the iterator of the chord
        }

        // addTriangulation(); // adding the initial root triangulation
        output();

        auto itr = GS.begin();
        visitAllBranches(itr); // Visit all branches recursively

        removeCurrentChordsFromPresent(); // unmarking the edges after finishing
    }
};