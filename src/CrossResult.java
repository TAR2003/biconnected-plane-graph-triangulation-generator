import java.io.File;
import java.io.FileNotFoundException;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Scanner;

public class CrossResult {
    public static void solve(String filename)
    {
        ArrayList<ArrayList<Integer>> allarrays = new ArrayList<>();

        // Read input from file
        try {
//            File file = new File("k23.txt");
            File file = new File(filename);
            Scanner sc = new Scanner(file);

            int totalFaces = sc.nextInt();
            for (int i = 0; i < totalFaces; i++) {
                int vertexCount = sc.nextInt();
                ArrayList<Integer> arr = new ArrayList<>();
                for (int j = 0; j < vertexCount; j++) {
                    arr.add(sc.nextInt());
                }
                allarrays.add(arr);
            }

            sc.close();
        } catch (FileNotFoundException e) {
            System.out.println("Error: input file not found.");
            return;
        }

        // Processing
        CrossMultiplcation crossMultiplcation = new CrossMultiplcation();

        for (ArrayList<Integer> arr : allarrays) {
            FaceTriangulation faceTriangulation = new FaceTriangulation(arr);
            faceTriangulation.getAllTriangulations();
            crossMultiplcation.add(faceTriangulation.allTriangulations);
        }

        crossMultiplcation.performCrossMultiplication();

        // Stats computation
        Stats stats = new Stats(allarrays, crossMultiplcation.result);
        stats.refine();
        stats.symmetry();

        // Get the final answer string
        String answer = stats.getResults();

        // Write answer to output.txt
        try {
            PrintWriter writer = new PrintWriter("output.txt");
            writer.println(answer);
            writer.close();
            System.out.println("Final answer written to output.txt");
        } catch (FileNotFoundException e) {
            System.out.println("Error: could not create output file.");
        }
    }

    public static void main(String[] args) {
        solve("./input/quadrilateral_with_diagonal.txt");
    }
}
