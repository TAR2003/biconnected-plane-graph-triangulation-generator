import java.lang.reflect.Array;
import java.util.ArrayList;

import static java.util.Collections.swap;



//TIP To <b>Run</b> code, press <shortcut actionId="Run"/> or
// click the <icon src="AllIcons.Actions.Execute"/> icon in the gutter.
public class Main {
    public static void main(String[] args) {

        for (int i = 4; i < 15; i++) {
            ParvezRahmanNakano parvezRahmanNakano = new ParvezRahmanNakano(i);
            parvezRahmanNakano.generateAllTriangulations();
        }
    }

}