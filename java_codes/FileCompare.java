import java.io.File;
import java.io.FileNotFoundException;
import java.util.Scanner;

public class FileCompare {
    public static boolean solve() {
        File file1 = new File("output.txt");
        File file2 = new File("output2.txt");
        boolean ret = false;

        try {
            Scanner sc1 = new Scanner(file1);
            Scanner sc2 = new Scanner(file2);

            boolean match = true;
            int lineNumber = 1;

            while (sc1.hasNextLine() || sc2.hasNextLine()) {
                String line1 = sc1.hasNextLine() ? sc1.nextLine() : null;
                String line2 = sc2.hasNextLine() ? sc2.nextLine() : null;

                if (line1 == null || line2 == null || !line1.equals(line2)) {
                    System.out.println("❌ Files differ at line " + lineNumber);
                    System.out.println("File1: " + line1);
                    System.out.println("File2: " + line2);
                    match = false;
                    break;
                }
                lineNumber++;
            }

            if (match) {
                System.out.println("✅ Files match exactly!");
                ret = true;
            }

            sc1.close();
            sc2.close();
        } catch (FileNotFoundException e) {
            System.out.println("❌ One of the files was not found.");
        }
        return ret;
    }

    public static void main(String[] args) {
        File file1 = new File("output.txt");
        File file2 = new File("output2.txt");

        try {
            Scanner sc1 = new Scanner(file1);
            Scanner sc2 = new Scanner(file2);

            boolean match = true;
            int lineNumber = 1;

            while (sc1.hasNextLine() || sc2.hasNextLine()) {
                String line1 = sc1.hasNextLine() ? sc1.nextLine() : null;
                String line2 = sc2.hasNextLine() ? sc2.nextLine() : null;

                if (line1 == null || line2 == null || !line1.equals(line2)) {
                    System.out.println("Files differ at line " + lineNumber);
                    System.out.println("File1: " + line1);
                    System.out.println("File2: " + line2);
                    match = false;
                    break;
                }
                lineNumber++;
            }

            if (match) {
                System.out.println("Files match exactly!");
            }

            sc1.close();
            sc2.close();
        } catch (FileNotFoundException e) {
            System.out.println("One of the files was not found.");
        }
    }
}
