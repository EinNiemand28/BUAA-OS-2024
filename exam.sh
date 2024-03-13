#!/bin/bash

mkdir test

cp -r code ./test/code

cat code/14.c


cd test/code
i=0
while [ $i -le 15 ]
do
	gcc -c $i.c
	i=$[$i+1]
done
cd ../../

gcc -o test/hello test/code/*.o

./test/hello 2> test/err.txt

mv test/err.txt err.txt
