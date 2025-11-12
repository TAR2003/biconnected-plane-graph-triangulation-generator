#include <bits/stdc++.h>
using namespace std;
#include "Edge.hpp"

class FaceTriangulation
{
public:
    /// @brief all the chords in the current cycle
    list<Edge *> chords;
    /// @brief the generating set of the cycle
    list<Edge *> GS;
    /// @brief the vertex number of the cycle
    int n;
    /// @brief all the triangulations generated
    vector<vector<pair<int, int>>> allTriangulations;

    /// @brief the constructor of the class
    /// @param n the number of vertices in the cycle
    FaceTriangulation(int n)
    {
        this->n = n;
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

    /// @brief the flip operation on a given edge
    /// @param e the edge to be flipped
    void flipit(list<Edge *>::iterator itr, list<Edge *>::iterator itr_next,
                pair<int, int> newChord, pair<int, int> oldChord)
    {
        if (itr_next == GS.end())
            return;

        Edge *next_e = *itr_next;

        int oldPoint = oldChord.first;
        if (oldPoint == next_e->first || oldPoint == next_e->second)
        {
            oldPoint = oldChord.second;
        }

        int newPoint = newChord.first;
        if (newPoint == next_e->first || newPoint == next_e->second)
        {
            newPoint = newChord.second;
        }

        if (next_e->opposite_first == oldPoint)
        {
            next_e->opposite_first = newPoint;
        }
        else if (next_e->opposite_second == oldPoint)
        {
            next_e->opposite_second = newPoint;
        }
    }

    void flip(list<Edge *>::iterator itr)
    {
        Edge *e = *itr;

        // Store the values BEFORE flipping
        pair<int, int> newChord = make_pair(e->opposite_first, e->opposite_second);
        pair<int, int> oldChord = make_pair(e->first, e->second);

        // Update neighbors with the stored values
        if (next(itr) != GS.end())
            flipit(itr, next(itr), newChord, oldChord);
        if (itr != GS.begin())
            flipit(itr, prev(itr), newChord, oldChord);

        // Now flip the edge
        e->flip();
    }

    void addTriangulation()
    {
        vector<pair<int, int>> currentTriangulation;
        for (auto &chord : chords)
        {
            currentTriangulation.push_back({chord->first, chord->second});
        }

        allTriangulations.push_back(currentTriangulation);
    }

    void generateChildTriangulations(list<Edge *>::iterator &itr)
    {

        flip(itr);
        bool lastChord = false;
        Edge *next_chord;
        if (next(itr) == GS.end())
        {
            lastChord = true;
        }
        else
        {
            next_chord = *next(itr);
        }
        Edge *c = *itr;
        list<Edge *>::iterator itrloop;
        if (itr == GS.begin())
        {
            itrloop = next(itr);
        }
        else
        {
            itrloop = prev(itr);
        }
        GS.erase(itr);

        int leftmost_blocking_b = min(c->first, c->second);

        addTriangulation();

        for (; itrloop != GS.end(); itrloop++)
        {
            if ((*itrloop)->second >= leftmost_blocking_b)
            {
                generateChildTriangulations(itrloop);
            }
        }
        if (lastChord)
        {
            GS.push_back(c);
            itr = prev(GS.end());
            c->chordItr = itr;
        }
        else
        {
            itr = GS.insert(next_chord->chordItr, c);
            c->chordItr = itr;
        }
        flip(itr);
    }

    void generateAllTriangulations()
    {
        for (int i = 2; i < n - 1; i++)
        {
            Edge *e = new Edge(0, i, i - 1, (i + 1) % n);
            GS.push_back(e);
            chords.push_back(e);
        }
        addTriangulation();
        for (auto itr = GS.begin(); itr != GS.end(); itr++)
        {
            generateChildTriangulations(itr);
        }
    }
};