#include <bits/stdc++.h>
using namespace std;
#include "PairHash.hpp"
#include "RotationSystem.hpp"

class Chord
{
public:
    long long first, second;
    long long oppositeFirst, oppositeSecond;
    Chord *nextChord, *prevChord;
    Chord *nextGS, *prevGS;
    Chord *nextVGS, *prevVGS;
    long long faceIndex;
    bool isValid = false;
    Chord(long long a = 0, long long b = 0, long long c = 0, long long d = 0) : first(a), second(b), oppositeFirst(c), oppositeSecond(d) {}
    void flip()
    {
        swap(first, oppositeFirst);
        swap(second, oppositeSecond);
    }
};

class CustomHashMap
{
public:
    unordered_multiset<pair<long long, long long>, PairHash> present;
    CustomHashMap()
    {
        present = unordered_multiset<pair<long long, long long>, PairHash>();
    }
    void makeSortedPair(pair<long long, long long> &p)
    {
        if (p.first > p.second)
        {
            swap(p.first, p.second);
        }
    }

    void insert(pair<long long, long long> p)
    {
        makeSortedPair(p);
        present.insert(p);
    }
    void erase(pair<long long, long long> p)
    {
        makeSortedPair(p);
        auto it = present.find(p);
        if (it != present.end())
        {
            present.erase(it);
        }
    }
    bool find(pair<long long, long long> p)
    {
        makeSortedPair(p);
        return present.find(p) != present.end();
    }
};

class GenerateTriangulations
{
public:
    long long totalVertices;
    long long totalTriangulations;
    long long triangulationLimit;
    vector<vector<long long>> adjacentSet;
    vector<vector<long long>> faces;
    vector<vector<long long>> positions;
    vector<Chord *> headChord;
    vector<Chord *> headGS;
    vector<Chord *> headVGS;
    CustomHashMap present;
    GenerateTriangulations(long long totalVertices, vector<vector<long long>> &adjacentSet, long long triangulationLimit = LONG_LONG_MAX)
    {
        this->totalVertices = totalVertices;
        this->triangulationLimit = triangulationLimit;
        this->adjacentSet = adjacentSet;
        this->totalTriangulations = 0;
        this->present = CustomHashMap();
        auto tempfaces = rotationSystemToFaces(totalVertices, adjacentSet);

        for (auto &face : tempfaces)
        {
            if (face.size() > 3)
            {
                faces.push_back(face);
            }
        }
        initChords();
    }
    ~GenerateTriangulations()
    {
        for (int i = 0; i < faces.size(); i++)
        {
            auto itr = headChord[i];
            while (itr != nullptr)
            {
                auto temp = itr;
                itr = itr->nextChord;
                delete temp;
            }
            itr = headGS[i];
            while (itr != nullptr)
            {
                auto temp = itr;
                itr = itr->nextGS;
                delete temp;
            }
            itr = headVGS[i];
            while (itr != nullptr)
            {
                auto temp = itr;
                itr = itr->nextVGS;
                delete temp;
            }
        }
    }
    void initChords()
    {
        headChord = vector<Chord *>(faces.size(), nullptr);
        headGS = vector<Chord *>(faces.size(), nullptr);
        headVGS = vector<Chord *>(faces.size(), nullptr);
        for (int i = 0; i < faces.size(); i++)
        {
            headChord[i] = new Chord();
            auto temp = headChord[i];
            for (int j = 0; j < faces[i].size() - 4; j++)
            {
                temp->nextChord = new Chord();
                temp->nextChord->prevChord = temp;
                temp = temp->nextChord;
            }
        }
    }

    void generateAllTriangulations()
    {
        generateFaceTriangulations(0);
    }

    void findSafeRoot(long long faceIndex)
    {
        long long startIndex = 0;
        long long endIndex = faces[faceIndex].size();
        while (startIndex < endIndex - 1)
        {
            if (present.find({faces[faceIndex][startIndex], faces[faceIndex][endIndex]}) || faces[faceIndex][startIndex] == faces[faceIndex][endIndex])
            {
                startIndex++;
            }
            else
            {
                endIndex--;
            }
        }
        // start Index is the safe root
        for (long long i = 0; i < faces[faceIndex].size(); i++)
        {
            positions[faceIndex][i] = faces[faceIndex][(startIndex + i) % faces[faceIndex].size()];
        }
    }

    void setupChords(long long faceIndex)
    {
        auto temp = headChord[faceIndex];
        for (int i = 2; i < faces[faceIndex].size() - 1; i++)
        {
            temp->first = 0;
            temp->second = i + 2;
            present.insert({temp->first, temp->second});
            temp->oppositeFirst = (i + 1) % faces[faceIndex].size();
            temp->oppositeSecond = (i + 3) % faces[faceIndex].size();
            temp->faceIndex = faceIndex;
            temp = temp->nextChord;
        }
    }

    void setupGS(long long faceIndex)
    {
        headGS[faceIndex] = headChord[faceIndex];
        auto temp = headChord[faceIndex];
        while (temp->nextChord != nullptr)
        {
            temp->nextGS = temp->nextChord;
            temp->nextChord->prevGS = temp;
            temp = temp->nextChord;
        }
    }

    void setupVGS(long long faceIndex)
    {
        headVGS[faceIndex] = nullptr;
        auto tempGS = headGS[faceIndex];
        auto tempVGS = headVGS[faceIndex];
        while (tempGS != nullptr)
        {
            if (!present.find({tempGS->oppositeFirst, tempGS->oppositeSecond}))
            {
                tempGS->isValid = true;
                if (headVGS[faceIndex] == nullptr)
                {
                    headVGS[faceIndex] = tempGS;
                    tempVGS = headVGS[faceIndex];
                    tempVGS->prevVGS = nullptr;
                    tempVGS->nextVGS = nullptr;
                }
                else
                {
                    tempVGS->nextVGS = tempGS;
                    tempGS->prevVGS = tempVGS;
                    tempGS->nextVGS = nullptr;
                    tempVGS = tempGS;
                }
            }
            else {
                tempGS->isValid = false;
            }
            tempGS = tempGS->nextGS;
        }
    }

    void setupRootTriangulation(long long faceIndex)
    {
        setupChords(faceIndex);
        setupGS(faceIndex);
        setupVGS(faceIndex);
    }

    void removeCurrentChordsFromPresent(long long faceIndex)
    {
        auto temp = headChord[faceIndex];
        while (temp != nullptr)
        {
            present.erase({temp->first, temp->second});
            temp = temp->nextChord;
        }
    }

    bool limitExceeded()
    {
        return totalTriangulations >= triangulationLimit;
    }

    void visitAllBranches(long long faceIndex, Chord *itr)
    {
        while (itr != nullptr)
        {
            if (limitExceeded())
            {
                return;
            }
            generateChildTriangulations(faceIndex, itr);
            itr = itr->nextVGS;
        }
    }

    void updateOppositePair(long long faceIndex, Chord *c, Chord *itr)
    {
        if (itr == nullptr)
        {
            return;
        }
        long long before, after;
        if(c->oppositeFirst == itr->first || c->oppositeFirst == itr->second)
        {
            before = c->oppositeSecond;
        }
        else
        {
            before = c->oppositeFirst;
        }
        if(c->first == itr->first || c->first == itr->second)
        {
            after = c->second;
        }
        else
        {
            after = c->first;
        }
        if(before == itr->oppositeFirst) {
            itr->oppositeFirst = after;
        }
        else {
            itr->oppositeSecond = after;
        }
        if(itr->isValid && present.find({itr->oppositeFirst, itr->oppositeSecond})) {
            itr->isValid = false;
        }
        else if(!itr->isValid && !present.find({itr->oppositeFirst, itr->oppositeSecond})) {
            itr->isValid = true;
        }
    }

    void updateAssociatedChords(long long faceIndex, Chord *c)
    {
        updateOppositePair(faceIndex, c, c->nextGS);
        updateOppositePair(faceIndex, c, c->prevGS);
    }

    void flip(long long faceIndex, Chord *c)
    {

        present.erase({c->first, c->second});
        c->flip();
        present.insert({c->first, c->second});
        updateAssociatedChords(faceIndex, c);
    }

    void generateChildTriangulations(long long faceIndex, Chord *itr)
    {
        updateAssociatedChords(faceIndex, itr);
        flip(faceIndex, itr);

        if (itr->prevGS != nullptr && itr->prevGS->isValid)
        {
            visitAllBranches(faceIndex, itr->prevGS);
        }
        else
        {
            visitAllBranches(faceIndex, itr->nextGS);
        }

        flip(faceIndex, itr);
    }

    void generateFaceTriangulations(long long faceIndex)
    {
        findSafeRoot(faceIndex);
        setupRootTriangulation(faceIndex);
        output(faceIndex);
        auto tempVGS = headVGS[faceIndex];
        while (tempVGS != nullptr)
        {
            generateChildTriangulations(faceIndex, tempVGS);
            tempVGS = tempVGS->nextVGS;
        }
        removeCurrentChordsFromPresent(faceIndex);
    }

    void printTriangulation()
    {
        cout << "-------------- Printing the triangulation -------------" << endl;
        for (int i = 0; i < faces.size(); i++)
        {
            cout << "Face: " << i << " : ";
            auto tempChord = headChord[i];
            while (tempChord != nullptr)
            {
                cout << "(" << positions[i][tempChord->first] << "," << positions[i][tempChord->second] << ") ";
                tempChord = tempChord->nextChord;
            }
            cout << endl;
        }
        cout << "-------------- End of triangulation -------------" << endl;
    }

    void output(long long faceIndex)
    {
        if (faceIndex == faces.size() - 1)
        {
            totalTriangulations++;
            printTriangulation();
            return;
        }
        else
        {
            generateFaceTriangulations(faceIndex + 1);
        }
    }
};