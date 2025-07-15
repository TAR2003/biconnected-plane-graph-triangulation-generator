import sys
from typing import List, Set, Tuple, Dict
from copy import deepcopy

# Global counters
geenralTriangulationsCall = 0
findLeftMostBlockingChordCall = 0
flipChordCall = 0

def makepair(a: int, b: int) -> Tuple[int, int]:
    return (a, b) if a < b else (b, a)

def printVector(v: List, st: int = -1, fin: int = -1) -> None:
    if st == -1:
        st = 0
    if fin == -1:
        fin = len(v)
    print(' '.join(map(str, v[st:fin])))
    
def printVectorPair(v: List[Tuple], st: int = -1, fin: int = -1) -> None:
    if st == -1:
        st = 0
    if fin == -1:
        fin = len(v)
    print(' '.join([f"({x[0]},{x[1]})" for x in v[st:fin]]))
    
def printMatrix(v: List[List], st: int = -1, fin: int = -1) -> None:
    if st == -1:
        st = 0
    if fin == -1:
        fin = len(v)
    for row in v[st:fin]:
        printVector(row)
        
def printStructure(t) -> None:
    print(' '.join(map(str, t)))
    
def printAllPairs(s: Set[Tuple[int, int]]) -> None:
    print(' '.join([f"({x[0]},{x[1]})" for x in s]]))
    
def copySetOfPairs(src: Set[Tuple[int, int]]) -> Set[Tuple[int, int]]:
    return {makepair(x[0], x[1]) for x in src}

def checkLeftmostBlockingChord(root: int, n: int, chords: Set[Tuple[int, int]], chord: Tuple[int, int]]) -> bool:
    global findLeftMostBlockingChordCall
    findLeftMostBlockingChordCall += 1
    
    def normalizeVertex(vertex: int) -> int:
        return (vertex - root) % n
    
    normalizedChords = set()
    for c in chords:
        first_norm = normalizeVertex(c[0])
        second_norm = normalizeVertex(c[1])
        normalizedChords.add(makepair(first_norm, second_norm))
    
    chord_first_norm = normalizeVertex(chord[0])
    chord_second_norm = normalizeVertex(chord[1])
    normalizedChord = makepair(chord_first_norm, chord_second_norm)
    
    currentBlockedChord = n - 1
    for i in range(n - 2, -1, -1):
        if makepair(0, i) not in normalizedChords:
            break
        else:
            currentBlockedChord = i
    
    leftBlockingChords = set()
    for a in normalizedChords:
        if a[1] == currentBlockedChord and a[0] > 0:
            leftBlockingChords.add(a)
    
    if not leftBlockingChords:
        return False
    
    leftmostChord = min(leftBlockingChords)
    return leftmostChord[0] == normalizedChord[0] and leftmostChord[1] == normalizedChord[1]

def flipChord(root: int, n: int, chords: Set[Tuple[int, int]], allPairs: Set[Tuple[int, int]], chord: Tuple[int, int]]) -> Tuple[int, int]:
    global flipChordCall
    flipChordCall += 1
    
    assocOfFirst = set()
    assocOfSecond = set()
    
    for a in allPairs:
        if a[0] == chord[0]:
            assocOfFirst.add(a[1])
        if a[1] == chord[0]:
            assocOfFirst.add(a[0])
    
    for a in allPairs:
        if a[0] == chord[1]:
            assocOfSecond.add(a[1])
        if a[1] == chord[1]:
            assocOfSecond.add(a[0])
    
    commons = []
    for a in assocOfFirst:
        if a in assocOfSecond:
            commons.append(a)
    
    chords.discard(chord)
    allPairs.discard(chord)
    new_chord = makepair(commons[0], commons[1])
    chords.add(new_chord)
    allPairs.add(new_chord)
    return new_chord

def generateAllTriangulations(root: int, n: int, allPairs: Set[Tuple[int, int]], chords: Set[Tuple[int, int]], answers: Set[Tuple[Tuple[int, int], ...]]) -> None:
    global geenralTriangulationsCall
    geenralTriangulationsCall += 1
    
    if n < 3:
        print("Invalid input for triangulation")
        return
    
    if n == 3:
        print("Base case: Triangulation of a triangle")
        return
    
    answers.add(frozenset(chords))
    
    for a in list(chords):
        allPairsCopy = copySetOfPairs(allPairs)
        chordsCopy = copySetOfPairs(chords)
        pair = flipChord(root, n, chordsCopy, allPairsCopy, a)
        isLeftmostBlockingChord = checkLeftmostBlockingChord(root, n, chordsCopy, pair)
        
        if isLeftmostBlockingChord:
            generateAllTriangulations(root, n, allPairsCopy, chordsCopy, answers)

def printAllTriangulations(answers: Set[Tuple[Tuple[int, int], ...]]) -> None:
    print(f"Total triangulations found: {len(answers)}")

def findHighestSafeRoot(n: int, innerchords: Set[Tuple[int, int]]) -> int:
    s = set(range(n))
    for a in innerchords:
        s.discard(a[0])
        s.discard(a[1])
    return max(s)

def generateOuterTriangulationsFromAnInnerTriangulation(root: int, n: int, allPairs: Set[Tuple[int, int]], chords: Set[Tuple[int, int]], innerChords: Set[Tuple[int, int]], answers: Set[Tuple[Tuple[int, int], ...]]) -> None:
    if n < 3:
        print("Invalid input for triangulation")
        return
    
    if n == 3:
        print("Base case: Triangulation of a triangle")
        return
    
    minChord = min(chords) if chords else (float('inf'), float('inf'))
    minInnerChord = min(innerChords) if innerChords else (float('inf'), float('inf'))
    
    if minChord < minInnerChord:
        return
    
    allChords = chords.union(innerChords)
    rightlength = (n - 3) * 2
    if len(allChords) < rightlength:
        return
    
    merged = chords.union(innerChords)
    answers.add(frozenset(merged))
    
    for a in list(chords):
        allPairsCopy = copySetOfPairs(allPairs)
        chordsCopy = copySetOfPairs(chords)
        pair = flipChord(root, n, chordsCopy, allPairsCopy, a)
        isLeftmostBlockingChord = checkLeftmostBlockingChord(root, n, chordsCopy, pair)
        
        if isLeftmostBlockingChord:
            generateOuterTriangulationsFromAnInnerTriangulation(root, n, allPairsCopy, chordsCopy, innerChords, answers)

def generateAllTriangulationsForBiconnectedGraph(root: int, n: int, allPairs: Set[Tuple[int, int]], chords: Set[Tuple[int, int]], answers: Set[Tuple[Tuple[int, int], ...]]) -> None:
    tempAnswers = set()
    innerChords = set()
    tempAllPairs = copySetOfPairs(allPairs)
    
    for i in range(2, n - 1):
        innerChords.add(makepair(0, i))
        tempAllPairs.add(makepair(0, i))
    
    generateAllTriangulations(root, n, tempAllPairs, innerChords, tempAnswers)
    
    for a in tempAnswers:
        innerChordsCopy = copySetOfPairs(a)
        initChords = set()
        tempAllPairs = copySetOfPairs(allPairs)
        highestSafeRoot = findHighestSafeRoot(n, innerChordsCopy)
        
        for i in range(2, n - 1):
            initChords.add(makepair(highestSafeRoot, (highestSafeRoot + i) % n))
            tempAllPairs.add(makepair(highestSafeRoot, (highestSafeRoot + i) % n))
        
        temptempAnswers = set()
        generateOuterTriangulationsFromAnInnerTriangulation(highestSafeRoot, n, tempAllPairs, initChords, innerChordsCopy, temptempAnswers)
        
        for b in temptempAnswers:
            answers.add(b)

def generateViaMergingAlgorithm(n: int, mergedAnswers: Set[Tuple[Tuple[int, int], ...]]) -> None:
    allPairs = set()
    chords = set()
    answers = set()
    
    for i in range(n):
        allPairs.add(makepair(i, (i + 1) % n))
    
    for i in range(2, n - 1):
        chords.add(makepair(0, i))
        allPairs.add(makepair(0, i))
    
    generateAllTriangulations(0, n, allPairs, chords, answers)
    
    answers_list = list(answers)
    for i in range(len(answers_list)):
        for j in range(len(answers_list)):
            if i == j:
                continue
            a = answers_list[i]
            b = answers_list[j]
            has_common = False
            for c in a:
                if c in b:
                    has_common = True
                    break
            if not has_common:
                merged = set(a).union(set(b))
                mergedAnswers.add(frozenset(merged))

def compareSet(a: Set[Tuple[int, int]], b: Set[Tuple[int, int]]) -> bool:
    if len(a) != len(b):
        return False
    return a == b

def compareTriangulations(a: Set[Tuple[Tuple[int, int], ...]], b: Set[Tuple[Tuple[int, int], ...]]) -> bool:
    if len(a) != len(b):
        return False
    a_sorted = sorted([sorted(list(x)) for x in a])
    b_sorted = sorted([sorted(list(x)) for x in b])
    return a_sorted == b_sorted

def calculate(n: int) -> None:
    global geenralTriangulationsCall, findLeftMostBlockingChordCall, flipChordCall
    
    allPairs = set()
    chords = set()
    answers = set()
    
    for i in range(n):
        allPairs.add(makepair(i, (i + 1) % n))
    
    mergedAnswers = set()
    generateViaMergingAlgorithm(n, mergedAnswers)
    
    geenralTriangulationsCall = 0
    findLeftMostBlockingChordCall = 0
    flipChordCall = 0
    
    generateAllTriangulationsForBiconnectedGraph(0, n, allPairs, chords, answers)
    
    bl = compareTriangulations(answers, mergedAnswers)
    if bl:
        print(f"For n={n}, sizes ={len(answers)}=={len(mergedAnswers)}")
    else:
        print(f"For n={n}, comparison failed")

def main():
    for i in range(4, 11):
        calculate(i)
        print(f"geenralTriangulationsCall: {geenralTriangulationsCall}")
        print(f"findLeftMostBlockingChordCall: {findLeftMostBlockingChordCall}")
        print(f"flipChordCall: {flipChordCall}")
        print("----------------------------------------")

if __name__ == "__main__":
    main()