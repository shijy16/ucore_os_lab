diff -urN lab2 lab1 > 1.patch

cd lab2

patch -p1 < ../1.patch
