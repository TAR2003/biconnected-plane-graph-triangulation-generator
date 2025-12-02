if(!settings.multipleView) settings.batchView=false;
settings.tex="pdflatex";
defaultfilename="main-1";
if(settings.render < 0) settings.render=4;
settings.outformat="";
settings.inlineimage=true;
settings.embed=true;
settings.toolbar=false;
viewportmargin=(2,2);

settings.tex = "pdflatex";
settings.outformat = "pdf";

size(4cm);
draw(circle((0,0),1));
label("$A$", (0,1), N);
