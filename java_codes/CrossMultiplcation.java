import java.util.ArrayList;

public class CrossMultiplcation {
    ArrayList<ArrayList<ArrayList<Pair>>> allTriangulations;
    ArrayList<ArrayList<Pair>> result;

    CrossMultiplcation() {
        result = new ArrayList<>(2);
        allTriangulations = new ArrayList<>(2);
    }

    void add(ArrayList<ArrayList<Pair>> triangulationset) {
        if (!triangulationset.isEmpty())
            allTriangulations.add(triangulationset);
    }

    void printAllTriangulations() {
        for (int i = 0; i < allTriangulations.size(); i++) {
            for (int j = 0; j < allTriangulations.get(i).size(); j++) {
                System.out.println(allTriangulations.get(i).get(j));
            }
            System.out.println("-------------------");
        }
    }

    void printResult() {
        for (int i = 0; i < result.size(); i++) {
            System.out.println(result.get(i));
        }
    }

    void performCrossMultiplication() {
        result.clear(); // reset

        if (allTriangulations.isEmpty()) return;

        // start with first face's triangulations
        result = new ArrayList<>(allTriangulations.get(0));

        // combine step by step with the next faces
        for (int face = 1; face < allTriangulations.size(); face++) {
            ArrayList<ArrayList<Pair>> newResult = new ArrayList<>();

            for (ArrayList<Pair> existing : result) {
                for (ArrayList<Pair> newTriangulation : allTriangulations.get(face)) {
                    ArrayList<Pair> combined = new ArrayList<>();

                    // copy existing triangulation
                    combined.addAll(existing);
                    // copy new triangulation
                    combined.addAll(newTriangulation);

                    newResult.add(combined);
                }
            }

            result = newResult; // update result
        }
    }


}
