
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Set;

// click the <icon src="AllIcons.Actions.Execute"/> icon in the gutter.
public class Main {
    public static void main(String[] args) {

        for(int num = 4; num < 14 ; num++)
        {
            ParvezRahmanNakano parvezRahmanNakano = new ParvezRahmanNakano(num);
            parvezRahmanNakano.generateAllTriangulations();
            ArrayList<ArrayList<Pair>> allTriangulations = parvezRahmanNakano.allTriangulations;
            int n = allTriangulations.size();
            // This array will store for each triangulation, the count of other
            // triangulations that share no common pair with it.
            int[] nonSharingCounts = new int[n];

            for (int i = 0; i < n; i++) {
                ArrayList<Pair> tri_i = allTriangulations.get(i);
                // Convert the current triangulation to a set of pairs for fast lookup.
                Set<Pair> set_i = new HashSet<>(tri_i);

                for (int j = 0; j < n; j++) {
                    if (i == j)
                        continue; // Skip comparing with itself.

                    ArrayList<Pair> tri_j = allTriangulations.get(j);
                    boolean sharesPair = false;
                    // Check if any pair in tri_j is present in set_i.
                    for (Pair pair : tri_j) {
                        if (set_i.contains(pair)) {
                            sharesPair = true;
                            break;
                        }
                    }
                    // If no common pair is found, increment the count for triangulation i.
                    if (!sharesPair) {
                        nonSharingCounts[i]++;
                    }
                }
            }

            // Now, nonSharingCounts[i] holds the number of other triangulations that share
            // no common pair with the i-th triangulation.
            // System.out.println(nonSharingCounts);
            int mn = nonSharingCounts[0];
            for (int i = 0; i < nonSharingCounts.length; i++) {
                // System.out.println(nonSharingCounts[i] + " ");
                mn = Math.min(mn, nonSharingCounts[i]);

            }
            System.out.println("number of vertices: " + num);
            System.out.println("Total Triangulations: " + allTriangulations.size());
            System.out.println("Minimum Triangulations for that face inside a biconnected graph without multi edge with otherr faces: " + mn);
        }

    }

}