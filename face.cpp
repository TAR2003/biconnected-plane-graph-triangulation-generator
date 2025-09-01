// Translated from Main.java to C++
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

	void flipit(const vector<Pair>& GS, vector<Pair>& OP, const Pair& newChord, const Pair& oldChord, int pos) {
		if (pos < 0 || pos >= (int)GS.size()) return;
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

	void flip(vector<Pair>& GS, vector<Pair>& OP, int i) {
		Pair newChord(OP[i].first, OP[i].second);
		Pair oldChord(GS[i].first, GS[i].second);

		flipit(GS, OP, newChord, oldChord, i - 1);
		flipit(GS, OP, newChord, oldChord, i + 1);

		GS[i] = newChord;
		OP[i] = oldChord;
	}

	void addTriangulation(vector<vector<Pair>>& allTriangulations, const vector<Pair>& GS, const vector<Pair>& T) {
		vector<Pair> temp = GS;
		temp.insert(temp.end(), T.begin(), T.end());
		allTriangulations.push_back(move(temp));
	}

	void generateChildTriangulations(vector<vector<Pair>>& allTriangulations, vector<Pair>& GS, vector<Pair>& OP, vector<Pair>& T, int leftmost) {
		addTriangulation(allTriangulations, GS, T);
		for (int i = 0; i < (int)GS.size(); ++i) {
			if (GS[i].first != 0) continue;
			if (OP[i].second < leftmost) continue;
			int newleftmost = OP[i].second;
			Pair oldChord = Pair(GS[i].first, GS[i].second);
			Pair newChord = Pair(OP[i].first, OP[i].second);

			flip(GS, OP, i);

			// remove at i
			GS.erase(GS.begin() + i);
			OP.erase(OP.begin() + i);

			T.push_back(newChord);
			generateChildTriangulations(allTriangulations, GS, OP, T, newleftmost);
			T.pop_back();

			// restore
			GS.insert(GS.begin() + i, newChord);
			OP.insert(OP.begin() + i, oldChord);

			flip(GS, OP, i);
		}
	}

	void generateAllTriangulations() {
		vector<Pair> GS;
		vector<Pair> OP;
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

