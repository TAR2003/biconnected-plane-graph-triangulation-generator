// Optimized version using doubly linked lists for GS and OP
// to achieve linear time complexity for erase and add operations
#include <iostream>
#include <vector>
#include <algorithm>

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
    int index; // Keep track of logical index for easy access
    
    DoublyLinkedNode(const T& value) : data(value), prev(nullptr), next(nullptr), index(-1) {}
};

// Doubly linked list implementation with O(1) insert/delete and O(n) access by index
template<typename T>
class DoublyLinkedList {
private:
    DoublyLinkedNode<T>* head;
    DoublyLinkedNode<T>* tail;
    int size_count;
    
    void updateIndices() {
        int idx = 0;
        DoublyLinkedNode<T>* current = head;
        while (current != nullptr) {
            current->index = idx++;
            current = current->next;
        }
    }
    
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
    
    void push_back(const T& value) {
        DoublyLinkedNode<T>* newNode = new DoublyLinkedNode<T>(value);
        if (tail == nullptr) {
            head = tail = newNode;
        } else {
            tail->next = newNode;
            newNode->prev = tail;
            tail = newNode;
        }
        newNode->index = size_count;
        size_count++;
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
        if (size_count > 0) updateIndices();
    }
    
    // O(n) access by index - acceptable since we need to traverse anyway
    T& operator[](int index) {
        if (index < 0 || index >= size_count) {
            throw out_of_range("Index out of bounds");
        }
        
        DoublyLinkedNode<T>* current = head;
        for (int i = 0; i < index; i++) {
            current = current->next;
        }
        return current->data;
    }
    
    const T& operator[](int index) const {
        if (index < 0 || index >= size_count) {
            throw out_of_range("Index out of bounds");
        }
        
        DoublyLinkedNode<T>* current = head;
        for (int i = 0; i < index; i++) {
            current = current->next;
        }
        return current->data;
    }
    
    // O(1) erase at specific node - this is the key optimization
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
        updateIndices(); // Update indices after removal
    }
    
    // O(n) erase by index - finds node first, then O(1) delete
    void erase(int index) {
        if (index < 0 || index >= size_count) return;
        
        DoublyLinkedNode<T>* current = head;
        for (int i = 0; i < index; i++) {
            current = current->next;
        }
        erase(current);
    }
    
    // O(n) insert at index - finds position, then O(1) insert
    void insert(int index, const T& value) {
        if (index < 0 || index > size_count) return;
        
        if (index == size_count) {
            push_back(value);
            return;
        }
        
        if (index == 0) {
            DoublyLinkedNode<T>* newNode = new DoublyLinkedNode<T>(value);
            newNode->next = head;
            if (head != nullptr) {
                head->prev = newNode;
            }
            head = newNode;
            if (tail == nullptr) {
                tail = newNode;
            }
            size_count++;
            updateIndices();
            return;
        }
        
        DoublyLinkedNode<T>* current = head;
        for (int i = 0; i < index; i++) {
            current = current->next;
        }
        
        DoublyLinkedNode<T>* newNode = new DoublyLinkedNode<T>(value);
        newNode->next = current;
        newNode->prev = current->prev;
        current->prev->next = newNode;
        current->prev = newNode;
        
        size_count++;
        updateIndices();
    }
    
    // Get node at index for O(1) operations later
    DoublyLinkedNode<T>* getNode(int index) {
        if (index < 0 || index >= size_count) return nullptr;
        
        DoublyLinkedNode<T>* current = head;
        for (int i = 0; i < index; i++) {
            current = current->next;
        }
        return current;
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

    void flipit(const DoublyLinkedList<Pair>& GS, DoublyLinkedList<Pair>& OP, const Pair& newChord, const Pair& oldChord, int pos) {
        if (pos < 0 || pos >= GS.size()) return;
        
        int oldPoint = oldChord.first;
        if (oldPoint == GS[pos].first || oldPoint == GS[pos].second) {
            oldPoint = oldChord.second;
        }
        int newPoint = newChord.first;
        if (newPoint == GS[pos].first || newPoint == GS[pos].second) {
            newPoint = newChord.second;
        }

        if (OP[pos].first == oldPoint) {
            OP[pos] = Pair(newPoint, OP[pos].second);
        } else {
            OP[pos] = Pair(OP[pos].first, newPoint);
        }
    }

    void flip(DoublyLinkedList<Pair>& GS, DoublyLinkedList<Pair>& OP, int i) {
        Pair newChord(OP[i].first, OP[i].second);
        Pair oldChord(GS[i].first, GS[i].second);

        flipit(GS, OP, newChord, oldChord, i - 1);
        flipit(GS, OP, newChord, oldChord, i + 1);

        GS[i] = newChord;
        OP[i] = oldChord;
    }

    void addTriangulation(vector<vector<Pair>>& allTriangulations, const DoublyLinkedList<Pair>& GS, const vector<Pair>& T) {
        vector<Pair> gsVec = GS.toVector();
        vector<Pair> temp = gsVec;
        temp.insert(temp.end(), T.begin(), T.end());
        allTriangulations.push_back(move(temp));
    }

    void generateChildTriangulations(vector<vector<Pair>>& allTriangulations, DoublyLinkedList<Pair>& GS, DoublyLinkedList<Pair>& OP, vector<Pair>& T, int leftmost) {
        addTriangulation(allTriangulations, GS, T);
        
        for (int i = 0; i < GS.size(); ++i) {
            if (GS[i].first != 0) continue;
            if (OP[i].second < leftmost) continue;
            
            int newleftmost = OP[i].second;
            Pair oldChord = Pair(GS[i].first, GS[i].second);
            Pair newChord = Pair(OP[i].first, OP[i].second);

            flip(GS, OP, i);

            // Store the values before removing
            Pair gsValue = GS[i];
            Pair opValue = OP[i];
            
            // Remove at i - this is now more efficient with our linked list
            GS.erase(i);
            OP.erase(i);

            T.push_back(newChord);
            generateChildTriangulations(allTriangulations, GS, OP, T, newleftmost);
            T.pop_back();

            // Restore - insert back at position i
            GS.insert(i, gsValue);
            OP.insert(i, opValue);

            flip(GS, OP, i);
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
