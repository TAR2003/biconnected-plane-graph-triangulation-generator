#pragma once
#include <bits/stdc++.h>
#include "FaceTriangulation.hpp"
#include "GraphTriangulation.hpp"
using namespace std;



class FaceTriangulationBiconnected : public FaceTriangulation
{
public:
    FaceTriangulationBiconnected(long long n, vector<long long> &elements, unordered_multiset<pair<long long, long long>, PairHash> &present, long long serial, GraphTriangulation *gt)
        : FaceTriangulation(n, elements, present, serial, gt) {}

    /// @brief Flips the edge pointed to by the iterator in the generating set
    /// @param itrVGS Iterator pointing to the edge to be flipped
    void flip(list<Edge *>::iterator iteratorToFlip)
    {
        auto itrVGS = iteratorToFlip; // Use the provided iterator directly
        Edge *e = *itrVGS; // Edge to be flipped

        // Store the values BEFORE flipping
        pair<long long, long long> newChord = make_pair(e->opposite_first, e->opposite_second); // New chord after flip
        pair<long long, long long> oldChord = make_pair(e->first, e->second);                   // Old chord before flip
        auto itr = e->chordItrGS;                                                   // Corresponding iterator in the generating set
        // Update neighbors with the stored values
        if (next(itr) != GS.end())
        {

            auto oldPair = getOppositePair(*next(itr));
            flipit(itr, next(itr), newChord, oldChord); // if it is not the last edge, update the next edge
            auto newPair = getOppositePair(*next(itr));
            auto nextItrGSChord = *(next(itr));

            if (present.find(oldPair) == present.end() && present.find(newPair) != present.end())
            {

                VGS.erase(nextItrGSChord->chordItrVGS); // if the next edge becomes invalid, remove it from the list of all edges
            }

            if (present.find(oldPair) != present.end() && present.find(newPair) == present.end())
            {
                auto insertpos = next(e->chordItrVGS);
                VGS.insert(insertpos, nextItrGSChord); // if the next edge becomes valid, add it to the list of all edges

                nextItrGSChord->chordItrVGS = prev(insertpos); // update the iterator of the chord
            }
        }
        if (itr != GS.begin())
        {
            auto oldPair = getOppositePair(*prev(itr));
            flipit(itr, prev(itr), newChord, oldChord); // if it is not the first edge, update the previous edge
            auto newPair = getOppositePair(*prev(itr));
            auto prevItrGSChord = *(prev(itr));

            if (present.find(oldPair) == present.end() && present.find(newPair) != present.end())
            {
                VGS.erase(prevItrGSChord->chordItrVGS); // if the previous edge becomes invalid, remove it from the list of all edges
            }
            if (present.find(oldPair) != present.end() && present.find(newPair) == present.end())
            {
                auto insertpos = e->chordItrVGS;
                VGS.insert(insertpos, prevItrGSChord); // if the previous edge becomes valid, add it to the list of all edges

                prevItrGSChord->chordItrVGS = prev(insertpos); // update the iterator of the chord
            }
        }
        // Now flip the edge
        present.erase(getPair(e));
        e->flip();
        present.insert(getPair(e));
    }

    /// @brief Generates child triangulations by flipping the edge pointed to by the iterator
    /// @param itr Iterator pointing to the edge to be flipped
    void generateChildTriangulations(list<Edge *>::iterator &iteratorToFlip)
    {
        auto itrVGS = iteratorToFlip;

        flip(itrVGS); // Flip the edge at the current iterator, and update neighbors accordingly

        bool lastChordGS = false;           // Flag to check if the current edge is the last in the generating set
        bool lastChordVGS = false;          // Flag to check if the current edge is the last in the list of all edges
        Edge *next_chord_gs;                // Pointer to the next chord in the generating set
        Edge *next_chord_vgs;               // Pointer to the next chord in the list of all valid generating set
        auto itrGS = (*itrVGS)->chordItrGS; // Get the corresponding iterator in the generating set
        if (next(itrGS) == GS.end())
        {
            lastChordGS = true; // If it is the last edge, set the flag to true
        }
        else
        {
            next_chord_gs = *next(itrGS); // Get the next chord
        }

        if (next(itrVGS) == VGS.end())
        {
            lastChordVGS = true; // If it is the last edge, set the flag to true
        }
        else
        {
            next_chord_vgs = *next(itrVGS); // Get the next valid chord
        }

        Edge *c = *itrVGS;              // Current chord to be processed
        list<Edge *>::iterator itrloop; // Iterator for looping through the generating set
        if (itrVGS == VGS.begin())
        {
            itrloop = next(itrVGS); // if it is the first edge, start from the next edge
        }
        else
        {
            auto prevItrVGS = prev(itrVGS);
            if ((*prevItrVGS)->second == min(c->first, c->second))
            {
                itrloop = prevItrVGS; // if the immidiate previous edge has the same second vertex, start from the previous edge
            }
            else
            {
                itrloop = next(itrVGS); // else start from the next edge
            }
        }

        GS.erase(itrGS);   // Remove the current edge from the generating set
        VGS.erase(itrVGS); // Remove the current edge from the list of all edges

        output();

        for (; itrloop != VGS.end(); itrloop++)
        {
            // Recursively generate child triangulations for edges that can block the current edge
            generateChildTriangulations(itrloop);
        }
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

        if (lastChordVGS) // If the current edge was the last in the list of all edges
        {
            // cout << "last chord" << endl;
            VGS.push_back(c);
            itrVGS = prev(VGS.end());
            c->chordItrVGS = itrVGS; // Update the iterator of the chord
        }
        else
        {
            // If there are more edges in the generating set
            itrVGS = VGS.insert(next_chord_vgs->chordItrVGS, c);
            c->chordItrVGS = itrVGS; // Update the iterator of the chord
        }

        flip(itrVGS); // Flip back the edge to restore the original state
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
            if (present.find(getOppositePair(e)) == present.end())
            {
                e->isValid = true;
                VGS.push_back(e); // adding the edge to the list of all edges for memory management
                auto itrVGS = prev(VGS.end());
                e->chordItrVGS = itrVGS; // setting the iterator of the chord
            }
            else
            {
                e->isValid = false;
            }
        }

        // addTriangulation(); // adding the initial root triangulation
        output();
        for (auto itr = VGS.begin(); itr != VGS.end(); itr++)
        {
            generateChildTriangulations(itr); // generating child triangulations recursively
        }

        for (auto &chord : chords)
        {
            // cout << "erasing the chords" << endl;
            present.erase(getPair(chord)); // unmarking the edges after finishing
            // cout << "we are done erasing the chords" << endl;
        }
    }

    void output()
    {
        gt->output(serial);
    }
};