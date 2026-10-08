#include <bits/stdc++.h>
using namespace std;
#include "PairHash.hpp"
#include "RotationSystem.hpp"

class Chord
{
public:
    long long first = -1, second = -1, oppositeFirst = -1, oppositeSecond = -1;
    Chord *nextChord = nullptr, *prevChord = nullptr;
    Chord *nextGS = nullptr, *prevGS = nullptr;
    Chord *nextVGS = nullptr, *prevVGS = nullptr;
    long long faceIndex = -1;
    bool isValid = false;
    Chord(long long a = 0, long long b = 0, long long c = 0, long long d = 0) : first(a), second(b), oppositeFirst(c), oppositeSecond(d) {}
    // ~Chord() {

    // }
    void flip()
    {
        swap(first, oppositeFirst);
        swap(second, oppositeSecond);
    }
    void clearValues()
    {
        first = second = oppositeFirst = oppositeSecond = -1;
        nextGS = prevGS = nextVGS = prevVGS = nullptr;
        isValid = false;
    }
};


struct ChordEntry
{
    static constexpr uint32_t EMPTY = UINT32_MAX;
    uint32_t u = EMPTY, v = 0;
    int32_t face[2] = {-1, -1};
    uint8_t cnt = 0; // 0, 1 or 2 occurrences

    void addFace(int f)
    {
        if (cnt == 0)
            face[0] = f;
        else
            face[1] = f;
        ++cnt;
    }
    void removeFace(int f)
    { // returns with cnt decremented
        if (face[1] == f)
            face[1] = -1;
        else
        {
            face[0] = face[1];
            face[1] = -1;
        }
        if (--cnt == 0)
            face[0] = -1;
    }
};

inline void normPair(long long a, long long b, uint32_t &u, uint32_t &v)
{
    if (a > b)
        swap(a, b);
    u = (uint32_t)a;
    v = (uint32_t)b;
}
inline uint64_t packKey(uint32_t u, uint32_t v) { return ((uint64_t)u << 32) | v; }

class FlatChordMap
{
    vector<ChordEntry> t;
    size_t mask;
    int shift;

    size_t home(uint32_t u, uint32_t v) const
    {
        return (packKey(u, v) * 0x9E3779B97F4A7C15ULL) >> shift;
    }
    ChordEntry *locate(uint32_t u, uint32_t v)
    {
        size_t i = home(u, v);
        while (t[i].u != ChordEntry::EMPTY)
        {
            if (t[i].u == u && t[i].v == v)
                return &t[i];
            i = (i + 1) & mask;
        }
        return nullptr;
    }
    void removeAt(size_t i)
    { // backward-shift deletion
        size_t j = i;
        for (;;)
        {
            j = (j + 1) & mask;
            if (t[j].u == ChordEntry::EMPTY)
                break;
            size_t h = home(t[j].u, t[j].v);
            bool stays = (i <= j) ? (i < h && h <= j) : (i < h || h <= j);
            if (stays)
                continue;
            t[i] = t[j];
            i = j;
        }
        t[i] = ChordEntry();
    }

public:
    explicit FlatChordMap(int n)
    {
        size_t maxChords = max(3 * (size_t)n, (size_t)8);
        int bits = 1;
        while ((1ULL << bits) < 2 * maxChords)
            ++bits;
        t.assign(1ULL << bits, ChordEntry());
        mask = (1ULL << bits) - 1;
        shift = 64 - bits;
    }
    void insert(pair<long long, long long> p, int face = -1)
    {
        uint32_t u, v;
        normPair(p.first, p.second, u, v);
        if (ChordEntry *e = locate(u, v))
        {
            e->addFace(face);
            return;
        }
        size_t i = home(u, v);
        while (t[i].u != ChordEntry::EMPTY)
            i = (i + 1) & mask;
        t[i].u = u;
        t[i].v = v;
        t[i].addFace(face);
    }
    void erase(pair<long long, long long> p, int face = -1)
    {
        uint32_t u, v;
        normPair(p.first, p.second, u, v);
        size_t i = home(u, v);
        while (t[i].u != ChordEntry::EMPTY)
        {
            if (t[i].u == u && t[i].v == v)
            {
                t[i].removeFace(face);
                if (t[i].cnt == 0)
                    removeAt(i);
                return;
            }
            i = (i + 1) & mask;
        }
    }
    bool find(pair<long long, long long> p)
    {
        uint32_t u, v;
        normPair(p.first, p.second, u, v);
        return locate(u, v) != nullptr;
    }
    // up to two face ids; returns how many
    int faces(pair<long long, long long> p, int out[2])
    {
        uint32_t u, v;
        normPair(p.first, p.second, u, v);
        ChordEntry *e = locate(u, v);
        if (!e)
            return 0;
        out[0] = e->face[0];
        out[1] = e->face[1];
        return e->cnt;
    }
    void print()
    {
        for (auto &e : t)
            if (e.u != ChordEntry::EMPTY)
                cout << "(" << e.u << ", " << e.v << ") ";
        cout << endl;
    }
};

class CustomHashMap
{
public:
    unordered_multiset<pair<long long, long long>, PairHash> presentChords;
    CustomHashMap()
    {
        presentChords = unordered_multiset<pair<long long, long long>, PairHash>();
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
        presentChords.insert(p);
    }
    void erase(pair<long long, long long> p)
    {
        makeSortedPair(p);
        auto it = presentChords.find(p);
        if (it != presentChords.end())
        {
            presentChords.erase(it);
        }
    }
    bool find(pair<long long, long long> p)
    {
        makeSortedPair(p);
        return presentChords.find(p) != presentChords.end();
    }
    void print() {
        for (auto &p : presentChords) {
            cout << "(" << p.first << ", " << p.second << ") ";
        }
        cout << endl;
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
    FlatChordMap present;
    string result;
    GenerateTriangulations(long long totalVertices, vector<vector<long long>> &adjacentSet, long long triangulationLimit = LONG_LONG_MAX) : present((int)totalVertices)
    {
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
        if(faces.size() == 0)
        {
            totalTriangulations = 0;
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
        if (headVGS[faceIndex] != nullptr)
        {
            headVGS[faceIndex]->prevVGS = nullptr;
        }
    }

    void setupRootTriangulation(long long faceIndex)
    {

        setupChords(faceIndex);
        setupGS(faceIndex);
        setupVGS(faceIndex);
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
        if (itr->isValid && present.find({positions[faceIndex][itr->oppositeFirst], positions[faceIndex][itr->oppositeSecond]}))
        {
            itr->isValid = false;
            removeChordFromVGS(faceIndex, itr);
        }
        if (!itr->isValid && !present.find({positions[faceIndex][itr->oppositeFirst], positions[faceIndex][itr->oppositeSecond]}))
        {
            itr->isValid = true;
            if (itr->prevGS == c)
            {
                itr->prevVGS = c;
                itr->nextVGS = c->nextVGS;
            }
            if (itr->nextGS == c)
            {
                itr->nextVGS = c;
                itr->prevVGS = c->prevVGS;
            }
            addChordToVGS(faceIndex, itr);
        }
    }

    void updateAssociatedChords(long long faceIndex, Chord *c)
    {
        updateOppositePair(faceIndex, c, c->nextGS);
        updateOppositePair(faceIndex, c, c->prevGS);
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

    void removeChordFromVGS(long long faceIndex, Chord *c)
    {
        if (c->prevVGS != nullptr)
        {
            c->prevVGS->nextVGS = c->nextVGS;
        }
        else
        {
            headVGS[faceIndex] = c->nextVGS;
        }
        if (c->nextVGS != nullptr)
        {
            c->nextVGS->prevVGS = c->prevVGS;
        }
    }
    void removeChord(long long faceIndex, Chord *c)
    {
        removeChordFromGS(faceIndex, c);
        removeChordFromVGS(faceIndex, c);
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

    void addChordToVGS(long long faceIndex, Chord *c)
    {
        if (c->prevVGS != nullptr)
        {
            c->prevVGS->nextVGS = c;
        }
        else
        {
            headVGS[faceIndex] = c;
        }
        if (c->nextVGS != nullptr)
        {
            c->nextVGS->prevVGS = c;
        }
    }

    void addChord(long long faceIndex, Chord *c)
    {
        addChordToGS(faceIndex, c);
        addChordToVGS(faceIndex, c);
    }

    void generateChildTriangulations(long long faceIndex, Chord *itr)
    {
        flip(faceIndex, itr);
        updateAssociatedChords(faceIndex, itr);
        removeChord(faceIndex, itr);
        output(faceIndex);
        if (itr->prevGS != nullptr && itr->prevGS->isValid)
        {
            visitAllBranches(faceIndex, itr->prevGS);
        }
        else
        {
            visitAllBranches(faceIndex, itr->nextVGS);
        }
        flip(faceIndex, itr);
        addChord(faceIndex, itr);
        updateAssociatedChords(faceIndex, itr);
    }

    void generateFaceTriangulations(long long faceIndex)
    {
        findSafeRoot(faceIndex);
        setupRootTriangulation(faceIndex);
        output(faceIndex);
        auto tempVGS = headVGS[faceIndex];
        visitAllBranches(faceIndex, tempVGS);
        removeCurrentChordsFromPresent(faceIndex);
        cleanUpChords(faceIndex);
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