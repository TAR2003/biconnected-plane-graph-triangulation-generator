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
    /// @brief the list of all edges in the cycle (for memory management)
    // list<Edge *> VGS;
    /// @brief the vertex number of the cycle
    int n;
    /// @brief set of all present chords in the original graph (reference to shared set)
    unordered_multiset<pair<int, int>, PairHash> &present;
    /// @brief set of all present chords in the current face, need to keep track for the root triangulation multi edge conflict
    unordered_multiset<pair<int, int>, PairHash> presentFace;
    /// @brief all the triangulations generated
    vector<vector<pair<int, int>>> allTriangulations;
    vector<int> positions; // vector to store the positions of the vertices in the cycle
    vector<int> elements;  // vector to store the elements of the cycle

    int problems;
    biconnected *bc;
    int serial;

    /// @brief the constructor of the class
    /// @param n the number of vertices in the cycle
    /// @param elements the elements of the cycle
    /// @param present the set of present chords
    /// @param serial the serial number of the face
    /// @param bc pointer to the biconnected class
    FaceTriangulation(int n, vector<int> &elements, unordered_multiset<pair<int, int>, PairHash> &present, int serial, biconnected *bc)
        : n(n), present(present), elements(elements), serial(serial), bc(bc), positions(n, -1),  problems(0)
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

    void printPair(pair<int, int> p)
    {
        cout << " (" << p.first << ", " << p.second << ") ";
    }

    pair<int, int> getPair(Edge *e)
    {
        return {min(positions[e->first], positions[e->second]), max(positions[e->first], positions[e->second])};
    }

    pair<int, int> getOppositePair(Edge *e)
    {
        pair<int, int> p = {min(positions[e->opposite_first], positions[e->opposite_second]), max(positions[e->opposite_first], positions[e->opposite_second])};
        return p;
    }

    bool returnTrue()
    {
        cout << "always return true" << endl;
        return true;
    }

    /// @brief finds a safe root for the cycle and updates the positions vector accordingly
    void findSafeRoot()
    {
        int startIndex = 0;
        int endIndex = n - 2;
        while (startIndex < endIndex - 1)
        {
            if (present.find({elements[startIndex], elements[endIndex]}) != present.end() || present.find({elements[endIndex], elements[startIndex]}) != present.end())
            {
                startIndex++;
            }
            else
            {
                endIndex--;
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
    /// @param itrVGS Iterator pointing to the edge to be flipped
    void flip(list<Edge *>::iterator itrGS)
    {
        // cout << "Flipping edge: " << (*itrGS)->first << " " << (*itrGS)->second << endl;
        Edge *e = *itrGS; // Edge to be flipped

        // Store the values BEFORE flipping
        pair<int, int> newChord = make_pair(e->opposite_first, e->opposite_second); // New chord after flip
        pair<int, int> oldChord = make_pair(e->first, e->second);                   // Old chord before flip
        auto itr = e->chordItrGS;                                                   // Corresponding iterator in the generating set
        // Update neighbors with the stored values
        if (next(itr) != GS.end())
        {

            auto nextitr = *next(itr);
            auto oldPair = getOppositePair(nextitr);
            flipit(itr, next(itr), newChord, oldChord); // if it is not the last edge, update the next edge
        }
        if (itr != GS.begin())
        {
            auto oldPair = getOppositePair(*prev(itr));
            flipit(itr, prev(itr), newChord, oldChord); // if it is not the first edge, update the previous edge
        }

        // cout << "=================================" << endl;
        // cout << "Printing GS : " << endl;
        // for (auto &edge : GS)
        // {
        //     cout << "(" << positions[edge->first] << ", " << positions[edge->second] << ") , ";
        // }
        // cout << endl;
        // cout << "Problems: " << problems << endl;

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
        // cout << "Flipping edge: " << getOppositePair(e).first << " " << getOppositePair(e).second << endl;
        // cout << "Flipped edge: " << positions[e->first] << " " << positions[e->second] << endl;
        // cout << "Printing GS : " << endl;
        // for (auto &edge : GS)
        // {
        //     cout << "(" << positions[edge->first] << ", " << positions[edge->second] << ") , ";
        // }
        // cout << endl;
        // cout << "Problems: " << problems << endl;
        // cout << "=================================" << endl;
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
    void generateChildTriangulations(list<Edge *>::iterator &itrGS)
    {
        bc->totalChecks++;
        auto oppositePair = getOppositePair(*itrGS);
        if (oppositePair.first == oppositePair.second)
        {
            return;
        }
        // cout << "On the matter of flipping the chord: " << oppositePair.first << " " << oppositePair.second << endl;
        if (present.find(oppositePair) != present.end())
        {
            if (presentFace.find(oppositePair) == presentFace.end())
            {
                return;
            }
        }
        bc->successfulChecks++;

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

        // cout << "Now the situation for GS" << endl;
        // printSet(GS);
        // cout << "Now the opposite pair for GS" << endl;
        // for (auto &edge : GS)
        // {
        //     cout << "Opposite pair for edge: (" << positions[edge->first] << ", " << positions[edge->second] << ") is: (" << getOppositePair(edge).first << ", " << getOppositePair(edge).second << ")" << endl;
        // }

        output();

        for (; itrloop != GS.end(); itrloop++)
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

        // cout << "Now performing the reverse flip for the edge : " << c->first << " " << c->second << endl;
        flip(itrGS); // Flip back the edge to restore the original state
        // cout << "Reverse flip done for the edge : " << c->first << " " << c->second << endl;

        // printSet(GS);
        // cout << "Now the opposite pair for GS" << endl;
        // for (auto &edge : GS)
        // {
        //     cout << "Opposite pair for edge: (" << positions[edge->first] << ", " << positions[edge->second] << ") is: (" << getOppositePair(edge).first << ", " << getOppositePair(edge).second << ")" << endl;
        // }
    }

    /// @brief generates all triangulations of the cycle
    void generateAllTriangulations()
    {
        // cout << "Started at generate all triangulation method" << endl;
        // cout << "A positions elements" << endl;
        // for (auto a : positions)
        // {
        //     cout << a << " ";
        // }
        // cout << endl;

        for (int i = 2; i < n - 1; i++)
        {
            
            Edge *e = new Edge(0, i, i - 1, (i + 1) % n); // creating a new edge object
            GS.push_back(e);                              // adding the edge to the generating set
            chords.push_back(e);                          // adding the edge to the list of all chords

            if (presentFace.find(getPair(e)) != presentFace.end() || positions[e->first] == positions[e->second] || present.find(getPair(e)) != present.end())
            {
                problems++;
            }


            present.insert(getPair(e));                   // marking the edge as present in the original graph
            presentFace.insert(getPair(e));
            auto itrGS = prev(GS.end());
            e->chordItrGS = itrGS; // setting the iterator of the chord
            
        }

        // cout << "Finished initializing triangulations" << endl;
        // addTriangulation(); // adding the initial root triangulation
        
        output();
        

        // printSet(GS);

        for (auto itr = GS.begin(); itr != GS.end(); itr++)
        {
            // cout << "Situation for GS in the main loop" << endl;
            // printSet(GS);
            // cout << "Generating child triangulations for edge: " << (*itr)->first << " " << (*itr)->second << endl;
            generateChildTriangulations(itr); // generating child triangulations recursively
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
};

// Include biconnected.hpp after class declaration to resolve circular dependency
#include "biconnected.hpp"

// Define output() method after including biconnected.hpp
inline void FaceTriangulation::output()
{
    // cout << "Problems: " << problems << endl;
    if(problems == 0) 
    {
        bc->output(serial);
    }
    else {
        bc->invalidTraversals++;
    }
    
}
