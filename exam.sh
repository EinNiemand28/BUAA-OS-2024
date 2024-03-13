#!/bin/bash

mkdir test

cp -r code ./test/code

cat code/14.c

i=0
while [ $i -le 15 ]
do
	gcc -c test/code/$i.c
	mv $i.o test/code/$i.o
	i=$[$i+1]
done

gcc -o test/hello test/code/*.o

./test/hello 2> test/err.txt

mv test/err.txt err.txt
