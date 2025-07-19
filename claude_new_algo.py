from collections import defaultdict, deque
from typing import List, Set, Tuple, Dict, Optional
import copy

class BiconnectedTriangulation:
    def __init__(self, faces: List[List[int]]):
        """
        Initialize with list of faces, where each face is a list of vertices in order.
        Example: faces = [[1,2,3,4], [2,3,4,5]] represents two adjacent faces
        """
        self.faces = faces
        self.n_faces = len(faces)
        self.global_chords = set()  # All chords created so far across all faces
        self.triangulations = []    # Store all valid triangulations
        self.face_order = list(range(self.n_faces))  # Process faces in this order
        
        # Precompute risky vertices for each face pair
        self.risky_vertices = self._compute_risky_vertices()
        
    def _compute_risky_vertices(self) -> Dict[Tuple[int, int], Set[int]]:
        """Compute shared vertices between each pair of faces"""
        risky = {}
        for i in range(self.n_faces):
            for j in range(i + 1, self.n_faces):
                shared = set(self.faces[i]) & set(self.faces[j])
                if len(shared) >= 2:  # Only consider if at least 2 shared vertices
                    risky[(i, j)] = shared
                    risky[(j, i)] = shared
        return risky
    
    def _get_risky_chords_for_face(self, face_idx: int) -> Set[Tuple[int, int]]:
        """Get all existing chords that have both endpoints in risky vertices for this face"""
        risky_chords = set()
        
        for other_face_idx in range(face_idx):  # Only consider earlier faces
            if (other_face_idx, face_idx) in self.risky_vertices:
                shared_vertices = self.risky_vertices[(other_face_idx, face_idx)]
                
                # Check existing chords for ones with both endpoints in shared vertices
                for chord in self.global_chords:
                    if chord[0] in shared_vertices and chord[1] in shared_vertices:
                        risky_chords.add(chord)
        
        return risky_chords
    
    def _find_safe_root(self, face: List[int], face_idx: int) -> Optional[int]:
        """Find a root vertex that won't create initial multi-edges"""
        risky_chords = self._get_risky_chords_for_face(face_idx)
        
        # Try each vertex as potential root
        for root_candidate in face:
            safe = True
            
            # Check if using this root would immediately create conflicts
            n = len(face)
            root_pos = face.index(root_candidate)
            
            # Check all potential initial chords from this root
            for i in range(2, n - 1):  # Skip adjacent vertices
                target_idx = (root_pos + i) % n
                target = face[target_idx]
                chord = tuple(sorted([root_candidate, target]))
                
                if chord in self.global_chords:
                    safe = False
                    break
            
            if safe:
                return root_candidate
        
        return None  # No safe root found
    
    def _generate_face_triangulation(self, face: List[int], face_idx: int, 
                                   current_chords: Set[Tuple[int, int]]) -> List[List[Tuple[int, int]]]:
        """Generate all valid triangulations for a single face using the paper algorithm"""
        
        if len(face) < 4:  # Triangle, no chords needed
            return [[]]
        
        root = self._find_safe_root(face, face_idx)
        if root is None:
            return []  # No valid triangulations possible
        
        # Create triangulation tree starting from root
        triangulations = []
        
        class TriangulationNode:
            def __init__(self, chords: List[Tuple[int, int]], generating_set: Set[Tuple[int, int]]):
                self.chords = chords
                self.generating_set = generating_set
        
        # Root triangulation: all chords from root vertex
        root_chords = []
        root_pos = face.index(root)
        n = len(face)
        
        for i in range(2, n - 1):
            target_idx = (root_pos + i) % n
            target = face[target_idx]
            chord = tuple(sorted([root, target]))
            
            # Check for multi-edge conflict
            if chord in current_chords:
                return []  # Conflict found, no valid triangulations
            
            root_chords.append(chord)
        
        # Check symmetry breaking for risky chords
        if not self._check_symmetry_constraint(root_chords, face_idx):
            return []
        
        # Initialize with root triangulation
        root_gs = set(root_chords)  # All chords are initially generating
        root_node = TriangulationNode(root_chords, root_gs)
        
        # BFS to generate all triangulations
        queue = deque([root_node])
        
        while queue:
            node = queue.popleft()
            triangulations.append(node.chords[:])
            
            # Generate children by flipping generating chords
            for gen_chord in list(node.generating_set):
                child_chords = self._flip_chord(face, node.chords, gen_chord)
                if child_chords is None:
                    continue
                
                # Check for conflicts with existing chords
                conflict = False
                for chord in child_chords:
                    if chord in current_chords:
                        conflict = True
                        break
                
                if conflict:
                    continue
                
                # Check symmetry constraint
                if not self._check_symmetry_constraint(child_chords, face_idx):
                    continue
                
                # Update generating set for child
                child_gs = self._update_generating_set(face, child_chords)
                child_node = TriangulationNode(child_chords, child_gs)
                queue.append(child_node)
        
        return triangulations
    
    def _flip_chord(self, face: List[int], chords: List[Tuple[int, int]], 
                   flip_chord: Tuple[int, int]) -> Optional[List[Tuple[int, int]]]:
        """Flip a chord and return new chord list, or None if invalid"""
        
        # Find the quadrilateral containing the chord to flip
        # This is a simplified version - in practice, you'd need more sophisticated
        # geometric reasoning to find the proper quadrilateral
        
        new_chords = []
        flipped = False
        
        for chord in chords:
            if chord == flip_chord:
                # Find opposite chord in quadrilateral (simplified)
                # This would need proper implementation based on face geometry
                opposite = self._find_opposite_chord(face, chord)
                if opposite:
                    new_chords.append(opposite)
                    flipped = True
                else:
                    return None
            else:
                new_chords.append(chord)
        
        return new_chords if flipped else None
    
    def _find_opposite_chord(self, face: List[int], chord: Tuple[int, int]) -> Optional[Tuple[int, int]]:
        """Find the opposite chord in a quadrilateral (simplified implementation)"""
        # This is a placeholder - actual implementation would require
        # proper geometric analysis of the face structure
        
        v1, v2 = chord
        if v1 not in face or v2 not in face:
            return None
        
        pos1 = face.index(v1)
        pos2 = face.index(v2)
        n = len(face)
        
        # Simple heuristic for opposite chord
        # In practice, this needs proper quadrilateral identification
        if abs(pos1 - pos2) == 2 or abs(pos1 - pos2) == n - 2:
            # Adjacent positions in quadrilateral
            mid1 = face[(pos1 + 1) % n] if (pos1 + 1) % n != pos2 else face[(pos1 - 1) % n]
            mid2 = face[(pos2 + 1) % n] if (pos2 + 1) % n != pos1 else face[(pos2 - 1) % n]
            return tuple(sorted([mid1, mid2]))
        
        return None
    
    def _update_generating_set(self, face: List[int], chords: List[Tuple[int, int]]) -> Set[Tuple[int, int]]:
        """Update generating set based on leftmost blocking chord rule"""
        # Simplified implementation - actual algorithm would need
        # proper LBC (Leftmost Blocking Chord) computation
        
        generating_set = set()
        
        # Find potential generating chords
        for chord in chords:
            # Check if this chord can be flipped to produce valid children
            # This is a simplified heuristic
            if self._can_generate_child(face, chords, chord):
                generating_set.add(chord)
        
        return generating_set
    
    def _can_generate_child(self, face: List[int], chords: List[Tuple[int, int]], 
                          chord: Tuple[int, int]) -> bool:
        """Check if flipping this chord can generate a valid child"""
        # Simplified check - actual implementation needs proper geometric analysis
        return len(chords) > 1  # Can flip if not the last chord
    
    def _check_symmetry_constraint(self, chords: List[Tuple[int, int]], face_idx: int) -> bool:
        """Check symmetry breaking constraint for risky chords"""
        
        for earlier_face_idx in range(face_idx):
            if (earlier_face_idx, face_idx) in self.risky_vertices:
                shared_vertices = self.risky_vertices[(earlier_face_idx, face_idx)]
                
                # Find risky chords in current triangulation
                risky_chords = []
                for chord in chords:
                    if chord[0] in shared_vertices and chord[1] in shared_vertices:
                        risky_chords.append(chord)
                
                # Find risky chords in earlier face
                earlier_risky_chords = []
                for chord in self.global_chords:
                    if chord[0] in shared_vertices and chord[1] in shared_vertices:
                        earlier_risky_chords.append(chord)
                
                # Apply symmetry breaking: current face should have larger minimum
                if risky_chords and earlier_risky_chords:
                    min_current = min(risky_chords)
                    min_earlier = min(earlier_risky_chords)
                    
                    if min_current <= min_earlier:
                        return False
        
        return True
    
    def generate_all_triangulations(self) -> List[List[Tuple[int, int]]]:
        """Generate all valid triangulations of the biconnected graph"""
        
        self.triangulations = []
        self.global_chords = set()
        
        self._generate_recursive(0, [])
        return self.triangulations
    
    def _generate_recursive(self, face_idx: int, current_triangulation: List[Tuple[int, int]]):
        """Recursively generate triangulations face by face"""
        
        if face_idx >= self.n_faces:
            # All faces processed, add complete triangulation
            self.triangulations.append(current_triangulation[:])
            return
        
        face = self.faces[face_idx]
        current_chords = set(current_triangulation)
        
        # Generate all valid triangulations for current face
        face_triangulations = self._generate_face_triangulation(face, face_idx, current_chords)
        
        # Recursively process each triangulation
        for face_tri in face_triangulations:
            # Add face triangulation to global state
            old_global_chords = self.global_chords.copy()
            self.global_chords.update(face_tri)
            
            # Recursively process next face
            self._generate_recursive(face_idx + 1, current_triangulation + face_tri)
            
            # Backtrack
            self.global_chords = old_global_chords
    
    def print_results(self):
        """Print all generated triangulations"""
        print(f"Found {len(self.triangulations)} valid triangulations:")
        for i, triangulation in enumerate(self.triangulations):
            print(f"Triangulation {i + 1}: {triangulation}")

# Example usage
if __name__ == "__main__":
    # Example: Two adjacent quadrilateral faces
    faces = [
        [1, 2, 3, 4, 5],  # Face 1
        [1, 2, 3, 4, 5]   # Face 2 (shares vertices 2, 3, 4 with Face 1)
    ]
    
    algorithm = BiconnectedTriangulation(faces)
    triangulations = algorithm.generate_all_triangulations()
    algorithm.print_results()
    
    print(f"\nRisky vertices between faces: {algorithm.risky_vertices}")