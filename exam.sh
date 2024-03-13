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

chmod 655 err.txt

n=2
if [ $# -eq 2 ]
then
	n=$(($1 + $2))
elif [ $# -eq 1 ]
then
	n=$(($1 + 1))
fi
sed -n ${n}p err.txt >&2

