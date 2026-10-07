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
    // ~Chord() {

    // }
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
    string result;
    GenerateTriangulations(long long totalVertices, vector<vector<long long>> &adjacentSet, long long triangulationLimit = LONG_LONG_MAX)
    {
        this->totalVertices = totalVertices;
        this->triangulationLimit = triangulationLimit;
        this->adjacentSet = adjacentSet;
        this->totalTriangulations = 0;
        this->present = CustomHashMap();
        this->result = "";
        auto tempfaces = rotationSystemToFaces(totalVertices, adjacentSet);

        for (auto &face : tempfaces)
        {
            if (face.size() > 3)
            {
                faces.push_back(face);
            }
        }
        this->positions = vector<vector<long long>>(faces.size());
        for (int i = 0; i < faces.size(); i++)
        {
            positions[i] = vector<long long>(faces[i].size());
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
        cout << "Generating all triangulations" << endl;
        generateFaceTriangulations(0);
        cout << "Total triangulations generated: " << totalTriangulations << endl;
        cout << result << endl;
    }

    void findSafeRoot(long long faceIndex)
    {
        long long startIndex = 0;
        long long endIndex = faces[faceIndex].size() - 2;
        while (startIndex < endIndex - 1)
        {
            cout << "Finding safe root for face: " << faceIndex << " with startIndex: " << startIndex << " and endIndex: " << endIndex << endl;
            cout << "Checking if edge exists for face: " << faceIndex << " with first: " << faces[faceIndex][startIndex] << " and second: " << faces[faceIndex][endIndex] << endl;
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
        cout << "Safe root: " << faces[faceIndex][startIndex] << endl;
        for (long long i = 0; i < faces[faceIndex].size(); i++)
        {
            positions[faceIndex][i] = faces[faceIndex][(startIndex + i) % faces[faceIndex].size()];
        }
    }

    void setupChords(long long faceIndex)
    {
        auto temp = headChord[faceIndex];
        for (int i = 0; i < faces[faceIndex].size() - 3; i++)
        {
            temp->first = 0;
            temp->second = i + 2;
            present.insert({positions[faceIndex][temp->first], positions[faceIndex][temp->second]});
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
        temp->prevGS = nullptr;
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
            if (!present.find({positions[faceIndex][tempGS->oppositeFirst], positions[faceIndex][tempGS->oppositeSecond]}))
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
            else
            {
                tempGS->isValid = false;
            }
            tempGS = tempGS->nextGS;
        }
        if(headVGS[faceIndex] != nullptr)
        {
            headVGS[faceIndex]->prevVGS = nullptr;
        }
    }

    void resetFaceVariables(long long faceIndex)
    {

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
            present.erase({positions[faceIndex][temp->first], positions[faceIndex][temp->second]});
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
        cout << "Printing variables again:: " << endl;
        printVariables();
        if (itr == nullptr)
        {
            return;
        }
        cout << "-=-=-=-=-= Updating opposite pair -=-=-=-" << endl;

        cout << "The current new chord: " << c->first << " and " << c->second << endl;
        cout << "the before chord: " << c->oppositeFirst << " and " << c->oppositeSecond << endl;
        if(itr == nullptr)
        {
            cout << "The iterator is null" << endl;
            
        }
        cout << "The opposite pair to update: " << itr->oppositeFirst << " and " << itr->oppositeSecond << endl;
        cout << "The current chord to update: " << itr->first << " and " << itr->second << endl;

        cout << "Before the opposite pair was: " << itr->oppositeFirst << " and " << itr->oppositeSecond << endl;
        if (itr == nullptr)
        {
            return;
        }
        long long before, after;
        if (c->oppositeFirst == itr->first || c->oppositeFirst == itr->second)
        {
            before = c->oppositeSecond;
        }
        else
        {
            before = c->oppositeFirst;
        }
        if (c->first == itr->first || c->first == itr->second)
        {
            after = c->second;
        }
        else
        {
            after = c->first;
        }
        if (before == itr->oppositeFirst)
        {
            itr->oppositeFirst = after;
        }
        else
        {
            itr->oppositeSecond = after;
        }
        cout << "After the opposite pair is: " << itr->oppositeFirst << " and " << itr->oppositeSecond << endl;
        cout << "Updating opposite pair for face: " << faceIndex << " with first: " << itr->first << " and second: " << itr->second << endl;
        if (itr->isValid && present.find({positions[faceIndex][itr->oppositeFirst], positions[faceIndex][itr->oppositeSecond]}))
        {
            itr->isValid = false;
        }
        else if (!itr->isValid && !present.find({positions[faceIndex][itr->oppositeFirst], positions[faceIndex][itr->oppositeSecond]}))
        {
            itr->isValid = true;
        }
    }

    void updateAssociatedChords(long long faceIndex, Chord *c)
    {
        cout << "Update associate chords" << endl;
        updateOppositePair(faceIndex, c, c->nextGS);
        updateOppositePair(faceIndex, c, c->prevGS);
    }

    void flip(long long faceIndex, Chord *c)
    {
        
        present.erase({positions[faceIndex][c->first], positions[faceIndex][c->second]});

        cout << "FLIPPED" << endl;
        c->flip();
        present.insert({positions[faceIndex][c->first], positions[faceIndex][c->second]});

        updateAssociatedChords(faceIndex, c);
      
    }

    void removeChord(long long faceIndex, Chord *c)
    {
        if (c->prevGS != nullptr)
        {
            c->prevGS->nextGS = c->nextGS;
        }
        if (c->nextGS != nullptr)
        {
            c->nextGS->prevGS = c->prevGS;
        }
        if (c->prevVGS != nullptr)
        {
            c->prevVGS->nextVGS = c->nextVGS;
        }
        if (c->nextVGS != nullptr)
        {
            c->nextVGS->prevVGS = c->prevVGS;
        }
    }

    void addChord(long long faceIndex, Chord *c)
    {
        if (c->prevGS != nullptr)
        {
            c->prevGS->nextGS = c;
        }
        if (c->nextGS != nullptr)
        {
            c->nextGS->prevGS = c;
        }
        if (c->prevVGS != nullptr)
        {
            c->prevVGS->nextVGS = c;
        }
        if (c->nextVGS != nullptr)
        {
            c->nextVGS->prevVGS = c;
        }
    }

    void generateChildTriangulations(long long faceIndex, Chord *itr)
    {
        printVariables();
        cout << "Front flip begin for face: " << faceIndex << " with first: " << itr->first << " and second: " << itr->second << " and opposite first: " << itr->oppositeFirst << " and opposite second: " << itr->oppositeSecond << endl;
        flip(faceIndex, itr);
        removeChord(faceIndex, itr);
        cout << "Front flip end" << endl;
        printVariables();
        output(faceIndex);
        if (itr->prevGS != nullptr && itr->prevGS->isValid)
        {
            visitAllBranches(faceIndex, itr->prevGS);
        }
        else
        {
            visitAllBranches(faceIndex, itr->nextVGS);
        }
        cout << "Back flip begin for face: " << faceIndex << " with first: " << itr->first << " and second: " << itr->second << " and opposite first: " << itr->oppositeFirst << " and opposite second: " << itr->oppositeSecond << endl;
        flip(faceIndex, itr);

        addChord(faceIndex, itr);
        cout << "Back flip end" << endl;
        printVariables();
    }

    void generateFaceTriangulations(long long faceIndex)
    {
        cout << "Generating triangulations for face: " << faceIndex << endl;
        findSafeRoot(faceIndex);
        cout << "Safe root found for face: " << faceIndex << endl;
        setupRootTriangulation(faceIndex);
        cout << "Root triangulation setup for face: " << faceIndex << endl;
        output(faceIndex);
        auto tempVGS = headVGS[faceIndex];
        cout << "Before visint all branches the present map is: " << endl;
        printVariables();
        visitAllBranches(faceIndex, tempVGS);
        removeCurrentChordsFromPresent(faceIndex);
        cout << "Finished generating triangulations for face: " << faceIndex << endl;
    }

    void printTriangulation()
    {

        cout << "-------------- Printing the triangulation -------------" << totalTriangulations << endl;
        result += "-------------- Printing the triangulation -------------\n" + to_string(totalTriangulations) + "\n";
        for (int i = 0; i < faces.size(); i++)
        {
            cout << "Face: " << i << " : ";
            result += "Face: " + to_string(i) + " : ";
            auto tempChord = headChord[i];
            while (tempChord != nullptr)
            {
                cout << "(" << positions[i][tempChord->first] << "," << positions[i][tempChord->second] << ") ";
                result += "(" + to_string(positions[i][tempChord->first]) + "," + to_string(positions[i][tempChord->second]) + ") ";
                tempChord = tempChord->nextChord;
            }
            result += "\n";
            cout << endl;
        }
        cout << "-------------- End of triangulation -------------" << endl;
        result += "-------------- End of triangulation -------------\n";
        printPresent();
    }

    void output(long long faceIndex)
    {
        cout << "output for face: " << faceIndex << "=========================================================================" << endl;
        // printVariables();
        if (faceIndex == faces.size() - 1)
        {
            totalTriangulations++;
            printTriangulation();
            printPresent();
            printGS();
            printVGS();
            return;
        }
        else
        {
            generateFaceTriangulations(faceIndex + 1);
        }
    }

    void printFaces()
    {
        cout << "Printing faces: " << endl;
        for (int i = 0; i < faces.size(); i++)
        {
            cout << "Face: " << i << " : ";
            for (auto v : faces[i])
            {
                cout << v << " ";
            }
            cout << endl;
        }
    }

    void printPositions()
    {
        cout << "Printing positions: " << endl;
        for (int i = 0; i < positions.size(); i++)
        {
            cout << "Face: " << i << " : ";
            for (auto v : positions[i])
            {
                cout << v << " ";
            }
            cout << endl;
        }
    }

    void printChords()
    {
        cout << "Printing all chords: " << endl;
        for (int i = 0; i < faces.size(); i++)
        {
            cout << "Face: " << i << " : ";
            auto tempChord = headChord[i];
            while (tempChord != nullptr)
            {
                cout << "(" << tempChord->first << "," << tempChord->second << ") ";
                tempChord = tempChord->nextChord;
            }
            cout << endl;
        }
    }

    void printGS()
    {
        cout << "Printing all GS: " << endl;
        for (int i = 0; i < faces.size(); i++)
        {
            cout << "Face: " << i << " : ";
            auto tempGS = headGS[i];
            while (tempGS != nullptr)
            {
                cout << "(" << tempGS->first << "," << tempGS->second << ") ";
                tempGS = tempGS->nextGS;
            }
            cout << endl;
        }
    }

    void printVGS()
    {
        cout << "Printing all VGS: " << endl;
        for (int i = 0; i < faces.size(); i++)
        {
            cout << "Face: " << i << " : ";
            auto tempVGS = headVGS[i];
            while (tempVGS != nullptr)
            {
                cout << "(" << tempVGS->first << "," << tempVGS->second << ") ";
                tempVGS = tempVGS->nextVGS;
            }
            cout << endl;
        }
    }

    void printPresent()
    {
        cout << "Printing all present: " << endl;
        for (auto p : present.present)
        {
            cout << "(" << p.first << "," << p.second << ") ";
        }
        cout << endl;
    }

    void printVariables()
    {
        cout << "-=================== Printing variables =================-" << endl;
        printFaces();
        printPositions();
        printChords();
        printGS();
        printVGS();
        printPresent();
        cout << "-=================== End of variables ==================-" << endl;
    }
};