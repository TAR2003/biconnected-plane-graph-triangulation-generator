if(!settings.multipleView) settings.batchView=false;
settings.tex="pdflatex";
defaultfilename="main-2";
if(settings.render < 0) settings.render=4;
settings.outformat="";
settings.inlineimage=true;
settings.embed=true;
settings.toolbar=false;
viewportmargin=(2,2);

size(150);

// ---------------- Settings ----------------
real nodeRadius = 0.25;
pen nodePen = black+1;
pen redEdgePen = red + 1.5bp + dotted;

// Function to draw a node
void drawNode(pair pos, string label, pen p = nodePen) {
draw(circle(pos, nodeRadius), p);
label(label, pos);
}

// Function to draw edges stopping at node borders
void drawEdge(pair A, pair B, real r = nodeRadius, pen p = black) {
pair dir = (B - A)/abs(B - A);
draw(A + r*dir -- B - r*dir, p);
}

// ---------------- Nodes ----------------
pair[] nodes = {
(0,2), // 1
(0,6), // 2
(2,6), // 3
(4,6), // 4
(4,4), // 5
(4,2), // 6
(2,2), // 7
(2,4) // 8
};

// Draw nodes
for(int i=0; i<nodes.length; ++i)
drawNode(nodes[i], string(i+1));

// Text labels F1 and F2
label("$F_1$", (1,4));
label("$F_2$", (3,4));

// ---------------- Edges ----------------
int[][] edges = {
{0,1},{1,2},{2,3},{3,4},{4,5},{5,6},{6,7},{0,6},{2,7}
};

for(int i=0;i<edges.length;++i)
drawEdge(nodes[edges[i][0]], nodes[edges[i][1]]);

// ---------------- Curved red dotted edges (left/right bends) ----------------
pair mid = (nodes[2] + nodes[6])/2; // midpoint between nodes 3 and 7
real bendOffset = 0.7; // horizontal bend amount

// Bend left (control point slightly to the left of midpoint)
draw(nodes[2] .. controls (mid - (bendOffset,0)) .. nodes[6], redEdgePen);

// Bend right (control point slightly to the right of midpoint)
draw(nodes[2] .. controls (mid + (bendOffset,0)) .. nodes[6], redEdgePen);

