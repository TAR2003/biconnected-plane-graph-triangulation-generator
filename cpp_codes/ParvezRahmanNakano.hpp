#include <bits/stdc++.h>
using namespace std;
#include "Edge.hpp"

class ParvezRahmanNakano
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
    ParvezRahmanNakano(int n)
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
    void flipit(list<Edge *>::iterator itr, list<Edge *>::iterator itr_next)
    {
        if (next(itr) == GS.end())
            return;
        Edge *e = *itr;
        Edge *next_e = *next(itr);
        pair<int, int> newChord = make_pair(e->opposite_u, e->opposite_v);
        pair<int, int> oldChord = make_pair(e->u, e->v);
        int oldPoint = oldChord.first;
        if (oldPoint == next_e->u || oldPoint == next_e->v)
        {
            oldPoint = oldChord.second;
        }
        int newPoint = newChord.first;
        if (newPoint == next_e->u || newPoint == next_e->v)
        {
            newPoint = newChord.second;
        }
        if (oldPoint == next_e->u)
        {
            next_e->opposite_u = newPoint;
        }
        if (oldPoint == next_e->v)
        {
            next_e->opposite_v = newPoint;
        }
    }

    void flip(list<Edge *>::iterator itr)
    {
        Edge *e = *itr;
        pair<int, int> newChord = make_pair(e->opposite_u, e->opposite_v);
        pair<int, int> oldChord = make_pair(e->u, e->v);

        if (next(itr) != GS.end())
            flipit(itr, next(itr));
        if (itr != GS.begin())
            flipit(itr, prev(itr));

        e->flip();
    }

    void addTriangulation()
    {
        cout << "Added a new triangulation" << endl;
        vector<pair<int, int>> currentTriangulation;
        for (auto &chord : chords)
        {
            currentTriangulation.push_back({chord->u, chord->v});
        }
        for(int i = 0 ; i < currentTriangulation.size() ; i++) {
            cout << "( " << currentTriangulation[i].first << " " << currentTriangulation[i].second << " ),";
        }
        cout << endl;
        allTriangulations.push_back(currentTriangulation);
    }

    void generateChildTriangulations(int leftmost_blocking_b)
    {
        addTriangulation();

        for (auto itr = GS.begin(); itr != GS.end(); itr++)
        {
            Edge *chord = *itr;
            if (chord->u >= leftmost_blocking_b)
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
                    Edge *next_chord = *next(itr);
                }

                GS.erase(itr);
                generateChildTriangulations(min(chord->u, chord->v));
                if (lastChord)
                {
                    GS.push_back(chord);
                    auto tempitr = prev(GS.end());
                    chord->chordItr = tempitr;
                }
                else
                {
                    auto tempitr = GS.insert(next_chord->chordItr, chord);
                    chord->chordItr = tempitr;
                }

                flip(itr);
            }
        }
    }

    void generateAllTriangulations()
    {
        for (int i = 2; i < n - 1; i++)
        {
            Edge *e = new Edge(0, i, i - 1, (i + 1) % n);
            GS.push_back(e);
            chords.push_back(e);
        }
        generateChildTriangulations(0);
    }
};