import java.io.File;

public class Test {
    public static void main(String[] args) {
        // Folder where input files are stored
        File folder = new File("input");

        // Get all .txt files from the folder
        File[] files = folder.listFiles((dir, name) -> name.toLowerCase().endsWith(".txt"));

        if (files == null || files.length == 0) {
            System.out.println("No input files found in 'input' folder.");
            return;
        }

        // Loop through all the files
        for (File file : files) {
            String filename = file.getPath(); // full path (or file.getName() if only name needed)
            System.out.println("Processing file: " + filename);
            BiconnectedGraph.solve(filename);
            CrossResult.solve(filename);
            FileCompare.solve();
        }
    }
}
