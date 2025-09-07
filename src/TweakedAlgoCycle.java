import java.util.ArrayList;
import java.util.HashSet;

public class TweakedAlgoCycle {
    int n;
    int total = 0;
    ArrayList<ArrayList<Pair>> allTriangulations;
    HashSet<Pair> permanentChords;
    HashSet<Pair> chords;
    ArrayList<ArrayList<Integer>> vertices;
    long totaliterations = 0;

    TweakedAlgoCycle(ArrayList<ArrayList<Integer>> vertices) {
        // this.n = n;
        this.vertices = vertices;
        allTriangulations = new ArrayList<>(2);
        permanentChords = new HashSet<>();
        chords = new HashSet<>();
        getPermanentChords();
    }

    void printAllTriangulations() {
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

    public void getPermanentChords() {
        for (int i = 0; i < vertices.size(); i++) {
            ArrayList<Integer> arr = vertices.get(i);
            // System.out.println("arr: " + arr);
            for (int j = 0; j < arr.size(); j++) {
                Pair p = new Pair(arr.get(j), arr.get((j + 1) % arr.size()));
                // System.out.println(p);
                permanentChords.add(p);
                // System.out.println("Successfully added chord");
            }
        }
    }

    int saferoot(ArrayList<Integer> face) {
        int start = face.size() - 1;
        int end = 1;
        while (start > end + 1) {
            boolean present = false;
            if (permanentChords.contains(new Pair(face.get(start), face.get(end)))) {
                present = true;
            }
            if (chords.contains(new Pair(face.get(start), face.get(end)))) {
                present = true;
            }
            if (present) {
                start--;
            } else {
                end++;
            }
        }
        return start;
    }

    void addTriangulation() {
        ArrayList<Pair> temp = new ArrayList<>();
        for (var v : chords) {
            temp.add(v);
        }
        allTriangulations.add(temp);

    }

    void generateChildTriangulations(ArrayList<ArrayList<Pair>> allTriangulations, ArrayList<Pair> GS,
            ArrayList<Pair> OP, ArrayList<Pair> T, int leftmost, ArrayList<Integer> adjustedFace, int faceno,
            Pair minChord) {
        generateAllTriangulations(faceno + 1);
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
            Pair oldGlobalChord = new Pair(adjustedFace.get(GS.get(i).first), adjustedFace.get(GS.get(i).second));
            Pair newGlobalChord = new Pair(adjustedFace.get(OP.get(i).first), adjustedFace.get(OP.get(i).second));
            if (chords.contains(newGlobalChord)) {
                continue;
            }
            if (permanentChords.contains(newGlobalChord)) {
                continue;
            }
            if (minChord.compareTo(newGlobalChord) > 0) {
                continue;
            }
            chords.remove(oldGlobalChord);
            chords.add(newGlobalChord);
            // System.out.println("Front Flip------- " + i);
            totaliterations++;
            flip(GS, OP, i);
            // System.out.println("Front flip ends--------- " + i);
            GS.remove(i);
            OP.remove(i);
            T.add(newChord);
            generateChildTriangulations(allTriangulations, GS, OP, T, newleftmost, adjustedFace, faceno, minChord);
            T.remove(T.size() - 1);
            GS.add(i, newChord);
            OP.add(i, oldChord);
            chords.add(oldGlobalChord);
            chords.remove(newGlobalChord);
            // System.out.println("Back flip ------------ " + i);
            flip(GS, OP, i);
            // System.out.println("Back flip ends---------------- " + i);

        }
    }

    void generateAllTriangulations(int faceno) {
        if (faceno >= vertices.size()) {
            addTriangulation();
            return;
        }
        ArrayList<Integer> currentface = vertices.get(faceno);
        int n = currentface.size();
        if (n < 4) {
            generateAllTriangulations(faceno + 1);
            return;
        }
        int root = 0;
        if (faceno > 0)
            root = saferoot(currentface);
        ArrayList<Integer> adjustedFace = new ArrayList<>(currentface);
        for (int i = 0; i < n; i++) {
            int pos = root + i;
            pos = pos % n;
            adjustedFace.set(i, currentface.get(pos));
        }
        ArrayList<Pair> GS = new ArrayList<>(2);
        ArrayList<Pair> OP = new ArrayList<>(2);
        ArrayList<Pair> T = new ArrayList<>(2);
        Pair minChord = new Pair(Integer.MAX_VALUE, Integer.MAX_VALUE);

        for (Pair v : chords) {
            if (v.compareTo(minChord) < 0) { // v is smaller than minChord
                minChord = v;
            }
        }
        Pair newMinChord = new Pair(Integer.MAX_VALUE, Integer.MAX_VALUE);
        for (int i = 2; i < n - 1; i++) {
            GS.add(new Pair(0, i));
            OP.add(new Pair(i - 1, (i + 1) % n));
            Pair p = new Pair(adjustedFace.get(0), adjustedFace.get(i));
            chords.add(p);
            if (p.compareTo(newMinChord) < 0) {
                newMinChord = p;
            }

        }
        totaliterations += n;
        if (minChord.compareTo(newMinChord) < 0 || faceno == 0) {
            if (faceno == 0)
                minChord = newMinChord;
            generateChildTriangulations(allTriangulations, GS, OP, T, 0, adjustedFace, faceno, minChord);
        }

        for (int i = 2; i < n - 1; i++) {
            chords.remove(new Pair(adjustedFace.get(0), adjustedFace.get(i)));
        }
        // printAllTriangulations(allTriangulations);
        // System.out.println("Total Triangulation number for n=" + n + " : " +
        // allTriangulations.size());

    }

    String getResult() {
        System.out.println("Total Iterations: " + totaliterations);
        System.out.println("Total Triangulations: " + allTriangulations.size());
        StringBuilder stringBuilder = new StringBuilder();
        stringBuilder.append(allTriangulations.size());
        stringBuilder.append('\n');
        for (int i = 0; i < allTriangulations.size(); i++) {
            allTriangulations.get(i).sort(Pair::compareTo);
        }
        allTriangulations.sort((list1, list2) -> {
            int n = Math.min(list1.size(), list2.size());
            for (int i = 0; i < n; i++) {
                int cmp = list1.get(i).compareTo(list2.get(i));
                if (cmp != 0)
                    return cmp; // first differing element decides order
            }
            return Integer.compare(list1.size(), list2.size()); // shorter list first if all equal
        });
        for (var v : allTriangulations) {
            stringBuilder.append(v);
            stringBuilder.append('\n');
        }

        return String.valueOf(stringBuilder);
    }

}
