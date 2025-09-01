public class Pair implements Comparable<Pair> {
    int first;
    int second;

    Pair(int first, int second) {
        this.first = Math.min(first, second);
        this.second = Math.max(first, second);
    }

    @Override
    public String toString() {
        return "( " + first + " , " + second + " )";
    }

    @Override
    public int compareTo(Pair other) {
        if (this.first != other.first) {
            return Integer.compare(this.first, other.first);
        }
        return Integer.compare(this.second, other.second);
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj) return true;
        if (!(obj instanceof Pair)) return false;
        Pair other = (Pair) obj;
        return this.first == other.first && this.second == other.second;
    }

    @Override
    public int hashCode() {
        return 31 * first + second;
    }
}
