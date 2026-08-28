#include <bits/stdc++.h>
using namespace std;
#pragma once
#include "Edge.hpp"
#include "pairHash.hpp"
#include "FaceTriangulation.hpp"
#include "ParvezRahmanNakano.hpp"

class triconnected
{
public:
    vector<vector<int>> faces;
    vector<vector<pair<int, int>>> allTriangulations;
    vector<vector<vector<pair<int, int>>>> triangulation_per_face;
    unordered_set<pair<int, int>, PairHash> present;

    triconnected(vector<vector<int>> &faces)
    {
        this->faces = faces;

        present = unordered_set<pair<int, int>, PairHash>();
        initiatePresent();
        triangulation_per_face = vector<vector<vector<pair<int, int>>>>(faces.size());
    }

    void initiatePresent()
    {
        for (auto face : faces)
        {
            for (int i = 0; i < face.size() - 1; i++)
            {
                present.insert(make_pair(min(face[i], face[i + 1]), max(face[i], face[i + 1])));
            }
            present.insert(make_pair(min(face[0], face[face.size() - 1]), max(face[0], face[face.size() - 1])));
        }
    }

    void getAllTriangulations()
    {
        ParvezRahmanNakano *ft;
        int pos = 0;
        for (auto &face : faces)
        {
            int n = face.size();
            ft = new ParvezRahmanNakano(n);
            ft->generateAllTriangulations();
            for (auto &triangulation : ft->allTriangulations)
            {
                vector<pair<int, int>> mappedTriangulation;
                for (auto &chord : triangulation)
                {
                    mappedTriangulation.push_back({min(face[chord.first], face[chord.second]), max(face[chord.first], face[chord.second])});
                }
                sort(mappedTriangulation.begin(), mappedTriangulation.end());
                triangulation_per_face[pos].push_back(mappedTriangulation);
            }
            delete ft;
            pos++;
        }
    }

    // Retained for external callers / backward compatibility. No longer used
    // internally by refineTriangulations (the DFS below builds combinations
    // incrementally instead of materializing the full cartesian product),
    // but left intact in case anything outside this class calls it directly.
    void combineTriangulations(
        const vector<vector<vector<pair<int, int>>>> &triangulation_per_face,
        int index,
        vector<pair<int, int>> &current,
        vector<vector<pair<int, int>>> &allTriangulations)
    {
        while (index < (int)triangulation_per_face.size() &&
               triangulation_per_face[index].empty())
        {
            index++;
        }

        if (index == (int)triangulation_per_face.size())
        {
            allTriangulations.push_back(current);
            return;
        }

        for (const auto &tri : triangulation_per_face[index])
        {
            size_t oldSize = current.size();
            current.insert(current.end(), tri.begin(), tri.end());
            combineTriangulations(triangulation_per_face, index + 1, current, allTriangulations);
            current.resize(oldSize);
        }
    }

    void sortTriangulations()
    {
        for (auto &triangulations : triangulation_per_face)
        {
            sort(triangulations.begin(), triangulations.end());
        }
    }

    // Retained for backward compatibility. No longer needed by
    // refineTriangulations, since the DFS now rejects invalid combinations
    // (duplicate/boundary chords) as soon as they'd be created, rather than
    // building everything and filtering afterwards.
    void removeDuplicated()
    {
        vector<vector<pair<int, int>>> uniqueTriangulations;
        int totalLength = allTriangulations[0].size();
        for (auto &triangulation : allTriangulations)
        {
            set<pair<int, int>> allPairsInsideThisTriangulation;
            bool arm = false;
            for (auto &p : triangulation)
            {
                int a = min(p.first, p.second);
                int b = max(p.first, p.second);
                allPairsInsideThisTriangulation.insert({a, b});
                if (present.find({a, b}) != present.end())
                {
                    arm = true;
                    break;
                }
            }
            if (allPairsInsideThisTriangulation.size() != totalLength)
            {
                continue;
            }
            if (arm)
            {
                continue;
            }
            else
            {
                uniqueTriangulations.push_back(triangulation);
            }
        }
        allTriangulations.clear();
        for (auto triangulation : uniqueTriangulations)
        {
            allTriangulations.push_back(triangulation);
        }
    }

    // ------------------------------------------------------------------
    // Incremental DFS + backtracking combiner.
    //
    // Instead of building the full cartesian product across all faces and
    // then throwing away invalid/duplicate results at the end (which blows
    // up memory when there are many faces, each with many local
    // triangulations), we build one candidate combination chord-by-face at
    // a time, validating as we go:
    //
    //   - a chord that is actually a boundary edge (in `present`) is never
    //     a valid diagonal -> reject this face-triangulation immediately.
    //   - a chord that duplicates a chord already chosen for an earlier
    //     face in this same partial combination -> reject immediately.
    //
    // Only when a face's triangulation passes both checks do we add its
    // chords to the running `current` combination and recurse into the
    // next face. On backtrack we remove exactly what we added, so `current`
    // and the `usedChords` tracking set stay in sync automatically.
    //
    // A full combination is only ever stored in `allTriangulations` once
    // every face has contributed a validated triangulation, so no invalid
    // or duplicate-chord combination is ever materialized in memory.
    // ------------------------------------------------------------------
    void combineTriangulationsDFS(
        int faceIndex,
        vector<pair<int, int>> &current,
        unordered_set<pair<int, int>, PairHash> &usedChords)
    {
        // skip faces that produced no triangulations at all (mirrors the
        // original behaviour of combineTriangulations)
        while (faceIndex < (int)triangulation_per_face.size() &&
               triangulation_per_face[faceIndex].empty())
        {
            faceIndex++;
        }

        if (faceIndex == (int)triangulation_per_face.size())
        {
            // every face has contributed a validated, non-duplicate,
            // non-boundary set of chords -> this is a complete, valid
            // triangulation of the triconnected component
            allTriangulations.push_back(current);
            return;
        }

        for (const auto &tri : triangulation_per_face[faceIndex])
        {
            // --- validate this candidate face-triangulation against the
            // --- state accumulated so far, before touching `current` ---
            bool valid = true;
            for (const auto &raw : tri)
            {
                int a = min(raw.first, raw.second);
                int b = max(raw.first, raw.second);

                // reject: chord is actually a boundary edge of the
                // triconnected component, not a valid internal diagonal
                if (present.find({a, b}) != present.end())
                {
                    valid = false;
                    break;
                }
                // reject: chord already used by an earlier face in this
                // partial combination (would create a size mismatch /
                // duplicate exactly like the old post-hoc check did)
                if (usedChords.find({a, b}) != usedChords.end())
                {
                    valid = false;
                    break;
                }
            }
            if (!valid)
                continue;

            // also guard against duplicate chords *within* this single
            // face's own triangulation (shouldn't normally happen, but
            // keeps behaviour identical to the strict size check before)
            {
                set<pair<int, int>> withinFace;
                bool internalDup = false;
                for (const auto &raw : tri)
                {
                    int a = min(raw.first, raw.second);
                    int b = max(raw.first, raw.second);
                    if (!withinFace.insert({a, b}).second)
                    {
                        internalDup = true;
                        break;
                    }
                }
                if (internalDup)
                    continue;
            }

            // --- commit: add this face's chords, recurse, then backtrack ---
            size_t oldSize = current.size();
            vector<pair<int, int>> addedChords;
            addedChords.reserve(tri.size());

            for (const auto &raw : tri)
            {
                int a = min(raw.first, raw.second);
                int b = max(raw.first, raw.second);
                current.push_back({a, b});
                usedChords.insert({a, b});
                addedChords.push_back({a, b});
            }

            combineTriangulationsDFS(faceIndex + 1, current, usedChords);

            // backtrack: undo exactly what we added for this branch
            current.resize(oldSize);
            for (const auto &c : addedChords)
            {
                usedChords.erase(c);
            }
        }
    }

    void refineTriangulations()
    {
        sortTriangulations();
        allTriangulations.clear();

        vector<pair<int, int>> current;
        unordered_set<pair<int, int>, PairHash> usedChords;
        combineTriangulationsDFS(0, current, usedChords);

        // no post-hoc removeDuplicated() needed: the DFS above only ever
        // commits a complete combination once every face's contribution
        // has already been checked against `present` and against chords
        // used by earlier faces, so every entry in allTriangulations here
        // is already valid and duplicate-free.
    }

    void printTriangulationsPerFace()
    {
        int face_no = 0;
        for (auto &face_triangulations : triangulation_per_face)
        {
            cout << "Face " << face_no << " has " << face_triangulations.size() << " triangulations:" << endl;
            for (auto &triangulation : face_triangulations)
            {
                for (auto &chord : triangulation)
                {
                    cout << "(" << chord.first << ", " << chord.second << ") , ";
                }
                cout << endl;
            }
            face_no++;
        }
    }

    void printAllTriangulations()
    {
        cout << "Total triangulations in triconnected component: " << allTriangulations.size() << endl;
        for (auto &triangulation : allTriangulations)
        {
            for (auto &chord : triangulation)
            {
                cout << "(" << chord.first << ", " << chord.second << ") , ";
            }
            cout << endl;
        }
    }
};