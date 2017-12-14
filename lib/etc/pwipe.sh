#!/bin/sh
# file: pwipe.sh by Dunkelzahn
# Think very carefully about what you are doing before running this script.
# (c)2001 The AwakeMUD Consortium

echo "Removing old Player Directory...";

rm -rf pfiles/;

echo "Generating Player File Directories...";

mkdir pfiles;
mkdir pfiles/A;
mkdir pfiles/B;
mkdir pfiles/C;
mkdir pfiles/D;
mkdir pfiles/E;
mkdir pfiles/F;
mkdir pfiles/G;
mkdir pfiles/H;
mkdir pfiles/I;
mkdir pfiles/J;
mkdir pfiles/K;
mkdir pfiles/L;
mkdir pfiles/M;
mkdir pfiles/N;
mkdir pfiles/O;
mkdir pfiles/P;
mkdir pfiles/Q;
mkdir pfiles/R;
mkdir pfiles/S;
mkdir pfiles/T;
mkdir pfiles/U;
mkdir pfiles/V;
mkdir pfiles/W;
mkdir pfiles/X;
mkdir pfiles/Y;
mkdir pfiles/Z;

echo "Generating Player File Symlinks...";

cd pfiles/;

ln -s A a;
ln -s B b;
ln -s C c;
ln -s D d;
ln -s E e;
ln -s F f;
ln -s G g;
ln -s H h;
ln -s I i;
ln -s J j;
ln -s K k;
ln -s L l;
ln -s M m;
ln -s N n;
ln -s O o;
ln -s P p;
ln -s Q q;
ln -s R r;
ln -s S s;
ln -s T t;
ln -s U u;
ln -s V v;
ln -s W w;
ln -s X x;
ln -s Y y;
ln -s Z z;

echo "Creating Player Index File...";

touch plr_index;

cd ..;

echo "Player Wipe Completed.";

exit
