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

j=0
while [ $j -le 15 ]
do
	gcc -o test/code/$i.o hello
	j=$[$j+1]
done

./test/hello 2> /test/err.txt

mv test/err.txt err.txt
