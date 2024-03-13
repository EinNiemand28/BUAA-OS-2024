#!/bin/bash

mkdir test

cp -r ./code ./test/code

cat ./code/14.c

i=0
while [ $i -le 15 ]
do
	gcc ./test/code/$i.c -c ./test/code/$i.o
	let i=i+1
done

gcc -o ./test/code/*.o hello
