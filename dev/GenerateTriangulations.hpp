#include <bits/stdc++.h>
using namespace std;
#include "Common.hpp"


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
    FlatChordMap present;
    string result;
    GenerateTriangulations(long long totalVertices, const vector<vector<long long>> &adjacentSet, long long triangulationLimit = LONG_LONG_MAX) : present((int)totalVertices)
    {
        cout << "In parent class triangulations" << endl;
        this->totalVertices = totalVertices;
        this->triangulationLimit = triangulationLimit;
        this->adjacentSet = adjacentSet;
        this->totalTriangulations = 0;
        // this->present = FlatChordMap((int) totalVertices);
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

        for (int i = 0; i < adjacentSet.size(); i++)
        {
            auto adjList = adjacentSet[i];
            for (auto adj : adjList)
            {
                int u = adj;
                int v = i;
                if (!present.find({u, v}))
                {
                    present.insert({u, v});
                }
            }
        }
    }

    void generateAllTriangulations()
    {
        if (faces.size() == 0)
        {
            totalTriangulations = 1;
            return;
        }
        generateFaceTriangulations(0);
    }

    void findSafeRoot(long long faceIndex)
    {
        long long startIndex = 0;
        long long endIndex = faces[faceIndex].size() - 2;
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

    virtual void setupRootTriangulation(long long faceIndex)
    {
        setupChords(faceIndex);
        setupGS(faceIndex);
    }

    void cleanUpChords(long long faceIndex)
    {
        auto temp = headChord[faceIndex];
        while (temp != nullptr)
        {
            temp->clearValues();
            temp = temp->nextChord;
        }
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

    virtual void visitAllBranches(long long faceIndex, Chord *itr)
    {
        while (itr != nullptr)
        {
            if (limitExceeded())
            {
                return;
            }
            generateChildTriangulations(faceIndex, itr);
            itr = itr->nextGS;
        }
    }

    virtual void updateAssociatedChord(long long faceIndex, Chord *c, Chord *itr)
    {
        updateOppositePair(faceIndex, c, itr);
    }

    void updateOppositePair(long long faceIndex, Chord *c, Chord *itr)
    {
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
    }



    void updateAssociatedChords(long long faceIndex, Chord *c)
    {
        updateAssociatedChord(faceIndex, c, c->nextGS);
        updateAssociatedChord(faceIndex, c, c->prevGS);
    }

    void flip(long long faceIndex, Chord *c)
    {

        present.erase({positions[faceIndex][c->first], positions[faceIndex][c->second]});
        c->flip();
        present.insert({positions[faceIndex][c->first], positions[faceIndex][c->second]});
    }

    void removeChordFromGS(long long faceIndex, Chord *c)
    {
        if (c->prevGS != nullptr)
        {
            c->prevGS->nextGS = c->nextGS;
        }
        else
        {
            headGS[faceIndex] = c->nextGS;
        }
        if (c->nextGS != nullptr)
        {
            c->nextGS->prevGS = c->prevGS;
        }
    }

    virtual void removeChord(long long faceIndex, Chord *c)
    {
        removeChordFromGS(faceIndex, c);
    }

    void addChordToGS(long long faceIndex, Chord *c)
    {
        if (c->prevGS != nullptr)
        {
            c->prevGS->nextGS = c;
        }
        else
        {
            headGS[faceIndex] = c;
        }
        if (c->nextGS != nullptr)
        {
            c->nextGS->prevGS = c;
        }
    }

    virtual void addChord(long long faceIndex, Chord *c)
    {
        addChordToGS(faceIndex, c);
    }

    virtual bool isValidChildTriangulation(long long faceIndex, Chord *c)
    {
        return true;
    }

    virtual Chord *getChordToFlip(long long faceIndex, Chord *c)
    {
        if(c == nullptr) return headGS[faceIndex];
        if(c->prevGS != nullptr) return c->prevGS;
        return c->nextGS;
    }
    void generateChildTriangulations(long long faceIndex, Chord *itr)
    {
        if (!isValidChildTriangulation(faceIndex, itr))
        {
            return;
        }
        flip(faceIndex, itr);
        updateAssociatedChords(faceIndex, itr);
        removeChord(faceIndex, itr);
        output(faceIndex);
        visitAllBranches(faceIndex, getChordToFlip(faceIndex, itr));
        flip(faceIndex, itr);
        addChord(faceIndex, itr);
        updateAssociatedChords(faceIndex, itr);
    }

    void generateFaceTriangulations(long long faceIndex)
    {
        findSafeRoot(faceIndex);
        setupRootTriangulation(faceIndex);
        output(faceIndex);
        visitAllBranches(faceIndex, getChordToFlip(faceIndex, nullptr));
        removeCurrentChordsFromPresent(faceIndex);
        cleanUpChords(faceIndex);
    }

    void output(long long faceIndex)
    {
        if (faceIndex == faces.size() - 1)
        {
            totalTriangulations++;
            return;
        }
        else
        {
            generateFaceTriangulations(faceIndex + 1);
        }
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
        cout << "Printing their opposite pairs: " << endl;
        for (int i = 0; i < faces.size(); i++)
        {
            cout << "Face: " << i << " : ";
            auto tempChord = headChord[i];
            while (tempChord != nullptr)
            {
                cout << "(" << tempChord->oppositeFirst << "," << tempChord->oppositeSecond << ") ";
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
        present.print();
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