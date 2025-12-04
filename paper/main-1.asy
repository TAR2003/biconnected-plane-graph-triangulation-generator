if(!settings.multipleView) settings.batchView=false;
settings.tex="pdflatex";
defaultfilename="main-1";
if(settings.render < 0) settings.render=4;
settings.outformat="";
settings.inlineimage=true;
settings.embed=true;
settings.toolbar=false;
viewportmargin=(2,2);

size(300);

// ---------------- Settings ----------------
real nodeRadius = 0.25;
pen nodePen = black+1;
pen redNodePen = red+1;
pair offset = (6,0); // horizontal shift for right graph

// Function to draw a node
void drawNode(pair pos, string label, pen p = nodePen) {
draw(circle(pos, nodeRadius), p);
label(label, pos);
}

// Function to draw an edge stopping at node borders
void drawEdge(pair A, pair B, real r = nodeRadius) {
pair dir = (B - A)/abs(B - A); // unit vector
draw(A + r*dir -- B - r*dir);
}

// ---------------- Left Graph ----------------
pair[] leftNodes = {
(0,2), // 1
(0,6), // 2
(2,6), // 3
(4,6), // 4
(4,4), // 5
(4,2), // 6
(2,2), // 7
(2,4) // 8
};

int[][] leftEdges = {
{0,1}, {1,2}, {2,3}, {3,4}, {4,5}, {5,6}, {6,7}, {0,6}, {2,7}
};

// Draw edges
for(int i=0; i<leftEdges.length; ++i)
drawEdge(leftNodes[leftEdges[i][0]], leftNodes[leftEdges[i][1]]);

// Draw nodes
for(int i=0; i<leftNodes.length; ++i)
drawNode(leftNodes[i], string(i+1));

// ---------------- Right Graph ----------------
pair[] rightNodes = new pair[8];
for(int i=0;i<8;++i) rightNodes[i] = leftNodes[i] + offset;

int[][] rightEdges = {
{0,1}, {3,4}, {4,5}
};

// Draw edges
for(int i=0; i<rightEdges.length; ++i)
drawEdge(rightNodes[rightEdges[i][0]], rightNodes[rightEdges[i][1]]);

// Draw nodes with red highlights for 3 and 7
for(int i=0; i<rightNodes.length; ++i) {
if(i==2 || i==6)
drawNode(rightNodes[i], string(i+1), redNodePen);
else
drawNode(rightNodes[i], string(i+1));
}
