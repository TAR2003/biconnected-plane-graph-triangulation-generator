if(!settings.multipleView) settings.batchView=false;
settings.tex="pdflatex";
defaultfilename="main-3";
if(settings.render < 0) settings.render=4;
settings.outformat="";
settings.inlineimage=true;
settings.embed=true;
settings.toolbar=false;
viewportmargin=(2,2);

size(400);

real nodeRadius = 0.2;
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

// ---------------- Left Graph ----------------
pair[] leftNodes = {
(0,2), //1
(0,6), //2
(2,6), //3
(4,6), //4
(4,4), //5
(4,2), //6
(2,2), //7
(2,4) //8
};

int[][] leftEdges = {
{0,1},{1,2},{2,3},{3,4},{4,5},{5,6},{6,7},{0,6},{2,7}
};

// Draw left edges
for(int i=0;i<leftEdges.length;++i)
drawEdge(leftNodes[leftEdges[i][0]], leftNodes[leftEdges[i][1]]);

// Draw left nodes
for(int i=0;i<leftNodes.length;++i)
drawNode(leftNodes[i], string(i+1));

// Labels F1, F2
label("$F_1$", (1,4));
label("$F_2$", (3,4));

// Optional multi-edge example (red dotted) for left graph
draw(leftNodes[2] .. controls ((leftNodes[2]+leftNodes[6])/2 - (0.7,0)) .. leftNodes[6], redEdgePen);
draw(leftNodes[2] .. controls ((leftNodes[2]+leftNodes[6])/2 + (0.7,0)) .. leftNodes[6], redEdgePen);

// ---------------- Right Graph ----------------
pair offset = (7,0); // shift right
pair[] rightNodes = {
(0,2), (0,6), (2,6), (6,6), (6,4), (6,2), (2,2), (4,4)
};
for(int i=0;i<rightNodes.length;++i)
rightNodes[i] += offset;

int[][] rightEdges = {
{0,1},{1,2},{2,3},{3,4},{4,5},{5,6},{2,6},{0,6},{2,7},{6,7}
};

// Draw right edges
for(int i=0;i<rightEdges.length;++i)
drawEdge(rightNodes[rightEdges[i][0]], rightNodes[rightEdges[i][1]]);

// Draw right nodes
for(int i=0;i<rightNodes.length;++i)
drawNode(rightNodes[i], string(i+1));

// Labels F1, F2 for right graph
label("$F_1$", (1,4)+offset);
label("$F_2$", (5,4)+offset);

