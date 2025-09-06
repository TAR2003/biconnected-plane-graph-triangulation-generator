import java.util.ArrayList;

class Pair {
    int first;
    int second;

    Pair(int first, int second) {
        this.first = Math.min(first, second);
        this.second = Math.max(first, second);
    }

    @Override
    public String toString() {
        return "( " + first + " , " + second + " )";
    }

}

class ParvezRahmanNakano {
    int n;
    int total = 0;

    ParvezRahmanNakano(int n) {
        this.n = n;
    }

    void printAllTriangulations(ArrayList<ArrayList<Pair>> allTriangulations) {
        for (int i = 0; i < allTriangulations.size(); i++) {
            System.out.println("Triangulation " + (i + 1) + ":" + allTriangulations.get(i));
        }
    }

    void flipit(ArrayList<Pair> GS, ArrayList<Pair> OP, Pair newChord, Pair oldChord, int pos) {
        if (pos < 0 || pos >= GS.size())
            return;
        int oldPoint = oldChord.first;
        if (oldPoint == GS.get(pos).first || oldPoint == GS.get(pos).second) {
            oldPoint = oldChord.second;
        }
        int newPoint = newChord.first;
        if (newPoint == GS.get(pos).first || newPoint == GS.get(pos).second) {
            newPoint = newChord.second;
        }
        if (OP.get(pos).first == oldPoint) {
            OP.set(pos, new Pair(newPoint, OP.get(pos).second));
        } else {
            OP.set(pos, new Pair(OP.get(pos).first, newPoint));
        }
    }

    void flip(ArrayList<Pair> GS, ArrayList<Pair> OP, int i) {
        // System.out.println("Before flipping");
        // System.out.println(GS);
        // System.out.println(OP);

        // Create fresh copies
        Pair newChord = new Pair(OP.get(i).first, OP.get(i).second);
        Pair oldChord = new Pair(GS.get(i).first, GS.get(i).second);

        flipit(GS, OP, newChord, oldChord, i - 1);
        flipit(GS, OP, newChord, oldChord, i + 1);

        GS.set(i, newChord);
        OP.set(i, oldChord);

        // System.out.println("After flipping");
        // System.out.println(GS);
        // System.out.println(OP);
    }

    void addTriangulation(ArrayList<ArrayList<Pair>> allTriangulations, ArrayList<Pair> GS, ArrayList<Pair> T) {
        ArrayList<Pair> temp = new ArrayList<>(GS);
        temp.addAll(T);
        allTriangulations.add(temp);
    }

    void generateChildTriangulations(ArrayList<ArrayList<Pair>> allTriangulations, ArrayList<Pair> GS,
            ArrayList<Pair> OP, ArrayList<Pair> T, int leftmost) {
        addTriangulation(allTriangulations, GS, T);
        for (int i = 0; i < GS.size(); i++) {

            if (GS.get(i).first != 0) {
                continue;
            }
            if (OP.get(i).second < leftmost) {
                continue;
            }
            int newleftmost = OP.get(i).second;
            Pair oldChord = new Pair(GS.get(i).first, GS.get(i).second);
            Pair newChord = new Pair(OP.get(i).first, OP.get(i).second);
            // System.out.println("Front Flip------- " + i);
            flip(GS, OP, i);
            // System.out.println("Front flip ends--------- " + i);
            GS.remove(i);
            OP.remove(i);
            T.add(newChord);
            generateChildTriangulations(allTriangulations, GS, OP, T, newleftmost);
            T.remove(T.size() - 1);
            GS.add(i, newChord);
            OP.add(i, oldChord);
            // System.out.println("Back flip ------------ " + i);
            flip(GS, OP, i);
            // System.out.println("Back flip ends---------------- " + i);

        }
    }

    void generateAllTriangulations() {
        ArrayList<Pair> GS = new ArrayList<>(2);
        ArrayList<Pair> OP = new ArrayList<>(2);
        ArrayList<Pair> T = new ArrayList<>(2);
        for (int i = 2; i < n - 1; i++) {
            GS.add(new Pair(0, i));
            OP.add(new Pair(i - 1, (i + 1) % n));
        }
        ArrayList<ArrayList<Pair>> allTriangulations = new ArrayList<>(2);
        generateChildTriangulations(allTriangulations, GS, OP, T, 0);
        // printAllTriangulations(allTriangulations);
        System.out.println("Total Triangulation number for n=" + n + " : " + allTriangulations.size());

    }
}

// TIP To <b>Run</b> code, press <shortcut actionId="Run"/> or
// click the <icon src="AllIcons.Actions.Execute"/> icon in the gutter.
public class Main {
    public static void main(String[] args) {

        for (int i = 4; i < 15; i++) {
            ParvezRahmanNakano parvezRahmanNakano = new ParvezRahmanNakano(i);
            parvezRahmanNakano.generateAllTriangulations();
        }
    }

}