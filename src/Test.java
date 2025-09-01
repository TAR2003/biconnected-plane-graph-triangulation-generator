import java.io.File;

public class Test {
    public static void main(String[] args) {
        String[] filenames = {"inputbiconnected.txt", "input.txt", "k23.txt" , "inputcycle.txt"};
        for (String filename : filenames) {
            BiconnectedGraph.solve(filename);
            CrossResult.solve(filename);
            FileCompare.solve();

        }
    }
}
