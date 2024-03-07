all: 
	make -C code
	gcc -o fibo code/fibo.o code/main.o
	
clean:
	rm -rf code/*.o

.PNONY: clean

