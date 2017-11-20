.PHONY: run clean

run: project2
	./project2

project2: project2_008.o
	gcc -o project2 project2_008.o
project2.o: project2.c
	gcc -c project2_008.c
clean:
	rm *.o project2
