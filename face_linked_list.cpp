#include <iostream>
#include <vector>
#include <algorithm>
#include <memory>

using namespace std;

struct Pair {
    int first;
    int second;
    Pair() : first(0), second(0) {}
    Pair(int a, int b) {
        first = min(a, b);
        second = max(a, b);
    }
};

static ostream& operator<<(ostream& os, const Pair& p) {
    os << "( " << p.first << " , " << p.second << " )";
    return os;
}

// Node for doubly linked list
template<typename T>
struct DoublyLinkedNode {
    T data;
    DoublyLinkedNode* prev;
    DoublyLinkedNode* next;
    
    DoublyLinkedNode(const T& value) : data(value), prev(nullptr), next(nullptr) {}
};

// Doubly linked list implementation with O(1) insert/delete
template<typename T>
class DoublyLinkedList {
private:
    DoublyLinkedNode<T>* head;
    DoublyLinkedNode<T>* tail;
    int size_count;
    
public:
    DoublyLinkedList() : head(nullptr), tail(nullptr), size_count(0) {}
    
    ~DoublyLinkedList() {
        clear();
    }
    
    void clear() {
        while (head != nullptr) {
            DoublyLinkedNode<T>* temp = head;
            head = head->next;
            delete temp;
        }
        tail = nullptr;
        size_count = 0;
    }
    
    int size() const {
        return size_count;
    }
    
    bool empty() const {
        return size_count == 0;
    }
    
    DoublyLinkedNode<T>* push_back(const T& value) {
        DoublyLinkedNode<T>* newNode = new DoublyLinkedNode<T>(value);
        if (tail == nullptr) {
            head = tail = newNode;
        } else {
            tail->next = newNode;
            newNode->prev = tail;
            tail = newNode;
        }
        size_count++;
        return newNode;
    }
    
    void pop_back() {
        if (tail == nullptr) return;
        
        if (head == tail) {
            delete head;
            head = tail = nullptr;
        } else {
            DoublyLinkedNode<T>* temp = tail;
            tail = tail->prev;
            tail->next = nullptr;
            delete temp;
        }
        size_count--;
    }
    
    // O(1) erase at specific node
    void erase(DoublyLinkedNode<T>* node) {
        if (node == nullptr) return;
        
        if (node->prev != nullptr) {
            node->prev->next = node->next;
        } else {
            head = node->next;
        }
        
        if (node->next != nullptr) {
            node->next->prev = node->prev;
        } else {
            tail = node->prev;
        }
        
        delete node;
        size_count--;
    }
    
    // Convert to vector for easy iteration
    vector<T> toVector() const {
        vector<T> result;
        DoublyLinkedNode<T>* current = head;
        while (current != nullptr) {
            result.push_back(current->data);
            current = current->next;
        }
        return result;
    }
    
    // Get head for iteration
    DoublyLinkedNode<T>* getHead() const { return head; }
    DoublyLinkedNode<T>* getTail() const { return tail; }
};

class ParvezRahmanNakano {
public:
    int n;
    int total = 0;

    ParvezRahmanNakano(int n_) : n(n_) {}

    void printAllTriangulations(const vector<vector<Pair>>& allTriangulations) {
        for (size_t i = 0; i < allTriangulations.size(); ++i) {
            cout << "Triangulation " << (i + 1) << ":";
            const auto& tri = allTriangulations[i];
            for (size_t j = 0; j < tri.size(); ++j) {
                cout << (j ? " " : "") << tri[j];
            }
            cout << '\n';
        }
    }

    void flipit(DoublyLinkedNode<Pair>* gsNode, DoublyLinkedNode<Pair>* opNode, 
                const Pair& newChord, const Pair& oldChord) {
        if (!gsNode || !opNode) return;
        
        int oldPoint = oldChord.first;
        if (oldPoint == gsNode->data.first || oldPoint == gsNode->data.second) {
            oldPoint = oldChord.second;
        }
        int newPoint = newChord.first;
        if (newPoint == gsNode->data.first || newPoint == gsNode->data.second) {
            newPoint = newChord.second;
        }

        if (opNode->data.first == oldPoint) {
            opNode->data = Pair(newPoint, opNode->data.second);
        } else {
            opNode->data = Pair(opNode->data.first, newPoint);
        }
    }

    void flip(DoublyLinkedNode<Pair>* gsNode, DoublyLinkedNode<Pair>* opNode) {
        if (!gsNode || !opNode) return;
        
        Pair newChord(opNode->data.first, opNode->data.second);
        Pair oldChord(gsNode->data.first, gsNode->data.second);

        // Get adjacent nodes
        DoublyLinkedNode<Pair>* prevGsNode = gsNode->prev;
        DoublyLinkedNode<Pair>* nextGsNode = gsNode->next;
        DoublyLinkedNode<Pair>* prevOpNode = opNode->prev;
        DoublyLinkedNode<Pair>* nextOpNode = opNode->next;

        // Update adjacent nodes - O(1) operations
        if (prevGsNode && prevOpNode) {
            flipit(prevGsNode, prevOpNode, newChord, oldChord);
        }
        if (nextGsNode && nextOpNode) {
            flipit(nextGsNode, nextOpNode, newChord, oldChord);
        }

        // Swap the chords - O(1)
        gsNode->data = newChord;
        opNode->data = oldChord;
    }

    void addTriangulation(vector<vector<Pair>>& allTriangulations, 
                         const DoublyLinkedList<Pair>& GS, const vector<Pair>& T) {
        vector<Pair> gsVec = GS.toVector();
        vector<Pair> temp = gsVec;
        temp.insert(temp.end(), T.begin(), T.end());
        allTriangulations.push_back(move(temp));
    }

    void generateChildTriangulations(vector<vector<Pair>>& allTriangulations, 
                                   DoublyLinkedList<Pair>& GS, DoublyLinkedList<Pair>& OP, 
                                   vector<Pair>& T, int leftmost) {
        addTriangulation(allTriangulations, GS, T);
        
        // Iterate through GS linked list directly
        DoublyLinkedNode<Pair>* gsCurrent = GS.getHead();
        DoublyLinkedNode<Pair>* opCurrent = OP.getHead();
        
        while (gsCurrent && opCurrent) {
            if (gsCurrent->data.first == 0 && opCurrent->data.second >= leftmost) {
                int newleftmost = opCurrent->data.second;
                
                // Store current state for restoration
                Pair oldGsData = gsCurrent->data;
                Pair oldOpData = opCurrent->data;
                
                // Perform flip - O(1)
                flip(gsCurrent, opCurrent);
                
                // Store nodes for removal and restoration
                DoublyLinkedNode<Pair>* gsToRemove = gsCurrent;
                DoublyLinkedNode<Pair>* opToRemove = opCurrent;
                
                // Get next nodes before removal
                DoublyLinkedNode<Pair>* nextGs = gsCurrent->next;
                DoublyLinkedNode<Pair>* nextOp = opCurrent->next;
                
                // Remove nodes - O(1)
                GS.erase(gsCurrent);
                OP.erase(opCurrent);
                
                T.push_back(oldOpData); // The new chord after flip
                generateChildTriangulations(allTriangulations, GS, OP, T, newleftmost);
                T.pop_back();
                
                // Restore nodes at their original positions - O(1)
                DoublyLinkedNode<Pair>* restoredGs = GS.push_back(oldGsData);
                DoublyLinkedNode<Pair>* restoredOp = OP.push_back(oldOpData);
                
                // Restore flip to original state - O(1)
                flip(restoredGs, restoredOp);
                
                // Continue with next nodes
                gsCurrent = nextGs;
                opCurrent = nextOp;
            } else {
                gsCurrent = gsCurrent->next;
                opCurrent = opCurrent->next;
            }
        }
    }

    void generateAllTriangulations() {
        DoublyLinkedList<Pair> GS;
        DoublyLinkedList<Pair> OP;
        vector<Pair> T;
        
        for (int i = 2; i < n - 1; ++i) {
            GS.push_back(Pair(0, i));
            OP.push_back(Pair(i - 1, (i + 1) % n));
        }
        
        vector<vector<Pair>> allTriangulations;
        generateChildTriangulations(allTriangulations, GS, OP, T, 0);
        cout << "Total Triangulation number for n=" << n << " : " << allTriangulations.size() << '\n';
    }
};

int main() {
    for (int i = 4; i < 15; ++i) {
        ParvezRahmanNakano p(i);
        p.generateAllTriangulations();
    }
    return 0;
}