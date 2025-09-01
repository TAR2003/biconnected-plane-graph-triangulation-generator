import java.lang.reflect.Array;
import java.util.ArrayList;
import java.util.HashSet;

public class Stats {
    ArrayList<ArrayList<Integer>> vertices;
    HashSet<Pair> permanentChords;
    ArrayList<ArrayList<Pair>> allTriangulations;
    ArrayList<HashSet<Pair>> allSetTriangulations;
    HashSet<HashSet<Pair>> finalSet;

    Stats(ArrayList<ArrayList<Integer>> vertices, ArrayList<ArrayList<Pair>> allTriangulations) {
        this.vertices = vertices;
        this.allTriangulations = allTriangulations;

        // initialize permanentChords as empty HashSet
        this.permanentChords = new HashSet<>();
        this.allSetTriangulations = new ArrayList<>();
        this.finalSet = new HashSet<>();
        // populate it
        getPermanentChords();
    }

    public void getPermanentChords() {
        for (int i = 0; i < vertices.size(); i++) {
            ArrayList<Integer> arr = vertices.get(i);
//            System.out.println("arr: " + arr);
            for (int j = 0; j < arr.size(); j++) {
                Pair p = new Pair(arr.get(j), arr.get((j + 1) % arr.size()));
//                System.out.println(p);
                permanentChords.add(p);
//                System.out.println("Successfully added chord");
            }
        }
    }

    public void refine() {
//        System.out.println("Refine method");
//        System.out.println("Permanent chords " + permanentChords);
        for (int i = 0; i < allTriangulations.size(); i++) {
            HashSet<Pair> temp = new HashSet<>();
            ArrayList<Pair> triangulation = allTriangulations.get(i);
            for (int j = 0; j < triangulation.size(); j++) {
                temp.add(triangulation.get(j));
            }
//            System.out.println(temp);
            boolean present = false;
            for (Pair chord : permanentChords) {
                if (temp.contains(chord)) {
                    present = true;
                    break; // stop at first match
                }
            }
            if (temp.size() < triangulation.size()) {
                present = true;
            }
            if (!present) {
//                System.out.println("Success");
                allSetTriangulations.add(temp);
            }

        }
//        System.out.println("All refined ones : ");
//        System.out.println(allSetTriangulations);
//        System.out.println(allSetTriangulations.size());

    }

    public void symmetry() {
        for (int i = 0; i < allSetTriangulations.size(); i++) {
            finalSet.add(allSetTriangulations.get(i));
        }
//        System.out.println("Final Set: ");

//        System.out.println(finalSet.size());


    }

    String getResults() {
        StringBuilder stringBuilder = new StringBuilder();
        stringBuilder.append(finalSet.size());
        stringBuilder.append('\n');
//        for (var v : finalSet) {
//            stringBuilder.append(v);
//            stringBuilder.append('\n');
//        }
        ArrayList<ArrayList<Pair>> finalResult = new ArrayList<>();
        for (var v : finalSet) {
            ArrayList<Pair> temparr = new ArrayList<>();
            for (var a : v) {
                temparr.add(a);
            }
            temparr.sort(Pair::compareTo);
            finalResult.add(temparr);
        }
        finalResult.sort((list1, list2) -> {
            int n = Math.min(list1.size(), list2.size());
            for (int i = 0; i < n; i++) {
                int cmp = list1.get(i).compareTo(list2.get(i));
                if (cmp != 0) return cmp; // first differing element decides order
            }
            return Integer.compare(list1.size(), list2.size()); // shorter list first if all equal
        });
        for (var v : finalResult) {
            stringBuilder.append(v);
            stringBuilder.append('\n');
        }

        return String.valueOf(stringBuilder);
    }
}
