LIB "tst.lib"; tst_init();
LIB "freegb.lib";
ring r = 0,(H2,H1,y6,y5,y4,y3,y2,y1,x6,x5,x4,x3,x2,x1),Dp;
int upToDeg = 5;
def R = makeLetterplaceRing(upToDeg);
setring(R);
ideal Id = x2*x1-x1*x2-x3,
x3*x1-x1*x3-2*x4,
x4*x1-x1*x4-3*x5,
x5*x1-x1*x5,
x6*x1-x1*x6,
y1*x1-x1*y1+H1,
y2*x1-x1*y2,
y3*x1-x1*y3+3*y2,
y4*x1-x1*y4+2*y3,
y5*x1-x1*y5+y4,
y6*x1-x1*y6,
H1*x1-x1*H1-2*x1,
H2*x1-x1*H2+x1,
x3*x2-x2*x3,
x4*x2-x2*x4,
x5*x2-x2*x5-x6,
x6*x2-x2*x6,
y1*x2-x2*y1,
y2*x2-x2*y2+H2,
y3*x2-x2*y3-y1,
y4*x2-x2*y4,
y5*x2-x2*y5,
y6*x2-x2*y6+y5,
H1*x2-x2*H1+3*x2,
H2*x2-x2*H2-2*x2,
x4*x3-x3*x4+3*x6,
x5*x3-x3*x5,
x6*x3-x3*x6,
y1*x3-x3*y1+3*x2,
y2*x3-x3*y2-x1,
y3*x3-x3*y3+H1+3*H2,
y4*x3-x3*y4-2*y1,
y5*x3-x3*y5,
y6*x3-x3*y6-y4,
H1*x3-x3*H1+x3,
H2*x3-x3*H2-x3,
x5*x4-x4*x5,
x6*x4-x4*x6,
y1*x4-x4*y1+2*x3,
y2*x4-x4*y2,
y3*x4-x4*y3-2*x1,
y4*x4-x4*y4+2*H1+3*H2,
y5*x4-x4*y5-y1,
y6*x4-x4*y6+y3,
H1*x4-x4*H1-x4,
H2*x4-x4*H2,
x6*x5-x5*x6,
y1*x5-x5*y1+x4,
y2*x5-x5*y2,
y3*x5-x5*y3,
y4*x5-x5*y4-x1,
y5*x5-x5*y5+H1+H2,
y6*x5-x5*y6-y2,
H1*x5-x5*H1-3*x5,
H2*x5-x5*H2+x5,
y1*x6-x6*y1,
y2*x6-x6*y2+x5,
y3*x6-x6*y3-x4,
y4*x6-x6*y4+x3,
y5*x6-x6*y5-x2,
y6*x6-x6*y6+H1+2*H2,
H1*x6-x6*H1,
H2*x6-x6*H2-x6,
y2*y1-y1*y2+y3,
y3*y1-y1*y3+2*y4,
y4*y1-y1*y4+3*y5,
y5*y1-y1*y5,
y6*y1-y1*y6,
H1*y1-y1*H1+2*y1,
H2*y1-y1*H2-y1,
y3*y2-y2*y3,
y4*y2-y2*y4,
y5*y2-y2*y5+y6,
y6*y2-y2*y6,
H1*y2-y2*H1-3*y2,
H2*y2-y2*H2+2*y2,
y4*y3-y3*y4-3*y6,
y5*y3-y3*y5,
y6*y3-y3*y6,
H1*y3-y3*H1-y3,
H2*y3-y3*H2+y3,
y5*y4-y4*y5,
y6*y4-y4*y6,
H1*y4-y4*H1+y4,
H2*y4-y4*H2,
y6*y5-y5*y6,
H1*y5-y5*H1+3*y5,
H2*y5-y5*H2-y5,
H1*y6-y6*H1,
H2*y6-y6*H2+y6,
H2*H1-H1*H2,
x1*x2*x3*x4;
option(redTail);
option(redSB);
std(Id);
tst_status(1);$
