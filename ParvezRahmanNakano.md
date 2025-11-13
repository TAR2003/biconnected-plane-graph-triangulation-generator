Journal of Graph Algorithms and Applications
http://jgaa.info/ vol. 15, no. 3, pp. 457–482 (2011)
Generating All Triangulations of Plane Graphs
Mohammad Tanvir Parvez1Md. Saidur Rahman1Shin-ichi
Nakano2
1Department of Computer Science and Engineering, Bangladesh Univ ersity of
Engineering and Technology (BUET), Dhaka-1000, Bangladesh.
2Department of Computer Science, Gunma University, Gunma 376-8 515,
Japan.
Abstract
In this paper, we deal with the problem of generating all tria ngulations
of plane graphs. We give an algorithm for generating all tria ngulations
of a triconnected plane graph Gofnvertices. Our algorithm establishes
a tree structure among the triangulations of G, called the “tree of tri-
angulations,” and generates each triangulation of GinO(1) time. The
algorithm uses O(n) space and generates all triangulations of Gwithout
duplications. To the best of our knowledge, our algorithm is the ﬁrst al-
gorithm for generating all triangulations of a triconnecte d plane graph;
although there exist algorithms for generating triangulat ed graphs with
certain properties. Our algorithm for generating all trian gulations of a
triconnected plane graph needs to ﬁnd all triangulations of each face (a
cycle) of the graph. We give an algorithm to generate all tria ngulations
of a cycle Cofnvertices in time O(1) per triangulation, where the ver-
tices ofCare numbered. Finally, we give an algorithm for generating a ll
triangulations of a cycle Cofnvertices in time O(n2) per triangulation,
where vertices of Care not numbered.
Key words: Triangulation; Graph; Cycle; Plane Graph; Genealogical
Tree.
Submitted:
May 2009Reviewed:
October 2009Revised:
July 2010Accepted:
October 2010
Final:
November 2010Published:
July 2011
Article type:
Regular paperCommunicated by:
S. Das and R. Uehara
E-mail addresses: tanvirparvez@cse.buet.ac.bd (MohammadTanvirParvez) saidurrahman@cse.buet.ac.bd
(Md. Saidur Rahman) nakano@cs.gunma-u.ac.jp (Shin-ichi Nakano)

458 Parvez et al. Generating All Triangulations of Plane Graphs
1 Introduction
In this paper, we consider the problem of generating all triangulatio ns of plane
graphs. Such triangulations have many applications in Computationa l Ge-
ometry [5, 11], VLSI ﬂoorplanning [14], and Graph Drawing [10]. The main
challenges in ﬁnding algorithms for generating all triangulations are a s follows.
Firstly, the number of such triangulations is exponential in general, and hence
listingallofthemrequireshugetimeandcomputationalpower. Seco ndly, gener-
ating algorithms produce huge output, and storing these output m ay dominate
the running time. For this reason, reducing the amount of output is essential.
Thirdly, checking for any repetitions must be very eﬃcient. Storing the entire
list of objects generated so far will not be eﬃcient, since checking e ach new
object with the entire list to prevent repetition would require huge a mount of
memory and overall time complexity would be very high.
There have been a number of methods for generating combinatoria l objects.
Classical algorithms ﬁrst generate combinatorial objects allowing d uplications,
but output only if the object has not been output yet. These meth ods require
huge space to store the list and a lot of time to check duplications. Or derly
algorithms [9] do not need to store the list of objects generated so far, they
output an object only if it is a canonical representation of an isomor phism class.
Reverse search algorithms [2] also do not need to store the list. The idea is to
implicitly deﬁne a connected graph Hsuch that the vertices of Hcorrespond
to the objects with the given property, and the edges of Hcorrespond to some
relation between the objects. By traversing an implicitly deﬁned spa nning tree
ofH, that means by checking each possible child of each vertex recursiv ely, one
can ﬁnd all the vertices of H, which correspond to all the graphs with the given
property.
There are some well known results for triangulating simple polygons a nd
ﬁnding bounds on the number of operations required to transform one triangu-
lation into another [4, 7, 15]. Researchers also have focused their a ttention on
generating triangulated polygons and graphs with certain propert ies. Hurtado
and Noy [6] built a tree of triangulations of convex polygons with any n umber
of vertices. Their construction is primarily of theoretical interest s; also all the
triangulations of convex polygons with number of vertices up to nneed to be
found before ﬁnding the triangulations of a convex polygon of nvertices. Also,
in [6] the authors did not discuss the time complexity of their method f or gen-
erating the tree of triangulations of convex n-gons. Li and Nakano [8] gave an
algorithm to generate all biconnected “based” plane triangulations with at most
nvertices. Their algorithm generates all graphs with some propertie s without
duplications. Here also, the biconnected “based” plane triangulatio ns ofnver-
tices are generated after the biconnected based plane triangulat ions of less than
nvertices are generated. Hence, if we need to generate the triang ulations of a
convex polygon or a plane graph of exactly nvertices, existing algorithms gen-
erate all the triangulations of convex polygons or plane graphs with less than n
vertices. This is not an eﬃcient way of generation.
There are a number of works concerning enumeration and generat ion of pla-

JGAA, 15(3) 457–482 (2011) 459
3
/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1
2F1F
/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1
3F
/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1
4F
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1
5F
/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1
6F/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/12 51
64FF
F
F
FF
Figure 1: Illustration of the algorithm for generating the triangulat ions of a
triconnected plane graph.
nar triangulations. Triangulations of convex polygons (with kvertices) are in
bijection with binary (rooted) trees having k−2 inner nodes. There exist a
number of eﬃcient algorithms that generate all such trees with loca l perturba-
tions, allowing to run in O(1) per tree [11]. The optimal algorithm for encoding
and generating planar triangulations given in [13] may work for triang ulations
without interior points. However, the algorithm in [13] deals with rand om tri-
angulations, rather than exhaustive generations. There also exis ts work [3]
concerning the generation of triangulations of npoints in the plane based on a
tree of triangulations and a lexicographic way of generating triangu lations, with
O(loglogn) time complexity per triangulation.
We now give the idea behind our algorithm for generating all triangulat ions
of a triconnected plane graph G. Consider Figure 1. For a particular tricon-
nected plane graph G, we treat each face of Gas a cycle and ﬁnd triangulations
of those cycles. These triangulations of the cycles correspond to particular tri-
angulations of the faces of the graph G. Combining the triangulations of the
faces ofGgives us a particular triangulation of G. Therefore, to generate all tri-
angulations of Gwe need to triangulate those intermediate cycles in all possible
ways and combine the triangulations eﬃciently so as to ﬁnd all triangu lations
ofG.

460 Parvez et al. Generating All Triangulations of Plane Graphs
Therefore, in this paper, we also give an algorithm to generate all tr ian-
gulations of a cycle Cofnvertices in O(1) time per triangulation, where the
vertices of Care numbered sequentially. In our algorithm, a new triangulation
ofCis generated from an existing one by making a constant number of ch anges.
The main feature of our algorithm is that we deﬁne a tree structure , based on
parent-child relationships, among those triangulations. In such a “ tree of tri-
angulations,” each node corresponds to a triangulation of Cand each node is
generatedfromitsparentinconstanttime, withuniqueparent-ch ildrelationship
to avoid duplications. In our algorithm, we construct the tree stru cture among
the triangulationsin sucha waythat the parent-childrelationshipis u nique, and
hence there is no chance of producing duplicate triangulations. Our algorithm
also generates the triangulations in place, that means, the space complexity is
onlyO(n).
Our algorithm for generating all triangulations of a triconnected pla ne graph
Ggenerates each triangulation of GinO(1) time and uses our algorithm for
generating all labeled triangulations of a cycle C. We also give the idea to
generate all unlabeled triangulations of a cycle Cofnvertices in time O(n2)
per triangulation, where the vertices of Care not numbered. Our algorithm for
generating all triangulations of a cycle can be used for ﬁnding all tria ngulations
of a simple polygon with “curved” diagonals and for ﬁnding all triangula tions
of a convex polygon with “straight” diagonals. An early version of th is paper is
presented at [12].
Note that, the generating algorithms proposed in this paper have s ome sim-
ilarities with the reverse search paradigm [2]. These similarities include t he use
of an edge ﬂipping operation, the deﬁnition of a spanning tree of the adjacency
graph of triangulations and the eﬃcient use of memory. However, t he main
feature of our algorithm for generating all triangulations of a plane graphGis
that we deﬁne a tree structure explicitly among the triangulations of G, called
“tree of triangulations”. By traversing this explicitly deﬁned spann ing tree,
that means by listing each exact child of each vertex recursively, ou r algorithm
output the triangulations of G. In contrast to the reverse search paradigm, our
algorithm does not need to ﬁnd any “graph of triangulations” of Gfrom which
it is necessary to ﬁnd a spanning tree.
However, the reverse search paradigm could be used to design enu meration
algorithms for the triangulations of a plane graph and the triangulat ions of a
cycle. Avis [1] presented two algorithms based on the reverse sear ch paradigm
[2] for enumerating all 3-connected (2-connected) r-rooted triangulations of n
vertices (with rvertices on the outer face). Such an algorithm can enumerate
all the triangulations of a cycle (the case where r=n, with no interior points).
However, the time complexity of the algorithms in [1] is O(n2g(n,r)), where
g(n,r) is the number of objects to enumerate (e.g. O(n2) per triangulation
for a cycle of nvertices). This is clearly diﬀerent from the O(1) time per
triangulation of a cycle of nvertices achieved by our algorithm. This is due
to our novel way of generating a triangulation from its parent in the tree of
triangulations. Moreover, in addition to a cycle, we have achieved O(1) time
complexity per triangulation for generating all triangulations of a tr iconnected

JGAA, 15(3) 457–482 (2011) 461
plane graph.
Therestofthepaperisorganizedasfollows. Section2givessomede ﬁnitions.
Section 3 gives the outline of the algorithm for generating all triangu lations of a
triconnected plane graph. Section 4 deals with generating all labeled triangula-
tions of a cycle. Section 5 deals with generating all unlabeled triangula tions of a
cycle, where the vertices are not numbered. Finally, Section 6 is the conclusion.
2 Preliminaries
In this section, we deﬁne some terms used in this paper.
LetG= (V,E) be a connected simple graph with vertex set Vand edge
setE. An edge connecting vertices viandvjinVis denoted by ( vi,vj). The
degreeof a vertex vis the number of edges incident to vinG. Theconnectivity
κ(G) of a graph Gis the minimum number of vertices whose removal results
in a disconnected graph or a single vertex graph. A graph is k-connected if
κ(G)≥k.
A graph Gisplanarif it can be embedded in the plane so that no two edges
intersect geometrically except at a vertex to which they are both in cident. A
planegraph is a planar graph with a ﬁxed embedding in the plane. A plane
graph divides the plane into connected regions called faces. The unbounded
face is called the outer face and the other faces are called inner faces . A plane
graphis called a plane triangulation if eachfaceboundarycontainsexactlythree
edges. In this paper, our notion of plane triangulation is such that t he edges
are not necessarily straight lines.
A cycleCin a graph Gis a sequence of distinct vertices v1,v2,···,vksuch
that (v1,vk)∈Eand (vi,vi+1)∈Efor 1≤i < k. A cycle Cin a plane graph
Gdivides the plane into two regions; one region is inside of Cand the other
region is outside of C. We call the region which is inside of a cycle Ctheinner
region of Cand call the other region of Ctheouter region of C. If a graph G
is a cycle then both of its inner region and outer region are faces.
LetCbe a cycle corresponding to the boundary of a face FofG. Let
v1,v2,···,vnbe the labels of the vertices on Cin counterclockwise order. We
represent Cby listing its vertices as C=< v1,v2,···,vn>. We call an edge
(vi,vj) between two vertices viandvjonCachordofCif (vi,vj) is not on
Cand contained in the face FofG. A chord ( vi,vj) divides the cycle into
two cycles: vi,vi+1,···,vjandvj,vj+1,···,vi. A decomposition of a cycle into
triangles by a set of non-intersecting chords is called a triangulation of the cycle .
Figure 2 shows two diﬀerent triangulationsof a cycle C. In a triangulation T
ofC, the set of chords is maximal, that means, every chord not in Tintersects
some chord in T. The sides of triangles in the triangulation are either the
chords or the sides of the cycle. We say a vertex yisvisiblefrom a vertex xin
a triangulation Tof a cycle Cif there exists a chord ( x,y) ofCinT.
Inthispaper,werepresenteachtriangulation TofCbylistingitschords. For
example,thetriangulationofFigure2(a)isrepresentedby T={(v4,v6),(v2,v6),
(v2,v4)}. Given the list of chords, we can uniquely construct the correspon ding

462 Parvez et al. Generating All Triangulations of Plane Graphs
(a)vv6v3
v2v5v4
1vv6v2v5v4
v3
/0/0/0/0/1/1/1/1 /0/0/0/0/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0
/1/1/1
/0/0/0/0/1/1/1/1 /0/0/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0
/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/1/1/0/0/0
/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0
/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1 /0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
(b)1
Figure 2: Two ways of triangulating a cycle of 6 vertices.
triangulation.
A triangulation of a cycle Cofnvertices is called a labeled triangulation , if
the vertices of Care numbered sequentially from v1tovn. The triangulations
of Figures 3(a) and 3(b) are labeled triangulations. On the other ha nd, if the
vertices of Care not numbered, then the triangulations of Care called unlabeled
triangulations . Both the triangulations of Figures 3(c) and 3(d) are unlabeled
triangulations.
/0/0/0/0/1/1/1/1/0/0/0
/1/1/1/0/0/1/1/0/0/1/1/0/0/1/1
/0/0/0
/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0/0/1/1/1/1/0/0/0
/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/1/1
/0/0/0/0/1/1/1/1
/0/0/1/1
/0/0/1/1/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1 /0/0/0/0/0/0/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0
/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0
/1/1/1/1/1/1/1 /0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1 /0/0/0/0/0/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/1/1/1/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/1/1/1/1/0/0/1/1/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0
/1/1/1
/0/0/1/1
/0/0/0
/1/1/1
/0/0/0
/1/1/1
/0/0/0/0/1/1/1/1/0/0/0/0/1/1/1/1/0/0/0/0/1/1/1/1/0/0/0/0/1/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/1/1
/0/0/0
/1/1/1
/0/0/0
/1/1/1
/0/0/0
/1/1/1/0/0/0/0/0/0/1/1/1/1/1/1
/0/0/0/0/0/0/0
/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0/0/0/1/1/1/1/1
/0/0/0/0/0/0/0
/1/1/1/1/1/1/1 /0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/1/1/1/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
(c) (d)vvvv
v
v
v 123v4 5
6
7
8
(a)vvvv
v
v
v 123v45
6
7
8
(b)
Figure 3: Illustration of labeled and unlabeled triangulations.
3 Triangulations of a Triconnected Plane Graph
In this section, we give an algorithm for generating all triangulations of a tri-
connected plane graph Gofnvertices. Our idea is to deﬁne a parent-child
relationship among the triangulations of Gsuch that all the triangulations of
Gform a tree structure. Our algorithm generates the triangulation s ofGin
the order they are present in that tree, called “tree of triangulat ions”, without
storing or building the entire tree at once in the memory.
Assume that Ghaskfaces, arbitrarily labeled F1,F2,···,Fk. For each face

JGAA, 15(3) 457–482 (2011) 463
FiofG, there is a cycle Ciassociated withFi. Here,Cihas equal number of
vertices as Fiand the labels of the vertices of Ciare similar to the vertices of Fi.
A particular triangulation of Fi, denoted T(Fi), corresponds to a triangulation
ofCi, denoted T(Ci). Therefore, triangulating the face Fiin all possible ways
is equivalent to triangulating the cycle Ciin all possible ways. This is true
since a triconnected plane graph has a unique embedding once the ou ter face
of the graph is ﬁxed. Therefore, our algorithm for generating all t riangulations
of a triconnected plane graph needs to ﬁnd all triangulations of eac h cycle, the
details of that is given in Section 4.
LetTbe a triangulation of a cycle Cofnvertices. To generate a new
triangulation from T, we use the following operation. Let ( vi,vj) be a shared
chord of two adjacent triangles of Twhich form a quadrilateral /an}bracketle{tvq,vi,vr,vj/an}bracketri}ht.
If we remove the chord ( vi,vj) fromTand add the chord ( vq,vr), we get a new
triangulation T′. The operation above is well known as ﬂipping. Therefore, we
ﬂipthe edge ( vi,vj) to generate a new triangulation T′, which we denote by
T(vi,vj).
Forexample, in Figure4(a), the twotriangles /an}bracketle{tv1,v2,v3/an}bracketri}htand/an}bracketle{tv1,v3,v4/an}bracketri}htform
the quadrilateral /an}bracketle{tv1,v2,v3,v4/an}bracketri}htand (v1,v3) is the shared chord. We remove
(v1,v3) from the triangulation of Figure 4(a) and add the chord ( v2,v4) to
generate the triangulation of Figure 4(b). Thus, we ﬂip the chord ( v1,v3) of the
triangulation of Figure 4(a) to generate the triangulation of Figure 4(b).
2 /0/0/0
/1/1/1
/0/0/0
/1/1/1
/0/0/1/1
/0/0/0
/1/1/1/0/0/0
/1/1/1
/0/0/1/1 /0/0/0
/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/1/1/1/1/1
(a)v4v3v2
v1
(b)v3
v4v1v
/0/0/0
/1/1/1
Figure 4: Illustration of ﬂipping operation; (a) old triangulation and ( b) new
triangulation.
Similarly, the operation of ﬂipping an edge of the triangulation T(Fi) is
deﬁned as the ﬂipping of the corresponding chord of the triangulat ionT(Ci).
Therefore, to generate new triangulations of the plane graph Gfrom an existing
triangulation TofG, we ﬂip some edges of T. In our algorithm, we deﬁne the
parent-childrelationship amongthe triangulationsof Gin such a waythat every
triangulation of G, except the root triangulation, is generated from its parent
by a single edge ﬂip. Such a tree of triangulations of a triconnected p lane graph
Gis called a genealogical tree and denoted by T(G). The genealogical tree of
the triconnected plane graph Gof Figure 5(a) is shown in Figure 5(b).
This deﬁnition of ﬂipping requires Gto be triconnected. This is because,
ifGhas a cut set of two vertices, then some ﬂip operations may introdu ce
multi-edges. If Gis triconnected then any ﬂip operation will generate a new
triangulation of G. Note that, while generating new triangulations from an
existing triangulation TofG, the edges of the graph Gcannot be ﬂipped.

464 Parvez et al. Generating All Triangulations of Plane Graphs
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1
v6
5v5vv6
5vv6
5vv65vv6
5vv6
5vv6v6
5v
v3
v1 2vv42v
2vv3
v3v1
v1v4v4
v1 2vv4v3v1 2vv3
v4
v1 2vv3
v4
v3
v1 2vv4v3
v1 2vv4
(b)v5v6
v12vv3
v4
(a)2
145 3F
FFFF
Figure 5: Illustration of (a) a triconnected plane graph Gwith ﬁve faces and
(b) genealogical tree T(G) ofG.
Therefore, for a triangulation TofG, we need to classify edges of Tas ﬂippable
and non-ﬂippable. We introduce the related concepts below.
LetTbe atriangulationofacycle Cofnvertices. Thechordsof Twhichcan
be ﬂipped to generate new triangulations of Care called generating chords of
T. In Section 4, we describe the way to ﬁnd the generating chords of T. The set
of generating chords of Tis called the generating set . The triangulation T(Fi)
of the face FiofGhas a generating set of edges equivalent to the generating set
of the triangulation T(Ci). Therefore, to generate new triangulations of G, we
ﬂip an edge from the generating set of a face FiofG.
The rest of this section is organized as follows. In Section 3.1 we deﬁn e the
root triangulations of T(G). We give the detail algorithm for generating all
triangulations of Gin Section 3.2.
3.1 Finding the Root
In this section, we describe the procedure for ﬁnding the root tria ngulation of
the genealogical tree T(G) of a triconnected plane graph Gofnvertices.
LetFibe a face of G. We can represent Fias a list of vertices on the
boundary of Fi. We choose a vertex vjon the boundary of Fiarbitrarily and
call it the root vertex ofFi. LetCibe the cycle associated with Fi. Thenvjis
also called the root vertex of Ci. Consider the triangulation T(Ci) ofCiwhere

JGAA, 15(3) 457–482 (2011) 465
all the chords of T(Ci) are incident to the root vertex vj. This triangulation
T(Ci) ofCigives us a triangulation of the face FiofG. Once all the faces of G
are triangulated in that way, we get a triangulation Tof the graph Gitself. In
our algorithm, such a triangulation TofGis taken as the root triangulation TR
of the genealogical tree T(G). Note that, the choice of the root triangulation
TRwill depend on the way the root vertices are chosen.
The procedurefor ﬁnding TRand correspondinggeneratingsets is as follows.
We traversethe face FiofGto ﬁnd the generatingset of T(Fi), denoted by GSi,
using the doubly connected adjacency list representation of G[10]. Face Fican
be traversed in time proportional to the number of vertices on the boundary of
it. Assume that we traverse the face Ficlockwise starting at vertex vjand take
vjas the root vertex of Ci. Letvk,vlandvmbe three consecutive vertices on
the boundary of Fi, wherevk/ne}ationslash=vjandvm/ne}ationslash=vj. We add the edge ( vj,vl) to the
generating set GSiofT(Fi). The generating set GSoof the root triangulation
ofT(G) can be found as follows. Let GS1,GS2,···,GSkbe thekgenerating
sets of the triangulations T1,T2,···,Tk. Initially, GSois empty. Once we ﬁnd
a generating set GSi, we concatenate GSiwith the existing list GSo. When all
theGSis are generated and concatenated with GSo, we get the generating set
of the root triangulation of T(G).
For example, Figure 6(a) shows a triconnected plane graph Gof eight ver-
tices. One possible root in T(G) is shown in Figure 6(b). In Figure 6(b),
GS1={(v1,v8)},GS2={(v2,v5)},GS3={(v3,v6)},GS4={(v3,v8)}and
GS5={(v6,v8)}. ThusGS0={(v1,v8),(v2,v5),(v3,v6),(v3,v8),(v6,v8)}.
/0/0/0/0/0/0/1/1/1/1/1/1/0/0/0/0/0/0/1/1/1/1/1/1
/0/0/0/0/0/0/1/1/1/1/1/1/0/0/0/0/0/0/1/1/1/1/1/1/0/0/0/0/0/0/1/1/1/1/1/1
/0/0/0/0/0/0/1/1/1/1/1/1/0/0/0/0/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/1/1/1/1/0/0/0/0/1/1/1/1
/0/0/0/0/1/1/1/1/0/0/0/0/1/1/1/1/0/0/0/0/1/1/1/1
/0/0/0/0/1/1/1/1/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/1/1/1/1/1/1
(b)v
vvv
v3
2
4
1v8
v
vvv
vv
1
23
2
6
54
14
65v87
v5v6
(a)F
FFFFv73F
Figure 6: Illustration of (a) a triconnected plane graph Gof eight vertices (b)
corresponding root in T(G).
We now have the following lemma.
Lemma 1 LetGbe a triconnected plane graph of nvertices. Then the root
triangulation TRof the genealogical tree T(G)ofGcan be found in O(n)time.
Proof:In atriconnected plane graph Gofnvertices, the number ofedgesis less
than 3n−6. The number of faces of Gis also bounded by a linear function of
n. Since each face FiofGcan be traversed in time proportional to the number
of edges on the boundary of Fiand each edge can be shared by at most two
faces ofG, traversing all the faces of Grequires time proportional to the total
number of edges of G. Thus the root of T(G) can be found in O(n) time. /square

466 Parvez et al. Generating All Triangulations of Plane Graphs
3.2 The Algorithm
In this section, we give the details of the algorithm for generating all triangula-
tions of a triconnected plane graph Gofnvertices.
Assume that the triconnected plane graph Ghaskfaces. For a particular
triangulation TofG, we generatenew triangulationsof GfromTasfollows. Ifa
triangulation T′ofGis a child of a triangulation TofGin the genealogicaltree,
andT′is generated from Tby ﬂipping a generating edge in Fj, then we say that
Fjiseligible. To generate all child triangulations of a triangulation Twe need
to ﬁnd all eligible faces of Tthen ﬂip each generating edge in each eligible face.
We can observe that if a face Fjis eligible then for each i, 1≤i≤j, faceFiis
also eligible. This simple condition for eligibility ensures eﬃcient generatio n of
the triangulations of G.
We now have the following algorithm for generating all triangulations o f a
triconnected plane graph Gwithkfaces.
Procedure ﬁnd-all-child-triangulations (T,i)
{Tis the current triangulation and Fiis an eligible face of T}
begin
1 Let EGbe the set of generating edges of T(Fi);
2ifEGis empty then return ;
3foreach edge e∈EG
4 Flip eto ﬁnd a new triangulation T′;
5 Output T′;
{ForT′, all the faces Fj, 1≤j≤i, are eligible }
6forj= 1toi
7ﬁnd-all-child-triangulations (T′,j);
end;
Algorithm ﬁnd-all-triangulations (G,k)
{The triconnected plane graph Ghaskfaces}
begin
Label the faces F1,F2,···,Fkarbitrarily;
Find root triangulation TRofT(G);
OutputrootTR;
{For the root TR, all the faces of Gare eligible }
fori= 1tok
ﬁnd-all-child-triangulations (TR,i);
end.
The correctness of the algorithm ﬁnd-all-triangulations depends on the
correct ﬁnding of the generating set of the triangulation T(Fi) of face Fiof
G(Step 1). We also have to ensure that ﬂipping the edges in the gener ating
set ofT(Fi) generates all the children of T(Fi) without duplications. Flipping
an edge of T(Fi) is equivalent to ﬂipping a chord of the triangulation T(Ci)
of the cycle Ciassociated with Fi. Therefore, we need to prove that for a
triangulation Tof a cycle C: (1) ﬂipping the generating chords of Tgenerates
all the child triangulations of Twithout duplications and (2) the number of

JGAA, 15(3) 457–482 (2011) 467
generating chords in any child triangulation of Tis less than in T. We prove
both of these in Section 4.
The time and space complexity of the algorithm ﬁnd-all-triangulations
can be found as follows. From Lemma 1, ﬁnding the root triangulation TRtakes
O(n) time. To ﬁnd the time requiredto generateeachnew triangulation T′from
a triangulation TofG, note that the diﬀerence between the representations of
T′andTcan be found in the triangulation of only one face, say Fi(Steps 3 - 5).
Assume that face Fihas the triangulation T(Fi) inTandT′(Fi) inT′. Now,
to get the representation of T′, all we need to do is to ﬁnd the representation
ofT′(Fi) from the representation of T(Fi). Equivalently, the problem reduces
to the following. Let TandT′be two triangulations of a cycle CandT′is
generated from Tby ﬂipping a generating chord of T. Then, how can we ﬁnd
the representation and the generating set of T′fromTeﬃciently? Section 4
shows that this can be done in O(1) time.
To ﬁnd the space complexity, note that, we can represent a triang ulation
TofGby listing its edges only. Therefore, it takes only O(n) space to store
a triangulation T. The height of the tree T(G) is bounded by the number
of edges in TR(since we may need to ﬂip each generating edge TRonce to
generate a triangulation of G), which is linear in n. The algorithm ﬁnd-all-
triangulations needs to store (1) the representation and generating set of the
current triangulation Tand (2) the information of the path from the root to the
current node of the tree. This implies that the space complexity of t he entire
algorithm can be reduced to O(n).
Therefore we have the following theorem.
Theorem 1 The algorithm ﬁnd-all-triangulations generates all the triangu-
lations of a triconnected plane graph Gofnvertices in time O(1)per triangu-
lation, with O(n)space complexity.
In the next section, we give the algorithm for generating all triangu lations
of a cycle Cofnvertices, where the vertices of Care labeled.
4 Labeled Triangulations of a Cycle
In this section, we give an algorithm to generate all labeled triangulat ions of a
cycleCofnvertices. Here we also deﬁne a unique parent for each triangulation
ofCso that it results in a tree structure among the triangulations of C, with a
suitable triangulation as the root. Once such a parent-child relation ship ofCis
established, we can generate all the triangulations of Cusing the relationship.
We need not to build or to store the entire tree of triangulations at o nce, rather
we generate each triangulation in the order (DFS order) it appears in the tree
structure.
In our algorithm in this section, to make the data structures easier to ma-
nipulate, we write the edge ( vi,vj) such that i < j. Thus the edge incident to
vertexv4andv1is denoted by ( v1,v4), and not by ( v4,v1).

468 Parvez et al. Generating All Triangulations of Plane Graphs
(1,3)4
v5
v63v
2v
1v
1v 1v1v
1v1v
1v1v1v 1v1v
1v1v2v
2v2v2v
2v2v2v
2v2v2v
2v
1v3v
3v
3v3v3v3v
3v
3v3v
3v
3v
3v3v
2vv4v4
v4v4v4
v4v4v4v4v4
v4v4v5
v5
v5v5v5v5
v5
v5v5v5v5
v5v5
v6
v6
v6v6v6v6v6v6v6v6v6
v6
v6v4
2v/0/0/0/0/1/1/1/1/0/0/0/0/1/1/1/1
/0/0/1/1 /0/0/1/1/0/0/0/0/1/1/1/1/0/0/0/0/1/1/1/1
/0/1 /0/0/1/1
/0/0/0/0/1/1/1/1/0/0/1/1/0/0/1/1 /0/1/0/0/1/1/0/0/0/0/1/1/1/1
/0/0/0/0/1/1/1/1/0/1/0/0/0/0/1/1/1/1/0/0/1/1/0/1
/0/0/1/1/0/0/1/1/0/0/0/0/1/1/1/1/0/0/0/0/1/1/1/1/0/0/1/1 /0/1
/0/1 /0/0/1/1/0/0/1/1/0/1 /0/0/1/1
/0/0/0/0/1/1/1/1/0/1/0/0/1/1 /0/1/0/0/1/1 /0/1
/0/0/1/1/0/0/1/1/0/0/1/1 /0/0/1/1/0/1 /0/0/1/1
/0/0/1/1 /0/1/0/0/0/0/1/1/1/1/0/0/1/1/0/1
/0/0/1/1/0/1/0/0/1/1 /0/0/1/1/0/0/1/1
/0/1
/0/0/1/1 /0/1
/0/1 /0/0/1/1/0/0/1/1/0/0/0/0/1/1/1/1/0/1 /0/0/1/1
/0/0/0/0/1/1/1/1/0/1/0/1 /0/0/1/1/0/0/0/0/1/1/1/1
/0/1 /0/0/1/1
/0/0/1/1 /0/1/0/0/0/0/1/1/1/1/0/0/1/1
/0/0/1/1/0/0/0/0/1/1/1/1
/0/0/1/1 /0/0/1/1/0/0/1/1 /0/0/1/1/0/1
/0/1
/0/1/0/0/1/1
/0/0/1/1/0/0/0/0/1/1/1/1
/0/0/0/1/1/1
/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1 /0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1 /0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1 /0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/1/1/1
/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/1/1/1/1/1/0/0/0/0/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0
/1/1/1
/0/0/0
/1/1/1
/0/0/0
/1/1/1
/0/0/0
/1/1/1/0/0/0/0/1/1/1/1
/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1
/0/0/0/0/0
/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1 /0/0/0/0/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0
/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1 /0/0/0/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1 /0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1 /0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/1/1/1/1/1/0/0/0/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1 /0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0
/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/1/1/1
/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/1/1/1/1
/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1 /0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1 /0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/1/1/1/1/1/1/0/0/0/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1 /0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1
/0/0/0/0
/1/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0
/1/1/1/0/0/0/0/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1 /0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0
/1/1/1/1/1/1/1
/0/0/0/1/1/1
/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0
/1/1/1 (1,5)
(1,4)(1,3)
(1,4)
(1,3) (1,3) (1,5) (1,5)(1,4)(1,5)(1,5)
(1,4)TT
T1
3 2v
Figure 7: Genealogical tree T(C) of a cycle Cof six vertices.
One can observe that, each triangulation of the tree of triangulat ions in
Figure 7, except the root, is generated from its parent by ﬂipping a single chord.
Each arrow is labeled in Figure 7 to indicate which chord has been ﬂipped to
generate a particular child. We call a tree of triangulations of a cycle Cofn
vertices a genealogical tree ofCand denote it by T(C). Figure 7 illustrates one
such genealogical tree. Let Tbe a triangulation of C, in which all the chords of
Tare incident to vertex v1. We regard Tas therootTrofT(C) and call v1as
the root vertex of C. Note that the choice of root vertex is arbitrary in ﬁnding
Tr. We can take any of the vertices of Cother than v1as the root vertex to
ﬁnd a root triangulation Tr. With this deﬁnition of the root vertex of C, we
describe the labeled triangulations of Casrooted triangulations .
Note that, in the root TrofT(C), every vertex of Cis visible from the root
vertexv1. We say that the root vertex v1hasfull vision inTr. Obviously, in a
non-roottriangulation TofC, vertexv1doesnothavethefullvision. Thereason
is thatThas some “blockingchords”which areblockingsome verticesof Cfrom
being visible from the root vertex v1. A chord ( vi,vj) of a triangulation TofC
is ablocking chord ofTif bothviandvjare adjacent to the root vertex of C.
We say that the root vertex of Chas ablocked vision in a non-roottriangulation
TofC. The following lemma characterizes the non-root triangulations of C.
Lemma 2 Each triangulation Tof a cycle C=/an}bracketle{tv1,v2,···,vn/an}bracketri}hthas at least one
blocking chord, if Tis not the root of T(C).
Proof:Letvjbe the vertex of Csuch that ( v1,vk) is a chord of T, for allk≥j.
Then there exists a vertex visuch that i < jand (vi,vj) is a chord of T(choose
ito be the minimum). Otherwise, all chords of Twould be incident to v1and
Twould be the root of T(C). Since Tis a triangulation of C, (v1,vi,vj) is a
triangle, and hence ( vi,vj) is a blocking chord. /square

JGAA, 15(3) 457–482 (2011) 469
Suppose we ﬂip a chord ( v1,vj) ofTto generate a new triangulation T′.
Let (vb,vb′),b < b′be the newly found chord in T′. Obviously ( vb,vb′) is a
blocking chord of T′. Similarly, if we ﬂip a blocking chord of Tto generate T′,
the newly found chord will be non-blocking, incident to vertex v1inT′. For
example, if we ﬂip the chord ( v1,v4) of the triangulation of Figure 8(a), we get
the triangulation of Figure 8(b), where ( v3,v6) is the newly found chord. This
new chord ( v3,v6) is a blocking chord of the triangulation of Figure 8(b).
(b)vv6v3
v2v5v4
1vv6v3
v2v5v4
/0/0/0/0/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0/0/1/1/1/1 /0/0/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0
/1/1/1
/0/0/1/1/0/0/0
/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
(a)1
Figure 8: Illustration of the generation of a blocking chord; (a) old t riangulation
and (b) new triangulation.
The rest of this section is organized as follows. Section 4.1 describes child
to parent relationship among the triangulations of a cycle Cofnvertices. Sec-
tion 4.2 deals with the generation of the children of a triangulation Tin the
genealogical tree T(C) ofC. Section 4.3 describes the data structures used to
represent a triangulation TofC. Finally, Section 4.4 describes the algorithm
to generate all triangulations of C.
4.1 Child-Parent Relationship
It is convenient to consider the child-parent relationship before co nsidering the
parent-child relationship. Throughout the section, we denote the parent of a
triangulation TbyP(T).
We deﬁne the child-parent relationships among the triangulations of Cwith
two goals in mind. First, the diﬀerences between a triangulation Tand its
parentP(T) should be minimal, so that Tcan be generated from P(T) with
the minimal eﬀort. Second, every triangulation TofCmust have a parent and
only one parent in the genealogical tree T(C). We achieve the ﬁrst goal by
ensuring that the parent P(T) of a triangulation Tcan be found by ﬂipping a
single chord of T. That means Tcan also be found from its parent P(T) by
ﬂipping a single chord of P(T). The second goal, that is the uniqueness of the
parent-child relationship, can be achieved as follows.
Our idea of deﬁning a parent-child relationship is that the parent of a trian-
gulation Tmust have a “clearer vision” than T. LetTandT′be two triangula-
tions ofC. We say that T′has aclearer vision thanTif the number of vertices
visible from v1inT′is greater than the number of vertices visible from v1inT.
For example, three vertices are visible from vertex v1in the triangulation T2of
Figure 7, whereas four vertices are visible from vertex v1in the triangulation
T1of Figure 7. Therefore T1has a clearer vision than T2in Figure 7.

470 Parvez et al. Generating All Triangulations of Plane Graphs
We can easily get a triangulation T′fromT, whereT′has a clearer vision
thanT, by ﬂipping a blocking chord( vb,vb′) ofT. We say that the triangulation
T′is theparentofTif the chord ( vb,vb′) is the “leftmost blocking chord” of T.
The chord ( vb,vb′),b < b′, ofTis theleftmost blocking chord ofTif no other
blocking chord of Tis incident to a higher indexed vertex than vb′inT. For
example, in Figure 7, ( v4,v6) is the leftmost blocking chord of the triangulation
T2. Therefore we ﬂip the chord ( v4,v6) ofT2to ﬁnd the parent triangulation T1
ofT2, as shown in Figure 7.
The above deﬁnition of the parent of a triangulation TofCensures that
we can always ﬁnd a unique parent of a non-root triangulation TofC. From
Lemma 2, a non-roottriangulation TofChas at leastone blocking chord. From
these blocking chords of Twe select the one which is the leftmost and ﬂip that
chord to ﬁnd the unique parent P(T) ofT. Based on the above parent-child
relationship, the following lemma claims that every triangulation of a cy cleC
ofnvertices is present in the genealogical tree T(C).
Lemma 3 For any triangulation Tof a cycle C=/an}bracketle{tv1,v2,···,vn/an}bracketri}ht, there is
a unique sequence of ﬂipping operations that transforms Tinto the root Trof
T(C).
Proof:LetTbe a triangulation other than the root of T(C). Then according
to Lemma 2, Thas at least one blocking chord. Let ( vb,vb′) be the leftmost
blocking chord of T. We ﬁnd the parent P(T) ofTby ﬂipping the leftmost
blocking chord of T. Since ﬂipping a blocking chord of Tresults in a chord
incident to vertex v1in the new triangulation, P(T)has onemore chordincident
tov1thanT. Now, if P(T) is the root, then we stop. Otherwise, we apply the
same procedure to P(T) and ﬁnd its parent P(P(T)). By continuously applying
this process of ﬁnding the parent, we eventually generate the roo t triangulation
TrofT(C). /square
Lemma 3 ensures that there can be no omission of triangulations in th e
genealogicaltree T(C)ofacycle Cofnvertices. Sincethereisauniquesequence
ofoperationsthat transformsatriangulation TofCintothe root TrofT(C), by
reversing the operations we can generate that particular triangu lation, starting
at the root. We give the details in the next section.
4.2 Generating the Children of a Triangulation in T(C)
In this section, we describe the method for generating the children of a trian-
gulation TinT(C).
To ﬁnd the parent P(T) of the triangulation T, we ﬂip the leftmost blocking
chord of T. That means P(T) has fewer blocking chords than T. Therefore, the
operation for generating the children of Tmust increase the number of blocking
chords in the children of T. Intuitively if we ﬂip a chord ( v1,vj) ofT, which
is incident to vertex v1inT, and generate a new triangulation T′, thenT′
contains one more blocking chord than T. We call all such chords ( v1,vj) as the
candidate chords ofT.

JGAA, 15(3) 457–482 (2011) 471
Note that, ﬂipping a candidate chord of Tmay not always preserve the
parent-child relationship described in Section 4.1. For example, we ge nerate
the triangulation of Figure 9(b) by ﬂipping the candidate chord ( v1,v3) of the
triangulation of Figure 9(a). The leftmost blocking chord of the tria ngulation
of Figure 9(b) is ( v4,v6); therefore the parent of the triangulation of Figure 9(b)
is the triangulation of Figure 9(c), not the triangulation of Figure 9( a).
(c)vv6v3
v2v5v4
1vv6v3
v2v5v4
1vv6v2v5v4
v3
/0/0/0/0/1/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0
/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0
/1/1/1
/0/0/0/0/1/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0
/1/1/1
/0/0/0/0/1/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0
/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/1/1/0/0/0/0/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0
/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0
/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0
/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1 /0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
(a) (b)1
Figure 9: Illustration of a ﬂipping that does not preserve parent-c hild relation-
ship.
Therefore to keep the parent-child relationship unique, we ﬂip a can didate
chord (v1,vj) ofTto generate a new triangulation T′if and only if ﬂipping
(v1,vj) ofTresults in the leftmost blocking chord of T′. We call such a can-
didate chord ( v1,vj) ofTas agenerating chord . The generating chords of a
triangulation TofCcan be found as follows. Let ( vb,vb′) be the leftmost block-
ing chord of a triangulation Tof a cycle Cofnvertices. Then ( v1,vj) is a
generating chord of Tifj≥b. IfThas no blocking chord then all chords of
Tare generating chords. Thus all the chords of the root TrofT(C) are gen-
erating chords. All other candidate chords of Tare called non-generating . We
call the set of generating chords of a triangulation Tas thegenerating set GS
ofT. For example, the triangulation in Figure 10(a) is the root triangulat ion of
T(C) of a cycle Cof 8 vertices. Therefore, all the chords of the triangulation
in Figure 10(a) are generating chords. In the triangulation of Figur e 10(b),
(v1,v4), (v1,v6) and (v1,v7) are three generating chords, whereas ( v1,v3) is a
non-generating chord.
/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/1/1/1/1/1/1 /0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1
v7
v8v1v5
v6 v3v4
v7
v8v1v5
v6 v3v4
(a) (b)v2v2
Figure 10: Illustration of generating chords.
We now have the following lemmas.
Lemma 4 The root Trof the genealogical tree T(C)of a cycle Cofnvertices
hasn−3generating chords and any other triangulations in T(C)has less than
n−3generating chords.

472 Parvez et al. Generating All Triangulations of Plane Graphs
Proof:The number of chords in any triangulation Tof a cycle Cofnvertices
isn−3. Thus the maximum number of possible generating chords is also n−3.
Since the root triangulation Trhas all its chords as generating, Trcontains n−3
generating chords. Any triangulation Tother than the root Trcontains at least
one blocking chord, which is not incident to vertex v1inT. Since generating
chords must be incident to vertex v1, any triangulation other than Trhas less
thann−3 generating chords. /square
Lemma 5 Let(v1,vj)be a generating chord of a triangulation Tof a cycle C
ofnvertices. Then ﬂipping (v1,vj)inTresults in the leftmost blocking chord
ofT(v1,vj).
Proof:Let (vr,vr′) be the leftmost blocking chord of T. We ﬁrst consider the
case where either vj=vrorvj=vr′.
Ifvj=vr, then/an}bracketle{tv1,vj,vr′/an}bracketri}htis a triangle of T(see Figure 11(a)) and after
ﬂipping ( v1,vj) ofTwe get (vi,vr′) as a chord in T(v1,vj), for some i < j(see
Figure 11(b)). Since every face of T(v1,vj) is a triangle, /an}bracketle{tv1,vi,vr′/an}bracketri}htis a triangle
ofT(v1,vj). Therefore,( vi,vr′)istheblockingchordof T(v1,vj). Since, ( vr,vr′)
is the leftmost blocking chord of Tand vertex vris not visible from vertex v1
inT(v1,vj), (vi,vr′) is the leftmost blocking chord of T(v1,vj)
Ifvj=vr′, then/an}bracketle{tv1,vr,vj/an}bracketri}htis a triangle of T(see Figure 11(c)) and after
ﬂipping ( v1,vj) ofTwe get (vi,vr) as a chord of T(v1,vj), for some i > j(see
Figure 11(d)). Since every face of T(v1,vj) is a triangle, /an}bracketle{tv1,vr,vi/an}bracketri}htis a triangle
ofT(v1,vj). Therefore, ( vr,vi) is a blocking chord of T(v1,vj). Since, ( vr,vr′)
is a leftmost blocking chord of Tand (vr,vi) is a blocking chord of T(v1,vj),
wherei > r′, (vr,vi) is the leftmost blocking chord of T(v1,vj)
We now consider the case where j > r′(see Figure 11(e)). Let ( vq,vq′)
be the chord which appears in T(v1,vj) after ﬂipping the chord ( v1,vj) ofT
(see Figure 11(f)). Every face of T(v1,vj) is a triangle. Thus, /an}bracketle{tv1,vq,vq′/an}bracketri}htis a
triangle of T(v1,vj) and (vq,vq′) is a blocking chord of T(v1,vj). Since, q′> j,
we have q′> r′. Therefore, ( vq,vq′) is the leftmost blocking chord of T(v1,vj).
/square
Lemma 6 LetTbe a triangulation of a cycle Cofnvertices. Let T(v1,vj)
be the triangulation generated by ﬂipping the chord (v1,vj)ofT. ThenTis
the parent of T(v1,vj)in the genealogical tree T(C)if and only if (v1,vj)is a
generating chord of T.
Proof:Necessity . Assume that ( v1,vj) is a non-generating chord of T. It is
suﬃcient to show that Tis not the parent of T(v1,vj). Here, we have j < r
(see Figure 12(a)). Let ( vq,vq′) be the chord which appears in T(v1,vj) after
ﬂipping ( v1,vj) ofT(see Figure 12(b)). Since the chord ( v1,vr) ofTis also
a chord of T(v1,vj), we have q′≤r. Therefore, q < r′. Thus, ( vr,vr′) is the
leftmost blocking chord of T(v1,vj) andTis not the parent of T(v1,vj).
Suﬃciency . Assume that ( v1,vj) is a generating chord of T. We show that
Tis the parent of T(v1,vj) inT(C).

JGAA, 15(3) 457–482 (2011) 473
q7v6
v8/0/0/1/1/0/0/1/1/0/0/0
/1/1/1/0/0/1/1/0/0/1/1
/0/0/0
/1/1/1
/0/0/0
/1/1/1
/0/0/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/1/1/0/0/1/1/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0
/1/1/1
/0/0/1/1
/0/0/1/1/0/0/0
/1/1/1/0/0/1/1/0/0/1/1/0/0/0
/1/1/1
/0/0/0
/1/1/1
/0/0/0
/1/1/1
/0/0/1/1
/0/0/1/1
/0/0/0
/1/1/1/0/0/0
/1/1/1/0/0/0
/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0
/1/1/1
/0/0/0
/1/1/1
/0/0/0
/1/1/1
/0/0/0
/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0/0/1/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/1/1/1/1
/0/0/0
/1/1/1/0/0/0
/1/1/1/0/0/0
/1/1/1/0/0/0
/1/1/1
/0/0/0
/1/1/1
/0/0/0
/1/1/1
/0/0/0
/1/1/1
/0/0/0
/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1 /0/0/0/0/0/0/0/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1 /0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1 /0/0/0/0/0/0/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1 /0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1 /0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1 /0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0/0/0/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1 /0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1 /0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1 /0/0/0/0/0/0/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/1/1/1/1/1/1/1 /0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1 /0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/1/1/1/1/1/1/1 /0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1 /0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1 /0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1vvvv
v
v1235
7
8
(a)v4vj
vv6= '
vvvv
v
v1235
7
8
(c)v4= vr
vv6= r'= vjvvvv v
v
v
v1234 5
7
8
(b)= v = v q6'
vvvv v
v
v
v1234 5
6
8
(d)= vq
= v7q'
vvvv
v1235
8v4
v6'v= r
v= r
(e)vvvv v
v
1234 5
7
(f)r= = vr
= vjq
= vq
= v'v
Figure 11: Illustration of Lemma 5.
Let (vq,vq′) be the chord which appears in T(v1,vj) after ﬂipping ( v1,vj)
ofT. To prove that Tis the parent of T(v1,vj) inT(C), we must show that
(vq,vq′) is the leftmost blocking chord of T(v1,vj).
We ﬁrst consider the case where Tis the root of T(C).Tdoes not have any
parent and all the chords of Tare incident to vertex v1. Therefore, ( vq,vq′) is
the only chord of T(v1,vj) which is not incident to vertex v1. Thus, ( vq,vq′) is
the leftmost blocking chord of T(v1,vj).
We now consider the case where Tis not the root of T(C). Then by Lemma
5, (vq,vq′) is the leftmost blocking chord of T(v1,vj). /square
r
7
/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0
/1/1/1/0/0/0
/1/1/1/0/0/0
/1/1/1/0/0/0
/1/1/1
/0/0/0
/1/1/1
/0/0/0
/1/1/1
/0/0/0
/1/1/1
/0/0/0
/1/1/1/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1 /0/0/0/0/0/0/0/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1 /0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
vvvv
v1235
8
(a)v4
v6'
vvvv v
v
1234 5
7
(b)v= r
v= = vj= 
= vqvq
v6
v8'
v
Figure 12: Illustration of Lemma 6.
According to Lemma 5 and 6, if the generatingset GSof a triangulation Tis

474 Parvez et al. Generating All Triangulations of Plane Graphs
non-empty, then we can generate each of the children of TinT(C) by ﬂipping a
generating chord of T. Therefore, the number of children of a triangulation Tin
T(C) will be equal to the cardinality of the generating set. Thus, the fo llowing
lemma holds.
Lemma 7 The number of children of a triangulation Tof a cycle Cis equal
to the number of chords in the generating set of T. The root of T(C)has the
maximum number of children.
4.3 The Representation of a Triangulation in T(C)
In this section, wedescribe a datastructure that we use to repre senta triangula-
tionTand that enables us to generate each child triangulation of Tin constant
time.
Foratriangulation TofC, wemaintainthreelists: L,GSandOtorepresent
Tcompletely. Here Lis the list of chords of TandGSis the generating set
ofT. For each chord ( v1,vj) in the generating set GSofT, we maintain a
corresponding opposite pair (vo,vo′), such that /an}bracketle{tv1,vo,vj,vo′/an}bracketri}htis a quadrilateral
ofT. Note that, o < jando′> j.Ois the list of such opposite pairs. For
example, in Figure 10(a), the generating chord ( v1,v4) has the opposite pair
(v3,v6).
Since we generate triangulations of Cstarting with the root Tr, we ﬁnd the
representation of Trﬁrst. The chords of Trare listed in Lin clockwise order.
That is, for Tr,L={(v1,vn−1),(v1,vn−2),···(v1,v3)}. The generating set GS
is exactly similar to the list LofTr:GS={(v1,vn−1),(v1,vn−2),···,(v1,v4),
(v1,v3)}. Thecorrespondinglistofoppositepairsis O={(vn−2,vn),(vn−3,vn−1),
···,(v3,v5),(v2,v4)}; that means, ( vj−1,vj+1) is the opposite pair of ( v1,vj) in
Tr, 3≤j≤n−1.
LetT(v1,vj) be a child triangulation of TinT(C) generated from Tby
ﬂipping the chord( v1,vj) ofT. Let (vb,vb′) be the blocking chordwhich appears
inT(v1,vj) after ﬂipping ( v1,vj) ofT. The list LofT(v1,vj) canbe found easily
fromthe representationof Tbyremoving( v1,vj) fromthe list LofTand adding
(vb,vb′) to it. Note that one can easily ﬁnd the blocking chord ( vb,vb′) ofT′,
since (vb,vb′) is the opposite pair of ( v1,vj) in the representation of T.
In the next section, we give the detailed algorithm for generating th e trian-
gulations of Cand show that the representation of a child triangulation T′of
Tcan be found from the representation of Tin constant time.
4.4 The Algorithm
In this section, we give an algorithm to generate all triangulations of a cycleC
ofnvertices.
Letvj1,vj2,···,vjk,j1> j2>···> jk, be the sequence of kvertices of a
triangulation TofC. Here (v1,vj1),(v1,vj2),···,(v1,vjk) are the chords of T
and the chords ( v1,vji),1≤i≤k,are generating chords of T. Then, Thas
a generating set GS={(v1,vj1),(v1,vj2),···,(v1,vjk)}ofkgenerating chords,

JGAA, 15(3) 457–482 (2011) 475
for 0≤k≤n−3. ForTr,GS={(v1,vn−1),(v1,vn−2),···,(v1,v4),(v1,v3)}.
For each chord ( v1,vj) ofT, we keep an opposite pair ( vo,vo′) inT.Ois the
set of such pairs. For Tr,O={(vn−2,vn),(vn−3,vn−1),···,(v3,v5),(v2,v4)}as
shown in Section 4.3. We ﬁnd the sets GSandOof a child T′ofTby updating
the lists GSandOofTwhile we generate T′.
We now describe a method for generating the children of a triangulat ionT
inT(C). We have two cases based on whether Tis the root of T(C) or not.
Case 1: Tis the root of T(C).
In this case, all the chords of Tare generating chords and there are a total
ofn−3 such chords in T. Any of these generating chords of Tcan be ﬂipped
to generate a child triangulation of T. For example, the root of the genealogical
tree in Figure 7 has three generating chords; thus it has three child ren as shown
in Figure 7.
Case 2: Tis not the root of T(C).
Let (vb,vb′) be the leftmost blocking chord of T. Consider a chord ( v1,vj)
ofT. Ifj≥b, then (v1,vj) is a generating chord of T. Therefore, according to
Lemma 6, T(v1,vj) is a child of TinT(C). Thus, for all chords ( v1,vj) ofT
such that j≥b, a new triangulation is generated by ﬂipping ( v1,vj).
Ifj < b, then (v1,vj) isa non-generatingchordof Tand accordingtoLemma
6, we cannot ﬂip ( v1,vj) to generate a new triangulation from T.
Based on the case analysis above, we can generate all triangulation s ofC.
The algorithm is as follows.
Procedure ﬁnd-all-child-triangulations-cycle (T)
begin
OutputT;{output the diﬀerence in representation from the
previous triangulation }
ifThas no generating chords then return ;
Let (vb,vb′) be the leftmost blocking chord of T;
forallj≥b
if(v1,vj) is a chord of Tthen
ﬁnd-all-child-triangulations-cycle (T(v1,vj));{Case 2}
end;
Algorithm ﬁnd-all-triangulations-cycle (n)
begin
OutputrootTr;
T=Tr;
forj=n−1to3
ﬁnd-all-child-triangulations-cycle (T(v1,vj));{Case 1}
end.
The following theorem describes the correctness and performanc e of the al-
gorithm ﬁnd-all-triangulations-cycle .
Theorem 2 Given a cycle Cofnvertices, we can generate all the triangula-
tions ofCinO(1)time per triangulation, without duplications and omission s.
The space complexity of the algorithm is O(n).

476 Parvez et al. Generating All Triangulations of Plane Graphs
Proof:LetTbe a triangulation of CandT(v1,vj) be the triangulation gen-
erated from Tby ﬂipping the chord ( v1,vj) ofT. The algorithm ﬁnd-all-
triangulations-cycle generates T(v1,vj) fromTif only if ( v1,vj) is a generat-
ing chord of T. Therefore, according to Lemma 6, Tis the parent of T(v1,vj).
That means, each triangulation TofCis generated from its parent only; there-
fore, duplication cannotoccur. To provethat noomissionoccurs, weuseLemma
3. Lemma 3 implies that for any triangulation TofC, there is a unique path
from the root TrtoTinT(C). Thus, to show that the algorithm ﬁnd-all-
triangulations-cycle does not omit any triangulation, it is suﬃcient to prove
that the algorithm ﬁnd-all-triangulations-cycle generates all the children of
a triangulation T. By Lemma 6, to generate the children of a triangulation T,
only the generating chords of Tneed to be ﬂipped. Since the algorithm ﬁnd-
all-triangulations-cycle ﬂips all the generating chords of a triangulation Tto
generate new triangulations from T, all the children of TinT(C) are generated.
The complexity of the algorithm can be found as follows. We need to st ore
the generating set GSfor the current triangulation TofC. Since the maximum
cardinality of GSisn−3, it takes O(n) space to store it. Along with GS,
we need to maintain for T, the set of opposite pairs Oand update it while
generating children. We also need to maintain another list Lfor listing the
chords of T. To generate the triangulations of C, we start at the root of T(C).
For the root of T(C),GSis identical to L. When a generating chord of Tis
ﬂipped, that chord is replaced in the list LofTby its opposite pair in Tto
get the list Lof the child. Since we use a recursive procedure to generate the
triangulations without constructing the whole T(C), and the depth of the tree
isn−2 (number of chords in the root plus one), the algorithm uses O(n) space.
Now the question is how can we update GSandO? By implementing these
two sets using linked lists and storing appropriate pointers at each n ode on the
path from the root of T(C) to the current triangulation T, we can do it in
constant time. Let ( v1,vj) be the chord of Tto be ﬂipped. The updated lists
GSandOcorrespond to the newly generated child of T.
Flipping the generating chord ( v1,vj) ofTcan change the opposite pairs of
maximum two other candidate chords of Tin the representation of T(v1,vj). In
our algorithm, we only need to change the opposite pairs of candidat e chords
ofT(v1,vj). Let (v1,vi),(v1,vj) and (v1,vk) be three candidate chords of T,
k < j < i , such that /an}bracketle{tv1,vk,vj,vi/an}bracketri}htis a quadrilateral of T, as shown in Figure 13.
We now ﬂip ( v1,vj) ofTto generate the child T(v1,vj) ofT. Flipping the chord
(v1,vj) ofTchanges the opposite pairs of the chords ( v1,vi) and (v1,vk) ofT
inT(v1,vj). The changes can be done as follows.
Let (vo,vo′) be the opposite pair of ( v1,vj) inT. Hereo=kando′=i, as
shown in Figure 13. Let the opposite pair of ( v1,vi) inTbe (vl,vl′). Thenl=j
and the opposite pair of ( v1,vi) inT(v1,vj) is (vo,vl′). Similarly, if the opposite
pair of (v1,vk) is (vs,vs′) inT, thens′=jand the opposite pair of ( v1,vk) in
T(v1,vj) will be ( vs,vo′). Figure 14 shows the update operations. Clearly, these
updates can be done in O(1) time.
Thus, if a triangulation Thaskchildren, all of them can be generated in
O(k) time. Therefore each child of Tis generated in O(1) time. /square

JGAA, 15(3) 457–482 (2011) 477
o
v'vj =vkv
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
v1=voi/0/0/0/0/1/1/1/1
/0/0/1/1/0/0/0/0/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0/0/1/1/1/1
Figure 13: Flipping ( v1,vj) can aﬀect two candidate chords.
1vv6=vl' v6=vl'
/0/0/1/1/0/0/0/0/1/1/1/1 /0/0/0
/1/1/1/0/0/0
/1/1/1/0/0/0
/1/1/1
/0/0/0
/1/1/1
/0/0/0/0/1/1/1/1/0/0/0/0/1/1/1/1 /0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0
/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0
/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1 /0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1 /0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1 /0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
(a) (b)v5=vi=vo'v4=v=v=vlj s '
v3=vk=vo
v2=vsv5=vi=vo'v4=v=v=vlj s '
v2=vsv3=vk=v
1o
v
Figure 14: Illustration of the update operations of the opposite pa irs of two
aﬀected edges; (a) parent and (b) child.
5 Generating Unlabeled Triangulations
In this section, we give the idea for generating all unlabeled triangula tions of a
cycleCofnvertices.
Generating all unlabeled triangulations of a cycle Cis more diﬃcult than
generating labeled triangulations. If the vertices of Care not numbered then we
need to avoid “rotational”and “mirror repetitions” among the trian gulations of
C. Two unlabeled triangulations of a cycle are rotationally equivalent to each
other, if one can be found by rotating the other one. Similarly, two u nlabeled
triangulations of a cycle are mirror image of each other, if one can be found
by taking the mirror image of the other one. For example, the triang ulations
of Figure 15(a) and (b) are rotationally similar if we remove the labels, since
then both of them are similar to the triangulation of Figure 15(c). Th e two
triangulations of Figure 16(a) and (b) are mirror images of the one a nother, if
no labels are used, and both of them are then similar to the triangulat ion of
Figure 16(c). In this section, we modify our algorithm for generatin g all trian-
gulations of a cycle to avoid such repetitions. For this purpose, we c onsider each
triangulation of Cas belonging to a particular class, in which the triangulations
ofCare rotationally equivalent or mirror images of one another. We choo se
one representative triangulation from each class. The modiﬁed algo rithm still
uses the labels while generating the triangulations, but avoids any ro tational or
mirror repetitions by outputting a triangulation only if it is the repres entative
of a particular class. Thus, our modiﬁed algorithm constructs the t ree of trian-
gulations T6of a cycle of six vertices as shown in Figure 17. Note that, only

478 Parvez et al. Generating All Triangulations of Plane Graphs
three triangulations are there in Figure 17, while the tree of triangu lations of
Figure 7 contains 14 triangulations.
/0/0/0
/1/1/1
/0/0/0
/1/1/1
/0/0/0/0/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0/0/1/1/1/1/0/0/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0
/1/1/1
/0/0/1/1
/0/0/0
/1/1/1/0/0/1/1/0/0/1/1
/0/0/1/1/0/0/0/0/1/1/1/1
/0/0/0/0/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0/0/1/1/1/1/0/0/0/0/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1 /0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1 /0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1 /0/0/0/0/0/0/0/1/1/1/1/1/1/1
(a) (c) (b)vvv v
v
v123 4
5
6 vvv v
v
v123 4
5
6
Figure 15: Two triangulations of (a) and (b) are rotationally equivale nt to the
triangulation of (c), when the labels are removed.
/0/0/0
/1/1/1
/0/0/0
/1/1/1
/0/0/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0/0/1/1/1/1/0/0/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0
/1/1/1
/0/0/1/1
/0/0/0
/1/1/1/0/0/1/1/0/0/1/1
/0/0/0/0/1/1/1/1/0/0/0/0/1/1/1/1
/0/0/0/0/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0/0/1/1/1/1/0/0/0/0/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1 /0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1 /0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
(a) (c) (b)vvv v
v
v 123 4
5
6 vvv v
v
v 123 4
5
6
Figure 16: Two triangulations of (a) and (b) are are mirror image of e ach other,
similar to the triangulation of (c), when the labels are removed.
/0/0/1/1
/0/0/0/0/1/1/1/1/0/0/1/1/0/0/0/0/1/1/1/1/0/0/1/1/0/0/0/0/1/1/1/1
/0/0/0/0/1/1/1/1
/0/0/0
/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0
/1/1/1/0/0/0
/1/1/1/0/0/0/0/1/1/1/1
/0/0/1/1/0/0/0/0/1/1/1/1/0/0/0/0/1/1/1/1/0/0/0/0/1/1/1/1/0/0/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1 /0/0/0/0/0/0/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1 /0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1 /0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1 /0/0/0/0/0/0/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/0/0/0/0/0/0/0/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1
/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0
/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1/1(1,4) (1,3)
vvv v
v
v123 4
5
6vvv v
v
v123 4
5
6vvv v
v
v123 4
5
6
Figure 17: Illustration of T(C) from Figure 7 when rotational and mirror rep-
etitions are not allowed.
We now give a new representation of each triangulation of a cycle tha t en-
ables us to avoid any rotational or mirror repetitions easily. Let Tbe a trian-
gulation of Cwhere the vertices of Care labeled sequentially from v1tovn.
Alabeled degree sequence /an}bracketle{td1,d2,···,dn/an}bracketri}htofTis the sequence of degrees of the
vertices, where diis the degree of viin the graph associated with T. A vertex
with degree 2 is called an earofT. We thus have the following lemma.
Lemma 8 LetTbe a labeled triangulation of a cycle Cofnvertices. Then T
can be represented uniquely by its labeled degree sequence.
Proof: Let/an}bracketle{td1,d2,···,dn/an}bracketri}htbe the labeled degree sequence of T. We note
thatThas at least two ears. Let viis the clockwise ﬁrst ear. Remove it and

JGAA, 15(3) 457–482 (2011) 479
decreasethe degreesof its twoneighboring verticesby one. Apply the procedure
recursivelyuntilthevertices v1andv2areleft. Thuswegetasequenceofvertices
vi1,vi2,···vin−2. Now adding the vertices in reverse order we can generate T.
/square
5.1 Removing Rotational Repetitions
In this section, we describe the procedure for avoiding rotational repetitions.
The following fact is crucial for that purpose.
Fact 9LetTandT′be two triangulations of a cycle Cofnvertices, which
are rotationally equivalent to each other. Then, by rotatin g the labeled degree
sequence of T, we get the labeled degree sequence of T′.
As an illustration of the Fact 9, the triangulations of Figure 15(a) an d
(b) have the labeled degree sequences /an}bracketle{t3,2,4,3,2,4/an}bracketri}htand/an}bracketle{t4,3,2,4,3,2/an}bracketri}htrespec-
tively. By right rotating the labeled degree sequence of the triangu lation of
Figure 15(a) four times, we get the labeled degree sequence of the triangulation
of Figure 15(b).
LetTandT′be two triangulations of a cycle Cofnvertices, which are
rotationally equivalent to each other. Let /an}bracketle{td1,d2,···,dn/an}bracketri}htand/an}bracketle{td′
1,d′
2,···,d′
n/an}bracketri}ht
be the labeled degree sequences of TandT′respectively. Let d1=d′
1,d2=
d′
2,···,dk−1=d′
k−1anddk> d′
kfor some k, 1≤k≤n. We say that the
sequence /an}bracketle{td1,d2,···,dn/an}bracketri}htisgreaterthan the sequence /an}bracketle{td′
1,d′
2,···,d′
n/an}bracketri}htandT
has agreatersequence than T′. For example, the triangulations of Figure 18(a)
and (b) have the degree sequences /an}bracketle{t5,2,5,2,3,4,3,2/an}bracketri}htand/an}bracketle{t4,2,3,4,3,3,2,5/an}bracketri}ht
respectively and the ﬁrst sequence is greater than the second on e. Thus the
triangulation of Figure 18(a) is greater than the triangulation of Fig ure 18(b).
(b)5v4v4v5/0/0/0/0/1/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0/0/1/1/1/1/0/0/0
/1/1/1
/0/0/0/0/0/0
/1/1/1/1/1/1/0/0/0/0/1/1/1/1/0/0/0
/1/1/1
/0/0/0/0/1/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/0/0/1/1/1/1/0/0/1/1/0/0/0
/1/1/1
/0/0/0/0/1/1/1/1/0/0/0
/1/1/1/0/0/0/0/0/0
/1/1/1/1/1/1
/0/0/1/1
vvv v
v
v 123 6
7
8 vvv v
v
v 123 6
7
8(a)v
Figure 18: Illustration of two triangulations where one has greater degree se-
quence.
LetSbe aset oftriangulationsof Cwherethe triangulationsarerotationally
equivalent to each other. Let Tbe the triangulation in Swhose degree sequence
is greater than all other triangulations in S. Then, the labeled degree sequence
ofTis thecanonical representation ofS. We say that Thas thegreatestlabeled
degree sequence and Tis therepresentative ofS. We output each triangulation
TofConly ifThas the greatest labeled degree sequence. Let /an}bracketle{td1,d2···dn/an}bracketri}htbe
the degree sequence of T. Ifd1> difor 2≤i≤n, thenThas the greatest
labeled degree sequence. This can be found in O(1) time as explained later.

480 Parvez et al. Generating All Triangulations of Plane Graphs
Otherwise, we generate n−1other degree sequences by right rotating T’sdegree
sequence and check whether T’s sequence is greater. In this case, it takes O(n2)
time to ﬁnd whether Thas the greatest labeled degree sequence.
For a triangulation TofC, we need to maintain an array Dto store the
degree sequence. It takes O(n) space. Let ( v1,vj) is a generating chord in T
with opposite pair ( vo,vo′). Flipping ( v1,vj) changes the degrees of four ver-
tices. The degrees of v1andvjare reduced by one and the degrees of voand
vo′are increased by one. All these updates can be done in O(1) time. Let
/an}bracketle{td′
1,d′
2,···,d′
n/an}bracketri}htbe the resultant degree sequence and T′is the new triangula-
tion. We can easily check whether d′
1> d′
ifor 2≤i≤nby storing the highest
degreedmaxamong nodes other than v1and updating it while generating a new
triangulation. Now there are three cases.
Case 1.Ifd′
1> dmax, then output T′.
Case 2.Ifd′
1=dmax, then check whether T′has the greatest labeled degree
sequence. If YES, then output T′.
Case 3.Ifd′
1< dmax, then ignore T′and prune the subtree of triangulations
rooted at T′.
5.2 Avoiding Mirror Repetitions
In this section, we describe the procedure to remove mirror image r epetitions
while generating all triangulations of a cycle Cofnvertices.
Let/an}bracketle{td1,d2,···,dn/an}bracketri}htbe the labeled degree sequence of a triangulation TofC.
Assume that Thas the greatest labeled degree sequence compared to all trian-
gulations of Cwhich are rotationally similar to T. LetT′be the triangulation
which is the mirror image of T. Using the following fact we can ﬁnd the labeled
degree sequence of T′.
Fact 10 LetTandT′be two triangulations of a cycle of nvertices, which are
mirror images of each other. Let Thas the labeled degree sequence /an}bracketle{td1,d2,···,dn/an}bracketri}ht.
Then the labeled degree sequence of T′is/an}bracketle{tdn,dn−1,···,d2,d1/an}bracketri}ht.
For example, the triangulation of Figure 16(a) has the degree sequ ence
/an}bracketle{t4,2,3,4,2,3/an}bracketri}ht. The triangulation of Figure 16(b), which is the mirror image of
thetriangulationofFigure16(a), hasthe reversedegreesequence /an}bracketle{t3,2,4,3,2,4/an}bracketri}ht.
Now, using the labeled degree sequence of T′, we can avoid mirror image
repetitions as follows. We start with the sequence /an}bracketle{tdn,dn−1,···,d2,d1/an}bracketri}ht, and
from it we generate n−1 other sequences by right rotation. These n−1
sequences corresponds to all the triangulations which are rotatio nally similar to
T′. We compare the degree sequence of Twith all these sequences to determine
whether T’s sequence is the greatest. Thus, we have to compare T’s sequence
with a total of nsequences. This takes O(n2) time. If T’s sequence is found

JGAA, 15(3) 457–482 (2011) 481
greater than all these sequences, then we output T. Otherwise we discard T
and prune the subtree rooted at T. Since all we need is to store the sequence
ofT, the space complexity is O(n).
Thus we have the following theorem.
Theorem 3 For a cycle Cofnvertices, all triangulations of Ccan be found
in timeO(n2)per triangulation, where the vertices of Care not numbered. The
space complexity is O(n).
6 Conclusion
In this paper we gave an algorithm to generate all triangulations of a tricon-
nected plane graph Gofnvertices in O(1) time per triangulation with linear
space complexity. We also gave an algorithm to generate all triangula tions of a
cycle ofnlabeled vertices in time O(1) per triangulation with O(n) space com-
plexity. The performance of the algorithms can be further improve d by using
parallel processing. Our algorithm also works for biconnected grap hs, but may
produce multi-edges occasionally. Finally, we described a method to e liminate
any rotational and mirror repetitions while generating all triangulat ions of a
cycleC, when the vertices of Care not numbered.
Acknowledgements
We thank the referees for their valuable comments which helped us t o improve
the presentation of the paper.

482 Parvez et al. Generating All Triangulations of Plane Graphs
References
[1] D. Avis. Generating rooted triangulations without repetitions. Algorith-
mica, 16(6):618–632, 1996.
[2] D. Avis and K. Fukuda. Reverse search for enumeration. Discrete Applied
Mathematics , 65(1-3):21–46, 1996.
[3] S.Bespamyatnikh. Aneﬃcientalgorithmforenumerationoftrian gulations.
Comput. Geom. Theory Appl. , 23(3):271–279, 2002.
[4] B. Chazelle. Triangulating a polygon in linear time. Discrete Comput.
Geom., 6:485–524, 1991.
[5] M. de Berg, M. van Krevald, M. Overmars, and O. Schwarzkopf. Compu-
tational Geometry: Algorithms and Applications . Springer-Verlag, Berlin,
2000.
[6] F. Hurtado and M. Noy. Graph of triangulations of a convex polyg on and
tree of triangulations. Computational Geometry , 13(3):179–188, 1999.
[7] H. Komuro, A. Nakamoto, and S. Negami. Diagonal ﬂips in triangula tions
on closed surfaces with minimum degree at least 4. Journal of Combinato-
rial Theory, Series B , 76(1):68–92, 1999.
[8] Z. Li and S. Nakano. Eﬃcient generation of plane triangulations w ithout
repetitions. In Proc. of ICALP 2001 , volume 2076 of Lecture Notes in
Computer Science , pages 433–443. Springer-Verlag, 2001.
[9] B. D. McKay. Isomorph-free exhaustive generation. J. Algorithms ,
26(2):306–324, 1998.
[10] T. Nishizeki and M. S. Rahman. Planar Graph Drawing . World Scientiﬁc,
Singapore, 2004.
[11] J. O’Rourke. Computational Geometry in C . Cambridge University Press,
Cambridge, 1998.
[12] M. T. Parvez, M. S. Rahman, and S. Nakano. Generating all tria ngulations
of plane graphs. In Proc. of WALCOM 2009 , volume 5431 of Lecture Notes
in Computer Science , pages 151–164. Springer, 2009.
[13] D. Poulalhon and G. Schaeﬀer. Optimal coding and sampling of tria ngu-
lations.Algorithmica , 46(3-4):505–527, 2006.
[14] S. M. Sait and H. Youssef. VLSI Physical Design Automation: Theory and
Practice. World Scientiﬁc, Singapore, 1999.
[15] D. D. Sleator, R. E. Tarjan, and W. P. Thurston. Rotation dist ance, trian-
gulations, and hyperbolic geometry. J. Amer. Math. Soc , 1:647–681, 1988.

