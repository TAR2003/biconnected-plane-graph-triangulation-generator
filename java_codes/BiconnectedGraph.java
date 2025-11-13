import java.io.File;
import java.io.FileNotFoundException;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Scanner;

public class BiconnectedGraph {
    public static void solve(String filename) {
        ArrayList<ArrayList<Integer>> allarrays = new ArrayList<>();

        // Read input from file
        try {
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


        if (allarrays.size() == 2) {
            TweakedAlgoCycle tweakedAlgo = new TweakedAlgoCycle(allarrays);
            tweakedAlgo.getPermanentChords();
//        System.out.println(tweakedAlgo.permanentChords);
            tweakedAlgo.generateAllTriangulations(0);
//        System.out.println("total: " + tweakedAlgo.allTriangulations.size());
            // Get the final answer string
            String answer = tweakedAlgo.getResult();

            // Write answer to output.txt
            try {
                String outfilename = "output2.txt";
                PrintWriter writer = new PrintWriter(outfilename);
                writer.println(answer);
                writer.close();
                System.out.println("Final answer written to " + outfilename);
            } catch (FileNotFoundException e) {
                System.out.println("Error: could not create output file.");
            }
            return;
        }
        TweakedAlgo tweakedAlgo = new TweakedAlgo(allarrays);
        tweakedAlgo.getPermanentChords();
//        System.out.println(tweakedAlgo.permanentChords);
        tweakedAlgo.generateAllTriangulations(0);
//        System.out.println("total: " + tweakedAlgo.allTriangulations.size());
        // Get the final answer string
        System.out.println("Total Triangulation Number: " + tweakedAlgo.allTriangulations.size());
        String answer = tweakedAlgo.getResult();

        // Write answer to output.txt
        try {
            String outfilename = "output2.txt";
            PrintWriter writer = new PrintWriter(outfilename);
            writer.println(answer);
            writer.close();
            System.out.println("Final answer written to " + outfilename);
        } catch (FileNotFoundException e) {
            System.out.println("Error: could not create output file.");
        }
    }

    public static void main(String[] args) {
    //    solve("./input/inputbiconnected.txt");
        // solve("inputcycle.txt");
        // solve("./input/share_face_vertices.txt");
        // solve("./input/hexagon.txt");
        // solve("./input/9_gon.txt");
        // solve("./big.txt");
        // solve("./complex.txt");
        solve("./supercomplex2.txt");
    }
}
