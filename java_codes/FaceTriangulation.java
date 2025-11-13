import java.util.ArrayList;

public class FaceTriangulation extends ParvezRahmanNakano {

    private ArrayList<Integer> vertices;

    FaceTriangulation(ArrayList<Integer> vertices) {
        super(vertices.size());
        this.vertices = vertices; // store vertices for later use
    }

    void getAllTriangulations() {
        generateAllTriangulations();
//        printAllTriangulations(); // here we are printing all triangulations

        for (int i = 0; i < allTriangulations.size(); i++) {
            ArrayList<Pair> triangulation = allTriangulations.get(i);
            ArrayList<Pair> remapped = new ArrayList<>();

            for (int j = 0; j < triangulation.size(); j++) {
                Pair p = triangulation.get(j);

                int mappedFirst = vertices.get(p.first);
                int mappedSecond = vertices.get(p.second);

                remapped.add(new Pair(mappedFirst, mappedSecond));
            }

            // replace original triangulation with remapped one
            allTriangulations.set(i, remapped);
        }
    }

}
