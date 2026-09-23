#pragma once
#include <bits/stdc++.h>
#include "FaceTriangulation.hpp"
#include "GraphTriangulation.hpp"
using namespace std;



class FaceTriangulationOneconnected : public FaceTriangulation
{
public:
    FaceTriangulationOneconnected(long long n, vector<long long> &elements, unordered_multiset<pair<long long, long long>, PairHash> &present, long long serial, GraphTriangulation *gt)
        : FaceTriangulation(n, elements, present, serial, gt) {}

    /// @brief Flips the edge pointed to by the iterator in the generating set
    /// @param itrVGS Iterator pointing to the edge to be flipped
    void flip(list<Edge *>::iterator iteratorToFlip)
    {
        // cout << "Flipping edge: " << (*itrGS)->first << " " << (*itrGS)->second << endl;
        auto itrGS = iteratorToFlip; // Use the provided iterator directly
        Edge *e = *itrGS; // Edge to be flipped

        // Store the values BEFORE flipping
        pair<long long, long long> newChord = make_pair(e->opposite_first, e->opposite_second); // New chord after flip
        pair<long long, long long> oldChord = make_pair(e->first, e->second);                   // Old chord before flip
        auto itr = e->chordItrGS;                                                   // Corresponding iterator in the generating set
        // Update neighbors with the stored values
        if (next(itr) != GS.end())
        {
            flipit(itr, next(itr), newChord, oldChord); // if it is not the last edge, update the next edge
        }
        if (itr != GS.begin())
        {
            flipit(itr, prev(itr), newChord, oldChord); // if it is not the first edge, update the previous edge
        }
        // Now flip the edge
        auto it = present.find(getPair(e));
        if (it != present.end())
            present.erase(it);
        auto it2 = presentFace.find(getPair(e));
        if (it2 != presentFace.end())
            presentFace.erase(it2);

        e->flip();

        auto oldPair = getOppositePair(e);
        auto newPair = getPair(e);
        if (presentFace.find(oldPair) != presentFace.end() || oldPair.first == oldPair.second || present.find(oldPair) != present.end())
        {
            problems--;
        }
        if (presentFace.find(newPair) != presentFace.end() || newPair.first == newPair.second || present.find(newPair) != present.end())
        {
            problems++;
        }

        present.insert(getPair(e));
        presentFace.insert(getPair(e));
    }

    /// @brief Generates child triangulations by flipping the edge pointed to by the iterator
    /// @param itr Iterator pointing to the edge to be flipped
    void generateChildTriangulations(list<Edge *>::iterator &iteratorToFlip)
    {
        auto itrGS = iteratorToFlip;
        gt->totalChecks++;
        auto oppositePair = getOppositePair(*itrGS);
        if (oppositePair.first == oppositePair.second)
        {
            return;
        }
        // cout << "On the matter of flipping the chord: " << oppositePair.first << " " << oppositePair.second << endl;
        if (present.find(oppositePair) != present.end())
        {
            if (oppositePair.first != positions[0] && oppositePair.second != positions[0])
            {
                return; // that means it does not conflict with any of the root triangulation generating set
            }
            if (presentFace.find(oppositePair) == presentFace.end())
            {
                return; // that means it might have a conflict with the border chord or other edges from other faces
            }
        }
        gt->successfulChecks++;

        // cout << "before flipping edge: " << (*itrGS)->first << " " << (*itrGS)->second << endl;
        flip(itrGS); // Flip the edge at the current iterator, and update neighbors accordingly
        // cout << "after flipping edge: " << (*itrGS)->first << " " << (*itrGS)->second << endl;

        bool lastChordGS = false; // Flag to check if the current edge is the last in the generating set
        Edge *next_chord_gs;      // Pointer to the next chord in the generating set
        if (next(itrGS) == GS.end())
        {
            lastChordGS = true; // If it is the last edge, set the flag to true
        }
        else
        {
            next_chord_gs = *next(itrGS); // Get the next chord
        }

        Edge *c = *itrGS;               // Current chord to be processed
        list<Edge *>::iterator itrloop; // Iterator for looping through the generating set
        if (itrGS == GS.begin())
        {
            itrloop = next(itrGS); // if it is the first edge, start from the next edge
        }
        else
        {
            auto prevItrGS = prev(itrGS);
            if ((*prevItrGS)->second == min(c->first, c->second))
            {
                itrloop = prevItrGS; // if the immidiate previous edge has the same second vertex, start from the previous edge
            }
            else
            {
                itrloop = next(itrGS); // else start from the next edge
            }
        }

          GS.erase(itrGS); // Remove the current edge from the generating set

          output();

            for (; itrloop != GS.end();)
        {
            // Recursively generate child triangulations for edges that can block the current edge
                Edge *child = *itrloop;
            generateChildTriangulations(itrloop);
                itrloop = next(child->chordItrGS);
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

        // cout << "Now performing the reverse flip for the edge : " << c->first << " " << c->second << endl;
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

            if (presentFace.find(getPair(e)) != presentFace.end() || positions[e->first] == positions[e->second] || present.find(getPair(e)) != present.end())
            {
                problems++;
            }

            present.insert(getPair(e)); // marking the edge as present in the original graph
            presentFace.insert(getPair(e));
            auto itrGS = prev(GS.end());
            e->chordItrGS = itrGS; // setting the iterator of the chord
        }
        // addTriangulation(); // adding the initial root triangulation

        output();

        // printSet(GS);

        for (auto itr = GS.begin(); itr != GS.end();)
        {
            Edge *child = *itr;
            generateChildTriangulations(itr); // generating child triangulations recursively
            itr = next(child->chordItrGS);
        }

        for (auto &chord : chords)
        {
            // cout << "erasing the chords" << endl;
            auto it = present.find(getPair(chord));
            if (it != present.end())
                present.erase(it); // unmarking the edges after finishing
            // cout << "we are done erasing the chords" << endl;
            auto it2 = presentFace.find(getPair(chord));
            if (it2 != presentFace.end())
                presentFace.erase(it2);
        }
    }

    void output()
    {
        // cout << "Problems: " << problems << endl;
        if (problems == 0)
        {
            gt->output(serial);
        }
        else
        {
            gt->invalidTraversals++;
            gt->onProgressTick(*gt); // same optional checkpoint hook as the valid-triangulation path
        }
    }
};